using namespace std;

#include<time.h>
#include<iostream>
#include<signal.h>

#ifndef COSMINNN
#define COSMINNN
#include "CosminFFT.h"
#endif

#ifndef PTHREAD
#define PTHREAD
#include <pthread.h>
#endif


#ifndef THREAD_MANAGERS
#define THREAD_MANAGERS
#include "Managers/Thread_Manager.h"
#endif  


/****************************************************************************************/
/******************************MemoryModels**********************************************/
/****************************************************************************************/

const int LDV_SZ 		= 8;     	//256
const int UNROLL     	= 4; 
const int ENTRY_SZ   	= UNROLL*entrysz; 

const int PROCS_NR   	= 4;
const int SHIFT_FACT 	= 8;

int PROC_NR;

//const unsigned long sz = (1<<LDV_SZ);

/****************************************************************************************/
/*****************************THREAD MANAGER and MEMORY MODELS***************************/
/****************************************************************************************/

FFThash hash(powdual);

typedef SpMod_SCcore<FFThash> 				SCm_core;
SCm_core 	m_core(hash);
typedef SpMod_SC<FFThash, &m_core, double>		SCm;
SCm			sc_m(ENTRY_SZ);


typedef USpModel<
			SCm, &sc_m,  ItIndAttr_HAfw, USpModelFP
		>
	UMeg;

UMeg umm;

class ThManager : public 
	Master_AbstractTM<ThManager, UMeg, &umm, PROCS_NR, 0, NormalizedChunks<ThManager, UNROLL> > {
  public:
	inline ThManager() { 
		Master_AbstractTM<ThManager, UMeg, &umm, PROCS_NR, 0, NormalizedChunks<ThManager, UNROLL> >::init(); 
	}
};
ThManager 				ttmm;


class Thread_FFT :
				public SpecMasterThread<Thread_FFT, UMeg, ThManager>
{
	private:
		typedef SpecMasterThread< Thread_FFT, UMeg, ThManager >	MASTER_TH;
		typedef Thread_FFT SELF;

		int a;
		double tmp_real, tmp_imag;
		
	public:
		inline void __attribute__((always_inline)) execSynchCode() { 
			tmp_real = w_real; tmp_imag = w_imag;
			double tmp_real1 = tmp_real, tmp_imag1 = tmp_imag; 
			for(int k=0; k<UNROLL; k++) {
				double tmp_real11 = tmp_real1 - s * tmp_imag1 - s2 * tmp_real1;
				tmp_imag1 = tmp_imag1 + s * tmp_real1 - s2 * tmp_imag1;
				tmp_real1 = tmp_real11;
			}
			w_real = tmp_real1; w_imag = tmp_imag1;
		}

		void init() { 
			specLdslow = UMeg::specLDslow<double,Thread_FFT>;
		}
		inline Thread_FFT(): MASTER_TH(UNROLL, NULL)	{ init(); }
		inline Thread_FFT(const unsigned long i, unsigned long* dummy) 
						: MASTER_TH(UNROLL,i,dummy)	{ init(); }

		inline int __attribute__((always_inline)) 
		end_condition() const {
			return (a>=dual);
		}

		inline void __attribute__((always_inline))
		updateInductionVars() {
			a = (this->id-1)*UNROLL + 1;
		}

		double (*specLdslow)(volatile double*, Thread_FFT*, void*);

		inline int __attribute__((always_inline)) 
		iteration_body() {  
			{
				double tmp_real1 = tmp_real - s * tmp_imag - s2 * tmp_real;
				tmp_imag =  tmp_imag + s * tmp_real - s2 * tmp_imag;
				tmp_real = tmp_real1;
			}
			

			for (int b = 0; b < n; b += 2 * dual) {
				int i = 2*(b + a);
				int j = 2*(b + a + dual);
/*
				double z1_real = data[j];
				double z1_imag = data[j+1];
              
				double wd_real = tmp_real * z1_real - tmp_imag * z1_imag;
				double wd_imag = tmp_real * z1_imag + tmp_imag * z1_real;

				double data_i    = data[i];
				double data_ip1  = data[i+1];

				data[i]   = data_i   + wd_real;
				data[i+1] = data_ip1 + wd_imag;

				data[j]   = data_i   - wd_real;
				data[j+1] = data_ip1 - wd_imag;
*/

				double z1_real = this->specLD<SCm,&sc_m>(data+j);
				double z1_imag = this->specLD<SCm,&sc_m>(data+j+1);
              
				//if(z1_real!=0.0 || z1_imag!=0.0) cout<<z1_real<<" "<<z1_imag<<endl;

				double wd_real = tmp_real * z1_real - tmp_imag * z1_imag;
				double wd_imag = tmp_real * z1_imag + tmp_imag * z1_real;

				double data_i    = this->specLD<SCm,&sc_m>(data+i);
				double data_ip1  = this->specLD<SCm,&sc_m>(data+i+1);

				this->specST<SCm,&sc_m>(data+i,   data_i   + wd_real);
				//data[i]   = data_i    + wd_real;
				this->specST<SCm,&sc_m>(data+i+1, data_ip1 + wd_imag);
				//data[i+1] = data_ip1 + wd_imag;
				this->specST<SCm,&sc_m>(data+j,   data_i   - wd_real);
				//data[j]   = data_i   - wd_real;
				this->specST<SCm,&sc_m>(data+j+1, data_ip1 - wd_imag);
				//data[j+1] = data_ip1 - wd_imag;
			}

			a++;
			this->resetThComp();
			//cout<<"r: "<<r<<" sum: "<<y[r]<<endl; 
		}

		inline int __attribute__((always_inline)) 
		non_spec_iteration_body() {}
};

