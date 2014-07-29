using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include<time.h>
#include<iostream>
#include<signal.h>

#define PI  3.1415926535897932

/*-----------------------------------------------------------------------*/

const unsigned long N = 1024*1024*128; //8; //*4;
volatile double data[N];
const int dual = 2048*2; //2048; //256; 
const int powdual = 12; //11; //8;
const int direction = 1;
const int entrysz = 2048*16; //512*4; //1024*4; //8192*4;

int n; double  theta, s, t, s2;

volatile double w_real, w_imag;

void init() {
	n = N/2;
	w_real = 1.0; w_imag = 0.0;

	theta = 2.0 * direction * PI / (2.0 * (double) dual);
	s = sin(theta); t = sin(theta / 2.0);
	s2 = 2.0 * t * t;	
	for(int i=0; i<N; i++) data[i] = (double)i*3/2;

	cout<<"w_real: "<<w_real<<" w_imag: "<<w_imag<<endl;
} 

static void FFT_loop(int start, int end/*int N, double* data, int direction, int dual*/) {
	int a; int b;
	int count_a = 0, count_b = 0;

	//for (a = 1; a < dual; a++) {
	for(a=start; a<end; a++) {
		//count_b = 0;
		//count_a++;

		/* trignometric recurrence for w-> exp(i theta) w */
		{
			double tmp_real = w_real - s * w_imag - s2 * w_real;
			double tmp_imag = w_imag + s * w_real - s2 * w_imag;
			w_real = tmp_real;
			w_imag = tmp_imag;
		}

		

		for (b = 0; b < n; b += 2 * dual) {
			//count_b++;
				int i = 2*(b + a);
				int j = 2*(b + a + dual);

				double z1_real = data[j];
				double z1_imag = data[j+1];
              
				double wd_real = w_real * z1_real - w_imag * z1_imag;
				double wd_imag = w_real * z1_imag + w_imag * z1_real;

				double data_i    = data[i];
				double data_ip1  = data[i+1];

				data[i]   = data_i   + wd_real;
				data[i+1] = data_ip1 + wd_imag;

				data[j]   = data_i   - wd_real;
				data[j+1] = data_ip1 - wd_imag;
				

		}
		//cout<<"Internal loop iter nr: "<<count_b<<endl;
	}
	//cout<<"External loop iter nr: "<<count_a<<endl;
	//cout<<"Internal loop iter nr: "<<count_b<<endl;
	
}
