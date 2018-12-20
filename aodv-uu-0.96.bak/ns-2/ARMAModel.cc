//
// Created by buaa on 12/7/18.
//
#include <stdio.h>
#include "ARMAModel.h"

ARMAModel::ARMAModel(std::vector<double> data, int p, int q) {
	this->data = data;
	this->p = p;
	this->q = q;
}

std::vector<std::vector<double> > ARMAModel::solveCoeOfARMA() {
	std::vector<std::vector<double> > vec;
	ARMAMath ar_math;
//fprintf(stderr, "begin solve coe of arma\n");
	std::vector<double> armaCoe(ar_math.computeARMACoe(this->data, p, q));
//    fprintf(stderr, "end computeARMAcoe\n");
	std::vector<double> arCoe(this->p + 1);
	for (int i = 0; i < arCoe.size(); i++) arCoe[i] = armaCoe[i];

	std::vector<double>  maCoe(this->q + 1);

	for (int i = 0; i < maCoe.size(); i++) {
		maCoe[i] = armaCoe[i + this->p + 1];
	}
	vec.push_back(arCoe);
	vec.push_back(maCoe);

	return vec;
}
