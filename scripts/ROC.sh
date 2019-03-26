#!/bin/sh
TMP=/tmp/ROC.$$
trap "/bin/rm -f $TMP; exit" 0 1 2 3 15
USAGE="$0 [-AllOrtho 0|1 {default 0}] [-R Resnik threshold] [-S seq thresh] {2 score columns to evaluate} {2-column truth file, eg pairs of orthologs} {G1 edgeList} {G2 edgeList} {p1 p2 2-or-more-scores file} [resnik file] [seqSim File] [complexes file]
The -AllOrtho flag means compute the AUPR with respect to *all* given orthologs, rather than only those in the scores file"
die() { echo "$@" >&2; exit 1
}
R=1e30
S=1e30
AllOrtho=0
while [ $# -gt 0 ]; do
	case $1 in
	-R) R=$2; shift 2 ;;
	-S) S=$2; shift 2 ;;
	-A*) AllOrtho=$2; shift 2;;
	*) break ;;
	esac
done

c1=$1
c2=$2
shift 2

[ $# -ge 4 ] || die "$USAGE"

hawk '
    function EvalScores(c,thresh){TP=TN=FP=FN=0;
	for(p1 in score)for(p2 in score[p1])
	    if(score[p1][p2][c]>thresh){
		if((p1 in O && p2 in O[p1]) || (p1 in R && p2 in R[p1] && R[p1][p2] > 1*'$R') || (p1 in S && p2 in S[p1] && S[p1][p2] > 1*'$S') || 
		    (p2 in complex && index(complex[p2],p1)) || (p1 in complex && index(complex[p1],p2))) ++TP;else ++FP
	    }else{
		if(p1 in O && p2 in O[p1])++FN;else ++TN
	    }
	if(TP+FP==0){Prec=1}else{Prec=TP/(TP+FP)}
	if(TP+FN==0){Rec=1}else{Rec=TP/(TP+FN)}
	if(Prec+Rec==0)F1=0;else F1=2*Prec*Rec/(Prec+Rec)
	TPR=TP/(TP+FN)
	FPR=FP/(FP+TN)
	printf " %6d %7d %6d %7d %.3f %.3f %.3f %.3f %.3f",TP,FP,FN,TN,Prec,Rec,F1,TPR,FPR
	printf " %6d %7d %6d %7d %.3f %.3f %.3f %.3f %.3f",TP,FP,FN,TN,Prec,Rec,F1,TPR,FPR > "'$TMP'"
    }

    BEGIN{c1='$c1';c2='$c2'
	col[1]=c1; col[2]=c2
	title[1]="column_"c1;title[2]="column_"c2;title[3]="Resnik";title[4]="SeqSim"
	scale[1]=1;scale[2]=1;scale[3]=14;scale[4]=100
    }

    ARGIND==1{
	totOrtho++
	O[$1][$2]=1 # Ortholog file, for now, has 2 columns

	# initialize all orthologs in case they are not in scores file?
	if('$AllOrtho') for(c=1;c<=2;c++) score[$1][$2][c] = 0
    }

    # Edge Lists
    ARGIND==2{D1[$1]++;D1[$2]++}
    ARGIND==3{D2[$1]++;D2[$2]++}

    # scores file: p1 p2 [list of 1 or more scores]
    ARGIND==4{
	degreeScore=1;
	if(($1 in D1) && ($2 in D2)&&D1[$1]&&D2[$2]) {
	    #degreeScore = (1-1/MIN(D1[$1],D2[$2]));
	    #degreeScore = (1-1/MAX(D1[$1],D2[$2]));
	    #degreeScore = (1-1/D1[$1])*(1-1/D2[$2]);
	}
	for(c=1;c<=2;c++){
	    score[$1][$2][c]=$col[c] * degreeScore
	    #if p1,p2 are orthologs, then record their column-c score:
	    if(($1 in O)&&($2 in O[$1]))StatAddSample(c,score[$1][$2][c])
	}
    }

    ARGIND==5&&!/None/{ R[$2][$3]=1*$4 } # Resnik score file
    ARGIND==6{ S[$1][$2]=1*$3 } # Sequence sims file
    ARGIND==7{for(i=1;i<=NF;i++)complex[$i]=complex[$i]" "$0} # Complexes file
    END{
	delete D1; delete D2; # do not need degrees any more
	printf "%d total orthologs, found %d ortholog pairs in our scores file\n", totOrtho,StatN(1)
	for(c=1;c<=2;c++){
	    printf "col[%d]=%d %10s mean %8.4f min %8.4f max %8.4f stdDev %8.4f\n",
		c, col[c], title[c],StatMean(c),StatMin(c),StatMax(c),StatStdDev(c)
	}
	printf "\nEvaluating the scores based on thresholds on core score:\n\
thresh  c_"c1":TP      FP     FN      TN  Prec   Rec   F1   TPR   FPR  c_"c2":TP      FP     FN      TN  Prec   Rec   F1   TPR   FPR\n"
	printf "Means  ";EvalScores(1,StatMean(1));EvalScores(2,StatMean(2));print""
	for(c=1;c<=2;c++){AUPR[c]=0;prevRec[c]=1;prevPrec[c]=0}
	for(thresh=1e-4;thresh<1.01;thresh+=MIN(thresh/2,.05)){
	    printf "%.5f",thresh
	    printf "%.5f",thresh > "'$TMP'"
	    done=0;
	    for(c=1;c<=2;c++){
		scaledThresh=scale[c]*thresh;EvalScores(c,scaledThresh) # this sets Prec and Rec
		h=Prec-prevPrec[c]
		AUPR[c]+=h * ((Rec+prevRec[c])/2)
		prevRec[c]=Rec; prevPrec[c]=Prec
		if(Prec==1&&Rec==0)done++;
	    }
	    print""
	    print "" > "'$TMP'"
	    if(done == length(col)) break;
	}
    }' "$@"

for c in 6 15; do
    sort -k ${c}g $TMP | hawk 'BEGIN{pr=1}/^0/{prec=$'$c';rec=$(1+'$c');h=prec-pp;pp=prec;AUPR+=h*((rec+pr)/2);pr=rec}END{printf "AUPR[%d] = %g\t", '$c',AUPR}'
done
echo "" # Final newline after both AUPRs are printed
