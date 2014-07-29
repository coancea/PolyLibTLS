#ifndef BUFF_TH
#define BUFF_TH


using namespace std;

#include "Threads/Spec_Thread0.h"

/****************** Thread Attributes *******************/

/** SC model: in order to deal with RAW iteration-independent dependencies:
 *    - Uses a vector to mark the locations written by the current iteration.
 *      If a location is determined not to have been written (fast path), 
 *        just return the original memory value
 *      Else search the thread buffer to find the correct value.
 *  Thread component extends `ItInd_SV'.
 */
const ID_WORD ItIndAttr_WAR_SV   = 1;

/** SC model: in order to deal with RAW iteration-independent dependencies:
 *    - Keep the highest written address (`hwa').
 *      If a to-be-read address is > `hwa' then it has not been written by
 *        the current iteration (fast path), just return the non-spec memory value
 *      Else search the thread buffer to find the correct value.
 *  Thread component extends `ItInd_Haddr<1>'.
 */
const ID_WORD ItIndAttr_HAfw     = 2; // `ItInd_Haddr<1>'

/** SC model: in order to deal with RAW iteration-independent dependencies:
 *    - Keep the lowest written address (`lwa').
 *      If a to-be-read address is < `lwa' then it has not been written by
 *         the current iteration (fast path), just return the non-spec memory value
 *      Else search the thread buffer to find the correct value.
 *  Thread component extends ItInd_Haddr<0>.
 */
const ID_WORD ItIndAttr_HAbw     = 3; // ItInd_Haddr<0>

const ID_WORD RO_ATTR            = 0;
const ID_WORD IP_ATTR            = 4;

/********************************************************/


class UnimplRollbackCleanUp { 
	public:
		inline __attribute__((always_inline)) UnimplRollbackCleanUp() {}
		inline __attribute__((always_inline)) void rollbackCleanUp()  {}
};

class BaseThComp {
  protected:
	Spec_Thread0	*	parentTh;
  public:
	inline void setParent(Spec_Thread0* th) {
		parentTh = th;
	}
	inline Spec_Thread0* getParent() {
		return parentTh;
	}
/*
	inline void setRollbackC(const ID_WORD roll) {
		parentTh->setRollback(roll);
	}
*/
};

/**
 *	Buffer support for threads corresponding to the 
 * 	"in place" speculative model
 */

template<class T, class M, M* m>
class BuffIPM : public BaseThComp {
  public:
	BuffIP_Resource<T> 	resource;	

  public:
	
	inline BuffIPM()  {
		ID_WORD len = m->getWritesPerIt();
		resource.init(len);
		m->acquireBuffer(&resource);
	}

	inline void __attribute__((always_inline)) 
	resetCursor(const ID_WORD new_id) {
		resource.id  = new_id;	
		resource.pos = &resource.buffer[0];
	}

	inline void __attribute__((always_inline)) 
	save(T* addr, const T& val, const ID_WORD time_stamp) {
		resource.pos->val 	= val;
		resource.pos->time	= time_stamp; 
		resource.pos->addr	= addr;
		resource.pos++;
	}
	
	inline SavedTupleIP<T>* __attribute__((always_inline)) 
	getCursor() {   return resource.pos;   }

	inline BuffIP_Resource<T>* getBufferResource(int i) {
		return &resource;
	}

	/**
	 * Speculative Load partial implementation
	 */ 

	inline T __attribute__((always_inline))
	specLD(volatile T* addr, const ID_WORD thread_id, const ID_WORD index)  {
		return m->specLD(addr, index, thread_id, this);
	}


	/**
	 * Speculative Store partial implementation
	 */ 
	inline void __attribute__((always_inline))
	specST(volatile T* addr, const T& val, const ID_WORD thread_id, const ID_WORD index)  {  
		m->specST(addr, val, index, thread_id, this);
	}

