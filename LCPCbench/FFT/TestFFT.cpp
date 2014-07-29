using namespace std;

//#define MODEL_LOCK
//#define MACHINE_X86_64
//#define MODEL_FENCE

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


#include "Managers/Thread_Manager.h"


//#define TRIVIAL_HASH
//#define NAIVE_HASH
#define OPTIM_HASH

#define SP_IP
//#define HAND_PAR

const int PROCS_NR   	= 4;


/****************************************************************************************/
/******************************MemoryModels**********************************************/
/****************************************************************************************/

const int LDV_SZ 	= 4; //8;     	//256
const int UNROLL     	= 4; //4; //4; 
const int ENTRY_SZ   	= UNROLL*entrysz; 

const int SHIFT_FACT 	= 3;//1; //8;


//const unsigned long sz = (1<<LDV_SZ);

/****************************************************************************************/
/*****************************THREAD MANAGER and MEMORY MODELS***************************/
/****************************************************************************************/

#if defined( TRIVIAL_HASH )

	typedef HashSeqPow2 Hash;  //HashSeqPow2Spaced
	Hash hash(23, 0);

#elif defined( NAIVE_HASH )

	typedef HashSeqPow2 Hash;  //HashSeqPow2Spaced
	Hash hash(12, 0);

#else //OPTIM_HASH or OPTIM_HASH_RO

	//typedef FFThash Hash;
	//Hash hash(6);    //Hash hash(5); //2*3 //powdual

	//typedef HashOneArr Hash;
	//Hash hash(8, 3);

	typedef HashSeqPow2 Hash;
	Hash hash(3, 3); 

#endif


/*
typedef HashSeqPow2 Hash;  //HashSeqPow2Spaced
Hash hash(LDV_SZ, SHIFT_FACT);
// if you want to test with SHIFT_FACT != 1 then you need
// to allign the array (the alpha factor)
*/

#if defined( SP_IP)

  typedef SpMod_IPcore<Hash> 				SCm_core;
  SCm_core 	m_core(hash);
  typedef SpMod_IP<Hash, &m_core, double>		SCm;
  SCm			sc_m(ENTRY_SZ);
  const int ATTR = IP_ATTR;
#else

  typedef SpMod_SCcore<Hash> 				SCm_core;
  SCm_core 	m_core(hash);
  typedef SpMod_SC<Hash, &m_core, double>		SCm;
  SCm			sc_m(ENTRY_SZ);
  const int ATTR = ItIndAttr_HAfw; //ItIndAttr_WAR_SV;
#endif

typedef USpModel<
			SCm, &sc_m,  ATTR, USpModelFP
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

		void init() { }
		inline Thread_FFT(): MASTER_TH(UNROLL, NULL)	{ init(); }
		inline Thread_FFT(const unsigned long i, unsigned long* dummy) 
						: MASTER_TH(UNROLL,i,dummy)	{ init(); }

		inline int __attribute__((always_inline)) 
		end_condition() const {
			return (a>=dual);
		}

		inline void __attribute__((always_inline))
		updateInductionVars() {
			a = (this->id - ttmm.firstID())*UNROLL + 1;
		}


		inline int __attribute__((always_inline)) 
		iteration_par() {
			{
				double tmp_real1 = tmp_real - s * tmp_imag - s2 * tmp_real;
				tmp_imag =  tmp_imag + s * tmp_real - s2 * tmp_imag;
				tmp_real = tmp_real1;
			}
			

			for (int b = 0; b < n; b += 2 * dual) {
				int i = 2*(b + a);
				int j = 2*(b + a + dual);

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
			}
			a++;
		}  

		inline int __attribute__((always_inline)) 
		iteration_spec() {  
			{
				double tmp_real1 = tmp_real - s * tmp_imag - s2 * tmp_real;
				tmp_imag =  tmp_imag + s * tmp_real - s2 * tmp_imag;
				tmp_real = tmp_real1;
			}
			

			for (int b = 0; b < n; b += 2 * dual) {
				int i = 2*(b + a);
				int j = 2*(b + a + dual);

				double z1_real = this->specLD<SCm,&sc_m>(data+j);
				double z1_imag = this->specLD<SCm,&sc_m>(data+j+1);

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
		iteration_body() {  
#if defined( HAND_PAR )
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

typedef Thread_FFT						SpecTh;

timeval start_time, end_time;
unsigned long running_time = 0;

/** Parallel Speculative**/
int testParallelSpec() {
		sc_m.setIntervalAddr((double*)&data[2], (double*)&data[N]);

		for(int i=0; i<PROCS_NR; i++) {
			SpecTh* thr = allocateThread<SpecTh>(i, 64);
			thr->init();
			ttmm.registerSpecThread(thr,i);
		}

		cout<<"Spec: ";
		gettimeofday ( &start_time, 0 );

		ttmm.speculate <SpecTh>();

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
	pthread_t 			pthread[PROCS_NR];
	pthread_attr_t 		thread_attr[PROCS_NR];
	struct sched_param 	thread_param[PROCS_NR];
	
	struct Args			th_args[PROCS_NR];

	unsigned long th_beg = 1; unsigned long third = dual/PROCS_NR; unsigned long th_end = th_beg + third;

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


/*
// TO REPEATEDLY RUN THE SAME SPECULATIVE LOOP
for(int j=0; j<4; j++) {
	ttmm.speculate <SpecTh>();
	cout<<"AFTER "<<j<<endl;
	ttmm.cleanUpAll<SpecTh>();
}  
*/

