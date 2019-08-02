#include <iostream>
#include <vector>

#include "IteratedAmeur.hpp"
#include "../utils/utils.hpp"
#include "Ameur.hpp"

using namespace std;

IteratedAmeur::IteratedAmeur(SANA *const sana) :
    ScheduleMethod(sana) {} 

void IteratedAmeur::computeTInitial() {
    TInitial = computeTempForPBad(targetInitialPBad);
}

void IteratedAmeur::computeTFinal() {
    TFinal = computeTempForPBad(targetFinalPBad);
}

//the method from the ameur paper computes a temperature that "fits" a target pbad for a given sample of EIncs
//james' idea is to iterate this process until convergence: using the resulting temperature,
//generate a new sample of EIncs by running at that temperature, 
//and use the ameur method again to find a temperature that "fits" the target pbad with the new EIncs
//this converges to the temperature that gives rise to that pbad at equilibrium
//step size should be <= 1
//with bigger step sizes, there may be a "bounce back and forth" effect:
double IteratedAmeur::computeTempForPBad(double targetPBad) {
    //configuration
    int maxIters = 30;
    double stepSize = 0.6;
    double startTempGuess = 1;
    double errorTolerance = Ameur::tolerance(targetPBad);

    cout << "Using iterated Ameur method" << endl;

    double tempGuess = startTempGuess;
    bool converged = false;
    int iteration = 0;

    while (not converged and iteration < maxIters) {
        cout << "Iteration " << iteration << ":" << endl;

        Ameur ameur(sana);
        double tempGuessPBad; //set by reference in call below
        vector<double> EIncs = ameur.getEIncSample(tempGuess, 10000, tempGuessPBad);

        converged = tempGuessPBad > targetPBad*(1-errorTolerance) and tempGuessPBad < targetPBad*(1+errorTolerance);
        if (converged) break;

        double nextTempGuess = tempGuess + stepSize*(ameur.computeTempForPBad(targetPBad, tempGuess, EIncs) - tempGuess);

        tempGuess = nextTempGuess;
        iteration++;
    }
    if (converged) {
        cout << "Iterated Ameur method converged after " << iteration << " iterations" << endl;
    } else {
        cout << "Iterated Ameur method did NOT converge after " << iteration << " iterations" << endl;
    }

    return tempGuess;
}