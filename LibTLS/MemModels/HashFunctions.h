#ifndef HASH_FCT
#define HASH_FCT

using namespace std;

/*****************************************************************************************/
/*** Hashing function mapping chunks of addresses to an index in the Load/Store Vector ***/
/*****************************************************************************************/

class HashGen {
  private:
	const unsigned int q;
	const unsigned int Q;
  public:
	inline HashGen(unsigned int qq, unsigned int QQ) : q(qq), Q(QQ) {};
	inline ID_WORD __attribute__((always_inline)) 
	operator() (const  ID_WORD addr) const { 
		return (addr / q) % Q; 
	}
	inline ID_WORD cardinal() const { return Q; }

};


class HashSeqPow2 {
  private:
 	const uint	  shift_term;

	/** check spec_size is a power of two **/
	const ID_WORD spec_size;   
  public:

	inline HashSeqPow2(const ID_WORD pow2, const uint shift) 
		: spec_size( (1<<pow2) - 1 ), shift_term(shift)	
          { /*cout<<"spec_size: "<<spec_size<<endl;*/}
	inline HashSeqPow2(const void* addr, const ID_WORD pow2, const uint shift) 
		: spec_size( (1<<pow2) - 1 ), shift_term(shift) { }
	inline HashSeqPow2(const HashSeqPow2& obj) 
		: spec_size(obj.spec_size), shift_term(obj.shift_term)  
          { cout<<"Hash attr: "<<spec_size<<" "<<shift_term<<endl; }

	inline ID_WORD __attribute__((always_inline)) 
	operator() (const  ID_WORD addr) const { 
		return (addr>>shift_term) & spec_size; 
	}
	inline ID_WORD cardinal() const { return spec_size+1; }
};

class HashSeqPow2Spaced {
  private:
	static const uint def_space = 4;
 	const uint	  shift_term;
	const uint	  shift_space;
	/** check spec_size is a power of two **/
	const ID_WORD spec_size;   
  public:

	inline HashSeqPow2Spaced(const ID_WORD pow2, const uint shift, const uint space=def_space) 
		: spec_size( (1<<pow2) - 1 ), shift_term(shift), shift_space(space)     { }
	inline HashSeqPow2Spaced(const void* addr, const ID_WORD pow2, const uint shift, const uint space=def_space) 
		: spec_size( (1<<pow2) - 1 ), shift_term(shift), shift_space(space)     { }
	inline HashSeqPow2Spaced(const HashSeqPow2Spaced& obj) 
		: spec_size(obj.spec_size),
		  shift_term(obj.shift_term) ,
		  shift_space(obj.shift_space)                                          { }

	inline ID_WORD __attribute__((always_inline)) 
	operator() (const  ID_WORD addr) const { 
		return ((addr>>shift_term) & spec_size)<<shift_space; 
	}
	inline ID_WORD cardinal() const { return (spec_size+1)<<shift_space; }
};


class FFThash {
  private:
 	const uint	  pow2dual_1;

  public:

	inline FFThash(const ID_WORD pow2) 
		: pow2dual_1( (1<<pow2) - 1 )		{ }

	inline ID_WORD __attribute__((always_inline)) 
	operator() (const  ID_WORD addr) const { 
		ID_WORD ind = (addr & pow2dual_1);
		//cout<<"Ind: "<<ind<<endl;
		return ind;
	}
	inline ID_WORD cardinal() const { return pow2dual_1+1; }
};

class HashOneArr {
  private:
	const ulong   base_addr;
 	const uint    shift_term;
	const ID_WORD spec_size;
  public:

	inline HashOneArr(const ID_WORD sz, const uint shift) 
		: base_addr(0), spec_size(sz), shift_term(shift)	{ }
	inline HashOneArr(const void* addr, const ID_WORD sz, const uint shift) 
		: base_addr((ulong)addr), spec_size(sz), shift_term(shift) { }
	inline HashOneArr(const HashOneArr& obj) 
		: base_addr(obj.base_addr), 
		  spec_size(obj.spec_size), 
		  shift_term(obj.shift_term) 						{ }

