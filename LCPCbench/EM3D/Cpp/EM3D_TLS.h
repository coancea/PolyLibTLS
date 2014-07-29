using namespace std;

#include<time.h>
#include<iostream>
#include<signal.h>

#ifndef PTHREAD
#define PTHREAD
#include <pthread.h>
#endif


#ifndef THREAD_MANAGERS
#define THREAD_MANAGERS
#include "Managers/Thread_Manager.h"
#endif  


// /home/co280/Work/NewTLS/FrSCNew/Tests/LCPCbench/BH/Cpp
// g++ -O3 -Wno-deprecated -pthread -I../../../../PolyLibTLSniagra/ -o em3d.exe em3d.cpp
// ./em3d.exe -n 1048576 -d 32 -m -e 3

/*********  STRUCTURED -- FEW ROLLS **********/
/**** ./em3d.exe -n 1048576 -d 32 -m -e 3 ****/
/*********************************************/


//#define TRIVIAL_HASH
//#define NAIVE_HASH
//#define OPTIM_HASH_RO


#define POW2_HASH_OPT
#define OPT_SZ 16

#define TLS_IP
//#define TLS_SC
//#define HAND_PAR

/****************************************************************************************/
/******************************MemoryModels**********************************************/
/****************************************************************************************/

const int UNROLL     	= 32; //32; //4; //4; 
const int ENTRY_SZ   	= 32*UNROLL + 512; 
const int PROCS_NR   	= 4;

//const int SHIFT_FACT 	= 3;//1; //8;
//const int LDV_SZ 		= 2; //4; //8;     	//256


//const unsigned long sz = (1<<LDV_SZ);

/****************************************************************************************/
/*****************************THREAD MANAGER and MEMORY MODELS***************************/
/****************************************************************************************/



#if defined( POW2_HASH_OPT )
	typedef HashSeqPow2 Hash;  //HashSeqPow2Spaced
	Hash hashV (OPT_SZ-1, 0);
    Hash hashD (OPT_SZ-1, 0);
    Hash hashN (OPT_SZ-1, 0);
	Hash hashNS(OPT_SZ-1, 0);
#elif defined( POW2_HASH_LARGE )
	typedef HashSeqPow2 Hash;  //HashSeqPow2Spaced
	Hash hashV (20, 0);
    Hash hashD (20, 0);
    Hash hashN (20, 0);
	Hash hashNS(20, 0);
#elif defined( NAIVE_HASH )

	typedef HashGen Hash;  //HashSeqPow2Spaced
	Hash hashV (100000, 1);
    Hash hashD (100000, 1);
    Hash hashN (100000, 1);
	Hash hashNS(100000, 0);

#else //OPTIM_HASH or OPTIM_HASH_RO

	typedef FFThash Hash;
	Hash hash(5); //2*3 //powdual

	//typedef HashSeqPow2 Hash;
	//Hash hash(2, 3); 

#endif


#if defined( TLS_IP )

typedef SpMod_IPcore<Hash>                    MD_core;
MD_core                                       md_core(hashD);
typedef SpMod_IP<Hash, &md_core, double>      MD;
MD                                            md(ENTRY_SZ);

typedef SpMod_IPcore<Hash>                  MN_core;
MN_core                                     mn_core(hashN);
typedef SpMod_IP<Hash, &mn_core, Node*>     MN;
MN                                          mn(ENTRY_SZ);

/*
typedef SpMod_IPcore<Hash>                  MN_core;
MN_core                                     mn_core(hashN);
typedef SpMod_IP<Hash, &mn_core, Node*>     MN;
MN                                          mn(ENTRY_SZ);

typedef SpMod_IPcore<Hash>                  MN_core_S;
MN_core_S                                   mn_core_S(hashNS);
typedef SpMod_IP<Hash, &mn_core_S, Node*>   MNS;
MNS                                         mns(ENTRY_SZ);
*/

const int ATTR = IP_ATTR;

# else

typedef SpMod_SCcore<Hash>                    MD_core;
MD_core                                       md_core(hashD);
typedef SpMod_SC<Hash, &md_core, double>      MD;
MD                                            md(ENTRY_SZ);

