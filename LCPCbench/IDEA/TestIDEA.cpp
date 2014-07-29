using namespace std;

//#define MACHINE_X86_64
//#define MODEL_LOCK
//#define MODEL_FENCE

#include<time.h>
#include<iostream>
#include<signal.h>

#ifndef COSMINIDEA
#define COSMINIDEA
#include "CosminIDEA.h"
#endif

#ifndef PTHREAD
#define PTHREAD
#include <pthread.h>
#endif


#ifndef THREAD_MANAGERS
#define THREAD_MANAGERS
#include "Managers/Thread_Manager.h"
#endif  


/** if OPTIM_HASH_RO is defined then the Read-Only model is used
      in addition to one IP or SC models
    if OPTIM_HASH is defined then an optimised hash function is used
      (of small cardinality)
    if TRIVIAL HASH is used then we assume a one-to-one-like mapping
      between memory locations and TLS metadata indexes
    if NAIVE HASH is used then we assume a hash of cardinality big 
      enough to disambiguate the concurrently iteration window.
**/

//#define TRIVIAL_HASH
//#define NAIVE_HASH
#define OPTIM_HASH
#define OPTIM_HASH_RO


//const int ROLL = 0;

/**************************************************************************/
/*********************MemoryModels*****************************************/
/**************************************************************************/

#if (VER==1)

	#if defined( TRIVIAL_HASH )
		const int LDV_SZ 	= 24;
		const int SHIFT_FACT 	= 0;
		const int LDV_SZro 	= 24;
		const int SHIFT_FACTro 	= 0;
	#elif defined( NAIVE_HASH )
		const int LDV_SZ 	= 12;
		const int SHIFT_FACT 	= 0;
		const int LDV_SZro 	= 12;
		const int SHIFT_FACTro 	= 0;
	#else //OPTIM_HASH or OPTIM_HASH_RO
		const int LDV_SZ 	= 4; //7;    //2
		const int SHIFT_FACT	= 9;//7 but UNROLL=128
                const int LDV_SZro 	= 4; //7; //7; //6;
		const int SHIFT_FACTro 	= 9; //7;
	#endif

#else //VER==2

	#if defined( TRIVIAL_HASH )
		const int LDV_SZ 	= 24;
		const int SHIFT_FACT 	= 0;
		const int LDV_SZro 	= 17;
		const int SHIFT_FACTro 	= 0;
	#elif defined( NAIVE_HASH )
		const int LDV_SZ 	= 14;
		const int SHIFT_FACT 	= 0;
		const int LDV_SZro 	= 17;
		const int SHIFT_FACTro 	= 0;
	#else //OPTIM_HASH or OPTIM_HASH_RO
		const int LDV_SZ 	= 4;    //6 2
		const int SHIFT_FACT 	= 9;//7 but UNROLL=128
                const int LDV_SZro 	= 0; //6
		const int SHIFT_FACTro 	= 0; //5
	#endif

#endif

/***
 * Take CARE: PROCS_NR and UNROLL determines what LDV_SZ and SHIFT_FACT are
 *   i.e. all values are correct, but some will perform much better than other:
 * FOR 8 processors (instead of 4) expand LDV_SZ by a factor of 2.
 */

const int UNROLL     	= 256; //512;//256; 
const int ENTRY_SZ   	= 6*UNROLL; 
const int PROCS_NR   	= 4;
const unsigned long 	R = ROUNDS;
//const unsigned long sz = (1<<LDV_SZ);



/** If HAND_PAR is defined then the code is executed without speculation **/
/** If SP_IP is defined then the In-Place Model is used **/
/** otherwise if neither SP_IP nor HAND_PAR is defined then the speculative Serial-Commit Model is used **/
#define SP_IP
//#define HAND_PAR




IDEAkey Z;
IDEAkey DK;



/*********************************************************************/
/*********************** CONSTRUCT THE MEMORY MODELS *****************/
/*********************************************************************/


