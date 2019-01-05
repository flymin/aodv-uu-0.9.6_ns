#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "svm.h"
#include "aodv-uu.h"

int print_null(const char *s,...) {return 0;}
static int (*info)(const char *fmt,...) = &printf;
struct svm_node *x;
int max_nr_attr = 64;
struct svm_model* model;
int predict_probability=0;


char train_model[100];		// added by czy

/* modify by 16071070 czy */
double NS_CLASS predict(struct svm_node *x)
{
    int correct = 0;
    int total = 0;
    double error = 0;
    double sump = 0, sumt = 0, sumpp = 0, sumtt = 0, sumpt = 0;

    int svm_type=svm_get_svm_type(model);
    int nr_class=svm_get_nr_class(model);
    double *prob_estimates=NULL;
    int j;
    double predict_label;
    predict_label = svm_predict(model, x);
    return predict_label;

}

//main function for svm
/* modify by 16071070 czy */
int NS_CLASS final_predict(float a, float b, float c, double d)
{

    int i;
    double result;
    // parse options
    predict_probability = 0;		// modified by czy

    strcpy(train_model, "/home/buaa/ns-allinone-2.35/ns-2.35/aodv-uu-0.9.6/ns-2/STABILITY_MODEL");
    train_model[strlen(train_model)] = 0;


    if ((model = svm_load_model(train_model)) == 0)				//modified by czy
    {
        fprintf(stderr,"can't open model file %s\n",train_model);
        exit(1);
    }


    x = (struct svm_node *) malloc(5*sizeof(struct svm_node));				// one node at a time

    x[0].index = 0;														// use it as end of kernel function calc
    x[0].value = a;
    x[1].index = 1;														// use it as end of kernel function calc
    x[1].value = b;
    x[2].index = 2;														// use it as end of kernel function calc
    x[2].value = c;
    x[3].index = 3;														// use it as end of kernel function calc
    x[3].value = d;
    x[4].index = -1;														// use it as end of kernel function calc
    x[4].value = 0.0;
    result = predict(x);
    //printf("svm predict result is: %lf\n", svm_predict_result); /* added by 16071070 czy */
    svm_free_and_destroy_model(&model);
    free(x);
    
    fprintf(stderr, "|svm_predict_result=%lf ", svm_predict_result);
    return svm_predict_result/1;
    //return rand()%2;
}
