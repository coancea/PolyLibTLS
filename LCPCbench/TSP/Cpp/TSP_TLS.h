//using namespace std;

//#define MODEL_FENCE
//#define MACHINE_X86_64
//#define MODEL_LOCK

//#include<time.h>
//#include<iostream>
//#include<signal.h>

#ifndef PTHREAD
#define PTHREAD
#include <pthread.h>
#endif


//#ifndef THREAD_MANAGERS
//#define THREAD_MANAGERS
//#include "Managers/Thread_Manager.h"
//#endif  


// g++ -O3 -Wno-deprecated -pthread -I../../../../PolyLibTLSniagra/ -o tsp.exe TSP.cpp
// ./tsp.exe -c 100000 -m -p -e 3


//#define OPTIM_HASH_RO



#define POW2_HASH_OPT
#define OPT_SZ 17 //19

#define TLS_IP
//#define TLS_SC
//#define HAND_PAR
#define NESTED_TLS

/****************************************************************************************/
/******************************MemoryModels**********************************************/
/****************************************************************************************/

const int UNROLL     	= 1; //4; //4; 
const int ENTRY_SZ   	= 1024*16*UNROLL;// + 2048; 
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

    Hash hashI(7,0);
#elif defined( POW2_HASH_LARGE )
	typedef HashSeqPow2 Hash;  //HashSeqPow2Spaced
	Hash hashV (20, 0);
    Hash hashD (20, 0);
    Hash hashN (20, 0);
	Hash hashNS(20, 0);

    Hash hashI(15,0);
#elif defined( NAIVE_HASH )

	typedef HashGen Hash;  //HashSeqPow2Spaced
	Hash hashV (100000, 1);
    Hash hashD (100000, 1);
    Hash hashN (100000, 1);
	Hash hashNS(100000, 1);

    Hash hashI(1,0);
#else //OPTIM_HASH or OPTIM_HASH_RO

	typedef FFThash Hash;
	Hash hash(5); //2*3 //powdual

	//typedef HashSeqPow2 Hash;
	//Hash hash(2, 3); 

#endif




#if defined( TLS_IP )

typedef SpMod_IPcore<Hash>                  MN_core;
MN_core                                     mn_core(hashN);
typedef SpMod_IP<Hash, &mn_core, Tree*>     MN;
MN                                          mn(ENTRY_SZ);

typedef SpMod_IPcore<Hash>                    MI_core;
MI_core                                       mi_core(hashI);
typedef SpMod_IP<Hash, &mi_core, int>         MI;
MI                                            mi(ENTRY_SZ);

/*
typedef SpMod_IPcore<Hash>                    MD_core;
MD_core                                       md_core(hashD);
typedef SpMod_IP<Hash, &md_core, double>      MD;
MD                                            md(ENTRY_SZ);

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

typedef SpMod_SCcore<Hash>                  MN_core;
MN_core                                     mn_core(hashN);
typedef SpMod_SC<Hash, &mn_core, Tree*>     MN;
MN                                          mn(ENTRY_SZ);

typedef SpMod_SCcore<Hash>                  MI_core;
MI_core                                     mi_core(hashI);
typedef SpMod_SC<Hash, &mi_core, int>       MI;
MI                                          mi(ENTRY_SZ);


/*
typedef SpMod_SCcore<Hash>                    MD_core;
MD_core                                       md_core(hashD);
typedef SpMod_SC<Hash, &md_core, double>      MD;
MD                                            md(ENTRY_SZ);

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

typedef USpModel< MN, &mn,  ATTR, USpModel<MI, &mi, ATTR, USpModelFP> >    UMeg;


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
/******************* TLS Support for conquer ***********************/
/*******************************************************************/
//#if ( PAR_MODEL == TLS_IP || PAR_MODEL == PAR)

class Thread_TSP_Conq_TLS :
				public SpecMasterThread<Thread_TSP_Conq_TLS, UMeg, ThManager>
{
	private:
		typedef SpecMasterThread< Thread_TSP_Conq_TLS, UMeg, ThManager >	MASTER_TH;
		typedef Thread_TSP_Conq_TLS SELF;

        TreeIterator*  body_iterator;
		int            body_index;
        TreeClos       body_buffer[UNROLL];
		
	public:
		inline void __attribute__((always_inline)) execSynchCode() { 
			int count = 0;
#if defined( HAND_PAR )
			for ( ; body_iterator->hasMoreElements() && count<UNROLL; ) {
				body_buffer[count++] = body_iterator->nextElement();
			}
			
#else 
            for ( ; body_iterator->hasMoreElements_TLS(this) && count<UNROLL; ) {
				body_buffer[count++] = body_iterator->nextElement_TLS(this);
			}
#endif

			if(count<UNROLL) { TreeClos clos; body_buffer[count] = clos; }
		}

		void init(TreeIterator* e) {
			body_iterator = e; 
			body_index    = 0;
		}
		void init() {}

		inline Thread_TSP_Conq_TLS(int i, unsigned long* dummy): MASTER_TH(UNROLL, i, dummy)	
			{ init(); }
		inline Thread_TSP_Conq_TLS(TreeIterator* e, int i, unsigned long* dummy) 
						: MASTER_TH(UNROLL,i,dummy)	{ init(e); }

		inline int __attribute__((always_inline)) 
		end_condition() const {
            TreeClos clos = body_buffer[body_index];
            return clos.empty();
		}

		inline void __attribute__((always_inline))
		updateInductionVars() {	body_index = 0; }

		inline int __attribute__((always_inline)) 
		iteration_body() {  
            TreeClos clos = body_buffer[body_index];
			Tree*    t    = clos.get();
#if defined( HAND_PAR )
            Tree* new_tree = t->conquer();
#else
			Tree* new_tree = t->conquer_TLS(this);
#endif
			clos.set(new_tree);
            body_index++;
		}

		inline int __attribute__((always_inline)) 
		non_spec_iteration_body() {
			if(body_iterator->hasMoreElements()) {
				TreeClos clos = body_iterator->nextElement(); 
				Tree*    t    = clos.get();
            	clos.set( t->conquer() );
            	body_index++;
			}	
		}
};

