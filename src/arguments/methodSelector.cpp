#include <iostream>

#include "methodSelector.hpp"

#include "measureSelector.hpp"

#include "../utils/Timer.hpp"
#include "../methods/NoneMethod.hpp"
#include "../methods/GreedyLCCS.hpp"
#include "../methods/WeightedAlignmentVoter.hpp"
#include "../methods/TabuSearch.hpp"
#include "../methods/HillClimbing.hpp"
#include "../methods/SANA.hpp"
#include "../methods/RandomAligner.hpp"
#include "../methods/wrappers/LGraalWrapper.hpp"
#include "../methods/wrappers/HubAlignWrapper.hpp"
#include "../methods/wrappers/NETALWrapper.hpp"
#include "../methods/wrappers/MIGRAALWrapper.hpp"
#include "../methods/wrappers/GHOSTWrapper.hpp"
#include "../methods/wrappers/PISwapWrapper.hpp"
#include "../methods/wrappers/OptNetAlignWrapper.hpp"
#include "../methods/wrappers/SPINALWrapper.hpp"
#include "../methods/wrappers/GREATWrapper.hpp"
#include "../methods/wrappers/NATALIEWrapper.hpp"
#include "../methods/wrappers/GEDEVOWrapper.hpp"
#include "../methods/wrappers/WAVEWrapper.hpp"
#include "../methods/wrappers/MagnaWrapper.hpp"
#include "../methods/wrappers/PINALOGWrapper.hpp"
#include "../methods/wrappers/SANAPISWAPWrapper.hpp"
#include "../methods/wrappers/CytoGEDEVOWrapper.hpp"
#include "../methods/Dijkstra.hpp"

using namespace std;

Method* initLgraal(Graph& G1, Graph& G2, ArgumentParser& args) {
    string objFunType = args.strings["-objfuntype"];

    double alpha;
    if (objFunType == "generic") {
        throw runtime_error("generic objective function not supported for L-GRAAL");
    } else if (objFunType == "alpha") {
        alpha = args.doubles["-alpha"];
    } else if (objFunType == "beta") {
        double beta = args.doubles["-beta"];
        alpha = betaDerivedAlpha("lgraal", G1.getName(), G2.getName(), beta);
    } else {
        throw runtime_error("unknown value of -objfuntype: "+objFunType);
    }

    double iters = args.doubles["-lgraaliter"];
    double seconds = args.doubles["-t"]*60;
    return new LGraalWrapper(&G1, &G2, alpha, iters, seconds);
}

Method* initHubAlign(Graph& G1, Graph& G2, ArgumentParser& args) {
    string objFunType = args.strings["-objfuntype"];

    double alpha;
    if (objFunType == "generic") {
        throw runtime_error("generic objective function not supported for HubAlign");
    } else if (objFunType == "alpha") {
        alpha = args.doubles["-alpha"];
    } else if (objFunType == "beta") {
        double beta = args.doubles["-beta"];
        alpha = betaDerivedAlpha("hubalign", G1.getName(), G2.getName(), beta);
    } else {
        throw runtime_error("unknown value of -objfuntype: "+objFunType);
    }

    return new HubAlignWrapper(&G1, &G2, alpha);
}

Method* initDijkstra(Graph& G1, Graph& G2, ArgumentParser& args, MeasureCombination& M) {
    double delta = args.doubles["-dijkstradelta"];
    if(delta < 0.0 || delta > 1.0){
        throw runtime_error("Dijkstra:delta not in valid range [0.0,1.0)");
    }
    return new Dijkstra(&G1, &G2, &M, delta);
}

Method* initTabuSearch(Graph& G1, Graph& G2, ArgumentParser& args, MeasureCombination& M) {

    double minutes = args.doubles["-t"];
    uint ntabus = args.doubles["-ntabus"];
    uint nneighbors = args.doubles["-nneighbors"];
    bool nodeTabus = args.bools["-nodetabus"];
    return new TabuSearch(&G1, &G2, minutes, &M, ntabus, nneighbors, nodeTabus);
}

