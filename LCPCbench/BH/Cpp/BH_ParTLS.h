//using namespace std;

//#define MODEL_LOCK
//#define MACHINE_X86_64

//#include<time.h>
//#include<iostream>
//#include<signal.h>

#ifndef PTHREAD
#define PTHREAD
#include <pthread.h>
#endif


#ifndef THREAD_MANAGERS
#define THREAD_MANAGERS
#include "Managers/Thread_Manager.h"
#endif  


// /home/co280/Work/NewTLS/FrSCNew/Tests/LCPCbench/BH/Cpp
// g++ -O3 -Wno-deprecated -pthread -I../../../../PolyLibTLSniagra/ -o BH.exe BH.cpp
// BH.exe -b 300000 -m  -e 2



//#define TRIVIAL_HASH
//#define NAIVE_HASH
//#define OPTIM_HASH_RO

const int TLS_IP = 1;
const int TLS_SC = 2;
const int PAR    = 3;

#define POW2_HASH_OPT
#define OPT_SZ 14
//#define PAR_MODEL TLS_SC
//#define PAR_MODEL_SC

/****************************************************************************************/
/******************************MemoryModels**********************************************/
/****************************************************************************************/

const int UNROLL     	= 400;//1000; //4; //4; 
const int ENTRY_SZ   	= 2*UNROLL; 
const int PROCS_NR   	= 8;

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


#if defined(PAR_MODEL_SC)

typedef SpMod_SCcore<Hash>                    MD_core;
MD_core                                       md_core(hashD);
typedef SpMod_SC<Hash, &md_core, double>      MD;
MD                                            md(ENTRY_SZ);

typedef SpMod_SCcore<Hash>                    MV_core;
MV_core                                       mv_core(hashV);
typedef SpMod_SC<Hash, &mv_core, MathVector>  MV;
MV                                            mv(ENTRY_SZ);

typedef SpMod_SCcore<Hash>                  MN_core;
MN_core                                     mn_core(hashN);
typedef SpMod_SC<Hash, &mn_core, Node*>     MN;
MN                                          mn(ENTRY_SZ);

typedef SpMod_SCcore<Hash>                  MN_core_S;
MN_core_S                                   mn_core_S(hashNS);
typedef SpMod_SC<Hash, &mn_core_S, Node*>   MNS;
MNS                                         mns(ENTRY_SZ);


const int ATTR = ItIndAttr_WAR_SV;

#else

typedef SpMod_IPcore<Hash>                    MD_core;
MD_core                                       md_core(hashD);
typedef SpMod_IP<Hash, &md_core, double>      MD;
MD                                            md(ENTRY_SZ);

typedef SpMod_IPcore<Hash>                    MV_core;
MV_core                                       mv_core(hashV);
typedef SpMod_IP<Hash, &mv_core, MathVector>  MV;
MV                                            mv(ENTRY_SZ);

typedef SpMod_IPcore<Hash>                  MN_core;
MN_core                                     mn_core(hashN);
typedef SpMod_IP<Hash, &mn_core, Node*>     MN;
MN                                          mn(ENTRY_SZ);

typedef SpMod_IPcore<Hash>                  MN_core_S;
MN_core_S                                   mn_core_S(hashNS);
typedef SpMod_IP<Hash, &mn_core_S, Node*>   MNS;
MNS                                         mns(ENTRY_SZ);


const int ATTR = IP_ATTR;

#endif

typedef USpModel<
			MD, &md,  ATTR, USpModel<
              MV, &mv, ATTR, USpModel<
                MN, &mn, ATTR, USpModel<
                  MNS, &mns, ATTR, USpModelFP
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
ThManager 				ttmm_vp;



/*******************************************************************/
/*******************************************************************/
/*******************************************************************/


/*******************************************************************/
/****************** Now for the hackGravity Loop *******************/
/*******************************************************************/


ThManager 				ttmm_hg;


