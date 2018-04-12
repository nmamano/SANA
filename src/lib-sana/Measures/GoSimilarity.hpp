//
// Created by taylor on 4/12/18.
//

#ifndef GOSIMILARITY_HPP
#define GOSIMILARITY_HPP


#include <Measure.hpp>

class GoSimilarity : public Measure {
public:
    GoSimilarity(){}
    GoSimilarity(Graph* G1, Graph* G2,
                 const vector<double>& countWeights,
                 double occurrencesFraction = 1);

    virtual ~GoSimilarity();
    //double eval(const Alignment& A);

    static string getGoSimpleFileName(const Graph& G);
    static void ensureGoFileSimpleFormatExists(const Graph& G);

    static vector<vector<uint> > loadGOTerms(const Graph& G,
                                             double occurrencesFraction = 1);

    //returns the number of times that each go term appears in G
    static unordered_map<uint,uint> getGoCounts(const Graph& G);

    static bool fulfillsPrereqs(Graph* G1, Graph* G2);

    static vector<uint> leastFrequentGoTerms(const Graph& G,
                                             double occurrencesFraction);

private:
    static const string biogridGOFile = "go/gene2go";

    vector<double> countWeights;
    double occurrencesFraction;

    void initSimMatrix();

    static void assertNoRepeatedEntries(const vector<vector<uint> >& goTerms);
    static void simpleToInternalFormat(const Graph& G, string GOFileSimpleFormat, string GOFileInternalFormat);
    static void ensureGOFileInternalFormatExists(const Graph& G);

    static ushort numberAnnotatedProteins(const Graph& G);

    static void generateGOFileSimpleFormat(string GOFile, string GOFileSimpleFormat);
    static void generateGene2GoSimpleFormat();

    static bool isBioGRIDNetwork(const Graph& G);
    static bool hasGOData(const Graph& G);

};


#endif //SANA_GOSIMILARITY_HPP