typedef HashSeqPow2 Hash;
Hash hash(LDV_SZ, SHIFT_FACT);



#if defined( SP_IP )
  typedef SpMod_IPcore<Hash>   IPm_core;
  IPm_core                     m_core(hash);
#else
  typedef SpMod_SCcore<Hash>   IPm_core;
  IPm_core                     m_core(hash);
#endif



#if (VER==1)

#if defined( SP_IP )
    typedef SpMod_IPbw<Hash, &m_core, u16>    IPm;
    IPm                                       ip_m(ENTRY_SZ);
	const int ATTR = IP_ATTR;
#else
    typedef SpMod_SCbw<Hash, &m_core, u16>  IPm;
    IPm                                     ip_m(ENTRY_SZ);
	const int ATTR = ItIndAttr_WAR_SV; 
//	const int ATTR = ItIndAttr_HAfw; 
#endif

#else

#if defined( SP_IP )
    typedef SpMod_IP<Hash, &m_core, u16>      IPm;
    IPm                                       ip_m(ENTRY_SZ);
	const int ATTR = IP_ATTR;
#else
    typedef SpMod_SC<Hash, &m_core, u16>      IPm;
    IPm                                       ip_m(ENTRY_SZ);  
	const int ATTR = ItIndAttr_WAR_SV;  
#endif

#endif


typedef SpMod_ReadOnly<u16> SpRO;
SpRO ro_m1;

#ifdef OPTIM_HASH_RO
	typedef SpRO SpROlike; 
	SpROlike	ro_m; 
	const int 	SpROlike_ATTR = 0;
#else

#if defined( SP_IP )
	Hash hash1(LDV_SZro, SHIFT_FACTro);
	IPm_core 								m_core1(hash1);
	typedef SpMod_IP<Hash, &m_core1, u16>	IPm1;

	typedef IPm1 SpROlike; 
	SpROlike	ro_m(4);

	const int 	SpROlike_ATTR = IP_ATTR;
#else
	Hash hash1(LDV_SZro, SHIFT_FACTro);

	IPm_core 								m_core1(hash1);
	typedef SpMod_SC<Hash, &m_core1, u16>	IPm1;

	typedef IPm1 SpROlike; 
	SpROlike	ro_m(4);

	const int 	SpROlike_ATTR = ItIndAttr_WAR_SV;
#endif

#endif


typedef USpModel<
			IPm, &ip_m, ATTR, 
			USpModel<
				SpROlike, &ro_m, SpROlike_ATTR, 
				USpModel<SpRO, &ro_m1, 0, USpModelFP>
			>
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

