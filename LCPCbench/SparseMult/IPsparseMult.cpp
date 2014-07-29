using namespace std;

//#define MODEL_FENCE
#define MODEL_LOCK
#define MACHINE_X86_64


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


//#define TRIVIAL_HASH
//#define NAIVE_HASH
//#define OPTIM_HASH
#define OPTIM_HASH_RO

/****************************************************************************************/
/******************************MemoryModels**********************************************/
/****************************************************************************************/

#if defined( TRIVIAL_HASH )
		const int LDV_SZ 		= 24;
		const int SHIFT_FACT 	= 0;
		const int LDV_SZro 		= 24;
		const int SHIFT_FACTro 	= 0;
#elif defined( NAIVE_HASH )
		const int LDV_SZ 		= 14;
		const int SHIFT_FACT 	= 0;
		const int LDV_SZro 		= 10;
		const int SHIFT_FACTro 	= 0;
#else //OPTIM_HASH or OPTIM_HASH_RO
		const int LDV_SZ = 4; //2    
                const int SHIFT_FACT = 8; //5; //4;//2; //5 -- for UNROLL==32;
		const int LDV_SZro = 0; //0; //2;
		const int SHIFT_FACTro = 0;
#endif


//const int LDV_SZ 		= 2; //10; 
const int UNROLL     	= 256; //16; //1; //16; //4; //32; // 256; 
const int ENTRY_SZ   	= UNROLL; 

const int PROCS_NR   	= 4;
//const int SHIFT_FACT 	= 5;


#define SP_IP
//#define HAND_PAR

//const unsigned long sz = (1<<LDV_SZ);

/****************************************************************************************/
/*****************************THREAD MANAGER and MEMORY MODELS***************************/
/****************************************************************************************/

typedef HashSeqPow2 Hash;
Hash hash(LDV_SZ, SHIFT_FACT);

#if defined(SP_IP)

	typedef SpMod_IPcore<Hash> 				IPm_core;
	IPm_core 	m_core(hash);
	typedef SpMod_IP<HashSeqPow2, &m_core, double>	IPm;
	IPm			ip_m(ENTRY_SZ);
 	const int ATTR = IP_ATTR;

#else

	typedef SpMod_SCcore<Hash>   IPm_core;
	IPm_core                     m_core(hash);
    typedef SpMod_SC<Hash, &m_core, double>  IPm;
    IPm                                     ip_m(ENTRY_SZ);
	const int ATTR = ItIndAttr_WAR_SV; 

#endif


#ifdef OPTIM_HASH_RO

	typedef SpMod_ReadOnly<double> SpROD;
	typedef SpMod_ReadOnly<double> SpROD1;
	SpROD	ro_m_d;
	SpROD1	ro_m_d1;

	typedef SpMod_ReadOnly<int> 	SpROI;
	SpROI	ro_m_i;

	const int 	SpROlike_ATTR = RO_ATTR;
#else

	Hash hash1(LDV_SZro, SHIFT_FACTro);
	IPm_core 	m_core_d(hash1), m_core_d1(hash1), m_core_i(hash1);

#if defined(SP_IP)

	typedef SpMod_IP<Hash, &m_core_d,  double>	SpROD;
	typedef SpMod_IP<Hash, &m_core_d1, double>	SpROD1;
	typedef SpMod_IP<Hash, &m_core_i,  int>		SpROI;

	SpROD		ro_m_d(4);
	SpROD1		ro_m_d1(4);
	SpROI		ro_m_i(4);
	const int 	SpROlike_ATTR = IP_ATTR;

#else

	typedef SpMod_SC<Hash, &m_core_d,  double>	SpROD;
	typedef SpMod_SC<Hash, &m_core_d1, double>	SpROD1;
	typedef SpMod_SC<Hash, &m_core_i,  int>		SpROI;

	SpROD		ro_m_d(4);
	SpROD1		ro_m_d1(4);
	SpROI		ro_m_i(4);
	const int 	SpROlike_ATTR = ItIndAttr_WAR_SV;

#endif

#endif


typedef USpModel<
			IPm, &ip_m,  ATTR, 
			USpModel<
				SpROD, &ro_m_d, SpROlike_ATTR,
				USpModel<
					SpROD1, &ro_m_d1, SpROlike_ATTR, 
					USpModel<
						SpROI, &ro_m_i, SpROlike_ATTR, USpModelFP
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


class Thread_SparseMult :
				public SpecMasterThread<Thread_SparseMult, UMeg, ThManager>
{
	private:
		typedef SpecMasterThread< Thread_SparseMult, UMeg, ThManager >	MASTER_TH;
		typedef Thread_SparseMult SELF;

		int r;
		
	public:
		void init() { }
		inline Thread_SparseMult(): MASTER_TH(UNROLL, NULL)	{ init(); }
		inline Thread_SparseMult(const unsigned long i, unsigned long* dummy) 
						: MASTER_TH(UNROLL,i,dummy)	{ init(); }

		inline int __attribute__((always_inline)) 
		end_condition() const {
			return (r>=N);
		}

		inline void __attribute__((always_inline))
		updateInductionVars() {
			r = ( this->id - ttmm.firstID() )*UNROLL;
			//cout<<"Iteration: "<<this->id<<endl;
		}

		inline void spec_iter() {
			double sum = 0.0; 
            int rowR = row[r];
            int rowRp1 = row[r+1];
            for (int i=rowR; i<rowRp1; i++) {
				double x_elem, val_elem; int col_elem;

				col_elem = this->specLD<SpROI,&ro_m_i>(col+i);
				x_elem = x[col_elem];
				val_elem = this->specLD<SpROD1,&ro_m_d1>(val+i);
				sum += x_elem * val_elem;				
			}
			
            this->specST<IPm,&ip_m>(y+r, sum); //y[r] = sum;
		}

		inline void par_iter() {
			double sum = 0.0; 
            int rowR = row[r];
            int rowRp1 = row[r+1];
			
            for (int i=rowR; i<rowRp1; i++) {
				double x_elem, val_elem; int col_elem;

				col_elem = col[i];
				x_elem   = x[col_elem];
				val_elem = val[i];
				sum += x_elem * val_elem;
			}
			
			y[r] = sum;
		}


		inline int __attribute__((always_inline)) 
		iteration_body() {  
#if defined( HAND_PAR )
			par_iter();
#else
			spec_iter();
#endif
			r++;
			//cout<<"r: "<<r<<" sum: "<<y[r]<<endl; 
		}

		inline int __attribute__((always_inline)) 
		non_spec_iteration_body() {
			par_iter();
			r++;
		}
};

typedef Thread_SparseMult						SpecTh;

timeval start_time, end_time;
unsigned long running_time = 0;

/** Parallel Speculative**/
int testParallelSpec() {
		ip_m.setIntervalAddr(&y[0], &y[N-1]+1);

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
	pthread_t 			pthread[PROCS_NR];
	pthread_attr_t 		thread_attr[PROCS_NR];
	struct sched_param 	thread_param[PROCS_NR];
	
	struct Args			th_args[PROCS_NR];

	unsigned long th_beg = 0; unsigned long third = N/PROCS_NR; unsigned long th_end = th_beg + third;

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
	cout<<"y[100]: "<<y[100]<<" y[1000]: "<<y[1000]<<" y[10000]: "<<y[10000]<<" y[29000]: "<<y[29000]
		/*<<"y[999999]: "<<y[999999]*/<<endl;
	
	//for(int i=0; i<256; i++) cout<<" Z[10000+"<<i<<"]: "<<ZZ[/*10000+*/i];
	//cout<<endl;
	return 1;

}

