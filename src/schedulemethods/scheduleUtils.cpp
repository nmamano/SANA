#include <vector>
#include <memory>

#include "scheduleUtils.hpp"
#include "ScheduleMethod.hpp"
#include "LinearRegressionVintage.hpp"
#include "LinearRegressionModern.hpp"
#include "PBadBinarySearch.hpp"
#include "Ameur.hpp"
#include "IteratedAmeur.hpp"
#include "StatisticalTest.hpp"
#include "../utils/utils.hpp"

using namespace std;

unique_ptr<ScheduleMethod> getScheduleMethod(string name) {
    if (name == LinearRegressionVintage::NAME) 
        return unique_ptr<LinearRegressionVintage>(new LinearRegressionVintage());
    if (name == LinearRegressionModern::NAME)  
        return unique_ptr<LinearRegressionModern>(new LinearRegressionModern());
    if (name == PBadBinarySearch::NAME)        
        return unique_ptr<PBadBinarySearch>(new PBadBinarySearch());
    if (name == Ameur::NAME)                   
        return unique_ptr<Ameur>(new Ameur());
    if (name == IteratedAmeur::NAME)           
        return unique_ptr<IteratedAmeur>(new IteratedAmeur());
    if (name == StatisticalTest::NAME)         
        return unique_ptr<StatisticalTest>(new StatisticalTest());

    throw runtime_error("schedule method "+name+" not found");
}

void scheduleMethodComparison(SANA *const sana) {

    //customizable parameters
    vector<string> testedMethodNames = {
         LinearRegressionVintage::NAME, 
         LinearRegressionModern::NAME,
         PBadBinarySearch::NAME,
         Ameur::NAME,
         IteratedAmeur::NAME,
         StatisticalTest::NAME
     };

    double targetInitialPBad = 0.95;
    double targetFinalPBad = 0.0001;
    int runsPerMethod = 1;
    double maxTime = 60;
    int maxSamples = 60;
    double sampleTime = 2; //max time for getPBad to reach equilibrium
    double errorTol = 0.01;
    int numValidationSamples = 5; //set to a high value for final experiment (30?)
            //to find the *real* pBad at the temperatures given by the methods
            //we compute a normal distribution of this many samples


    ScheduleMethod::setSana(sana);

    vector<vector<string>> table;
    table.push_back({"Method",
        "TInitial","x","PBadMean","PBadSD","Accuracy","Samp","Time",
        "TFinal","x","PBadMean","PBadSD","Accuracy","Samp","Time",
        "AllSamp","AllTime"});

    Timer T;
    T.start();
    for (string methodName : testedMethodNames) {
        auto method = getScheduleMethod(methodName);
        method->setTargetInitialPBad(targetInitialPBad);
        method->setTargetFinalPBad(targetFinalPBad);
        method->setSampleTime(sampleTime);
        method->setErrorTol(errorTol);

        for (int i = 0; i < runsPerMethod; i++) {
            table.push_back(methodData(method, maxTime, maxSamples, numValidationSamples, sampleTime));
        }
    }
    double totalTime = T.elapsed();


    // ScheduleMethod::tempsFromRegressionAllSamples({});


    //apply LR on all the samples taken by all the methods
    // LinearRegressionModern LRM(sana);
    // LRM.setTInitialTFinalFromRegression(true);
    // int totalSamples = ScheduleMethod::allTempToPBad.size();

    // table.push_back(methodDataForComparison("superLR", totalTime, -1, totalSamples, -1, true));
    
    cout << endl;
    cout << "=================================================" << endl;
    cout << "=================================================" << endl;
    cout << "=================================================" << endl;
    cout << endl;

    cout << "Automatic Temperature Schedule Comparison" << endl;
    cout << "Max time: " << maxTime << " Max samples: " << maxSamples << endl;
    cout << "Sample time: " << sampleTime << endl;

    cout << "Target Initial PBad: " << targetInitialPBad << " range ";
    ScheduleMethod::printTargetRange(targetInitialPBad, errorTol);
    cout << endl << "Target Final PBad:   " << targetFinalPBad << " range ";
    ScheduleMethod::printTargetRange(targetFinalPBad, errorTol);
    cout << endl << endl;

    printTable(table, 1, cout);

    cout << endl;
    cout << "=================================================" << endl;
    cout << "=================================================" << endl;
    cout << "=================================================" << endl;
    cout << endl;
}

vector<string> methodData(const unique_ptr<ScheduleMethod>& method, double maxTime, int maxSamples, int numValidationSamples, double sampleTime) {

    bool singleTime = method->getName() == LinearRegressionVintage::NAME or
                      method->getName() == LinearRegressionModern::NAME;

    double TInitial, TFinal;
    if (singleTime) {
        TInitial = method->computeTInitial(maxTime, maxSamples);
        TFinal = method->computeTFinal(0, 0);
    } else {
        TInitial = method->computeTInitial(maxTime/2, maxSamples/2);
        TFinal = method->computeTFinal(maxTime/2, maxSamples/2);
    }

    NormalDistribution TIniPBadDis = getPBadDis(TInitial, numValidationSamples, sampleTime);
    double TIniPBadAccuracy = TIniPBadDis.getMean()/method->targetInitialPBad;
    bool TIniSuccess = method->isWithinTargetRange(TIniPBadDis.getMean(), method->targetInitialPBad);

    NormalDistribution TFinPBadDis = getPBadDis(TFinal, numValidationSamples, sampleTime);
    double TFinPBadAccuracy = TFinPBadDis.getMean()/method->targetFinalPBad;
    bool TFinSuccess = method->isWithinTargetRange(TFinPBadDis.getMean(), method->targetFinalPBad);

    vector<pair<double,int>> dataAndPrec =
        {
          {TInitial, 6}, {(double)TIniSuccess, 0}, {TIniPBadDis.getMean(), 6}, {TIniPBadDis.getSD(), 6},
          {TIniPBadAccuracy, 6}, {(double)method->TInitialSamples, 0}, {method->TInitialTime, 2},

          {TFinal, 9}, {(double)TFinSuccess, 0}, {TFinPBadDis.getMean(), 9}, {TFinPBadDis.getSD(), 9},
          {TFinPBadAccuracy, 6}, {(double)method->TFinalSamples, 0}, {method->TFinalTime, 2},

          {(double)method->totalSamples(), 0}, {method->totalTime(), 2}
      };    

    vector<string> row = {method->getName()};
    if (method->getName() == LinearRegressionVintage::NAME) row[0] = "LR-vintage";
    if (method->getName() == LinearRegressionModern::NAME) row[0] = "LR-modern";

    for (auto p : dataAndPrec) {
        double val = p.first;
        double precision = p.second;
        row.push_back(toStringWithPrecision(val, precision));
    }
    
    if (singleTime) {
        row[6] = row[7] = row[13] = row[14] = "NA";
    }

    return row;

}

NormalDistribution getPBadDis(double temp, int numSamples, double sampleTime) {
    vector<double> samples;
    for (int i = 0; i < numSamples; i++) {
        samples.push_back(ScheduleMethod::sGetPBad(temp, sampleTime));
    } 
    return NormalDistribution(samples);
}
