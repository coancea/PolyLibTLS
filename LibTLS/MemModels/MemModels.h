#ifndef MEM_MODELS
#define MEM_MODELS


#ifndef STL_VECTOR
#define STL_VECTOR
#include <vector>
#endif 

#ifndef STL_ALG
#define STL_ALG
#include <algorithm>
#endif

#include "MemModels/Dep_Violation.h"
#include "MemModels/HashFunctions.h"

using namespace std;


class IntervalExc { };


#ifdef SLOW_BOUNDS_CHECK
/** if the interval is not correctly guessed by the user/compiler
 *    all the intervals in the unified speculative model are going 
 *    to be checked.
 */
#define THROW_BOUNDS_EXC(x,y) throw IntervalExc()
#else
/** if the interval is not correctly guessed by the user/compiler
 *    treat it as a dependency violation -- this compiles to faster
 *    code if the guess is accurate!
 */
#define THROW_BOUNDS_EXC(x,y) throw Dependence_Violation(th_num-1, th_num);
#endif


/********************************************************************/
/*********************** THE READ-ONLY MODEL ************************/
/********************************************************************/


template<class T>
class SpMod_ReadOnly : public VoidSpecModel {
  private:
    int P;
  public:
    typedef T ElemType;

    inline ID_WORD __attribute__((always_inline))
    checkBounds(T* addr, const ID_WORD th_num = 1) throw(IntervalExc){
      if(addr>=this->addr_beg && addr<this->addr_end) return 1;
      else THROW_BOUNDS_EXC(th_num-1, th_num)
    }

    inline ID_WORD __attribute__((always_inline))
    checkBoundsSlow(T* addr, const ID_WORD th_num = 1) {
      if(addr>=this->addr_beg && addr<this->addr_end) return 1;
      else return ID_WORD_MAX;
    }

    inline SpMod_ReadOnly() { }

    template<class TH>
    inline T  __attribute__((always_inline))
    specLD(volatile T* address, const ID_WORD ind = 0, const ID_WORD th_id=0, TH* th = NULL) {
      return *address;
    }

    template<class TH> 
    inline void __attribute__((always_inline))
    specST(volatile T* address, const T& val, const ID_WORD ind = 0, const ID_WORD th_id=1, TH* th = NULL) 
               throw(Dependence_Violation) {  
      throw Dependence_Violation(th_id-1, th_id);
    }


    inline void __attribute__((always_inline))
    setNumProc(const int num_proc) { 
        P = num_proc;
    }

};




/********************************************************************/
/********************* THE SERIAL_COMMIT MODEL **********************/
/********************************************************************/

/*** First the core -- implementing the specLD/specST functions ***/

template<class HashFct>
class SpMod_SCcore  {
  private:
    volatile ID_WORD*  LdVct;
    const ID_WORD      spec_size;
    int                P;

  public:
    HashFct  hashFct;

    inline SpMod_SCcore(HashFct obj) : 
      spec_size( obj.cardinal()<<2 ), hashFct(obj)  
    { 
      P = 8;
      LdVct = new ID_WORD[spec_size];
      reset();
    }	

    inline ~SpMod_SCcore() { delete[] LdVct; }	

    inline void __attribute__((always_inline))
    setNumProc(const int num_proc) { 
        P = num_proc;
    }

    /** NOT USED ??? **/
    inline ID_WORD __attribute__((always_inline))
    getHighestLoad(const ID_WORD ind) const {
      int index = ind<<2;
      FULL_WORD val1 = LdVct[index], val2 = LdVct[index+1];
      return (val2 > val1)? val2 : val1;
    }
		
    inline ID_WORD getIndex(const ID_WORD addr) { return hashFct(addr); }

    inline void __attribute__((always_inline))
    markLD(FULL_WORD index, const FULL_WORD th_id_wd) {
      const    WORD_PER_4 th_id    = convert (th_id_wd);
      volatile FULL_WORD* full_pos = LdVct + (index<<2);

      WORD_PER_4 prev_ld =  *full_pos;
      if(prev_ld >= th_id) return;

      *(full_pos+3) = th_id;

      prev_ld = *full_pos;
      if(prev_ld < th_id) *full_pos = th_id;

      WORD_PER_4 syncW = *(full_pos+3);
      if(syncW != th_id) {
        *(full_pos+1) = th_id + P;     //// SHOULD BE NUM_PROCS!!!
      }
    }