typedef Thread_TSP_Conq_TLS    SpTh;


/***********************************************/
/************ TLS Support for MERGE ************/
/***********************************************/

class Thread_TSP_Merge_TLS :
				public SpecMasterThread<Thread_TSP_Merge_TLS, UMeg, ThManager>
{
	private:
		typedef SpecMasterThread< Thread_TSP_Merge_TLS, UMeg, ThManager >	MASTER_TH;
		typedef Thread_TSP_Merge_TLS SELF;

		int level;
        TreeIterator*  body_iterator;
        TreeIterator*  body_iterator_p_1;
		int            body_index;
        TreeClos       body_buffer[UNROLL];
        TreeClos       body_buffer_p_1[(UNROLL<<1)];
		bool           empty_iteration;
		
	public:
		inline void __attribute__((always_inline)) execSynchCode() { 
			int count = 0;
#if defined (NESTED_TLS)
				if(empty_iteration) { empty_iteration = false; return; }
#endif

#if defined( HAND_PAR )
			for ( ; body_iterator->hasMoreElements() && count<UNROLL; ) {
				int count_m_2 = (count << 1);
				body_buffer[count] = body_iterator->nextElement();
				body_buffer_p_1[count_m_2]   = body_iterator_p_1->nextElement();
				body_buffer_p_1[count_m_2+1] = body_iterator_p_1->nextElement();
				count++;
			}			
#else
            for ( ; body_iterator->hasMoreElements_TLS(this) && count<UNROLL; ) {
				int count_m_2 = (count << 1);
				body_buffer[count] = body_iterator->nextElement_TLS(this);
				body_buffer_p_1[count_m_2]   = body_iterator_p_1->nextElement_TLS(this);
				body_buffer_p_1[count_m_2+1] = body_iterator_p_1->nextElement_TLS(this);
				count++;
			}
#endif

			if(count<UNROLL) { TreeClos clos; body_buffer[count] = clos; }
		}

		void init(TreeIterator* e, TreeIterator* e_p_1, int lev) {
			body_iterator = e; 
            body_iterator_p_1 = e_p_1;
			body_index    = 0;
			level = lev;
			empty_iteration = false;
		}
		void init() {}

		inline Thread_TSP_Merge_TLS(int i, unsigned long* dummy): MASTER_TH(UNROLL, i, dummy)	
			{ init(); }
		inline Thread_TSP_Merge_TLS(TreeIterator* e, TreeIterator* e_p_1, int lev, int i, unsigned long* dummy) 
						: MASTER_TH(UNROLL,i,dummy)	{ init(e, e_p_1, lev); }

		inline int __attribute__((always_inline)) 
		end_condition()  {
            TreeClos clos = body_buffer[body_index];
#if defined NESTED_TLS
            return clos.empty();
#else
  			if(clos.empty()) {
				level--;
				if(level<0) return true;
    			TreeIterator iter(level);   		*body_iterator     = iter;
				TreeIterator iter_p_1(level+1);		*body_iterator_p_1 = iter_p_1;
				empty_iteration = true;
			}
			return false;
#endif
		}

		inline void __attribute__((always_inline))
		updateInductionVars() {	body_index = 0; }

		inline int __attribute__((always_inline)) 
		iteration_body() {  
			int body_index_m_2 = (body_index << 1);
			TreeClos clos   = body_buffer[body_index];
			Tree*    t      = clos.get();
			TreeClos clos_l = body_buffer_p_1[body_index_m_2];  
			TreeClos clos_r = body_buffer_p_1[body_index_m_2+1];
//			cout<<"Before merge: "<<this->getID()<<endl;
#if defined( HAND_PAR )
			Tree* merged = t->merge(clos_l.get(), clos_r.get());
#else
			Tree* merged = t->merge_TLS(clos_l.get(), clos_r.get(), this);			
#endif
//			cout<<"Before merge: "<<this->getID()<<endl;
			clos.set(merged);
			body_index++;
		}

		inline int __attribute__((always_inline)) 
		non_spec_iteration_body() {
			if(body_iterator->hasMoreElements()) {
				int body_index_m_2 = (body_index << 1);
				TreeClos clos   = body_iterator->nextElement();
				Tree*    t      = clos.get();
				TreeClos clos_l = body_iterator_p_1->nextElement();   
				TreeClos clos_r = body_iterator_p_1->nextElement();
				Tree* merged = t->merge(clos_l.get(), clos_r.get());
				clos.set(merged);
				body_index++;
			}	
		}
};



typedef Thread_TSP_Merge_TLS    SpThMerge;
