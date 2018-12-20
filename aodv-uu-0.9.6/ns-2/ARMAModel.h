//
// Created by buaa on 12/7/18.
//
#ifndef ARMAMODEL_H
#define ARMAMODEL_H


#include <vector>
#include "ARMAMath.h"

class ARMAModel{
private:
    std::vector<double> data;
    int p;
    int q;

public:
    ARMAModel(std::vector<double> data, int p, int q);

    std::vector<std::vector<double> > solveCoeOfARMA();
};
#endif //ARMAMODEL_H
