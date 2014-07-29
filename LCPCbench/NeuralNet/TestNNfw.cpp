using namespace std;

//#define MACHINE_X86_64
//#define MODEL_LOCK
//#define MODEL_FENCE

#include<time.h>
#include<iostream>
#include<signal.h>

#ifndef COSMINNN
#define COSMINNN
#include "CosminNeuralNet.h"
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

//#define TRIVIAL_HASH
//#define NAIVE_HASH
#define OPTIM_HASH
#define OPTIM_HASH_RO


#define SP_IP
//#define HAND_PAR

const int PROCS_NR   	= 4;

//const int N = 10;



#if defined( TRIVIAL_HASH )
	const int LDV_SZ 		= 24;
	const int SHIFT_FACT 	= 0;
	const int LDV_SZro 		= 24;
	const int SHIFT_FACTro 	= 0;
#elif defined( NAIVE_HASH )
	const int LDV_SZ 		= 12;
	const int SHIFT_FACT 	= 0;
	const int LDV_SZro 		= 12;
	const int SHIFT_FACTro 	= 0;
#else //OPTIM_HASH or OPTIM_HASH_RO
	const int LDV_SZ = 4;    //2
	const int SHIFT_FACT = 5;//7 but UNROLL=128
	const int LDV_SZro = 2;
	const int SHIFT_FACTro = 17;
#endif


//const int LDV_SZ 		= 2; //8;     	//256
const int UNROLL     	= 64; //32; //32; //128; //32 
const int ENTRY_SZ   	= N*UNROLL; 


//const int SHIFT_FACT 	= 7; //6;

//const unsigned long sz = (1<<LDV_SZ);

/****************************************************************************************/
/*****************************THREAD MANAGER and MEMORY MODELS***************************/
/****************************************************************************************/

typedef HashSeqPow2 Hash;  
Hash hash(LDV_SZ, SHIFT_FACT);


#if defined(SP_IP)
  typedef SpMod_IPcore<Hash> 				IPm_core; 
  IPm_core 								    m_core(hash);
  typedef SpMod_IP<Hash, &m_core, double>	IPm;
  IPm										ip_m(ENTRY_SZ);
  const int ATTR = IP_ATTR;
#else
  typedef SpMod_SCcore<Hash>                IPm_core;
  IPm_core                                  m_core(hash);
  typedef SpMod_SC<Hash, &m_core, double>   IPm;
  IPm                                       ip_m(ENTRY_SZ);
  const int ATTR = ItIndAttr_WAR_SV; 
#endif

typedef SpMod_ReadOnly<double> SpRO;
SpRO ro_m2;




#ifdef OPTIM_HASH_RO
  typedef SpRO SpROlike1; typedef SpRO SpROlike2;
  SpROlike1	ro_m1; //, ro_m2;
  const int 	SpROlike_ATTR = 0;
#else

  Hash hash1(LDV_SZro, SHIFT_FACTro);
  IPm_core 									m_core1(hash1);

#if defined(SP_IP)
	typedef SpMod_IP<Hash, &m_core1, double>	IPm1;

	typedef IPm1 SpROlike1; 
	SpROlike1	ro_m1(4);
	const int 	SpROlike_ATTR = IP_ATTR;
#else
	typedef SpMod_SC<Hash, &m_core1, double>	IPm1;

	typedef IPm1 SpROlike1; 
	SpROlike1	ro_m1(4);
	const int 	SpROlike_ATTR = ItIndAttr_WAR_SV;	
#endif

#endif


typedef USpModel<
			IPm, &ip_m,  ATTR, /*ItIndAttr_HAfw,*/
			USpModel<
				SpROlike1, &ro_m1, SpROlike_ATTR, 
				USpModel<
//					SpROlike2, &ro_m2, SpROlike_ATTR, USpModelFP
					SpRO, &ro_m2, 0, USpModelFP
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


