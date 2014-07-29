using namespace std;

#include<time.h>
#include<iostream>
#include<signal.h>

#ifndef COSMINNN
#define COSMINNN
#include "CosminSparseMult.h"
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
#define FIFO_SCHED true
const int R = 1;
const int LDV_SZ 		= 6;     	//256
const int UNROLL     	= 32; //256; 
const int ENTRY_SZ   	= UNROLL*R; 

const int PROCS_NR   	= 4;
const int SHIFT_FACT 	= 5;


//const unsigned long sz = (1<<LDV_SZ);

/****************************************************************************************/
/*****************************THREAD MANAGER and MEMORY MODELS***************************/
/****************************************************************************************/

HashSeqPow2 hash(LDV_SZ, SHIFT_FACT);

typedef SpMod_SCcore<HashSeqPow2> 				SCm_core;
SCm_core 	m_core(hash);
typedef SpMod_SC<HashSeqPow2, &m_core, double>		SCm;
SCm			sc_m(ENTRY_SZ);
/*
typedef SpMod_SCcore<HashSeqPow2> 				SCm_core_ro;
SCm_core_ro 	m_core_ro(hash);
typedef SpMod_SC<HashSeqPow2, &m_core_ro, int>		SpROI;
typedef SpMod_SC<HashSeqPow2, &m_core_ro, double>	SpROD;

SpROI			ro_m_i(16);
SpROD           ro_m_d(16);
*/


typedef SpMod_ReadOnly<double> SpROD;
SpROD	ro_m_d;

typedef SpMod_ReadOnly<int> SpROI;
SpROI	ro_m_i;


typedef USpModel<
			SCm, &sc_m,  ItIndAttr_HAfw, 
			USpModel<
				SpROD, &ro_m_d, 0/*ItIndAttr_HAfw*/, 
				USpModel<
					SpROI, &ro_m_i, 0 /*ItIndAttr_HAfw*/, USpModelFP
				>
			>
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


class Thread_SparseMult :
				public SpecMasterThread<Thread_SparseMult, UMeg, ThManager>
{
	private:
		typedef SpecMasterThread< Thread_SparseMult, UMeg, ThManager >	MASTER_TH;
		typedef Thread_SparseMult SELF;

		int r;
		
	public:
		void init() { 
			specLdslow = UMeg::specLDslow<double,Thread_SparseMult>;
		}
		inline Thread_SparseMult(): MASTER_TH(UNROLL, NULL)	{ init(); }
		inline Thread_SparseMult(const unsigned long i, unsigned long* dummy) 
						: MASTER_TH(UNROLL,i,dummy)	{ init(); }

		inline int __attribute__((always_inline)) 
		end_condition() const {
			return (r>=N);
		}

		inline void __attribute__((always_inline))
		updateInductionVars() {
			r = (this->id-1)*UNROLL;
		}

		double (*specLdslow)(volatile double*, Thread_SparseMult*, void*);

		inline int __attribute__((always_inline)) 
		iteration_body() {  
			double sum = 0.0; 
            int rowR = row[r];
            int rowRp1 = row[r+1];
			
			for(int k=0;k<R; k++) {
            for (int i=rowR; i<rowRp1; i++) {
				double x_elem, val_elem; int col_elem;

				col_elem = this->specLD<SpROI,&ro_m_i>(col+i);
				x_elem   = this->specLD<SpROD,&ro_m_d>(x+col_elem);
				val_elem = this->specLD<SpROD,&ro_m_d>(val+i);
				sum += x_elem * val_elem;
				
/*
				if(col+i>=col && col+i<col_end)
					col_elem = col[i];
				else throw Dependence_Violation(this->id-1, this->id);

				if(x+col_elem>=x && x+col_elem<x_end)
					x_elem = x[col_elem];
				else throw Dependence_Violation(this->id-1, this->id);

				if(val+i>=val && val+i<val_end)
					val_elem = val[i];
				else throw Dependence_Violation(this->id-1, this->id);


                sum += x_elem * val_elem;
*/				
			}
			
            this->specST<SCm,&sc_m>(y+r, sum); //y[r] = sum;

			}
			
			r++;
			//cout<<"r: "<<r<<" sum: "<<y[r]<<endl; 
		}

		inline int __attribute__((always_inline)) 
		non_spec_iteration_body() {
			double sum = 0.0; 
            int rowR = row[r];
            int rowRp1 = row[r+1];
			
			for(int k=0;k<R; k++) {
            for (int i=rowR; i<rowRp1; i++) {
				double x_elem, val_elem; int col_elem;

				col_elem = col[i];
				x_elem   = x[col_elem];
				val_elem = val[i];
				sum += x_elem * val_elem;
			}
			
			y[r] = sum;
			}
			
			r++;
		}
};

typedef Thread_SparseMult						SpecTh;

timeval start_time, end_time;
unsigned long running_time = 0;

/** Parallel Speculative**/
int testParallelSpec() {
		sc_m.setIntervalAddr(&y[0], &y[N-1]+1);

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

	SparseCompRow_matmult(N, y, val, row, col, x, start, end);	
	
}
int testOptimalParallel() {
	pthread_t 			pthread[4];
	pthread_attr_t 		thread_attr[4];
	struct sched_param 	thread_param[4];
	
	struct Args			th_args[4];

	unsigned long th_beg = 0; unsigned long third = N/PROCS_NR; unsigned long th_end = th_beg + third;

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

	SparseCompRow_matmult(N, y, val, row, col, x);	

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
	cout<<" y[100]"<<y[100]<<" y[1000]: "<<y[1000]<<" y[10000]: "<<y[10000]<<" y[20000]: "<<y[20000]<<"y[29000]: "<<y[29000]<<endl;
	
	//for(int i=0; i<256; i++) cout<<" Z[10000+"<<i<<"]: "<<ZZ[/*10000+*/i];
	//cout<<endl;
	return 1;

}

