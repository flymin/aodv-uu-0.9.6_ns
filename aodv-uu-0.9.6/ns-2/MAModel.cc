//
// Created by buaa on 12/7/18.
//

#include "MAModel.h"


MAModel::MAModel(std::vector<double> data, int p) {
    this->data=data;
    this->p=p;
}

std::vector<std::vector<double> > MAModel::solveCoeOfMA() {
    std::vector<std::vector<double> > vec;
    ARMAMath ar_math;
    std::vector<double>  maCoe(ar_math.computeMACoe(this->data,this->p));
    vec.push_back(maCoe);
    return vec;
}