typedef Thread_FFT						SpecTh;

timeval start_time, end_time;
unsigned long running_time = 0;

/** Parallel Speculative**/
int testParallelSpec() {
		sc_m.setIntervalAddr((double*)&data[0], (double*)&data[N-1]+1);

		for(int i=0; i<PROCS_NR; i++) {
			SpecTh* thr = allocateThread<SpecTh>(i, 64);
			thr->init();
			ttmm.registerSpecThread(thr,i);
		}

		cout<<"Spec: ";
		gettimeofday ( &start_time, 0 );
	
		ttmm.speculate<SpecTh>();  

		gettimeofday( &end_time, 0 );
		running_time += DiffTime(&end_time,&start_time);
}


/** Parallel Optimal **/
struct Args { unsigned long start; unsigned long end; };

inline void* ParallelFour(void* args) {
	struct Args* params = (struct Args*)args;

	unsigned long start = params->start; 
	unsigned long end   = params->end;

	//cout<<"New Thread: "<<start<<" "<<end<<endl;

	FFT_loop(start, end);
}
int testOptimalParallel() {
	pthread_t 			pthread[4];
	pthread_attr_t 		thread_attr[4];
	struct sched_param 	thread_param[4];
	
	struct Args			th_args[4];

	unsigned long th_beg = 1; unsigned long third = dual/PROCS_NR; unsigned long th_end = th_beg + third;

	cout<<"Para: ";

	gettimeofday ( &start_time, 0 );

	for(int i=0; i<PROCS_NR; i++) {
			if(FIFO_SCHED)
				Spec_Thread0::setFIFOthread(&thread_attr[i], &thread_param[i]);
			else
				Spec_Thread0::setUSUALthread(&thread_attr[i]);
			
			th_args[i].start = th_beg; th_args[i].end = th_end; 
			int success = pthread_create( &pthread[i], &thread_attr[i], &ParallelFour, &th_args[i] );
			th_beg += third; th_end = th_beg + third;
	}
	for(int i=0; i<PROCS_NR; i++) pthread_join(pthread[i], NULL);

	gettimeofday( &end_time, 0 );
	running_time += DiffTime(&end_time,&start_time);
}

/** Sequential **/

int testSequential() {
	double t1, t2, t3;
	cout<<"Sequ: ";
	//cout<<"Doing: "<<0<<" "<<N<<endl;
	gettimeofday ( &start_time, 0 );

	FFT_loop(1, dual);

	gettimeofday( &end_time, 0 );
	running_time += DiffTime(&end_time,&start_time);
}

pthread_attr_t 		thread_attr;
struct sched_param 	thread_param;
		

int main(int argc, char* argv[]) {	
	init();

	//PROC_NR = atoi(argv[2]);

	if(atoi(argv[1])==33) {
		testSequential();
	} else if(atoi(argv[1])==34) {
		testOptimalParallel();
	} else if(atoi(argv[1])==35) {
		testParallelSpec();
		cout<<"Nr Rollbacks: "<<ttmm.nr_of_rollbacks<<endl;
	}

	cout<<"Running time: "<<running_time<<endl;
	cout<<" data[100]"<<data[100]<<" data[1000]: "<<data[1000]<<" data[10000]: "<<
		data[10000]<<" data[100000]: "<<data[100000]<<"data[999999]: "<<data[999999]<<endl;
	
	return 1;

}