class Thread_NNfw :
				public SpecMasterThread<Thread_NNfw, UMeg, ThManager>
{
	private:
		typedef SpecMasterThread< Thread_NNfw, UMeg, ThManager >	MASTER_TH;

		int neurode;
		const double* addr_beg;
		const double* addr_end;
		//ID_WORD padding[64];
	public:
		void init() { 
			specLdslow = UMeg::specLDslow<double,Thread_NNfw>;
			addr_beg = (double*)ro_m1.addr_beg;
			addr_end = (double*)ro_m1.addr_end;
		}
		inline Thread_NNfw(): MASTER_TH(UNROLL, NULL)	{ init(); }
		inline Thread_NNfw(const unsigned long i, unsigned long* dummy) 
						: MASTER_TH(UNROLL,i,dummy)	{ /*init();*/ }

		inline int __attribute__((always_inline)) 
		end_condition() const {
			return (neurode>=U_SIZE);
		}

		inline void __attribute__((always_inline))
		updateInductionVars() {
			neurode = (this->id-ttmm.firstID())*UNROLL;
		}

		double (*specLdslow)(volatile double*, Thread_NNfw*, void*);

		inline int __attribute__((always_inline)) 
		iteration_spec() {  
			double  sum;
			int     i;
		 	for(int j=0; j<N; j++) {
	        	sum = 0.0;
	        	for (i=0; i<U_SIZE; i++) {
            	    sum += 
						this->specLD<SpROlike1,&ro_m1>(&mid_wts[neurode][i]) *
						this->specLD<SpRO,&ro_m2>(&in_pats[0][i]);
	        	}
	        	sum = 1.0/(1.0+exp(-sum));
 		  
	        	this->specST<IPm,&ip_m>(&mid_out[neurode], sum);
			}
			neurode++;
		}


		inline int __attribute__((always_inline)) 
		iteration_par() {  
			double  sum;
			int     i;
		 	for(int j=0; j<N; j++) {
	        	sum = 0.0;
	        	for (i=0; i<U_SIZE; i++) {
            	    sum +=  mid_wts[neurode][i] * in_pats[0][i];
	        	}
	        	sum = 1.0/(1.0+exp(-sum));
 		  
	        	mid_out[neurode] = sum;
			}
			neurode++;
		}

		inline int __attribute__((always_inline)) 
		iteration_body() {
#if defined(HAND_PAR)
			iteration_par();
#else
			iteration_spec();
#endif
		}

		inline int __attribute__((always_inline)) 
		non_spec_iteration_body() {
			iteration_par();
		}
};

typedef Thread_NNfw						SpecTh;

timeval start_time, end_time;
unsigned long running_time = 0;

/** Parallel Speculative**/
int testParallelSpec() {
		ip_m.setIntervalAddr(&mid_out[0], &mid_out[U_SIZE-1]+1);

#ifndef OPTIM_HASH_RO
		ro_m1.setIntervalAddr( &mid_wts[0][0] , &mid_wts[U_SIZE-1][U_SIZE] );
		ro_m2.setIntervalAddr( &in_pats[0][0] , &in_pats[0][U_SIZE] );
#endif

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

	for(int i=start; i<end; i++)
	  for(int j=0;j<N; j++)
		unified_do_forward_pass(i);
	
}
int testOptimalParallel() {
	pthread_t 			pthread[PROCS_NR];
	pthread_attr_t 		thread_attr[PROCS_NR];
	struct sched_param 	thread_param[PROCS_NR];
	
	struct Args			th_args[PROCS_NR];

	unsigned long th_beg = 0; unsigned long third = U_SIZE/PROCS_NR; unsigned long th_end = th_beg + third;

	cout<<"Para: ";

	gettimeofday ( &start_time, 0 );

	for(int i=0; i<PROCS_NR; i++) {
			//if(FIFO_SCHED)
			//	Spec_Thread0::setFIFOthread(&thread_attr[i], &thread_param[i]);
			//else
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
	cout<<"Doing: "<<0<<" "<<U_SIZE<<endl;
	gettimeofday ( &start_time, 0 );

	for(int i=0; i<U_SIZE; i++)
	  for(int j=0;j<N; j++)
		unified_do_forward_pass(i);	

	gettimeofday( &end_time, 0 );
	running_time += DiffTime(&end_time,&start_time);
}

pthread_attr_t 		thread_attr;
struct sched_param 	thread_param;
		

int main(int argc, char* argv[]) {	
	init();

	if(atoi(argv[1])==33) {
		testSequential();
	} else if(atoi(argv[1])==34) {
		testOptimalParallel();
	} else if(atoi(argv[1])==35) {
		testParallelSpec();
		cout<<"Nr Rollbacks: "<<ttmm.nr_of_rollbacks<<endl;
	}

	cout<<"Running time: "<<running_time<<endl;
//	cout<<" DK[100]"<<DK[100]<<" DK[1000]: "<<DK[1000]<<" DK[10000]: "<<DK[10000]<<endl;
	
	//for(int i=0; i<256; i++) cout<<" Z[10000+"<<i<<"]: "<<ZZ[/*10000+*/i];
	//cout<<endl;
	return 1;

}

