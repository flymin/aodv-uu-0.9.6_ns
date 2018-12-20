//
// Created by buaa on 12/7/18.
//

#ifndef ARMODEL_H
#define ARMODEL_H

#include <vector>
#include "ARMAMath.h"

class ARModel{
private:
    std::vector<double> data;
    int p;

public:
    ARModel(std::vector<double> data, int p);

    std::vector<std::vector<double> > solveCoeOfAR();
};


#endif //ARMODEL_H
