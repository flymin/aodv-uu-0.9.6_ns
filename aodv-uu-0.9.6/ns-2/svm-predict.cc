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

//static char *line = NULL;
//static int max_line_len;

char pretrained_model[100];		// added by zwy
/*
static char* readline(FILE *input)
{
	int len;

	if(fgets(line,max_line_len,input) == NULL)
		return NULL;

	while(strrchr(line,'\n') == NULL)
	{
		max_line_len *= 2;
		line = (char *) realloc(line,max_line_len);
		len = (int) strlen(line);
		if(fgets(line+len,max_line_len-len,input) == NULL)
			break;
	}
	return line;
}
*/
//void predict(FILE *input, FILE *output)		//modified by zwy
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
    // modified by zwy
    double predict_label;
    predict_label = svm_predict(model, x);
    return predict_label;

}
//	modified by zwy, this function is useless


//int main(int argc, char **argv)
int NS_CLASS svm_predict_main(float a, float b, float c, double d)
{
    //FILE *input, *output;
    int i;
    double svm_predict_result;
    // parse options
    predict_probability = 0;		// modified by zwy
    //strcpy_s(pretrained_model, 100, "E:\\WirelessSys\\svm_draft.txt.model");
    strcpy(pretrained_model, "/home/buaa/ns-allinone-2.35/ns-2.35/aodv-uu-0.9.6/ns-2/fake.model");
    pretrained_model[strlen(pretrained_model)] = 0;

    //if((model=svm_load_model(argv[i+1]))==0)
    if ((model = svm_load_model(pretrained_model)) == 0)				//modified by zwy
    {
        fprintf(stderr,"can't open model file %s\n",pretrained_model);
        exit(1);
    }

    //x = (struct svm_node *) malloc(max_nr_attr*sizeof(struct svm_node));	//can we change max_nr_attr to 1?  by zwy
    x = (struct svm_node *) malloc(5*sizeof(struct svm_node));				// one node at a time
    for (i = 0; i < 3; i++) {												//��ʱֻ֧��3������ֵ
        //x[i].value = input_node_info[i];
        //x[i].index = i;
    }
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


    svm_predict_result = predict(x);
    //printf("svm predict result is: %lf\n", svm_predict_result);
    svm_free_and_destroy_model(&model);		//	Is this really neccessary��multi time call only! by zwy
    free(x);

    return svm_predict_result/1;
}
