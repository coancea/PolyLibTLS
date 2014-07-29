using namespace std;

//#define MODEL_FENCE
//#define MODEL_LOCK
//#define MACHINE_X86_64

//#define TRIVIAL_HASH
//#define NAIVE_HASH
//#define OPTIM_HASH
//#define OPTIM_HASH_RO

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


#define OPTIM_HASH
#define OPTIM_HASH_RO

#define SP_IP
#define HAND_PAR

const int PROCS_NR   	= 4;

 
#if defined( TRIVIAL_HASH )
	const int LDV_SZ = 25;
	const int SHIFT_FACT = 0;
#elif defined( NAIVE_HASH )
	const int LDV_SZ = 13;
	const int SHIFT_FACT = 0;
#else //OPTIM_HASH or OPTIM_HASH_RO
	const int LDV_SZ = 6;
    const int SHIFT_FACT = 9; //9;
#endif


/****************************************************************************************/
/******************************MemoryModels**********************************************/
/****************************************************************************************/

//const int LDV_SZ 	    = 13; //2; //13; //2; //6;     	//256
const int UNROLL     	= 1; 
const int ENTRY_SZ   	= U_SIZE*UNROLL; 

//const int SHIFT_FACT 	= 0; //10; //0; //10; //8;

/****************************************************************************************/
/*****************************THREAD MANAGER and MEMORY MODELS***************************/
/****************************************************************************************/


typedef HashSeqPow2 Hash;  //HashSeqPow2Spaced
Hash hash(LDV_SZ, SHIFT_FACT);
//typedef HashGen Hash;
//Hash hash(U_SIZE/PROCS_NR,12);

#if defined(SP_IP)
  typedef SpMod_IPcore<Hash> 				IPm_core;
  IPm_core 								    m_core(hash);
  typedef SpMod_IP<Hash, &m_core, double>	IPm;
  IPm										ip_m1(ENTRY_SZ), ip_m2(ENTRY_SZ);
  const int ATTR = IP_ATTR;
#else
  typedef SpMod_SCcore<Hash>                IPm_core;
  IPm_core                                  m_core(hash);
  typedef SpMod_SC<Hash, &m_core, double>   IPm;
  IPm                                       ip_m1(ENTRY_SZ), ip_m2(ENTRY_SZ);
  const int ATTR = ItIndAttr_WAR_SV; //ItIndAttr_HAfw; //ItIndAttr_WAR_SV; 
#endif

typedef SpMod_ReadOnly<double> SpRO;


//IPm ro_m(16); 
//typedef IPm SpRO;

#ifdef OPTIM_HASH_RO
	typedef SpRO SpROlike1; typedef SpRO SpROlike2;
	SpROlike1	ro_m1, ro_m2, ro_m;
	const int 	SpROlike_ATTR = 0;
#else

#if defined( SP_IP )
	IPm_core 									m_core1(hash);
	IPm_core 									m_core2(hash);
	typedef SpMod_IP<Hash, &m_core1, double>	IPm1;
	typedef SpMod_IP<Hash, &m_core2, double>	IPm2;

	typedef IPm1 SpROlike1; 
	typedef IPm1 SpROlike2;
	SpROlike1	ro_m1(4);
	SpROlike2 	ro_m2(4);
	SpRO		ro_m;
	const int 	SpROlike_ATTR = IP_ATTR;
#else
	IPm_core 									m_core1(hash);
	IPm_core 									m_core2(hash);
	typedef SpMod_SC<Hash, &m_core1, double>	IPm1;
	typedef SpMod_SC<Hash, &m_core2, double>	IPm2;

	typedef IPm1 SpROlike1; 
	typedef IPm1 SpROlike2;
	SpROlike1	ro_m1(4);
	SpROlike2 	ro_m2(4);
	SpRO		ro_m;
	const int 	SpROlike_ATTR = ItIndAttr_WAR_SV; //ItIndAttr_HAfw; //ItIndAttr_WAR_SV;
#endif

#endif


typedef USpModel<
			IPm, &ip_m1,  ATTR, 
			USpModel<
				IPm, &ip_m2, ATTR,
				USpModel<
					SpROlike1, &ro_m1, SpROlike_ATTR, 
					USpModel<
						SpROlike2, &ro_m2, SpROlike_ATTR, 
						USpModel<SpRO, &ro_m, 0, USpModelFP>
					>
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