typedef SpMod_SCcore<Hash>                  MN_core;
MN_core                                     mn_core(hashN);
typedef SpMod_SC<Hash, &mn_core, Node*>     MN;
MN                                          mn(ENTRY_SZ);

/*
typedef SpMod_SCcore<Hash>                  MN_core;
MN_core                                     mn_core(hashN);
typedef SpMod_SC<Hash, &mn_core, Node*>     MN;
MN                                          mn(ENTRY_SZ);

typedef SpMod_SCcore<Hash>                  MN_core_S;
MN_core_S                                   mn_core_S(hashNS);
typedef SpMod_SC<Hash, &mn_core_S, Node*>   MNS;
MNS                                         mns(ENTRY_SZ);
*/

const int ATTR = ItIndAttr_WAR_SV;

#endif
/*
typedef USpModel<
			MD, &md,  ATTR, USpModel<
                MN, &mn, ATTR, USpModel<
                  MNS, &mns, ATTR, USpModelFP
                >
            >     
		>
	UMeg;
*/

typedef USpModel< MD, &md,  ATTR, USpModel< MN, &mn, ATTR, USpModelFP> >    UMeg;


UMeg umm;



class ThManager : public 
	Master_AbstractTM<ThManager, UMeg, &umm, PROCS_NR, 0, NormalizedChunks<ThManager, UNROLL> > {
  public:
	inline ThManager() { 
		Master_AbstractTM<ThManager, UMeg, &umm, PROCS_NR, 0, NormalizedChunks<ThManager, UNROLL> >::init(); 
	}
};
ThManager 				ttmm;


/*******************************************************************/
/************************* TLS Support *****************************/
/*******************************************************************/
//#if ( PAR_MODEL == TLS_IP || PAR_MODEL == PAR)

class Thread_EM3D_TLS :
				public SpecMasterThread<Thread_EM3D_TLS, UMeg, ThManager>
{
	private:
		typedef SpecMasterThread< Thread_EM3D_TLS, UMeg, ThManager >	MASTER_TH;
		typedef Thread_EM3D_TLS SELF;

        Enumeration*  body_iterator;
		int           body_index;
        Node*         body_buffer[UNROLL];
		
	public:
		inline void __attribute__((always_inline)) execSynchCode() { 
			int count = 0; int sum = 0;
#if defined( HAND_PAR )
			for ( ; body_iterator->hasMoreElements() && count<UNROLL; ) {
				Node* n = body_iterator->nextElement();
				body_buffer[count++] = n;
			}			
#else
            for ( ; body_iterator->hasMoreElements_TLS(this) && count<UNROLL; ) {
				Node* n = body_iterator->nextElement_TLS(this);
				body_buffer[count++] = n;
			}
#endif
			if(count<UNROLL) body_buffer[count] = NULL;
		}

		void init(Enumeration* e) {
			body_iterator = e; 
			body_index    = 0;
		}
		void init() {}

		inline Thread_EM3D_TLS(int i, unsigned long* dummy): MASTER_TH(UNROLL, i, dummy)	
			{ init(); }
		inline Thread_EM3D_TLS(Enumeration* e, int i, unsigned long* dummy) 
						: MASTER_TH(UNROLL,i,dummy)	{ init(e); }

		inline int __attribute__((always_inline)) 
		end_condition() const {
			return body_buffer[body_index]==NULL;
		}

		inline void __attribute__((always_inline))
		updateInductionVars() {	body_index = 0; }

		inline int __attribute__((always_inline)) 
		iteration_body() {  
            Node* node = body_buffer[body_index++];
#if defined( HAND_PAR )
            node->Node::computeNewValue();
#else
            node->Node::computeNewValue_TLS(this);
#endif
		}

		inline int __attribute__((always_inline)) 
		non_spec_iteration_body() {
			if(body_iterator->hasMoreElements()) {
				Node* node = body_iterator->nextElement();
				node->computeNewValue();
			}
				
		}
};

typedef Thread_EM3D_TLS    SpTh;