    /**
     * marks the highest load and ensures synchronization
     */
    template<class T, class TH>
    inline T __attribute__((always_inline))
    specLD(volatile T* address, const ID_WORD ind, const ID_WORD th_id, TH* th) {
      markLD(ind, th_id);
      __asm__("mfence;");
      if( th->getHasNotWrote(th_id, (T*)address) ) { return * ((T*)address);              }
      else { /*cout<<"UGLY, EXPENSIVE load"<<endl;*/ return th->getLoopIndWrite(address); }
    }


    /**
     * Records the store information to be commited in sequential order at 
     * the end of the thread
     */
    template<class T, class TH> 
    inline void __attribute__((always_inline))
    specST(volatile T* address, const T& val, 
           const ID_WORD ind, const ID_WORD th_id, TH* th ) {  
      th->store(address, val, ind);
      th->markInternalWrite(th_id, (T*)address/*ind*/);
    }

    inline void reset() {
      for(int i=0; i<spec_size; i++) {
         LdVct[i] = 0;
      }
    }

    inline void reset(const int id, const int highestThNr) {
      reset();
    }

    inline ID_WORD cardinal() const {
      return hashFct.cardinal();
    }
};


/*** First the core -- implementing the serial-commit phase ***/


template<class HashFct, SpMod_SCcore<HashFct>* LSmem, class T> 
class SpMod_SC0 : public VoidSpecModel {
  private:
    const ID_WORD wr_per_it;

  public:
    typedef T ElemType;

    inline SpMod_SC0()                                        : wr_per_it(64)  {  }
    inline SpMod_SC0(const ID_WORD wrs)                       : wr_per_it(wrs) {  }
    inline SpMod_SC0(T* addr1, T* addr2, const ID_WORD wrs)   : wr_per_it(wrs) { 
      this->setIntervalAddr(addr1, addr2);
    }
    inline SpMod_SC0(T* addr1, ID_WORD len, const ID_WORD wrs): wr_per_it(wrs) { 
      this->setIntervalAddr(addr1, addr1+len);
    }

    inline void __attribute__((always_inline))
    setNumProc(const int num_proc) { 
        LSmem->setNumProc(num_proc);
    }

    inline ID_WORD getWritesPerIt() {  return wr_per_it;  }

    /** forwards invocation to SpMod_SCcore **/
    inline void __attribute__((always_inline))
    markLD(ID_WORD index, ID_WORD thread_id) {
      LSmem->markLD(index, thread_id);
    }

    /*****************************************/
    /** forward invocations to SpMod_SCcore **/
    /*****************************************/

    template<class TH>
    inline T  __attribute__((always_inline))
    specLD(volatile T* address, const ID_WORD ind, const ID_WORD th_id, TH* th) {
      return LSmem->specLD(address, ind, th_id, th);
    }

    template<class TH> 
    inline void __attribute__((always_inline))
    specST(volatile T* address, const T& val, const ID_WORD ind, const ID_WORD th_id, TH* th) {
      return LSmem->specST(address, val, ind, th_id, th);
    }

    inline void    reset   (const ID_WORD safe_th, const ID_WORD highest_th) { LSmem->reset();           }
    inline void    reset   ()                                                { LSmem->reset();           }
    inline ID_WORD cardinal() const                                          { return LSmem->cardinal(); }


    /**
     * Commits the stores at the end of the principal thread;
     * All the other threads are in busy waiting
     * Design a light mechanism to handle the loop-independent writes! 
     */
    template<class TH>
    inline void __attribute__((always_inline))
    commit_at_iteration_end	(TH* th, const ID_WORD thread)  {
      ID_WORD         roll = ID_WORD_MAX;
      SavedTupleSC<T>* beg  = th->getBeginCursor();
      SavedTupleSC<T>* cur  = beg;
      SavedTupleSC<T>* end  = th->getCursor();
		
      int count = 0;
      while(cur<end) {
        T* address = (T*)cur->addr;
        *address = cur->val;

        __asm__("mfence;");

        ID_WORD highestLoad = LSmem->getHighestLoad(cur->index);

        if(highestLoad>thread && roll>highestLoad) { roll = highestLoad; }
        count++; cur++;
      }
		
      th->resetCursor();
      if(roll == ID_WORD_MAX)     return;
      else {
        throw Dependence_Violation(roll, thread); 
      }
    }