class Thread_NNbw :
				public SpecMasterThread<Thread_NNbw, UMeg, ThManager>
{
	private:
		typedef SpecMasterThread< Thread_NNbw, UMeg, ThManager >	MASTER_TH;
		int neurode;
		const double learn, alph;
		int msk;
	public:
		inline Thread_NNbw(): MASTER_TH(UNROLL, NULL), learn(BETA), alph(ALPHA)	{  }
		inline Thread_NNbw(const unsigned long i, unsigned long* dummy) 
						: MASTER_TH(UNROLL,i,dummy), learn(BETA), alph(ALPHA)	{  }

		void init(int msk) { this->msk=msk; }

		inline int __attribute__((always_inline)) 
		end_condition() const {
			return (neurode>=U_SIZE);
		}

		inline void __attribute__((always_inline))
		updateInductionVars() {
			neurode = ((this->id - ttmm.firstID())/PROCS_NR)*(UNROLL);
		}

		inline int __attribute__((always_inline)) 
		iteration_body_par() {

			int weight;
			double delta;

			long int fourth = U_SIZE/PROCS_NR;
			long ind = fourth*msk;

			volatile double*  ptrwts = &out_wts[neurode][ind];
			volatile double*  ptr    = &out_wt_cum_change[neurode][ind];
			volatile double*  ptr_outwc = &out_wt_change[neurode][ind];
			volatile double*  ptr_midout= &mid_out[ind];
			double out_err = out_error[neurode];
			const double learn_mult_out_err = learn*out_err;

			for (weight=0; weight<U_SIZE/PROCS_NR; weight++) {
                delta = learn_mult_out_err * (*ptr_midout); 

                delta += alph * (*ptr_outwc); 

				*ptrwts = (*ptrwts) + delta;
			
				*ptr = (*ptr) + delta;
				
				ptrwts++; ptr++; ptr_outwc++; ptr_midout++;
			}
		}

		inline int __attribute__((always_inline)) 
		iteration_body_spec() {
			int weight;
			double delta;

			long int fourth = U_SIZE/PROCS_NR;
			long ind = fourth*msk;

			volatile double*  ptrwts = &out_wts[neurode][ind];
			volatile double*  ptr    = &out_wt_cum_change[neurode][ind];
			volatile double*  ptr_outwc = &out_wt_change[neurode][ind];
			volatile double*  ptr_midout= &mid_out[ind];
			double out_err = this->specLD<SpRO,&ro_m>(&out_error[neurode]);
			const double learn_mult_out_err = learn*out_err;

			for (weight=0; weight<U_SIZE/PROCS_NR; weight++) {
                delta = learn_mult_out_err *
						//*ptr_midout; 
						this->specLD<SpROlike1,&ro_m1>(ptr_midout);

                delta += alph *
						//*ptr_outwc; 
						this->specLD<SpROlike2, &ro_m2>(ptr_outwc);

				this->specST<IPm,&ip_m1>(ptrwts, 
					this->specLD<IPm,&ip_m1>(ptrwts) + delta
				);
			
				this->specST<IPm,&ip_m2>(ptr, 
					this->specLD<IPm,&ip_m2>(ptr) + delta
				);
				
				ptrwts++; ptr++; ptr_outwc++; ptr_midout++;
			}
		}

		inline int __attribute__((always_inline)) 
		iteration_body() {
#if defined( HAND_PAR )
			iteration_body_par ();
#else
			iteration_body_spec();
#endif
		}

		inline int __attribute__((always_inline)) 
		non_spec_iteration_body() { iteration_body_par(); }
};

typedef Thread_NNbw						SpecTh;

timeval start_time, end_time;
unsigned long running_time = 0;



/** Parallel Speculative**/
int testParallelSpec() {
		ip_m1.setIntervalAddr(&out_wts[0][0], &out_wts[U_SIZE-1][U_SIZE-1]);
		ip_m2.setIntervalAddr(&out_wt_cum_change[0][0], &out_wt_cum_change[U_SIZE-1][U_SIZE-1]);

#ifndef OPTIM_HASH_RO
		ro_m1.setIntervalAddr( &mid_out[0] , &mid_out[U_SIZE-1] );
		ro_m2.setIntervalAddr( &out_wt_change[0][0] , &out_wt_change[U_SIZE-1][U_SIZE-1] );
#endif

		for(int i=0; i<PROCS_NR; i++) {
			SpecTh* thr = allocateThread<SpecTh>(i, 64);
			ttmm.registerSpecThread(thr,i);
			thr->init(i);
		}

		cout<<"Num Threads: "<<PROCS_NR<<endl;
		cout<<" Spec: ";
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

	for(int i=start; i<end; i++)
		unified_do_backward_pass(i);
	
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

	gettimeofday ( &start_time, 0 );

	for(int i=0; i<U_SIZE; i++)
		unified_do_backward_pass(i);	

	gettimeofday( &end_time, 0 );
	running_time += DiffTime(&end_time,&start_time);
}

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
	cout<<" DK[0]"<<out_wt_cum_change[0][100]<<" DK[1000]: "<<out_wt_cum_change[500][100]<<" DK[10000]: "<<out_wt_cum_change[1000][100]<<" DK[0]"<<out_wt_cum_change[0][700]<<" DK[1000]: "<<out_wt_cum_change[500][700]<<" DK[10000]: "<<out_wt_cum_change[1000][700]<<endl;
	
	return 1;

}