class Thread_BH_HG_PAR :
				public SpecMasterThread<Thread_BH_HG_PAR, UMeg, ThManager>
{
	private:
		typedef SpecMasterThread< Thread_BH_HG_PAR, UMeg, ThManager >	MASTER_TH;
		typedef Thread_BH_HG_PAR SELF;

		
        Body*       body_buffer[UNROLL];
        Enumerate*  body_iterator;
		Node*       root;
		int         body_index;
		double      rsize;
		
	public:
		inline void __attribute__((always_inline)) execSynchCode() { 
			int count = 0;
			for ( ; body_iterator->hasMoreElements() && count<UNROLL; ) {
				Body* b = body_iterator->nextElement();
				body_buffer[count++] = b;
			}
			if(count<UNROLL) body_buffer[count] = NULL;
		}

		void init(Enumerate* e, Node* t, double step) {
			body_iterator = e; 
			root          = t;
			body_index    = 0;
			rsize         = step;
			//specLdslow = UMeg::specLDslow<double,Thread_FFT>;
		}
		void init() {}

		inline Thread_BH_HG_PAR(int i, unsigned long* dummy): MASTER_TH(UNROLL, i, dummy)	{ init(); }
		inline Thread_BH_HG_PAR(Enumerate* e, Node* t, double s, int i, unsigned long* dummy) 
						: MASTER_TH(UNROLL,i,dummy)	{ init(e, t, s); }

		inline int __attribute__((always_inline)) 
		end_condition() const {
			return body_buffer[body_index]==NULL;
		}

		inline void __attribute__((always_inline))
		updateInductionVars() {	body_index = 0; }

		inline int __attribute__((always_inline)) 
		iteration_body() {  
			//for(body_index=0; body_index<UNROLL; body_index++) {
				Body* body = body_buffer[body_index++];
				//if(body == NULL) { break; }
				//body->hackGravity_iterPAR(root, rsize);
                body->hackGravity(rsize, root);
			//}
			//this->resetThComp();
		}

		inline int __attribute__((always_inline)) 
		non_spec_iteration_body() {}
};

typedef Thread_BH_HG_PAR    SpecTh_hg;








/*******************************************************************/
/********************* TLS In-Place Support ************************/
/*******************************************************************/

ThManager 				ttmm_TLS_hg;


class Thread_BH_HG_TLS :
				public SpecMasterThread<Thread_BH_HG_TLS, UMeg, ThManager>
{
	private:
		typedef SpecMasterThread< Thread_BH_HG_TLS, UMeg, ThManager >	MASTER_TH;
		typedef Thread_BH_HG_TLS SELF;

		
        Body*       body_buffer[UNROLL];
        Enumerate*  body_iterator;
		Node*       root;
		int         body_index;
		double      rsize;
		
	public:
		inline void __attribute__((always_inline)) execSynchCode() { 
			int count = 0;
			//cout<<"SYNCH TH_ID: "<<this->getID()<<endl;

#if defined(PAR_MODEL_SC)
			for ( ; body_iterator->hasMoreElements() && count<UNROLL; ) {
				Body* b = body_iterator->nextElement();
				body_buffer[count++] = b;
			}

#else
			for ( ; body_iterator->hasMoreElements_TLS(this) && count<UNROLL; ) {
				Body* b = body_iterator->nextElement_TLS(this);
				body_buffer[count++] = b;
			}
#endif

			if(count<UNROLL) body_buffer[count] = NULL;
		}

		void init(Enumerate* e, Node* t, double step) {
			body_iterator = e; 
			root          = t;
			body_index    = 0;
			rsize         = step;
			//specLdslow = UMeg::specLDslow<double,Thread_FFT>;
		}
		void init() {}

		inline Thread_BH_HG_TLS(int i, unsigned long* dummy): MASTER_TH(UNROLL, i, dummy)	
			{ init(); }
		inline Thread_BH_HG_TLS(Enumerate* e, Node* t, double s, int i, unsigned long* dummy) 
						: MASTER_TH(UNROLL,i,dummy)	{ init(e, t, s); }

		inline int __attribute__((always_inline)) 
		end_condition() const {
			return body_buffer[body_index]==NULL;
		}

		inline void __attribute__((always_inline))
		updateInductionVars() {	body_index = 0; }

		inline int __attribute__((always_inline)) 
		iteration_body() {  
			//for(body_index=0; body_index<UNROLL; body_index++) {
				Body* body = body_buffer[body_index++];
				//if(body == NULL) { break; }
				//body->hackGravity_iterPAR(root, rsize);
                body->hackGravity_TLS(rsize, root, this);
			//}
			//this->resetThComp();
		}

		inline int __attribute__((always_inline)) 
		non_spec_iteration_body() {
			if(body_iterator->hasMoreElements()) {
				Body* b = body_iterator->nextElement();
				b->hackGravity(rsize, root);
			}
				
		}
};

typedef Thread_BH_HG_TLS    SpTh;