class Thread_DeKey :
				public SpecMasterThread<Thread_DeKey, UMeg, ThManager>
{
	private:
		typedef SpecMasterThread< Thread_DeKey, UMeg, ThManager >	MASTER_TH;
		unsigned long 	j;
		IDEAKeyPiece	origZ;
		IDEAKeyPiece	ZZ;
		unsigned long 	r;		
		IDEAKeyPiece	origP;
		IDEAKeyPiece	PP;
		
	public:
		inline Thread_DeKey(): MASTER_TH(UNROLL, NULL)	{ 
		}
		inline Thread_DeKey(const unsigned long i, unsigned long* dummy) 
						: MASTER_TH(UNROLL,i,dummy)	{ 
			//cout<<"Pre read: "<<umm.specLD<SpROlike, &ro_m>(Z+26, this)<<endl;
		}

		inline void initVars(IDEAKeyPiece ZZZ, IDEAKeyPiece PPP) {
			origZ = ZZZ; origP = PPP; 
		}

		inline int __attribute__((always_inline)) 
		end_condition() const {
			return (j>=R);
		}

		inline void __attribute__((always_inline))
		updateInductionVars() {
			unsigned long it  = ( this->id - ttmm.firstID() )*UNROLL, it4 = it<<2, it2 = it<<1; 
			unsigned long tmp = it4+it2;
			j = it;
			ZZ = (IDEAKeyPiece)(origZ+tmp);
			PP = (IDEAKeyPiece)(origP-tmp);
			//r = ROUNDS;
		}

		inline int __attribute__((always_inline)) 
		iteration_body() {
#if defined( HAND_PAR )
			de_key_idea_iter_par(ZZ, PP);
#else

			//cout<<"     ZZ: "<<ZZ<<" PP: "<<PP<<endl;
			de_key_idea_iter_spec(ZZ, PP);
#endif
			j++;
		}

		inline int __attribute__((always_inline)) 
		non_spec_iteration_body() {
			de_key_idea_iter_par(ZZ, PP);
			j++;
		}

		inline void de_key_idea_iter_par(IDEAKeyPiece& Z, IDEAKeyPiece& p) {
			u16 t1, t2, t3;
			
			t1=*Z++;			*--p=*Z++;			*--p=t1;			t1=inv(*Z++);			t2=-*Z++;			t3=-*Z++;			*--p=inv(*Z++);
			*--p=t2;			*--p=t3;			*--p=t1;
		}


		inline void de_key_idea_iter_spec(IDEAKeyPiece& Z, IDEAKeyPiece& p) {

			u16 t1, t2, t3;

			t1=this->specLD<SpROlike, &ro_m>(Z); Z++;
			this->specST<IPm, &ip_m>( --p, this->specLD<SpROlike, &ro_m>(Z) );   Z++; 
			this->specST<IPm, &ip_m>( --p, t1 );   

			t1 = inv(this->specLD<SpROlike, &ro_m>(Z)); Z++;
			t2 = -this->specLD<SpROlike, &ro_m>(Z); Z++;
			t3 = -this->specLD<SpROlike, &ro_m>(Z); Z++;

			this->specST<IPm, &ip_m>(
				--p, inv(this->specLD<SpROlike, &ro_m>(Z)) );   Z++;
			this->specST<IPm, &ip_m>(--p, t2);  
			this->specST<IPm, &ip_m>(--p, t3);  
			this->specST<IPm, &ip_m>(--p, t1);
		}
};

//template <class TM, const unsigned long uroll>
class Thread_Cipher : 
		public SpecMasterThread<Thread_Cipher, UMeg, ThManager>

