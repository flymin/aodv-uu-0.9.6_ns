//
// Created by buaa on 12/7/18.
//
#ifndef MAMODEL_H
#define MAMODEL_H

#include <vector>
#include "ARMAMath.h"

class MAModel{
private:
    std::vector<double> data;
    int p;

public:
    MAModel(std::vector<double> data,int p);

    std::vector<std::vector<double> > solveCoeOfMA();
};


#endif //MAMODEL_H