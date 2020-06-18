#ifndef REPORT_HPP_
#define REPORT_HPP_

#include <string>
#include "Graph.hpp"
#include "Alignment.hpp"
#include "measures/MeasureCombination.hpp"
#include "methods/Method.hpp"

using namespace std;

class Report {
public:

static void saveReport(const Graph& G1, const Graph& G2, const Alignment& A,
    const MeasureCombination& M, const Method* method, const string& reportFileName, bool longVersion);
static void saveLocalMeasures(const Graph& G1, const Graph& G2, const Alignment& A,
    const MeasureCombination& M, const Method* method, const string& localMeasureFile);
static void saveCoreScores(const Graph& G1, const Graph& G2, const Alignment& A, const Method*,
        Matrix<unsigned long>& pegHoleFreq, vector<unsigned long>& numPegSamples,
        Matrix<double>& weightedPegHoleFreq_pBad, vector<double>& totalWeightedPegWeight_pBad,
        Matrix<double>& weightedPegHoleFreq_1mpBad, vector<double>& totalWeightedPegWeight_1mpBad,
        Matrix<double>& weightedPegHoleFreq_pwPBad, vector<double>& totalWeightedPegWeight_pwPBad,
        Matrix<double>& weightedPegHoleFreq_1mpwPBad, vector<double>& totalWeightedPegWeight_1mpwPBad,
        const string& outputFileName);

private:

//print the alignment in edge list format, using node names, and sorted from least frequent to most frequent color
static void saveAlignmentAsEdgeList(const Alignment& A, const Graph& G1, const Graph& G2, const string& fileName);

//this function does so many things I can't give it a more relevant name -Nil
static string formattedFileName(const string& outFileName, const string& extension, 
    const string& G1Name, const string& G2Name, const Method* method, Alignment const & A);

static void printGraphStats(const Graph& G, uint numCCsToPrint, ofstream& ofs);
};

#endif /* REPORT_HPP_ */