    inline void __attribute__((always_inline))
    recoverFromRollback(const int safe_thread, const int highest_id_thread) { } 
};


/*** And now the user class that adds coarse-grained interval bounds checks ***/

template< class HashFct, SpMod_SCcore<HashFct>* LSmem, class T > 
class SpMod_SC : public SpMod_SC0< HashFct, LSmem, T > {
  public:
    inline SpMod_SC(const ID_WORD wrs) : SpMod_SC0<HashFct, LSmem, T>(wrs) {  }

    inline ID_WORD checkBounds(T* addr, const ID_WORD th_num=1) {
      ID_WORD diff = (ID_WORD)(addr-(T*)this->addr_beg);
      // addr>=addr_beg && addr<addr_end  iff dif<A_m_a
      if(diff<=this->A_m_a) return LSmem->getIndex( diff );
      else THROW_BOUNDS_EXC(th_num-1, th_num);
    }

    ID_WORD checkBoundsSlow(T* addr, const ID_WORD th_num = 1);
};


template< class HashFct, SpMod_SCcore<HashFct>* LSmem, class T > 
class SpMod_SCbw : public SpMod_SC0<HashFct, LSmem, T> {
  public:
    inline SpMod_SCbw(const ID_WORD wrs) : SpMod_SC0<HashFct, LSmem, T>(wrs) {  }

    /*** `diff' is computed w.r.t. the end address for codes such as: ***/
    /***  for(i=N; i>=0; i--) arr[i] += 2;                            ***/ 
    inline ID_WORD checkBounds(T* addr, const ID_WORD th_num=1) {
      ID_WORD diff = (ID_WORD)((T*)this->addr_end - addr);

      if(diff<=this->A_m_a) return LSmem->getIndex( diff );
      else THROW_BOUNDS_EXC(th_num-1, th_num);
    }

    ID_WORD checkBoundsSlow(T* addr, const ID_WORD th_num = 1);
};



/********************************************************************/
/************************ THE IN-PLACE MODEL ************************/
/********************************************************************/

// I HAVE REACHED HERE WITH POLISHING, PLEASE CONTINUE!!!!



/**                                    
 *   In-Place Speculative Model 	              
 *   COMMENTS:                                
 *      1. the loads and stores are performed in parallel.
 *      2. the writes are expected to occur in program order otherwise 
 *             yields violation               
 *      3. a load registers the highest loading thread per memory location
 *      4. Each thread writes in a buffer, and overwrites whatever it is written  
 *         in that buffer!     
 *      5. Layout with interleaved load/store/syncR/syncW vectors               
 */

template<class HashFct>
class SpMod_IPcore  {
  private:
    typedef  volatile WORD_PER_4 WD_LINE[CACHE_LINE/sizeof(WORD_PER_4)]; //WD_LINE[NrElemPerCacheLine];

    volatile  WD_LINE *   LdStVct;
    const     ID_WORD     spec_size;
    const     HashFct     hashFct;
    long*                 tmp;
    int                   P;

  public:

    /** each entry occupies exactly one cache line **/
    inline SpMod_IPcore(HashFct& obj) : 
            spec_size(obj.cardinal()), hashFct(obj)
    {
      P = 8;
      //LdStVct = new WD_LINE[spec_size];
      unsigned long long REM = CACHE_LINE*8;
      tmp = new long[spec_size*sizeof(WD_LINE)+REM];
      unsigned long long  offset = ((unsigned long long)&tmp[0])%REM;
      LdStVct = (WD_LINE*)(tmp + ((REM - offset)/8) );

      reset();
    }

    inline ~SpMod_IPcore() { delete[] tmp; }	

    inline void __attribute__((always_inline))
    setNumProc(const int num_proc) { 
        P = num_proc;
    }


    inline ID_WORD __attribute__((always_inline))
    getIndex(const ID_WORD addr) { return hashFct(addr); }

