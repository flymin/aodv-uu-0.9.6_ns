//
// Created by buaa on 12/7/18.
//
#include <vector>
#include "ARModel.h"

ARModel::ARModel(std::vector<double> data, int p) {
    this->data.assign(data.begin(), data.end());
    this->p = p;
}

std::vector<std::vector<double> > ARModel::solveCoeOfAR() {
    std::vector<std::vector<double> > vec;
    ARMAMath ar_math;
    std::vector<double>  arCoe(ar_math.computeARCoe(this->data, this->p));
    vec.push_back(arCoe);
    return vec;
}