	inline void __attribute__((always_inline))
	resetIndDepComp() { }

	inline __attribute__((always_inline)) 
	void rollbackCleanUp()  { resource.pos = &resource.buffer[0]; }

};

/**********************************************************************************/
/**********************************************************************************/
/*********************** Serial Commit Model **************************************/
/**********************************************************************************/
/**********************************************************************************/

/**
 * Thread-support for resolving the iteration independent RAW dependences
 * for the "serial commit" speculative model! 
 */

class ItInd_HaddrBase {
  protected:
	void* addr;
	ItInd_HaddrBase() { addr = NULL; }
	//ID_WORD buff[10]; //padding
	
  public:
	void initWARvct(int sz) { }
}; 

/** i != 0 means monotonically increasing range of addresses **/
template<const int i>
class ItInd_Haddr : public ItInd_HaddrBase {
  public:
	inline ItInd_Haddr() {  resetIndDepComp(); }

	inline void __attribute__((always_inline))
	resetIndDepComp() {   addr = NULL;         }

	inline void __attribute__((always_inline))
	rollbackCleanUp() {   resetIndDepComp();   }


	inline void __attribute__((always_inline))
	markInternalWrite(const ID_WORD thread, void* address) {
		if(address>this->addr) {  this->addr = address; }
	}

	inline bool __attribute__((always_inline))
	getHasNotWrote(const ID_WORD thread, void* address) {
		return (address>this->addr);
	}	
};

/** i == 0 means monotonically decreasing range of addresses **/
template<>
class ItInd_Haddr<0> : public ItInd_HaddrBase {
  public:
	inline ItInd_Haddr() { resetIndDepComp(); }

	inline void __attribute__((always_inline))
	resetIndDepComp() {   addr = (void*)(FULL_WORD)(0 - 1);  }

	inline void __attribute__((always_inline))
	rollbackCleanUp() {   resetIndDepComp();   }

	inline void __attribute__((always_inline))
	markInternalWrite(const ID_WORD thread, void* address) {
		if(address<this->addr) this->addr = address;
	}

	inline bool __attribute__((always_inline))
	getHasNotWrote(const ID_WORD thread, const void* address) {
		return (address<this->addr);
	}	
};


//template<class HashFct>
class ItInd_SV  {
  private:
	ID_WORD*			buffer2;
	ID_WORD				len;
	//HashFct			hash;
  public:

	void initWARvct(int sz);

	//ItInd_SV() :len(1024) { init(len); };
	inline ItInd_SV() { /*init(WAR_SIZE);*/ }
	inline ItInd_SV(const int sz)  {
		initWARvct(sz);
	} 

	~ItInd_SV() { delete[] buffer2; } 


	inline void __attribute__((always_inline))
	markInternalWrite(const ID_WORD thread, void* addr) {
		ID_WORD ind = (ID_WORD) ( ((FULL_WORD)addr) & len );//hash((ID_WORD)addr);
		buffer2[ind] = thread;
	}

	inline bool __attribute__((always_inline))
	getHasNotWrote(const ID_WORD thread, void* addr) {
		//return true;

		ID_WORD ind = (ID_WORD) ( ((FULL_WORD)addr) & len );
		return (thread!=buffer2[ind]);
		//return (thread==buffer2[hash((ID_WORD)addr)]);

	}

	inline void __attribute__((always_inline)) reset() { 
		for(unsigned int i=0; i<=len; i++) 
			buffer2[i] = ID_WORD_MAX; 
	}

	inline void __attribute__((always_inline))
	rollbackCleanUp() { reset(); }

	inline void __attribute__((always_inline))
	resetIndDepComp() { /*reset();*/ }
}; 

/**
 *	Buffer support for threads corresponding to the 
 * 	"serial commit" speculative model
 */

class Not_IndCas   { }; 
class Not_Cachable { };

