using namespace std;

#define MACHINE_X86_64
//#define MODEL_LOCK
//#define MODEL_FENCE

#ifndef PTHREAD
#define PTHREAD
#include <pthread.h>
#endif

#include <iostream>

#ifndef THREAD_MANAGERS
#define THREAD_MANAGERS
#include "Managers/Thread_Manager.h"
#endif

#include "config.h"


//#define TRIVIAL_HASH
//#define NAIVE_HASH
#define OPTIM_HASH
#define OPTIM_HASH_RO

//const int ROLL = 0;

const int PROCS_NR      = NUM_THREADS;
const int UNROLL     	= 1;
const int ENTRY_SZ   	= 1*UNROLL;


/**************************************************************************/
/*********************MemoryModels*****************************************/
/**************************************************************************/

const unsigned LDV_SZ       = logN;
const unsigned SHIFT_FACT   = logM;
const unsigned LDV_SZro     = PROCS_NR;
const unsigned SHIFT_FACTro = logM;


/*********************************************************************/
/*********************** CONSTRUCT THE MEMORY MODELS *****************/
/*********************************************************************/

typedef unsigned    u32;
typedef float       f32;

//typedef HashSeqPow2Arr Hash;
//Hash hash(&nums[0], LDV_SZ, SHIFT_FACT);


typedef HashSeqPow2 Hash;
Hash hash(LDV_SZ, SHIFT_FACT);

typedef SpMod_IPcore<Hash>              IPm_core;
IPm_core                                m_core(hash);
typedef SpMod_IP<Hash, &m_core, f32>    IPm;
IPm                                     ip_m(ENTRY_SZ);
const int ATTR = IP_ATTR;


typedef SpMod_ReadOnly<u32> SpRO;
SpRO    ro_m;

typedef USpModel<
			IPm, &ip_m, ATTR,
				USpModel<SpRO, &ro_m, 0, USpModelFP>
		>
	UMeg;

UMeg umm;


/********************************************************/
/*********************** THREAD MANAGER *****************/
/********************************************************/


class ThManager : public
	Master_AbstractTM<ThManager, UMeg, &umm, PROCS_NR, 0, NormalizedChunks<ThManager, UNROLL> > {
  public:
	inline ThManager() {
		Master_AbstractTM<ThManager, UMeg, &umm, PROCS_NR, 0, NormalizedChunks<ThManager, UNROLL> >::init();
	}
};
ThManager 				ttmm;

/************************************************************/
/*********************** SPECULATIVE THREAD *****************/
/************************************************************/

class Thread_Max :
				public SpecMasterThread<Thread_Max, UMeg, ThManager>
{
	private:
		typedef SpecMasterThread< Thread_Max, UMeg, ThManager >	MASTER_TH;
		unsigned long j;

	public:
		inline Thread_Max(): MASTER_TH(UNROLL, NULL)	{
		}
		inline Thread_Max(const unsigned long i, unsigned long* dummy)
						: MASTER_TH(UNROLL,i,dummy)	{

        }

		inline void initVars() {
			this->j = 0;
		}

		inline int __attribute__((always_inline))
		end_condition() const {
			return this->j >= N;
		}

		inline void __attribute__((always_inline))
		updateInductionVars() {
			unsigned long it = (this->id - ttmm.firstID()) * UNROLL;
			this->j = it;
		}

		inline int __attribute__((always_inline))
		iteration_body() {  
            unsigned row_nr = umm.specLDslow(&X[this->j], this); 
            ////this->specLD<SpRO, &ro_m>(&X[this->j]);

            float* row = &nums[row_nr*M];
            float  sum = 0.0;

            for(int i=0; i<M; i++) {
                sum += (i < M-32) ? ( this->specLD<IPm, &ip_m>(&row[i]) + 
                                      this->specLD<IPm, &ip_m>(&row[i + 32]) ) / 
                                    ( this->specLD<IPm, &ip_m>(&row[i + 32]) + 1.0 ) 
                                  : 
                                    ( this->specLD<IPm, &ip_m>(&row[i]) + 
                                      this->specLD<IPm, &ip_m>(&row[i - 32]) ) / 
                                    ( this->specLD<IPm, &ip_m>(&row[i - 32]) + 1.0) 
                                  ;
            }

            this->specST<IPm, &ip_m>(&row[0], sum);

			this->j++;
		}

		inline int __attribute__((always_inline))
		non_spec_iteration_body() {
            float* row = &nums[X[j]*M];
            float  sum = 0.0;

            for(int i=0; i<M; i++) {
                sum += (i < M-32) ? (row[i] + row[i + 32]) / (row[i + 32] + 1.0) : 
                                    (row[i] + row[i - 32]) / (row[i - 32] + 1.0) ;
            }
            row[0] = sum;

			this->j++;
		}
};

typedef Thread_Max SpecTh;

int run_tls()
{
    unsigned max = 0;

    ip_m.setIntervalAddr(&nums[0], &nums[N*M]);
    for (int i = 0; i < PROCS_NR; i++) {
        SpecTh* thr = allocateThread<SpecTh>(i, 64);
        thr->initVars();
        ttmm.registerSpecThread(thr,i);
    }

    std::cout << "Starting Speculative Execution ..." << ttmm.firstID() << endl;

	struct timeval start, end;
	unsigned long running_time = 0;
	gettimeofday(&start, NULL);
    ttmm.speculate<SpecTh>();
    gettimeofday(&end, NULL);
    
	running_time = DiffTime(&end, &start);
	printf("Time = %lu\n", running_time);

    std::cout << "Nr Rollbacks: " << ttmm.nr_of_rollbacks << std::endl;

    return 0;

}


//#pragma omp parallel for default(shared) schedule(dynamic)

