#include <cassert>
#include "ParetoMode.hpp"

#include "../utils/utils.hpp"

#include "../arguments/measureSelector.hpp"
#include "../arguments/methodSelector.hpp"
#include "../arguments/graphLoader.hpp"

#include "../report.hpp"
#include <iostream>

void ParetoMode::run(ArgumentParser& args) {
    createFolders();
    Graph G1, G2;
    initGraphs(G1, G2, args);

    //setArgsForParetoMode(args); //Commented to let the user set which measures to include.
    MeasureCombination M;
    initMeasures(M, G1, G2, args);
    Method* method;
    method = initMethod(G1, G2, args, M);
    ParetoFront paretoFront;
    vector<Alignment> alignments = runParetoMode(method, &G1, &G2, args.strings["-o"]);
    printAlignments(alignments, args.strings["-o"]);
    printEdgeLists(&G1, &G2, alignments, args.strings["-o"]);
    for(unsigned int i = 0; i < alignments.size(); i++) {
        Alignment A = Alignment(alignments[i]);

        A.printDefinitionErrors(G1,G2);
        assert(A.isCorrectlyDefined(G1, G2) and "Resulting alignment is not correctly defined");

        string reportName = args.strings["-o"] + "_pareto_" + to_string(i);
        saveReport(G1, G2, A, M, method, reportName);
        string localMeasuresFileName = args.strings["-localScoresFile"] + "_pareto_" + to_string(i);
        saveLocalMeasures(G1, G2, A, M, method, localMeasuresFileName);
    }
    delete method;
}

string ParetoMode::getName(void) {
    return "ParetoMode";
}


void ParetoMode::createFolders(void) {
    createFolder("matrices");
#if USE_CACHED_FILES
// By default, USE_CACHED_FILES is 0 and SANA does not cache files. Change USE_CACHED_FILES at your own risk.
    createFolder("matrices/autogenerated");
#endif
    createFolder("tmp");
    createFolder("alignments");
    createFolder("go");
    createFolder("go/autogenerated");
}

void ParetoMode::setArgsForParetoMode(ArgumentParser& args) {
    args.doubles["-ec"] = args.doubles["-s3"] = args.doubles["-tc"] = args.doubles["-sec"]
                        = args.doubles["-wec"] /*= args.doubles["-nodec"]*/ = args.doubles["local"]//= args.doubles["-noded"]
                        //= args.doubles["-edgec"] /*= args.doubles["-edged"]*/ = args.doubles["-go"]
                        //= args.doubles["-importance"] = args.doubles["-sequence"] = args.doubles["-graphlet"]
                        //= args.doubles["-graphletlgraal"] = args.doubles["-graphletcosine"] /*= args.doubles["-spc"]*/
                        = args.doubles["-nc"] = args.doubles["-mec"] /*= args.doubles["-ewec"]*/ = args.doubles["-ses"]
                        = 1;

    args.strings["-combinedScoreAs"] = "pareto";
    args.strings["-method"] = "sana";
}

vector<Alignment> ParetoMode::runParetoMode(Method *method, Graph *G1, Graph *G2, const string &fileName) {
    cout << "Start execution of " << method->getName() << " in Pareto Mode." << endl;
    Timer T;
    T.start();
    unordered_set<vector<uint>*> *A = static_cast<SANA*>(method)->paretoRun(fileName);

    vector<Alignment> alignments;
    for(auto i = A->begin(); i != A->end(); i++)
        alignments.push_back( Alignment(**i) );

    T.elapsed();
    cout << "Executed " << method->getName() << " in " << T.elapsedString() << endl;

    // Re Index back to normal (Method #3 of locking)

    // Needs to be reimplemented with unordered_set<vector<uint>>*

    for(unsigned int i = 0; i < alignments.size(); i++) {
        if(G1->hasNodeTypes()){
            G1->reIndexGraph(getReverseMap(G1->getNodeTypes_ReIndexMap()));
            alignments[i].reIndexAfter_Iterations(G1->getNodeTypes_ReIndexMap());
        }
        // if locking is enabled but hasnodeType is not
        else if(G1->getLockedCount() > 0){
            G1->reIndexGraph(getReverseMap(G1->getLocking_ReIndexMap()));
            alignments[i].reIndexAfter_Iterations(G1->getLocking_ReIndexMap());
        }
        method->checkLockingBeforeReport(alignments[i]);
        method->checkLockingBeforeReport(alignments[i]);
    }
    return alignments;
}

void ParetoMode::printAlignments(vector<Alignment>& alignments, const string &fileName) {
    string outputFileName = fileName;
    if(outputFileName.rfind(".out") + 4 != outputFileName.size())
        outputFileName = outputFileName + ".out";
    ofstream output(outputFileName);
    for(unsigned int j = 0; j < alignments[0].size(); j++) {
        for(unsigned int i = 0; i < alignments.size(); i++) {
            output << alignments[i][j];
            if(i < alignments.size()-1)
                output << '\t';
        }
        if(j < alignments[0].size() - 1)
            output << endl;
    }
    output.close();
}

typedef unordered_map<uint,string> NodeIndexMap;
void ParetoMode::printEdgeLists(Graph* G1, Graph* G2, vector<Alignment>& alignments, const string &fileName) {
    string outputFileName = fileName;
    if(outputFileName.find(".out") != string::npos && outputFileName.rfind(".out") + 4 == outputFileName.size())
        outputFileName = outputFileName.substr(0, outputFileName.size() - 4);
    outputFileName = outputFileName + ".align";
    NodeIndexMap mapG1 = G1->getIndexToNodeNameMap();
    NodeIndexMap mapG2 = G2->getIndexToNodeNameMap();
    ofstream edgeListStream(outputFileName);
    for (unsigned int j = 0; j < alignments[0].size(); j++) {
        edgeListStream << mapG1[j];
        for(unsigned int i = 0; i < alignments.size(); i++)
            edgeListStream << "\t" << mapG2[alignments[i][j]];
        if(j < alignments[0].size() - 1)
            edgeListStream << endl;
    }
    edgeListStream.close();
}
