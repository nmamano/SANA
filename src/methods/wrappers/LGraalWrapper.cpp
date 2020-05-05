#include "LGraalWrapper.hpp"
#include "../../measures/localMeasures/Sequence.hpp"
#include "../../arguments/GraphLoader.hpp"
using namespace std;

const string LGraalWrapper::GDVCounterProgram = "./wrappedAlgorithms/LGRAAL/ncount4.exe";
const string LGraalWrapper::lgraalProgram = "./wrappedAlgorithms/LGRAAL/L-GRAAL.exe";

LGraalWrapper::LGraalWrapper(const Graph* G1, const Graph* G2, double alpha, uint iterlimit, uint timelimit):
    Method(G1, G2, "LGRAAL"), g1Name(G1->getName()), g2Name(G2->getName()), alpha(alpha),
    iterlimit(iterlimit), timelimit(timelimit) {
    g1Folder = "networks/" + g1Name + "/";
    g2Folder = "networks/" + g2Name + "/";
    
    // TEMPORRAY CODE UNTIL WE INHERIT FROM WrappedMethod
    string TMP = "_tmp" + to_string(randInt(0, 2100000000)) + "_";
    string g1TmpName = "LGRAAL" + TMP + g1Name;
    string g2TmpName = "LGRAAL" + TMP + g2Name;
    string alignmentTmpName = "LGRAAL" + TMP + "align_" + g1Name + "_" + g2Name + "_";
    // end temporary code

    g1NetworkFile = "tmp/"+g1TmpName+".gw";
    g2NetworkFile = "tmp/"+g2TmpName+".gw";
    vector<uint> unusedRefParam;
    Graph H1 = G1->shuffledGraph(unusedRefParam);
    GraphLoader::saveInGWFormat(H1, g1NetworkFile);
    Graph H2 = G2->shuffledGraph(unusedRefParam);
    GraphLoader::saveInGWFormat(H2, g2NetworkFile);
    g1GDVFile = g1Folder + "autogenerated/" + g1Name + "_lgraal_gdvs.ndump2";
    g2GDVFile = g2Folder + "autogenerated/" + g2Name + "_lgraal_gdvs.ndump2";

    createFolder("sequence");
    createFolder("sequence/bitscores");
    similarityFile = "sequence/bitscores/" + g1Name + "_" + g2Name + ".bitscores";

    lgraalOutputFile = "tmp/"+g1TmpName+"_"+g2TmpName+"_"+"_lgraalOutput.txt";
}

void LGraalWrapper::generateGDVFile(int graphNum) {

    string rawGDVFileName;
    if (graphNum == 1) {
        int lastindex = g1GDVFile.find_last_of(".");
        rawGDVFileName = g1GDVFile.substr(0, lastindex);
    }
    else {
        int lastindex = g2GDVFile.find_last_of(".");
        rawGDVFileName = g2GDVFile.substr(0, lastindex);
    }

    exec("chmod +x "+GDVCounterProgram);
    string cmd;
    if (graphNum == 1) {
        cmd = GDVCounterProgram+" "+g1NetworkFile+" "+rawGDVFileName;
    }
    else {
        cmd = GDVCounterProgram+" "+g2NetworkFile+" "+rawGDVFileName;
    }
    execPrintOutput(cmd);

    //clean auxiliar files (leave only .ndump2)
    deleteFile(rawGDVFileName);
    deleteFile(rawGDVFileName+".gr_freq");
    for (int i = 0; i < 10; i++) {
        deleteFile(rawGDVFileName+".cl_0"+to_string(i)+"_freq");
    }
    for (int i = 10; i < 73; i++) {
        deleteFile(rawGDVFileName+".cl_"+to_string(i)+"_freq");
    }
}

string LGraalWrapper::generateDummySimilarityFile() {
    string fileName = "tmp/"+G1->getName()+"_"+G2->getName()+"_dummy.logval";
    return fileName;
}

void LGraalWrapper::generateAlignment() {
    //exec("chmod +x "+lgraalProgram);
    //Example, when aligning RNorvegicus and SPombe:
    //./L-GRAAL -Q RNorvegicus.gw -T SPombe.gw -q RNorvegicus.ndump2 -t SPombe.ndump2 -B blast_evalues.txt -o my_output
    // L-GRAAL expects the network with the fewer EDGES to be first, not fewer nodes.  Sometimes it's backwards.
    int swap = (G1->getNumEdges() > G2->getNumEdges());
    string cmd = lgraalProgram + " -Q " + (swap?g2NetworkFile:g1NetworkFile) + " -T " + (swap?g1NetworkFile:g2NetworkFile);
    cmd += " -q " + (swap?g2GDVFile:g1GDVFile) + " -t " + (swap?g1GDVFile:g2GDVFile);
    cmd += " -B " + similarityFile + " -o " + lgraalOutputFile;
    cmd += " -I " + to_string(iterlimit) + " -L " + to_string(timelimit);
    cmd += " -a " + to_string(alpha);
    cout << "Executing " << cmd << endl;
    execPrintOutput(cmd);
    cout << "Done" << endl;
}

void LGraalWrapper::deleteAuxFiles() {
    deleteFile(g1NetworkFile);
    deleteFile(g2NetworkFile);
    deleteFile(lgraalOutputFile);
}

Alignment LGraalWrapper::run() {
    if (not fileExists(similarityFile)) {
        if (alpha > 0) {
            Sequence sequence(G1, G2);
            sequence.generateBitscoresFile(similarityFile);
        }
        else {
            similarityFile = generateDummySimilarityFile();
        }
    }
    if (not fileExists(g1GDVFile)) {
        generateGDVFile(1);
    }
    if (not fileExists(g2GDVFile)) {
        generateGDVFile(2);
    }
    generateAlignment();
    Alignment result = Alignment::loadPartialEdgeList(*G1, *G2, lgraalOutputFile, true);
    deleteAuxFiles();
    return result;
}

void LGraalWrapper::describeParameters(ostream& stream) {
    stream << "alpha: " << alpha << " (alpha = 0 means only topology)" << endl;
    stream << "iter limit: " << iterlimit << endl;
    stream << "time limit: " << timelimit << "s" << endl;
    if (alpha > 0) {
        stream << "Similarity file: " << similarityFile << endl;
    }
}

string LGraalWrapper::fileNameSuffix(const Alignment& A) const {
    if (alpha == 1) return "_alpha_1";
    if (alpha == 0) return "_alpha_0";
    return "_alpha_0" + extractDecimals(alpha,1);
}
