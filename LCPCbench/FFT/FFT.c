using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include<time.h>
#include<iostream>
#include<signal.h>

#include "FFT.h"

#define PI  3.1415926535897932

/*-----------------------------------------------------------------------*/

static int int_log2(int n);

double FFT_num_flops(int N)
{

     double Nd = (double) N;
     double logN = (double) int_log2(N);

     return (5.0*Nd-2)*logN + 2*(Nd+1);
}

static int int_log2 (int n)
{
    int k = 1;
    int log = 0;
    for(/*k=1*/; k < n; k *= 2, log++);
    if (n != (1 << log))
    {
      printf("FFT: Data length is not a power of 2!: %d ",n);
      exit(1);
    }
    return log; 
}


//int rec[256][8192][2];


static void FFT_loop(int N, double* data, int direction, int dual) {
	int n = N/2;
	double w_real = 1.0;
	double w_imag = 0.0;
	int a;
	int b;

	double theta = 2.0 * direction * PI / (2.0 * (double) dual);
	double s = sin(theta);
	double t = sin(theta / 2.0);
	double s2 = 2.0 * t * t;

	int count_a = 0, count_b = 0;

	for (a = 1; a < dual; a++) {
		count_b = 0;

		/* trignometric recurrence for w-> exp(i theta) w */
		{
			double tmp_real = w_real - s * w_imag - s2 * w_real;
			double tmp_imag = w_imag + s * w_real - s2 * w_imag;
			w_real = tmp_real;
			w_imag = tmp_imag;
		}
		for (b = 0; b < n; b += 2 * dual) {
			int i = 2*(b + a);
			int j = 2*(b + a + dual);

			//rec[count_a][count_b][0] = i;
			//rec[count_a][count_b][1] = j;

			count_b++;


			double z1_real = data[j];
			double z1_imag = data[j+1];
              
			double wd_real = w_real * z1_real - w_imag * z1_imag;
			double wd_imag = w_real * z1_imag + w_imag * z1_real;

			double data_i    = data[i];
			double data_ip1  = data[i+1];

			data[j]   = data_i   - wd_real;
			data[j+1] = data_ip1 - wd_imag;
			data[i]   = data_i   + wd_real;
			data[i+1] = data_ip1 + wd_imag;
		}
		count_a++;
		//cout<<"Internal loop iter nr: "<<count_b<<endl;
	}
	cout<<"External loop iter nr: "<<count_a<<endl;
	cout<<"Internal loop iter nr: "<<count_b<<endl;
	
}



int main() {
	const int S = 1024*1024*8;
	double* d = new double[S];
	
	FFT_loop(S, d, 1, 2048*2);
}

static void FFT_transform_internal (int N, double *data, int direction) {
    int n = N/2;
    int bit = 0;
    int logn;
    int dual = 1;

    if (n == 1) return;         /* Identity operation! */
    logn = int_log2(n);


    if (N == 0) return;    

    /* bit reverse the input data for decimation in time algorithm */
    FFT_bitreverse(N, data) ;

    /* apply fft recursion */
    /* this loop executed int_log2(N) times */
    for (bit = 0; bit < logn; bit++, dual *= 2) {
      double w_real = 1.0;
      double w_imag = 0.0;
      int a;
      int b;

      double theta = 2.0 * direction * PI / (2.0 * (double) dual);
      double s = sin(theta);
      double t = sin(theta / 2.0);
      double s2 = 2.0 * t * t;

      for (a=0, b = 0; b < n; b += 2 * dual) {
        int i = 2*b ;
        int j = 2*(b + dual);

        double wd_real = data[j] ;
        double wd_imag = data[j+1] ;
          
        data[j]   = data[i]   - wd_real;
        data[j+1] = data[i+1] - wd_imag;
        data[i]  += wd_real;
        data[i+1]+= wd_imag;
      }
      
      /* a = 1 .. (dual-1) */
      for (a = 1; a < dual; a++) {
        /* trignometric recurrence for w-> exp(i theta) w */
        {
          double tmp_real = w_real - s * w_imag - s2 * w_real;
          double tmp_imag = w_imag + s * w_real - s2 * w_imag;
          w_real = tmp_real;
          w_imag = tmp_imag;
        }
        for (b = 0; b < n; b += 2 * dual) {
          int i = 2*(b + a);
          int j = 2*(b + a + dual);

          double z1_real = data[j];
          double z1_imag = data[j+1];
              
          double wd_real = w_real * z1_real - w_imag * z1_imag;
          double wd_imag = w_real * z1_imag + w_imag * z1_real;

		  double data_i    = data[i];
		  double data_ip1  = data[i+1];

          data[j]   = data_i   - wd_real;
          data[j+1] = data_ip1 - wd_imag;
          data[i]   = data_i   + wd_real;
          data[i+1] = data_ip1 + wd_imag;

        }
      }
    }
  }