{
	private:
		typedef SpecMasterThread< Thread_Cipher, UMeg, ThManager >	MASTER_TH;

		IDEAKeyPiece	plain;
		unsigned long 	j;		
		IDEAKeyPiece	crypt;
		IDEAKeyPiece	Z;
		
	public:
		inline Thread_Cipher(): MASTER_TH(UNROLL, NULL)	{ }
		inline Thread_Cipher(const unsigned long i, unsigned long* dummy) 
						: MASTER_TH(UNROLL,i,dummy)	{ }

		inline void initVars(IDEAKeyPiece plain1, IDEAKeyPiece crypt1, IDEAKeyPiece Z1) {
			Z = Z1; plain = plain1; crypt = crypt1; 
		}

		inline int __attribute__((always_inline)) 
		end_condition() const {
			return (j>=ARRAY_SIZE);
		}

		inline void __attribute__((always_inline))
		updateInductionVars() {
			j = (( ( this->id - ttmm.firstID() )*UNROLL )<<2);
		}

		inline int __attribute__((always_inline)) 
		iteration_body() {
#if defined( HAND_PAR )
			cipher_idea_par ( (plain+j), (crypt+j), Z );
#else
			cipher_idea_spec( (plain+j), (crypt+j), Z );
#endif
			j+=4;
		}

		inline int __attribute__((always_inline)) 
		non_spec_iteration_body() {
			cipher_idea_par( (plain+j), (crypt+j), Z );
			j+=4;
		}

    inline void cipher_idea_par (u16 in[4], u16 out[4], IDEAkey Z) {
		register u16 x1, x2, x3, x4, t1, t2, tmp;
		int r=ROUNDS;

		x1=*in++;
		x2=*in++;
		x3=*in++;
		x4=*in;

		do {
    	    MUL(x1,*Z++);			x2+=*Z++;
			x3+=*Z++;
			MUL(x4,*Z++);

			t2=x1^x3;
			MUL(t2,*Z++);			t1=t2+(x2^x4);
			MUL(t1,*Z++);
			t2=t1+t2;

			x1^=t1;
			x4^=t2;
        
			t2^=x2;
			x2=x3^t1;
			x3=t2;
		} while(--r);
		MUL(x1,*Z++);
		*out++=x1;
		*out++=x3+*Z++;
		*out++=x2+*Z++;
		MUL(x4,*Z);
		*out=x4;
	}

	inline void cipher_idea_spec(u16 in[4], u16 out[4], IDEAkey Z) {
		register u16 x1, x2, x3, x4, t1, t2, tmp;
		int r=ROUNDS;

		x1 = this->specLD<SpRO, &ro_m1>(in); in++;
		x2 = this->specLD<SpRO, &ro_m1>(in); in++;
		x3 = this->specLD<SpRO, &ro_m1>(in); in++;
		x4 = this->specLD<SpRO, &ro_m1>(in); 

		do {
			tmp = this->specLD<SpROlike, &ro_m>(Z);
	        MUL(x1, tmp); Z++;

	        x2+=this->specLD<SpROlike, &ro_m>(Z);      Z++;
	        x3+=this->specLD<SpROlike, &ro_m>(Z);      Z++;

			tmp = this->specLD<SpROlike, &ro_m>(Z);
	        MUL(x4, tmp); Z++;

	        t2=x1^x3;
			tmp = this->specLD<SpROlike, &ro_m>(Z);
	        MUL(t2, tmp); Z++;
	        t1=t2+(x2^x4);
			tmp = this->specLD<SpROlike, &ro_m>(Z);
	        MUL(t1, tmp); Z++;
	        t2=t1+t2;

	        x1^=t1;
	        x4^=t2;
        
	        t2^=x2;
	        x2=x3^t1;
	        x3=t2;
		} while(--r);
	
		tmp = this->specLD<SpROlike, &ro_m>(Z);

		MUL(x1, tmp);										Z++;
		this->specST<IPm, &ip_m>(out, x1);           		out++;

		tmp = x3 + this->specLD<SpROlike, &ro_m>(Z);
		this->specST<IPm, &ip_m>(out, tmp);					Z++; out++;

		tmp = x2 + this->specLD<SpROlike, &ro_m>(Z);
		this->specST<IPm, &ip_m>(out, tmp);					Z++; out++;

		tmp = this->specLD<SpROlike, &ro_m>(Z);
		MUL(x4, tmp);
		this->specST<IPm, &ip_m>(out, x4);
	}  //umm.specLD<SpRO, &ro_m>(Z, this);
};


/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#if (VER==1)
	typedef Thread_DeKey						SpecTh;
#else
	typedef Thread_Cipher						SpecTh;
#endif

timeval start_time, end_time;
unsigned long running_time = 0;




#if(VER==1)

/**************************** DeKey ******************************/

/** Parallel Speculative**/
int testParallelSpec(IDEAKeyPiece Z, IDEAKeyPiece P) {
		ip_m.setIntervalAddr(P-KEYLEN, P-1);

#ifndef OPTIM_HASH_RO
		ro_m.setIntervalAddr( Z ,  Z + KEYLEN );
#endif

		cout<<"Zbeg: "<<(void*)Z<<" Zend: "<<(void*)(Z+KEYLEN)<<" Pbeg: "<<P-KEYLEN<<" Pend: "<<P-1<<" "<<
		( (void*)(Z+KEYLEN) < (void*)(P-KEYLEN) )<<" "<<( (void*)(P-KEYLEN) < (void*)(P-1) )<<endl;

		for(int i=0; i<PROCS_NR; i++) {
			SpecTh* thr = allocateThread<SpecTh>(i, 64);
			ttmm.registerSpecThread(thr,i);
			thr->initVars(Z, P);
		}

		cout<<"Spec: ";
		gettimeofday ( &start_time, 0 );
	
		ttmm.speculate<SpecTh>();  

		gettimeofday( &end_time, 0 );
		running_time += DiffTime(&end_time,&start_time);
}


