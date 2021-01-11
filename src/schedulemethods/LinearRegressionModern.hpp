#ifndef LINEARREGRESSIONMODERN_HPP
#define LINEARREGRESSIONMODERN_HPP

#include "ScheduleMethod.hpp"

/* New Linear Regression implementation better integrated with the other schedule methods */
class LinearRegressionModern : public ScheduleMethod {
public:
    LinearRegressionModern();

    static constexpr auto NAME = "linear-regression-modern";
    static const int EXTRA_SAMPLES = 4;
    virtual string getName() override { return NAME; }

    void setTargetInitialPBad(double pBad) override; 
    void setTargetFinalPBad(double pBad) override;
protected:
    void vComputeTInitial(Resources maxRes) override;
    void vComputeTFinal(Resources maxRes) override;

    virtual void computeBoth(Resources maxRes);

private:
    bool alreadyComputed;


};



#endif