#ifdef MULTI_PAIRWISE
Method* initSANA(Graph& G1, Graph& G2, ArgumentParser& args, MeasureCombination& M, string startAligName) {
#else
Method* initSANA(Graph& G1, Graph& G2, ArgumentParser& args, MeasureCombination& M) {
#endif

    string TIniArg = args.strings["-tinitial"];
    string TDecayArg = args.strings["-tdecay"];
    string TBothArg = args.strings["-tparams"];

    string pBadBinMethod = "pbad-binary-search";
    string linRegMethod = "linear-regression";
    string ameurMethod = "ameur-method";
    string statMethod = "statistical-test";
    string bayesMethod = "bayesian-opt";
    vector<string> autoTempMethods = { pBadBinMethod, linRegMethod, ameurMethod, statMethod, bayesMethod };
    string defaultMethod = linRegMethod; 

    //this is a special argument value that does a comparison between all temperature schedule methods
    //made for the purpose of running the experiments for the paper on the tempertaure schedule
    if (TBothArg == "comparison") {
        SANA sana(&G1, &G2, 0, 0, 0, args.bools["-usingIterations"], 0, &M, args.strings["-combinedScoreAs"]);
        sana.setOutputFilenames(args.strings["-o"], args.strings["-localScoresFile"]);

        vector<string> methods = { pBadBinMethod, linRegMethod, ameurMethod };
        vector<vector<double>> res;
        for (string method : methods) {
            double TIniTime, TFinalTime;
            Timer T;
            if (method == linRegMethod) {
                T.start();
                sana.setTInitialAndTFinalByLinearRegression();
                TIniTime = T.elapsed(); 
                TFinalTime = 0;//linear regression sets both simultaneously
            } else if (method == statMethod) {
                T.start();
                sana.setTInitialByStatisticalTest();
                TIniTime = T.elapsed();
                T.start();
                sana.setTFinalByCeasedProgress();
                TFinalTime = T.elapsed();
            } else if (method == ameurMethod) {
                T.start();
                sana.setTInitialByAmeurMethod();
                TIniTime = T.elapsed();
                T.start();
                sana.setTFinalByAmeurMethod();
                TFinalTime = T.elapsed();
            } else if (method == pBadBinMethod) {
                T.start();
                sana.setTInitialByPBadBinarySearch();
                TIniTime = T.elapsed();
                T.start();
                sana.setTFinalByPBadBinarySearch();
                TFinalTime = T.elapsed();
            } else throw runtime_error("unexpected method");

            double TIni = sana.TInitial;
            double TIniPBad = sana.getPBad(TIni, 5);
            double TIniPBadRelative = TIniPBad/sana.TARGET_INITIAL_PBAD;
            double TFinal = sana.TFinal;
            double TFinalPBad = sana.getPBad(TFinal, 5);
            double TFinalPBadRelative = TFinalPBad/sana.TARGET_FINAL_PBAD;
            double totalTime = TIniTime + TFinalTime;
            res.push_back({TIni, TIniPBad, TIniPBadRelative, TIniTime,
                TFinal, TFinalPBad,TFinalPBadRelative, TFinalTime, totalTime});
        }
        cout << endl << endl;
        cout << "Automatic Temperature Schedule Comparison" << endl;
        cout << "Target Initial PBad: " << sana.TARGET_INITIAL_PBAD << endl;
        cout << "Target Final PBad:   " << sana.TARGET_FINAL_PBAD << endl;
        vector<vector<string> > table;
        table.push_back({"Method","TInitial","PBad","(relative)","Time",
            "TFinal","PBad","(relative)","Time","TotalTime"});
        vector<int> precision = {6,6,6,2,9,9,6,2,2};
        for (uint i = 0; i < methods.size(); i++) {
            table.push_back(vector<string> ());
            table[i+1].push_back(methods[i]);
            for (int j = 0; j < 9; j++) {
                table[i+1].push_back(toStringWithPrecision(res[i][j], precision[j]));
            }
            if (methods[i] == linRegMethod) {
                table[i+1][8] = "N/A";
            }
        }
        printTable(table, 3, cout);
        cout << endl;
        exit(0);
    }



    //override -tinitial and -tdecay with -tschedule (if it's not use-tinitial-tdecay)
    if (TBothArg != "use-tinitial-tdecay") {
        TIniArg = TBothArg;
        TDecayArg = TBothArg;
    } 
    //if user uses 'auto', choose a method here for them (should be the one regarded as best)
    if (TIniArg == "auto") {
	   args.strings["-tinitial"] = defaultMethod; //can this line be removed?
       TIniArg = defaultMethod;
    }
    if (TDecayArg == "auto") {
        args.strings["-tdecay"] = defaultMethod; //can this line be removed?
        TDecayArg = linRegMethod;        
    }

    bool useMethodForTIni = contains(autoTempMethods, TIniArg);
    bool useMethodForTDecay = contains(autoTempMethods, TDecayArg);

    //read explicit values for TInitial or TDecay if not using an automatic method
    double TInitial = 0, TDecay = 0;
    if (not useMethodForTIni) {
        try {
            TInitial = stod(TIniArg);
        } catch (const invalid_argument& ia) {
            cerr << "invalid -tinitial argument: " << TIniArg << endl;
            throw ia;
        }
    }
    if (not useMethodForTDecay) {
        try {
            TDecay = stod(TDecayArg);
        } catch (const invalid_argument& ia) {
            cerr << "invalid -tdecay argument: " << TDecayArg << endl;
            throw ia;
        }
    }

    Method* sana;

    double time = args.doubles["-t"];
#ifdef MULTI_PAIRWISE
    sana = new SANA(&G1, &G2, TInitial, TDecay, time, args.bools["-usingIterations"], args.bools["-add-hill-climbing"], &M, args.strings["-combinedScoreAs"], startAligName);
#else
    sana = new SANA(&G1, &G2, TInitial, TDecay, time, args.bools["-usingIterations"], args.bools["-add-hill-climbing"], &M, args.strings["-combinedScoreAs"]);
#endif
    ((SANA*) sana)->setOutputFilenames(args.strings["-o"], args.strings["-localScoresFile"]);

    if (useMethodForTIni) {
        Timer T;
        T.start();
        if (TIniArg == linRegMethod) {
            //linear regression sets both tini and tfinal simultaneously
            ((SANA*) sana)->setTInitialAndTFinalByLinearRegression();
        } else if (TIniArg == statMethod) {
            ((SANA*) sana)->setTInitialByStatisticalTest();
        } else if (TIniArg == ameurMethod) {
            ((SANA*) sana)->setTInitialByAmeurMethod();
        } else if (TIniArg == bayesMethod) {
            ((SANA*) sana)->setTInitialByBayesOptimization();
        } else if (TIniArg == pBadBinMethod) {
            ((SANA*) sana)->setTInitialByPBadBinarySearch();
        } else {
            throw runtime_error("every method should have been handled as an 'if', so we should not reach here");
        }
        cout << endl << "TInitial took " << T.elapsed() << "s to compute" << endl << endl;
    }    

    if (useMethodForTDecay) {
        Timer T;
        T.start();

        //first 'TFinal' is set using the specified method
        //then 'TDecay' is set based on TFinal

        if(TDecayArg == linRegMethod) {
            if (TIniArg == linRegMethod) {
                //nothing to do, TFinal is already set
            } else {
                /* this is not using linear regression as the user indicated. */
        	    ((SANA*) sana)->setTFinalByDoublingMethod();
            }
        } else if(TDecayArg == statMethod) {
            ((SANA*) sana)->setTFinalByCeasedProgress();
        } else if (TDecayArg == ameurMethod) {
            ((SANA*) sana)->setTFinalByAmeurMethod();
        } else if (TDecayArg == bayesMethod) {
            ((SANA*) sana)->setTFinalByBayesOptimization();
        } else if (TDecayArg == pBadBinMethod) {
            ((SANA*) sana)->setTFinalByPBadBinarySearch();
        } else {
            throw runtime_error("every method should have been handled as an 'if', so we should not reach here");
        }
        ((SANA*) sana)->setTDecayFromTempRange();
        cout << endl << "TFinal took " << T.elapsed() << "s to compute" << endl << endl;
    }
    if (useMethodForTIni or useMethodForTDecay) {
        ((SANA*) sana)->printScheduleStatistics();
    }


    if (args.bools["-restart"]) {
        double tnew = args.doubles["-tnew"];
        uint iterperstep = args.doubles["-iterperstep"];
        uint numcand = args.doubles["-numcand"];
        double tcand = args.doubles["-tcand"];
        double tfin = args.doubles["-tfin"];
        ((SANA*) sana)->enableRestartScheme(tnew, iterperstep, numcand, tcand, tfin);
    }

    if (args.bools["-dynamictdecay"]) {
       ((SANA*) sana)->setDynamicTDecay();
    }
    if (args.strings["-lock"] != ""){
      sana->setLockFile(args.strings["-lock"] );
    }
    if(args.bools["-lock-same-names"] && args.strings["-lock"].size()== 0){
        sana->setLockFile("/dev/null");
    }
    return sana;
}

Method* initMethod(Graph& G1, Graph& G2, ArgumentParser& args, MeasureCombination& M) {

    string aligFile = args.strings["-eval"];
    if (aligFile != "")
        return new NoneMethod(&G1, &G2, aligFile);
    string name = toLowerCase(args.strings["-method"]);
    string startAligName = args.strings["-startalignment"];
    double alpha = args.doubles["-alpha"];
    string wrappedArgs = args.strings["-wrappedArgs"];

    if (name == "greedylccs")
        return new GreedyLCCS(&G1, &G2, startAligName);
    if (name == "waveSim") {
        LocalMeasure* waveNodeSim =
            (LocalMeasure*) M.getMeasure(args.strings["-wavenodesim"]);
        return new WeightedAlignmentVoter(&G1, &G2, waveNodeSim);
    }

    if (name == "lgraal")
        return initLgraal(G1, G2, args);
    if (name == "hubalign")
        return initHubAlign(G1, G2, args);
    if (name == "tabu")
        return initTabuSearch(G1, G2, args, M);
    if (name == "dijkstra")
        return initDijkstra(G1, G2, args, M);
        //return new Dijkstra(&G1, &G2, &M);
    if (name == "netal")
        return new NETALWrapper(&G1, &G2, wrappedArgs);
    if (name == "mi-graal" || name == "migraal")
        return new MIGRAALWrapper(&G1, &G2, wrappedArgs);
    if (name == "ghost")
        return new GHOSTWrapper(&G1, &G2, wrappedArgs);
    if (name == "piswap")
        return new PISwapWrapper(&G1, &G2, alpha, startAligName, wrappedArgs);
    if (name == "optnetalign")
       return new OptNetAlignWrapper(&G1, &G2, wrappedArgs);
    if (name == "spinal")
        return new SPINALWrapper(&G1, &G2, alpha, wrappedArgs);
    if (name == "great")
        return new GREATWrapper(&G1, &G2, wrappedArgs);
    if (name == "natalie")
        return new NATALIEWrapper(&G1, &G2, wrappedArgs);
    if (name == "gedevo")
        return new GEDEVOWrapper(&G1, &G2, wrappedArgs);
    if (name == "wave")
        return new WAVEWrapper(&G1, &G2, wrappedArgs);
    if (name == "sana")
#ifdef MULTI_PAIRWISE
        return initSANA(G1, G2, args, M, startAligName);
#else
        return initSANA(G1, G2, args, M);
#endif
    if (name == "hc")
        return new HillClimbing(&G1, &G2, &M, startAligName);
    if (name == "random")
        return new RandomAligner(&G1, &G2);
    if (name == "none")
        return new NoneMethod(&G1, &G2, startAligName);
    if (name == "magna")
        return new MagnaWrapper(&G1, &G2, wrappedArgs);
    if (name == "pinalog")
        return new PINALOGWrapper(&G1, &G2, wrappedArgs);
    if (name == "sana+piswap")
        return new SANAPISWAPWrapper(&G1, &G2, args, M);
    if (name == "cytogedevo")
        return new CytoGEDEVOWrapper(&G1, &G2, wrappedArgs);

    throw runtime_error("Error: unknown method: " + name);
}