/** Parallel Optimal **/
struct Args { unsigned long start; unsigned long end; unsigned long id; IDEAKeyPiece Zorig; IDEAKeyPiece Porig; };

inline void* ParallelFour(void* args) {
	struct Args* params = (struct Args*)args;

	unsigned long start = params->start; 
	unsigned long end   = params->end;
	IDEAKeyPiece ZZ     = params->Zorig;
	IDEAKeyPiece PP     = params->Porig;
	IDEAKeyPiece Z = ZZ+6*(start-1), p = PP-6*(start-1);
	u16 t1, t2, t3;

	for(int j=start;j<end;j++)
	{   
	    t1=*Z++;
        *--p=*Z++;
        *--p=t1;
        t1=inv(*Z++);
        t2=-*Z++;
        t3=-*Z++;
        *--p=inv(*Z++);
        *--p=t2;
        *--p=t3;
        *--p=t1;
	}
}
int testOptimalParallel(IDEAKeyPiece Z, IDEAKeyPiece P) {
	pthread_t 			pthread[PROCS_NR];
	pthread_attr_t 		thread_attr[PROCS_NR];
	struct sched_param 	thread_param[PROCS_NR];
	
	struct Args			th_args[PROCS_NR];

	unsigned long th_beg = 1; unsigned long third = R/PROCS_NR; unsigned long th_end = th_beg + third;

	cout<<"Para: "<<third<<" R: "<<R<<" R/8: "<<R/8<<endl;

	gettimeofday ( &start_time, 0 );

	for(int i=0; i<PROCS_NR; i++) {
			//if(FIFO_SCHED)
			//	Spec_Thread0::setFIFOthread(&thread_attr[i], &thread_param[i]);
			//else
				Spec_Thread0::setUSUALthread(&thread_attr[i]);
			
			th_args[i].start = th_beg; th_args[i].end = th_end; th_args[i].id = i; th_args[i].Zorig = Z; th_args[i].Porig = P;
			if(i==PROCS_NR-1) th_args[i].end = R+1;
			if(i==0) th_args[i].start = 1;
			int success = pthread_create( &pthread[i], &thread_attr[i], &ParallelFour, &th_args[i] );
			th_beg += third; th_end = th_beg + third;
	}
	for(int i=0; i<PROCS_NR; i++) pthread_join(pthread[i], NULL);

	gettimeofday( &end_time, 0 );
	running_time += DiffTime(&end_time,&start_time);
}

/** Sequential **/


int testSequential(IDEAKeyPiece Zorig, IDEAKeyPiece Porig) {
	u16 t1, t2, t3;
	cout<<"Sequ: ";

	gettimeofday ( &start_time, 0 );

	IDEAKeyPiece Z = Zorig, p = Porig;
	for(int j=1;j<ROUNDS+1;j++)
	{      
		t1=*Z++;
        *--p=*Z++;
        *--p=t1;
        t1=inv(*Z++);
        t2=-*Z++;
        t3=-*Z++;
        *--p=inv(*Z++);
        *--p=t2;
        *--p=t3;
        *--p=t1;
	}

	gettimeofday( &end_time, 0 );
	running_time += DiffTime(&end_time,&start_time);
	//cout<<"ROUNDS: "<<ROUNDS<<endl;
}

//const int ITER_SIZE = 3000000;
pthread_attr_t 		thread_attr;
struct sched_param 	thread_param;
		
//IDEAkey Z, DK;