    inline void reset() { 
      for(int i=0; i<spec_size; i++) {
        for(int j=0; j<NrElemPerCacheLine; j++)
          LdStVct[i][j] = 0;
      }
    }
		
    inline void reset(const unsigned long id, const unsigned long highestThNr) {
      reset();
    }

    //*************************************************************************
    //********************   SpecLD/SpecST               **********************
    //*************************************************************************

#if defined( MODEL_LOCK )

    template<class T>
    inline T __attribute__((always_inline))
    specLD ( volatile T* address, const ID_WORD index, const ID_WORD th_id_wd ) 
    throw(Dependence_Violation)  {
      const WORD_PER_4      th_id        =  convert(th_id_wd);
      volatile WORD_PER_4*  pos      =  LdStVct[index];
      volatile FULL_WORD*   full_pos = (volatile FULL_WORD*) pos;
      volatile FULL_WORD*   lock     = full_pos+2;

      non_blocking_synch33(lock);
      WORD_PER_4 prev_ld = *pos;

      if(th_id > prev_ld) *pos = th_id;

      T* addr = (T*) address;
      T val = *addr;

      WORD_PER_4 prev_store  = *(pos+2);
      release_synch33(lock);

      if(prev_store > th_id) { 
        //cout<<"WAR!!!"<<*lock<<" "<<th_id<<endl;
        throw Dependence_Violation(th_id_wd-1, th_id_wd, Dependence_Violation::LOAD_TP);
      }

      return val;
    }


    template<class T, class TH>
    inline void __attribute__((always_inline)) specST(
      volatile T* address, const T& val,  const ID_WORD index, const ID_WORD th_id_wd, TH* th
    ) throw(Dependence_Violation) {
      const WORD_PER_4     th_id    = convert(th_id_wd);
      volatile WORD_PER_4* pos      = LdStVct[index];
      volatile FULL_WORD*  full_pos = (volatile FULL_WORD*) pos;
      volatile FULL_WORD*  lock     = full_pos+2;
		
      non_blocking_synch33(lock);

      WORD_PER_4 prev_str = *(pos+2);

      if(prev_str > th_id) {
        release_synch33(lock);
        throw Dependence_Violation(th_id_wd-1, th_id_wd, Dependence_Violation::STORE_TP);			
      }

      *(pos+2) = th_id;

      T* addr = (T*) address;
      T old_val = *addr;
      FULL_WORD stamp_val = ++(* (full_pos + 1) );
      th->save( addr, old_val, stamp_val );
		
      *addr = val;
		
      const WORD_PER_4 highest_ld = *pos;
      
      release_synch33(lock);

      if(highest_ld > th_id) {
        throw Dependence_Violation(th_id_wd-1, th_id_wd, Dependence_Violation::STORE_TP); 
      }
	       
      return;
    } 

#else

    inline void __attribute__((always_inline))
    markReadLock(volatile WORD_PER_4* pos, const WORD_PER_4 th_id) {
      volatile FULL_WORD* full_pos = (volatile FULL_WORD*)pos;
      non_blocking_synch33(full_pos+1);
      WORD_PER_4 syncR = read_4(full_pos, 1);
      if(syncR < th_id) *(pos+1) = th_id;
        *(full_pos+1) = 0; //release_synch(CR);
    }

    inline void __attribute__((always_inline)) 
    markRead(volatile WORD_PER_4* pos, const WORD_PER_4 th_id, const ID_WORD th_id_wd) {
      volatile FULL_WORD* full_pos = (volatile FULL_WORD*)pos;

      WORD_PER_4 prev_ld = read_4(full_pos, 0);

      *(pos+3) = th_id;                         // *(full_pos+1) = th_id;
 
      prev_ld = read_4(full_pos, 0);
      if(prev_ld < th_id) *pos = th_id;
      else return;

      WORD_PER_4 syncW = read_4(full_pos, 3);   // FULL_WORD syncW = *(full_pos+1);
      if(syncW != th_id) { 
        *(pos+1) = th_id + P;                       // CHANGE THIS TO BE THE NUMBER OF PROCESSORS!!!
      }
    }

