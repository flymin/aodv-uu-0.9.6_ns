//
// Created by buaa on 12/7/18.
//


#ifndef ARIMAMODEL_H
#define ARIMAMODEL_H
#include <vector>
#include <cmath>
#include <stdlib.h>
#include <math.h>
#include "ARModel.h"
#include "MAModel.h"
#include "ARMAModel.h"
#include "aodv-uu.h"

class ARIMAModel{
private:
    std::vector<double> dataArray;
    std::vector<double> dataFirDiff;

    std::vector<std::vector<double> > arima;
public:
    ARIMAModel(std::vector<double> dataArray);

    std::vector<double> preFirDiff(std::vector<double> preData);

    std::vector<double> preSeasonDiff(std::vector<double> preData);
    std::vector<double> preDealDiff(int period);

    std::vector<int> getARIMAModel(int period, std::vector<std::vector<int> > notModel, bool needNot);

    int aftDeal(int predictValue, int period);

    double gaussrand();


    int predictValue(int p, int q, int period);

};
#endif //ARIMAMODEL_H
