#include <math.h>

/** DataStructures **/

const int N = 1;

const int U_SIZE  = 1024*4; //1024*4;//1024; //for FW *7
const int U_SIZE1 = 1024*7;
const int MAXPATS = 1;

double  in_pats[MAXPATS][U_SIZE];      /* input patterns */

double  out_error[U_SIZE];
double  mid_out[U_SIZE];              /* middle layer output */
double  mid_error[U_SIZE];            /* middle layer errors */
double  mid_wts[U_SIZE][U_SIZE];     /* middle layer weights */
double  out_wt_change[U_SIZE][U_SIZE]; /* storage for last wt change */
double  out_wts[U_SIZE][U_SIZE];    /* output layer weights */
double  out_wt_cum_change[U_SIZE][U_SIZE]; /* accumulated wt changes */

#define BETA 0.09		/* beta learning constant */
#define ALPHA 0.09		/* momentum term constant */


void init() {
	for(int i=0; i<U_SIZE; i++) {
		in_pats[0][i] = (double)i; mid_out[i] = (double)i; mid_error[i] = (double)i; out_error[i] = i; 
		for(int j=0; j<U_SIZE; j++)  {
			mid_wts[i][j] = (double)j; out_wt_change[i][j] = (double)j; 
			out_wts[i][j] = (double)j; out_wt_cum_change[i][j] = (double)j;
		}
	}
}

inline void __attribute__((always_inline))
unified_do_forward_pass(int neurode) {
	double  sum, sum1;
	int     i;

//	for (neurode=0;neurode<U_SIZE; neurode++)
//	{
        sum = 0.0;
        for (i=0; i<U_SIZE; i++)
        {       /* compute weighted sum of input signals */
                sum += mid_wts[neurode][i]*in_pats[0][i];
        }
        /*
        ** apply sigmoid function f(x) = 1/(1+exp(-x)) to weighted sum
        */
        sum = 1.0/(1.0+exp(-sum));
        mid_out[neurode] = sum;

		/**********************************************************************/
/*
        sum = 0.0;
        for (i=0; i<U_SIZE; i++)
        {       
                sum += out_wts[neurode][i]*mid_out[i];
        }
        sum = 1.0/(1.0+exp(-sum));
        out_out[neurode] = sum;
*/
//	}

	return;
}



inline void __attribute__((always_inline))
unified_do_backward_pass(int neurode) {
	int i;
	double error,tot_error, sum, sum1;
	int weight;
	double learn,delta,alph;
	tot_error = 0.0;
	sum = 0.0;
	learn = BETA;
	alph  = ALPHA;

//	for (neurode=0; neurode<U_SIZE; neurode++) {
/*
        out_error[neurode] = out_pats[patt][neurode] - out_out[neurode];
        
        error = out_error[neurode];
        if (error <0.0)
        {
                sum += -error;
                if (-error > tot_error)
                        tot_error = -error; 
        }
        else
        {
                sum += error;
                if (error > tot_error)
                        tot_error = error; 
        }
*/

		/**********************************************************************/
/*
        sum1 = 0.0;
        for (i=0; i<U_SIZE; i++)
                sum1 += out_wts[i][neurode]*out_error[i];

        mid_error[neurode] = mid_out[neurode]*(1-mid_out[neurode])*sum1;
*/
		/**********************************************************************/
		for(int k=0;k<N; k++) {
        for (weight=0; weight<U_SIZE; weight++) {
                delta = learn * out_error[neurode] * mid_out[weight];
                delta += alph * out_wt_change[neurode][weight];

                out_wts[neurode][weight] += delta;
                out_wt_cum_change[neurode][weight] += delta;
        }
		}

		/**********************************************************************/
/*
        for (weight=0; weight<U_SIZE; weight++) {                delta = learn * mid_error[neurode] * in_pats[0][weight];
                delta += alph * mid_wt_change[neurode][weight];
                mid_wts[neurode][weight] += delta;
                mid_wt_cum_change[neurode][weight] += delta;
        }
*/
//	}

	return;
}