    template<class T>
    inline T __attribute__((always_inline))
    specLD ( volatile T* address, const ID_WORD index, const ID_WORD th_id_wd ) 
    throw(Dependence_Violation)  {
      const WORD_PER_4 th_id =  convert(th_id_wd);
      volatile WORD_PER_4* pos =      LdStVct[index];
      volatile FULL_WORD*  full_pos = (volatile FULL_WORD*) pos;

      markRead(pos, th_id, th_id_wd);

#if defined( MODEL_FENCE )		
      __asm__("mfence;");
      T* addr = (T*) address;
      T val = *addr;
#else
      __asm__("nop;");
      FULL_WORD www = *full_pos;
      T* addr = (T*) address;
      T val = *addr;
      //cout<<"good read!!!"<<endl;
#endif

      WORD_PER_4 prev_store  = read_4(full_pos, 2); 		

      if(prev_store > th_id) { 
        throw Dependence_Violation(th_id_wd-1, th_id_wd, Dependence_Violation::LOAD_TP);
      }

      return val;
    }

    template<class T, class TH>
    inline void __attribute__((always_inline)) specST(
      volatile T* address, const T& val,  const ID_WORD index, const ID_WORD th_id_wd, TH* th
    ) throw(Dependence_Violation) {
      const WORD_PER_4     th_id    = convert(th_id_wd);
      volatile WORD_PER_4* pos      = LdStVct[index];
      volatile FULL_WORD*  full_pos = (volatile FULL_WORD*) pos;

      *(pos+3) = th_id;                             //set syncW
      WORD_PER_4 prev_store = read_4(full_pos, 2); 
      if(th_id>=prev_store) {   
        *(pos+2) = th_id;                         //  update last store
      } else {
        throw Dependence_Violation(th_id_wd-1, th_id_wd, Dependence_Violation::STORE_TP);			
      }

      WORD_PER_4 syncW = read_4(full_pos, 3);
      if(syncW != th_id) {                          // check mutual exclusion
        throw Dependence_Violation(th_id_wd-1, th_id_wd, Dependence_Violation::STORE_TP);			
      }
	
      T* addr = (T*) address;
      { // store the former ``original content'' in the thread's buffer
        T old_val = *addr;
        FULL_WORD stamp_val = ++(* (full_pos + 1) );
        th->save( addr, old_val, stamp_val );
      }

      *addr = val;

#if defined( MODEL_FENCE )		
      __asm__("mfence;");
#else
      /**
       * with the below instruction specST: *addr = ..; *(pos+3) = ..;
       *                            specLD: .. = *pos+3; .. = *addr;
       * hence the value of `addr' is sequentially consistent.
       */
      *(pos+3) = th_id;		
      __asm__("nop;");
#endif

      const WORD_PER_4 current_st  = read_4(full_pos, 2); //*posLS; 
      const WORD_PER_4 highest_ld1 = read_4(full_pos, 0);
      const WORD_PER_4 highest_ld2 = read_4(full_pos, 1);

      //if( current_st==th_id && greater_eq(th_id, highest_ld1) && greater_eq(th_id, highest_ld2) /*highest_ld<=th_id*/ ) {  
      if( current_st==th_id && th_id >= highest_ld1 && th_id >=highest_ld2 ) {
        return;   
      } else {
        if(current_st!=th_id) {
          //cout<<"Store: Synch_Viol curr: "<<th_id<<" other: "<<current_st<<endl;
        } else { //if(highest_ld>th_id) {
          //cout<<"Store: True_Dep load: "<<highest_ld1<<" store: "<<th_id<<endl; 
        }
        throw Dependence_Violation(th_id_wd-1, th_id_wd, Dependence_Violation::STORE_TP); 
      }
    } 
#endif

};   // END class SpMod_IPcore



/**
 * The necessary invariant have to be ensured by the thread manager
 */
template<
          class HashFct, SpMod_IPcore<HashFct>* LSmem, class T
> 
class SpMod_IP0 : public VoidSpecModel {
  private:
    const  ID_WORD      wr_per_it;
    ID_WORD             SZ;
    BuffIP_Resource<T>* aliased_buff[32];  // assumes  CW <= 32, where CW is the window of concurrent, consecutive (extended) iterations.

//  DynVector  < SavedTupleIP<T>* > address_vect;
    std::vector< SavedTupleIP<T>* > address_vect;

  public:	
    typedef T ElemType;

    inline ID_WORD getWritesPerIt() const {  
      return wr_per_it;  
    } 

