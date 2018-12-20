//
// Created by yue on 18-3-16.
//

#include <vector>
#include <math.h>
#include "cstdio"
#include "iostream"
#include "fstream"
#include "ARIMAModel.h"
#include "aodv-uu.h"

float NS_CLASS armamain(int array[], int len) {
    std::vector<double> dataArray(array, array + len);

    ARIMAModel arima(dataArray);
    int period = 7;
    int modelCnt = 7;
    int cnt = 0;
    std::vector <std::vector<int> > list;
    std::vector<int> tmpPredict(modelCnt);
    for (
            int k = 0;
            k < modelCnt;
            ++k)            //控制通过多少组参数进行计算最终的结果
    {
        std::vector<int> bestModel = arima.getARIMAModel(period, list, (k == 0) ? false : true);
        if (bestModel.size() == 0) {
            tmpPredict[k] = (int) dataArray[dataArray.size() - period];
            cnt++;
            break;
        } else {
            int predictDiff = arima.predictValue(bestModel[0], bestModel[1], period);
            tmpPredict[k] = arima.aftDeal(predictDiff, period
            );
            cnt++;
        }
        list.push_back(bestModel);
    }
    double sumPredict = 0.0;
    for (int k = 0;k < cnt;++k) {
        sumPredict += ((double) tmpPredict[k]) / (double)cnt;
    }
    int predict = (int) round(sumPredict);
    //fprintf(stderr,"ARIMA TEST: predict %d\n", predict);
    return float(predict);
}

/*
int NS_CLASS armamain(int array[], int len){
    std::vector<double> dataArray(array, array + len);

    ARIMAModel* arima = new ARIMAModel(dataArray);

    int period = 7;
    int modelCnt=5;
    int cnt=0;
    std::vector< std::vector<int> > list;
    std::vector<int> tmpPredict(modelCnt);

    for (int k = 0; k < modelCnt; ++k)			//控制通过多少组参数进行计算最终的结果
    {
        std::vector<int> bestModel = arima->getARIMAModel(period, list, (k == 0) ? false : true);
        //std::cout<<bestModel.size()<<std::endl;

        if (bestModel.size() == 0)
        {
            tmpPredict[k] = (int)dataArray[dataArray.size() - period];
            cnt++;
            break;
        }
        else
        {
            //std::cout<<bestModel[0]<<bestModel[1]<<std::endl;
            int predictDiff = arima->predictValue(bestModel[0], bestModel[1], period);
            //std::cout<<"fuck"<<std::endl;
            tmpPredict[k] = arima->aftDeal(predictDiff, period);
            cnt++;
        }
        //std::cout<<bestModel[0]<<" "<<bestModel[1]<<endl;
        list.push_back(bestModel);
    }

    double sumPredict = 0.0;
    for (int k = 0; k < cnt; ++k)
    {
        fprintf(stderr, "here\n");
        sumPredict += ((double)tmpPredict[k])/(double)cnt;
    }
    int predict = (int)round(sumPredict);
   //std::cout<<"Predict value="<<predict<<endl;
    return predict;
}
 */