int main(int argc, char* argv[]) {	
	int i,j;     
	
	u16 userkey[8];
	u16 *plain1;               /* First plaintext buffer */
	u16 *crypt1;               /* Encryption buffer */
	u16 *plain2;               /* Second plaintext buffer */

	/*
	** Re-init random-number generator.
	*/
	randnum(3L);
	/*
	** Build an encryption/decryption key
	*/
	for (i=0;i<8;i++)
        userkey[i]=(u16)(abs_randwc(60000L) & 0xFFFF);
	for(i=0;i<KEYLEN;i++)
        Z[i]=0;
    
	
	/*
	** Compute encryption/decryption subkeys
	*/
	en_key_idea(userkey,Z);

	u16 *p;
	u16* ZZ = Z;
	/****/
	{
		
		u16 t1, t2, t3;
		
		p=(u16 *)(DK+KEYLEN);

		t1=inv(*ZZ++); 
		t2=-*ZZ++;
		t3=-*ZZ++;
		*--p=inv(*ZZ++);
		*--p=t3;
		*--p=t2;
		*--p=t1;
	}
	/****/


	if(atoi(argv[1])==33) {
		testSequential(ZZ, p);
	} else if(atoi(argv[1])==34) {
		testOptimalParallel(ZZ, p);
	} else if(atoi(argv[1])==35) {
		testParallelSpec(ZZ, p);
		cout<<"Nr Rollbacks: "<<ttmm.nr_of_rollbacks<<endl;
	}

	cout<<"Running time: "<<running_time<<endl;
	cout<<" DK[100]"<<DK[100]<<" DK[1000]: "<<DK[1000]<<" DK[10000]: "<<DK[10000]<<endl;
	
	//for(int i=0; i<256; i++) cout<<" Z[10000+"<<i<<"]: "<<ZZ[/*10000+*/i];
	//cout<<endl;
	return 1;

}

#else

/**************************** CIPHER ******************************/

/** Parallel Speculative**/
int testParallelSpec(IDEAKeyPiece plain1, IDEAKeyPiece crypt1, IDEAKeyPiece Z) {

		ip_m.setIntervalAddr(crypt1, crypt1+ARRAY_SIZE);

#ifndef OPTIM_HASH_RO
		ro_m.setIntervalAddr( Z ,  Z + KEYLEN );
#endif

		for(int i=0; i<PROCS_NR; i++) {
			SpecTh* thr = allocateThread<SpecTh>(i, 64);
			ttmm.registerSpecThread(thr,i);
			thr->initVars(plain1, crypt1, Z);
		}

		cout<<"Spec: ";
		gettimeofday ( &start_time, 0 );
	
		ttmm.speculate<SpecTh>();  

		gettimeofday( &end_time, 0 );
		running_time += DiffTime(&end_time,&start_time);
}

/** Parallel Optimal **/
struct Args { unsigned long start; unsigned long end; unsigned long id; IDEAKeyPiece plain1; IDEAKeyPiece crypt1; IDEAKeyPiece Zorig; };

inline void* ParallelFour(void* args) {
	struct Args* params = (struct Args*)args;

	unsigned long start = params->start; 
	unsigned long end   = params->end;
	IDEAKeyPiece ZZ     = params->Zorig;
	IDEAKeyPiece plain  = params->plain1;
	IDEAKeyPiece crypt  = params->crypt1;
	

	for(int j=start; j<end; j+=4)
    	cipher_idea( (plain+j),(crypt+j),ZZ);       // Encrypt 
}
int testOptimalParallel(IDEAKeyPiece plain1, IDEAKeyPiece crypt1, IDEAKeyPiece Z) {
	pthread_t 			pthread[PROCS_NR];
	pthread_attr_t 		thread_attr[PROCS_NR];
	struct sched_param 	thread_param[PROCS_NR];
	
	struct Args			th_args[PROCS_NR];

	unsigned long th_beg = 0; unsigned long third = ARRAY_SIZE/PROCS_NR; unsigned long th_end = th_beg + third;

	cout<<"Para: ";

	gettimeofday ( &start_time, 0 );

	for(int i=0; i<PROCS_NR; i++) {
			//if(FIFO_SCHED)
			//	Spec_Thread0::setFIFOthread(&thread_attr[i], &thread_param[i]);
			//else
				Spec_Thread0::setUSUALthread(&thread_attr[i]);
			
			th_args[i].start = th_beg; th_args[i].end = th_end; th_args[i].id = i; 
			th_args[i].Zorig = Z; th_args[i].plain1 = plain1; th_args[i].crypt1 = crypt1;

			if(i==PROCS_NR-1) th_args[i].end = ARRAY_SIZE;
			int success = pthread_create( &pthread[i], &thread_attr[i], &ParallelFour, &th_args[i] );
			th_beg += third; th_end = th_beg + third;
	}
	for(int i=0; i<PROCS_NR; i++) pthread_join(pthread[i], NULL);

	gettimeofday( &end_time, 0 );
	running_time += DiffTime(&end_time,&start_time);
}