    inline SpMod_IP0(int wrs) : wr_per_it(wrs), SZ(0) { }

    inline void __attribute__((always_inline))
    setNumProc(const int num_proc) { 
        LSmem->setNumProc(num_proc);
    }


    template<class THbuf>
    inline void __attribute__((always_inline)) 
    acquireResources(THbuf* thread, const ID_WORD id) { 
      thread->resetCursor(id);
    }

    /** is called from the bufferable thread constructor **/
    inline void acquireBuffer(BuffIP_Resource<T>* res) {
      aliased_buff[SZ++] = res;
    }


    /**
     * this will be executed sequentially (mutual exclusion) to recover from deadlock!
     */
    inline void recoverFromRollback1(
     const ID_WORD safe_thread,
     const ID_WORD highest_id_thread
    ) {}

    void recoverFromRollback( const ID_WORD safe_thread, const ID_WORD highest_id_thread );

    //****************************************************************************************
    //******************   FORWARDING METHODS                          ***********************
    //****************************************************************************************
	
    template<class TH>
    inline T  __attribute__((always_inline)) 
    specLD(volatile T* address, const ID_WORD index, const ID_WORD th_nr, TH* th) 
            throw(Dependence_Violation) {
      return LSmem->specLD(address, index, th_nr);
    }

    template<class TH>
    inline void __attribute__((always_inline)) specST(
        volatile T* address, const T& val,  const ID_WORD index, const ID_WORD th_nr, TH* th
    ) throw(Dependence_Violation) {
      return LSmem->specST(address, val, index, th_nr, th);
    }

    //****************************************************************************************
    //****************************************************************************************

    inline void reset() {
      LSmem->reset();
    }
		
    inline void reset(const unsigned long id, const unsigned long highestThNr) {
      this->reset();
    }
};


template< class HashFct, SpMod_IPcore<HashFct>* LSmem, class T > 
class SpMod_IP : public SpMod_IP0<HashFct, LSmem, T> {
  public:
    inline SpMod_IP(int wrs) : SpMod_IP0<HashFct, LSmem, T>(wrs) { }

    inline ID_WORD checkBounds(T* addr, const ID_WORD th_num=1) {
      ID_WORD diff = (ID_WORD)(addr-(T*)this->addr_beg);

      // addr>=addr_beg && addr<addr_end  iff dif<A_m_a
      if(diff<=this->A_m_a) return LSmem->getIndex( diff );
      else THROW_BOUNDS_EXC(th_num-1, th_num); 
    }

    ID_WORD checkBoundsSlow(T* addr, const ID_WORD th_num = 1); 
    //{ ID_WORD diff = (ID_WORD)(addr-(T*)this->addr_beg);
    //  if(diff<=this->A_m_a) return LSmem->getIndex( diff );
    //  else return ID_WORD_MAX; }
    
    void printIndexForAddr(T* addr);
};

template< class HashFct, SpMod_IPcore<HashFct>* LSmem, class T > 
class SpMod_IPbw : public SpMod_IP0<HashFct, LSmem, T> {
  public:
    inline SpMod_IPbw(int wrs) : SpMod_IP0<HashFct, LSmem, T>(wrs) { }

    inline ID_WORD checkBounds(T* addr, const ID_WORD th_num=1) {
      ID_WORD diff = (ID_WORD)((T*)this->addr_end - addr);

      if(diff<=this->A_m_a) return LSmem->getIndex( diff );
      else THROW_BOUNDS_EXC(th_num-1, th_num);
    }

    ID_WORD checkBoundsSlow(T* addr, const ID_WORD th_num = 1);  
    //{  ID_WORD diff = (ID_WORD)((T*)this->addr_end - addr);
    //   if(diff<=this->A_m_a) return LSmem->getIndex( diff );
    //   else return ID_WORD_MAX;  }
};



  

#endif // define MEM_MODELS


/* 
	//In SpecST one can inline the old value saving!
	//val = *address; time = ++(*(posSS+1));
	BuffIP_Resource<T>* const res = &th->resource;
	res->pos->val 	= *address;
	res->pos->time	= ++(*(posLS+1));    //++global_counter; 
	res->pos->addr	= (T*)address;
	res->pos++;
*/

