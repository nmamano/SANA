#include <vector>
#include <iostream>
#include <sstream>
#include "OptNetAlignWrapper.hpp"

using namespace std;

const string CONVERTER = "python converter.py";
const string PROGRAM   = "./optnetalign";
const string GLOBAL_PARAMETERS = "--total --cxrate 0.05 --cxswappb 0.75 --mutrate 0.05 --mutswappb 0.0001 --oneobjrate 0.75 --dynparams --generations 1000000000 --hillclimbiters 10000 --finalstats";

OptNetAlignWrapper::OptNetAlignWrapper(Graph* G1, Graph* G2, string args): WrappedMethod(G1, G2, "OPTNETALIGN", args) {
	wrappedDir = "wrappedAlgorithms/OptNetAlign";
}

void OptNetAlignWrapper::loadDefaultParameters() {
	parameters = "--s3 --nthreads 1 --popsize 2000 --timelimit 5";
}

string OptNetAlignWrapper::convertAndSaveGraph(Graph* graph, string name) {
	string txtFile = name + ".txt";
	graph->writeGraphEdgeListFormat(txtFile);
	return txtFile;
}

string OptNetAlignWrapper::generateAlignment() {
	string outputname = "out";
	//exec("cd " + wrappedDir + "/src; make optnetalignubuntu; mv optnetalign ../");
	//exec("cd " + wrappedDir + "; chmod +x " + PROGRAM);
	execPrintOutput("cd " + wrappedDir + "; " + PROGRAM + " --net1 " +
			g1File + " --net2 " + g2File + " " + GLOBAL_PARAMETERS + " " + parameters + " --outprefix " + outputname);

	outputname += "_0.aln";

	return wrappedDir + "/" + outputname;
}

Alignment OptNetAlignWrapper::loadAlignment(Graph* G1, Graph* G2, string fileName) {
	return Alignment::loadPartialEdgeList(G1, G2, fileName, false);
}

void OptNetAlignWrapper::deleteAuxFiles() {
    //exec("cd " + wrappedDir + "; rm " + g1File + " " + g2File);
}