static void FFT_transform_internal_Mod (int N, double *data, int direction) {
    int n = N/2;
    int bit = 0;
    int logn;
    int dual = 1;

    if (n == 1) return;         /* Identity operation! */
    logn = int_log2(n);


    if (N == 0) return;    

    /* bit reverse the input data for decimation in time algorithm */
    FFT_bitreverse(N, data) ;

    /* apply fft recursion */
    /* this loop executed int_log2(N) times */
    for (bit = 0; bit < logn; bit++, dual *= 2) {
		cout<<"PASS : "<<bit<<"!!!!"<<endl;
		
      double w_real = 1.0;
      double w_imag = 0.0;
      int a;
      int b;

      double theta = 2.0 * direction * PI / (2.0 * (double) dual);
      double s = sin(theta);
      double t = sin(theta / 2.0);
      double s2 = 2.0 * t * t;

      for (a=0, b = 0; b < n; b += 2 * dual) {
        int i = 2*b ;
        int j = 2*(b + dual);

        double wd_real = data[j] ;
        double wd_imag = data[j+1] ;
          
        data[j]   = data[i]   - wd_real;
        data[j+1] = data[i+1] - wd_imag;
        data[i]  += wd_real;
        data[i+1]+= wd_imag;
      }
      
      /* a = 1 .. (dual-1) */
      for (a = 1; a < dual; a++) {
		cout<<endl;
		cout<<"a : "<<a<<"!!!!"<<endl;
        /* trignometric recurrence for w-> exp(i theta) w */
        {
          double tmp_real = w_real - s * w_imag - s2 * w_real;
          double tmp_imag = w_imag + s * w_real - s2 * w_imag;
          w_real = tmp_real;
          w_imag = tmp_imag;
        }
        for (b = 0; b < n; b += 2 * dual) {
          int i = 2*(b + a);
          int j = 2*(b + a + dual);

			cout<<"(i,j: "<<i<<" "<<j<<") ";

          double z1_real = data[j];
          double z1_imag = data[j+1];
              
          double wd_real = w_real * z1_real - w_imag * z1_imag;
          double wd_imag = w_real * z1_imag + w_imag * z1_real;

          data[j]   = data[i]   - wd_real;
          data[j+1] = data[i+1] - wd_imag;
          data[i]  += wd_real;
          data[i+1]+= wd_imag;
        }
      }
    }
  }


void FFT_bitreverse(int N, double *data) {
    /* This is the Goldrader bit-reversal algorithm */
    int n=N/2;
    int nm1 = n-1;
    int i=0; 
    int j=0;
    for (; i < nm1; i++) {

      /*int ii = 2*i; */
      int ii = i << 1;

      /*int jj = 2*j; */
      int jj = j << 1;

      /* int k = n / 2 ; */
      int k = n >> 1;

      if (i < j) {
        double tmp_real    = data[ii];
        double tmp_imag    = data[ii+1];
        data[ii]   = data[jj];
        data[ii+1] = data[jj+1];
        data[jj]   = tmp_real;
        data[jj+1] = tmp_imag; }

      while (k <= j) 
      {
        /*j = j - k ; */
        j -= k;

        /*k = k / 2 ;  */
        k >>= 1 ; 
      }
      j += k ;
    }
  }


void FFT_transform(int N, double *data)
{
    FFT_transform_internal(N, data, -1);
}


void FFT_inverse(int N, double *data)
{
    int n = N/2;
    double norm = 0.0;
    int i=0;
    //FFT_transform_internal(N, data, +1);

	FFT_transform_internal_Mod(N, data, +1);

    /* Normalize */


    norm=1/((double) n);
    for(i=0; i<N; i++)
      data[i] *= norm;
  
}

/*
int main() {
	double data[1024];
	int N = 1024;
		FFT_inverse(N, data);
}
*/