	inline ID_WORD __attribute__((always_inline)) 
	operator() (const  void* addr) const { 
		ID_WORD res = (ulong)addr-base_addr;
		return (res>>shift_term) % spec_size; 
	}
	inline ID_WORD __attribute__((always_inline))
	operator() (const  ID_WORD addr) const { 
		return (addr>>shift_term) % spec_size; 
	}
	inline ID_WORD cardinal() const { /*cout<<"CARDINAL: "<<spec_size<<endl;*/ return spec_size; }
};


template<const uint shift_term>
class GenHashOneArr {
  private:
	const ulong   base_addr;
	const ID_WORD spec_size;
  public:

	inline GenHashOneArr(const ID_WORD sz) 
		: base_addr(0), spec_size(sz)				{ }
	inline GenHashOneArr(const void* addr, const ID_WORD sz) 
		: base_addr((ulong)addr), spec_size(sz) 	{ }
	inline GenHashOneArr(const GenHashOneArr<shift_term>& obj) 
		: base_addr(obj.base_addr), 
		  spec_size(obj.spec_size)	 				{ }

	inline ID_WORD __attribute__((always_inline)) 
	operator() (const  void* addr) const { 
		ID_WORD res = (ulong)addr-base_addr;
		return (res>>shift_term) % spec_size; 
	}
	inline ID_WORD cardinal() const { return spec_size; }
};


/***********************************************************/
/********************** BaseSpecModel **********************/
/***********************************************************/

class VoidSpecModel {
  public:
	const void* 	addr_beg;
	const void* 	addr_end;
	FULL_WORD      	A_m_a;
  public:

	void inline setIntervalAddr(void* addr1, void* addr2) {
		addr_beg = addr1;	addr_end = addr2;	A_m_a = (FULL_WORD)addr_end - (FULL_WORD)addr_beg;
	}

	inline VoidSpecModel() : addr_beg(NULL), addr_end((const void*)ID_WORD_MAX) { A_m_a = FULL_WORD_MAX; }

	inline static void __attribute__((always_inline))
	recoverFromRollback	(
			const ID_WORD safe_thread, 
			const ID_WORD highest_id_thread
	){ return; }

	template<class THbuff>
	inline static void __attribute__((always_inline))      
		commit_at_iteration_end	(THbuff* th, const ID_WORD th_id = 0) 
		throw(Dependence_Violation)	{ return;  }

	template<class TH>
	inline static void __attribute__((always_inline))      
	releaseResources		(TH* thread)
	{ return; }

	inline static void __attribute__((always_inline))
	reset( 
		const ID_WORD safe_thread, 
		const ID_WORD highest_id_thread)
	{ return; }

	inline void printThreadsEntries() {}


	template<class THbuf>
	inline static void __attribute__((always_inline)) 
	acquireResources(THbuf* thread, const ID_WORD id) { }
};

/****************************************************************/
/*********** Optimizing Cache Behaviour *************************/ 
/****************************************************************/


//const unsigned int CACHE_LINE 		= 64; // in bytes
//const unsigned int NrElemPerCacheLine 	= CACHE_LINE/sizeof(ID_WORD);

#define CACHE_LINE            64
#define NrElemPerCacheLine    CACHE_LINE/sizeof(ID_WORD)

inline int getCacheLine()         { return CACHE_LINE; }
inline int getNrElemOnCacheLine() { return CACHE_LINE/sizeof(ID_WORD); }

/*
void printTypesSize() {
  cout<<"ID_WORD: "<<sizeof(ID_WORD)<<endl;
  cout<<"u long:  "<<sizeof(unsigned long)<<endl;
  cout<<"long int:"<<sizeof(unsigned long int)<<endl;
  cout<<"int:     "<<sizeof(int)<<endl;
}
*/


#endif   // define HASH_FCT