template<
	class T, class M, M* m, 
	class ATTR_INDDEP, // = Not_IndCas, 
	class ATTR_CACHABLE = Not_Cachable
> 
class BuffSCM :  public BaseThComp, public ATTR_INDDEP, public ATTR_CACHABLE  {
  private:
	SavedTupleSC<T>*	buffer;	
	SavedTupleSC<T>*	pos;    
 	SavedTupleSC<T>*	end;	   

  protected:
	inline BuffSCM() { 
		/*this->ATTR_INDDEP::init(m->cardinal());*/ 
		this->init(m->getWritesPerIt()); 
	}
	
  public:

	inline BuffSCM(const ID_WORD sz) { 
		/*this->ATTR_INDDEP::init(m->cardinal());*/ 
		this->init(sz); 
	}

  	void resize     ();
	void init       ( const ID_WORD sz );

	inline ~BuffSCM() { delete[] buffer; }

	inline void reset() { pos = buffer; this->ATTR_INDDEP::reset(); }

	inline void __attribute__((always_inline))  
	store(volatile T* address, const T& val, const ID_WORD ind) {
		if(pos >= end) resize();
		pos->addr  = (T*)address; //(unsigned long)address;
		pos->val   = val;
		pos->index = ind;
		pos++;
	}

	inline SavedTupleSC<T>* __attribute__((always_inline))
	getBeginCursor() {
		return buffer;
	}

	inline SavedTupleSC<T>* __attribute__((always_inline))
	getCursor() const {
		return pos;
	}

	inline void __attribute__((always_inline))
	resetCursor()  {
		pos = buffer;
	}

	T getLoopIndWrite(volatile T* address); /*{
		SavedTupleSC<T>* beg = pos-1;
		while(buffer<=beg) {
			if( ((unsigned long)address) == beg->addr ) return beg->val;
			beg--;
		}
		return *((T*)address);
	}*/

	inline void rollbackCleanUp() {
		resetCursor();
		this->ATTR_INDDEP::rollbackCleanUp();
	}

	/**
	 * Speculative Load partial implementation
	 */ 

	//template<const int var>
	inline T __attribute__((always_inline))
	specLD(volatile T* addr, const ID_WORD thread_id, const ID_WORD index)  {
		m->markLD(index, thread_id);		
		__asm__("mfence;");
		if( this->ATTR_INDDEP::getHasNotWrote(thread_id, (T*)addr) ) { 	
			//cout<<"fast write"<<endl;	
			return *((T*)addr);
		} else return this->getLoopIndWrite(addr);
	}


	/**
	 * Speculative Store partial implementation
	 */ 
	//template<const int var>
	inline void __attribute__((always_inline))
	specST(volatile T* addr, const T& val, const ID_WORD thread_id, const ID_WORD index)  {  
		//ID_WORD index = m->template checkBoundsWind<var> (addr);
		this->store(addr, val, index);
		this->markInternalWrite(thread_id, (T*)addr/*index*/);
	}
}; 


/***************************************************************/
/********************* Read Only Buffarable ********************/ 
/***************************************************************/



template<class T, class M, M* m>
class Buff_RO :  public BaseThComp {
  public:
	//ID_WORD padd[64];
	inline void __attribute__((always_inline))
	resetIndDepComp() { }

	inline void __attribute__((always_inline))
	rollbackCleanUp() { }

	//template<const int var>
	inline T __attribute__((always_inline))
	specLD(volatile T* addr, const ID_WORD thread_id=0, const ID_WORD ind = 0)  {				
		return *addr;
	}

	//template<const int var>
	inline void __attribute__((always_inline))
	specST(volatile T* addr, const T& val, const ID_WORD thread_id, const ID_WORD ind=0)  { 
		//cout<<"Throwing Dependence Violation"<<endl;
		throw Dependence_Violation(thread_id-1, thread_id);
		//this->setRollbackC(thread_id-1);
	}	
};


#endif  // define BUFF_TH