/** Sequential **/


int testSequential(IDEAKeyPiece plain1, IDEAKeyPiece crypt1, IDEAKeyPiece Z) {
	cout<<"Sequ: ";

	gettimeofday ( &start_time, 0 );

	for(int j=0;j<ARRAY_SIZE;j+=4)
    	cipher_idea( (plain1+j),(crypt1+j),Z);       // Encrypt 

	gettimeofday( &end_time, 0 );
	running_time += DiffTime(&end_time,&start_time);
	//cout<<"ROUNDS: "<<ROUNDS<<endl;
}


int main(int argc, char* argv[]) {	

	int i,j;     
	
	u16 userkey[8];
	u16 *plain1;               // First plaintext buffer 
	u16 *crypt1;               // Encryption buffer 
	u16 *plain2;               // Second plaintext buffer 

	
	// Re-init random-number generator.
	
	randnum(3L);
	
	// Build an encryption/decryption key
	
	for (i=0;i<8;i++)
        userkey[i]=(u16)(abs_randwc(60000L) & 0xFFFF);
	for(i=0;i<KEYLEN;i++)
        Z[i]=0;
    
	
	
	// Compute encryption/decryption subkeys
	
	en_key_idea(userkey,Z);

	de_key_idea(Z,DK);


	plain1=new u16[ARRAY_SIZE];
	crypt1=new u16[ARRAY_SIZE];
	plain2=new u16[ARRAY_SIZE]; 


	for(i=0;i<ARRAY_SIZE;i++)
        plain1[i]=(abs_randwc(255) & 0xFF);
	
    //for(j=0;j<ARRAY_SIZE;j+=4)
    //	cipher_idea( (plain1+j),(crypt1+j),Z);       // Encrypt 

	if(atoi(argv[1])==33) {
                // SEQUENTIAL
		testSequential(plain1, crypt1, Z);
	} else if(atoi(argv[1])==34) {
		// OPTIMAL PARALLEL
		testOptimalParallel(plain1, crypt1, Z);
	} else if(atoi(argv[1])==35) {
		// POTENTIALLY SPECULATIVE
		testParallelSpec(plain1, crypt1, Z);
		cout<<"Nr Rollbacks: "<<ttmm.nr_of_rollbacks<<endl;
	}

	cout<<"Running time: "<<running_time<<endl;
	cout<<" crypt1[100]: "<<crypt1[100]<<" crypt1[1000]: "<<crypt1[1000]<<" crypt1[10000]: "<<crypt1[10000]
		<<" crypt1[100000]: "<<crypt1[100000]<<" crypt1[300000]: "<<crypt1[300000]<<endl;
	
	return 1;
}


#endif

//
//g++ -I/scl/people/coancea/TLSframework/currentTLS/Cpp/STLcontainersSpec/ -pthread -O3 -w SpecThreadImpl.cpp
// g++ -I /scl/people/coancea/TLSproject/current/currentTLS/CppTLS/ -pthread -O3 -w SpecThreadImpl.cpp
// g++ -I ~/work/CppTLS -D_REENTRANT -lpthread -O3 -w SpecThreadImpl.cpp

