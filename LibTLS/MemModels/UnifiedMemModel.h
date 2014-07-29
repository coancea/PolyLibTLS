#ifndef UN_MEM_MODEL
#define UN_MEM_MODEL


#include "MemModels/MemModels.h"
#include "MemModels/MemModels.cpp"

/********************************************/
/**************** enable_if *****************/
/********************************************/
/*
template <bool B, class T = void>
struct enable_if_c {
  typedef T type;
};

template <class T>
struct enable_if_c<false, T> {};
*/

template <bool B, class T = void>
struct enable_if_c {};

template <class T>
struct enable_if_c<true, T> {
  typedef T type;
};

template <class Cond, class T = void>
struct enable_if : public enable_if_c<Cond::value, T> {};


template <bool B, class T = void>
struct disable_if_c {
  	typedef T type;
};

template <class T>
struct disable_if_c<true, T> {};

template <class Cond, class T = void>
struct disable_if : public disable_if_c<Cond::value, T> {};


template <int> struct DummyOverload { inline DummyOverload(int) {} };

/********************************************/
/**************** is_same_obj ***************/
/********************************************/

template<class M>
struct is_SpRO_model { 
	static const bool value = false; 
};
template<class T>
struct is_SpRO_model< SpMod_ReadOnly<T> > {
	static const bool value = true;
};


/********************************************/
/**************** is_same_obj ***************/
/********************************************/

template<class T1, class T2>
struct is_same_type {
	static const bool value = false;
};

template<class T>
struct is_same_type<T,T> {
	static const bool value = true;
};

template<class M1, class M2, M1* m1, M2* m2>
struct is_same_obj{
	static const bool value = false;
};

template<class M, M* m>
struct is_same_obj<M,M,m,m> {
	static const bool value = true;
};

/*
template<class M1, class M2, M1* m1, M2* m2>
class is_diff_obj{
  public:
	static const bool value = true;
};

template<class M, M* m>
class is_diff_obj<M,M,m,m> { };
*/

/********************************************/
/********** valid_SpMod_Buff_pair ***********/
/********************************************/

#include "Threads/Bufferable_Threads.h"

template<class M, M* m, const ID_WORD TH_ATTR>
class valid_SpMod_Buff_pair{ };
/*
template<class HashFct, SpMod_SCcore<HashFct>* m_core, class T, SpMod_SC<HashFct, m_core, T>* m>
class valid_SpMod_Buff_pair< SpMod_SC<HashFct, m_core, T>, m, ItIndAttr_SV > {
  public:
	typedef BuffSCM<T, SpMod_SC<HashFct, m_core, T>, m, ItInd_SV>	type;
	static const bool value = true;
};
*/

template<class HashFct, SpMod_SCcore<HashFct>* m_core, class T, SpMod_SC<HashFct, m_core, T>* m>
class valid_SpMod_Buff_pair< SpMod_SC<HashFct, m_core, T>, m, ItIndAttr_WAR_SV > {
  public:
	typedef BuffSCM<T, SpMod_SC<HashFct, m_core, T>, m, ItInd_SV>	type;
	static const bool value = true;
};

template<class HashFct, SpMod_SCcore<HashFct>* m_core, class T, SpMod_SCbw<HashFct, m_core, T>* m>
class valid_SpMod_Buff_pair< SpMod_SCbw<HashFct, m_core, T>, m, ItIndAttr_WAR_SV > {
  public:
	typedef BuffSCM<T, SpMod_SCbw<HashFct, m_core, T>, m, ItInd_SV>	type;
	static const bool value = true;
};

template<class HashFct, SpMod_SCcore<HashFct>* m_core, class T, SpMod_SC<HashFct, m_core, T>* m>
class valid_SpMod_Buff_pair< SpMod_SC<HashFct, m_core, T>, m, ItIndAttr_HAbw > {
  public:
	typedef BuffSCM<T, SpMod_SC<HashFct, m_core, T>, m, ItInd_Haddr<0> >	type;
	static const bool value = true;
};

template<class HashFct, SpMod_SCcore<HashFct>* m_core, class T, SpMod_SCbw<HashFct, m_core, T>* m>
class valid_SpMod_Buff_pair< SpMod_SCbw<HashFct, m_core, T>, m, ItIndAttr_HAbw > {
  public:
	typedef BuffSCM<T, SpMod_SCbw<HashFct, m_core, T>, m, ItInd_Haddr<0> >	type;
	static const bool value = true;
};

template<class HashFct, SpMod_SCcore<HashFct>* m_core, class T, SpMod_SC<HashFct, m_core, T>* m>
class valid_SpMod_Buff_pair< SpMod_SC<HashFct, m_core, T>, m, ItIndAttr_HAfw > {
  public:
	typedef BuffSCM<T, SpMod_SC<HashFct, m_core, T>, m, ItInd_Haddr<1> >	type;
	static const bool value = true;
};

template<class HashFct, SpMod_SCcore<HashFct>* m_core, class T, SpMod_SCbw<HashFct, m_core, T>* m>
class valid_SpMod_Buff_pair< SpMod_SCbw<HashFct, m_core, T>, m, ItIndAttr_HAfw > {
  public:
	typedef BuffSCM<T, SpMod_SCbw<HashFct, m_core, T>, m, ItInd_Haddr<1> >	type;
	static const bool value = true;
};

template<class HashFct, SpMod_IPcore<HashFct>* m_core, class T, SpMod_IP<HashFct, m_core, T>* m>
class valid_SpMod_Buff_pair< SpMod_IP<HashFct, m_core, T>, m, IP_ATTR > {
  public:
	typedef BuffIPM<T, SpMod_IP<HashFct, m_core, T>, m >	type;
	static const bool value = true;
};

template<class HashFct, SpMod_IPcore<HashFct>* m_core, class T, SpMod_IPbw<HashFct, m_core, T>* m>
class valid_SpMod_Buff_pair< SpMod_IPbw<HashFct, m_core, T>, m, IP_ATTR > {
  public:
	typedef BuffIPM<T, SpMod_IPbw<HashFct, m_core, T>, m >	type;
	static const bool value = true;
};

template<class T, SpMod_ReadOnly<T>* m>
class valid_SpMod_Buff_pair< SpMod_ReadOnly<T>, m, RO_ATTR > {
  public:
	typedef Buff_RO<T, SpMod_ReadOnly<T>, m>		type;
	static const bool value = true;
};

/***********************************************************/
/*********************** USpModel **************************/
/***********************************************************/


template<class M, M* m, class TH_BUFF, class Continuation> 
class USpModelRecImpl {
  public:
	inline static void __attribute__((always_inline))
	recoverFromRollback	(
			const ID_WORD safe_thread, 
			const ID_WORD highest_id_thread
	){  
		m->				recoverFromRollback(safe_thread, highest_id_thread);
		Continuation::	recoverFromRollback(safe_thread, highest_id_thread);
	}

	template<class THbuff>
	inline static void __attribute__((always_inline))      
	commit_at_iteration_end	(THbuff* th) 
	throw(Dependence_Violation)	{ 
		bool is_roll = false;
		Dependence_Violation v(ID_WORD_MAX);

		try { 
			m->				commit_at_iteration_end(static_cast<TH_BUFF*>(th), th->getID());
		} catch(Dependence_Violation e) {
			if(is_roll && v.roll_id>e.roll_id) { v = e; }    
			else if(!is_roll)                  { is_roll = true; v = e; }
		}

		try {
			Continuation::	commit_at_iteration_end(th);
		} catch(Dependence_Violation e) {
			if(is_roll && v.roll_id>e.roll_id) { v = e; }    
			else if(!is_roll)                  { is_roll = true; v = e; }
		}

		if(is_roll) throw v;
	}

	template<class TH>
	inline static void __attribute__((always_inline))      
	releaseResources		(TH* thread)
	{ 
		m->				releaseResources(thread);
		Continuation::	releaseResources(thread);
	}

	inline static void __attribute__((always_inline))
	reset( 
		const ID_WORD safe_thread, 
		const ID_WORD highest_id_thread)
	{ 
		m->				reset(safe_thread, highest_id_thread);
		Continuation::	reset(safe_thread, highest_id_thread);
	}

	inline static void __attribute__((always_inline))
	reset() { reset(0,0); } 

	inline static void __attribute__((always_inline))
	setNumProc(const int num_proc) { 
		m->setNumProc(num_proc);
		Continuation::setNumProc(num_proc);
	}

	inline void printThreadsEntries() {}

	template<class THbuf>
	inline static void __attribute__((always_inline)) 
	acquireResources(THbuf* thread, const ID_WORD id) { 
		m->				acquireResources(static_cast<TH_BUFF*>(thread), id);
		Continuation::	acquireResources(thread, id);
	}
};

/**
 * USpModel represents the unified speculative model.
 * The user has to construct this and the rest should be done
 * more or less automatically.
 */


template<
		class M, M* m, const int TH_ATTR
>
class USpModelNever : public VoidSpecModel {
  public:

	inline static void __attribute__((always_inline))
	setNumProc(const int num_proc) { return; }

	template<class TH>
  	inline static __attribute__((always_inline)) 
	void cleanUp(TH* th)  { return; }


	template<class TH>
	inline static __attribute__((always_inline)) 
	void rollbackCleanUp(TH* th) { return; }

	template<class TH>
	inline static void __attribute__((always_inline))
	resetThComp(TH* th) 	{ return; }

	template<class TH>
	inline static void __attribute__((always_inline))
	setThCompParent(TH* th)	{ return; };

	template<class T1, class TH>
	inline static T1 specLDslow(volatile T1* addr, TH* th) {
		//cout<<"WRONG PATH slow"<<endl;
		return * ((T1*)addr); //static_cast<T1*>(addr);
	}
	template<class T1, class TH>
	inline static void specSTslow(volatile T1* addr, const T1& val, TH* th) {  
		//cout<<"WRONG PATH slow"<<endl;
		throw Dependence_Violation(th->getID()-1, th->getID());	
	}
	
	template<class M1, M1* m1, const int var, class T1, class TH>
	inline static T1 specLD(volatile T1* addr, TH* th) {
		//cout<<"WRONG PATH moderate"<<endl; 
		return *addr;
	}

	template<class M1, M1* m1, const int var, class T1, class TH>
	inline static T1 specST(volatile T1* addr, const T1& val, TH* th) {
		//cout<<"WRONG PATH moderate"<<endl; 
		throw Dependence_Violation(th->getID()-1, th->getID());	
	}

	template<class M1, M1* m1, const int var, class T1, class TH>
	inline static T1 specLDmodRec(volatile T1* addr, TH* th) { 
		//cout<<"WRONG PATH modRec LD"<<endl; 
		return *addr;
	}

	template<class M1, M1* m1, const int var, class T1, class TH>
	inline static T1 specSTmodRec(volatile T1* addr, const T1& val, TH* th) { 
		//cout<<"WRONG PATH modRec ST"<<endl; 
		throw Dependence_Violation(th->getID()-1, th->getID());
	}

};

/**
 *   The GADT approach
 */
template<class T, class MemTp, MemTp* m, class BuffThC, class Continuation>
class BaseUSpModel : public USpModelRecImpl<MemTp, m, BuffThC, Continuation> {
  private:
	typedef BaseUSpModel<T,MemTp,m,BuffThC,Continuation>	BSELF;

  public:

	template<class TH>
	inline static __attribute__((always_inline)) 
	void rollbackCleanUp(TH* th)  {
		th->BuffThC ::rollbackCleanUp();
		Continuation::rollbackCleanUp(th);
	}

	template<class TH>
	inline static void __attribute__((always_inline))
	resetThComp(TH* th) { 
		th->BuffThC::resetIndDepComp();
		Continuation::resetThComp(th);
	}

	template<class TH>
	inline static void /*__attribute__((always_inline))*/
	setThCompParent(TH* th)  {
		th->BuffThC::setParent(th);
		Continuation::setThCompParent(th);
	};


	/************************************************/
	/******************* specLD/STslow **************/
	/************************************************/

	template<class T1, class TH>
	inline static T1 specLDslow (
          volatile T1* addr, TH* th, typename enable_if<is_same_type<T,T1> >::type* = NULL 
        ) {
		ID_WORD ind = m->checkBoundsSlow((T1*)addr);
		if(ind!=ID_WORD_MAX) {
			return m->specLD(addr, ind, th->getID(), static_cast<BuffThC*>(th));
		} else
			return Continuation::specLDslow(addr, th);
	}

	template<class T1, class TH>
	inline static T1 specLDslow (
          volatile T1* addr, TH* th, typename disable_if<is_same_type<T,T1> >::type* = NULL
        ) {
		return Continuation::specLDslow(addr, th);
	}

	template<class T1, class TH>
	inline static void specSTslow (
          volatile T1* addr, const T1& val, TH* th, typename enable_if<is_same_type<T,T1> >::type* = NULL 
        ) {  
		ID_WORD ind = m->checkBoundsSlow((T1*)addr);
		if(ind!=ID_WORD_MAX) 
			m->specST(addr, val, ind, th->getID(), static_cast<BuffThC*>(th));
		else
			Continuation::template specSTslow<T1, TH>(addr, val, th);
	}

	template<class T1, class TH>
	inline static void specSTslow (
          volatile T1* addr, const T1& val, TH* th, typename disable_if<is_same_type<T,T1> >::type* = NULL
        ) {  
		Continuation::template specSTslow<T1, TH>(addr, val, th);
	}

	/************************************************/
	/************************************************/
	/************************************************/

	template<class M1, M1* m1, const int var, class T1, class TH>
	inline static T1 specLD(volatile T1* addr, TH* th) {
		return specLDmodRec<M1, m1, var, T1, TH>(addr, th);
/*
		try {
			return specLDmodRec<M1, m1, var, T1, TH>(addr, th);
		} catch (IntervalExc exc) {
			//cout<<"SSSSSSLLLLLLLOOOOOOOWWWWW!!!!!"<<endl;
			return BSELF::template specLDslow<T1, TH>(addr, th);
		}
*/
/*
		ID_WORD index = m1->template checkBoundsSlow((T1*)addr);   // was m->check... ???
		if(index<ID_WORD_MAX) 
			return specLDmodRec<M1, m1, var, T1, TH>(addr, th, index);
		else
			return BSELF::template specLDslow<T1, TH>(addr, th);
*/
	} 

	template<class M1, M1* m1, class T1, class TH>
	inline static T1 specLD(volatile T1* addr, TH* th)  {
		return specLD<M1, m1, 0, T1, TH>(addr, th);
	}

	template<class M1, M1* m1, const int var, class T1, class TH>
	inline static T1 specST(volatile T1* addr, const T1& val, TH* th)  {
		return specSTmodRec<M1, m1, var, T1, TH>(addr, val, th);

/*
		ID_WORD index = m1->checkBoundsSlow((T1*)addr);    // was m->check... ???
		if(index<ID_WORD_MAX) 
			specSTmodRec(addr, val, index, th);
		else BSELF::template specSTslow<T1, TH>(addr, val, th);
*/
/*
		try {
			return specSTmodRec<M1, m1, var, T1, TH>(addr, val, th);
		} catch (IntervalExc exc) {
			BSELF::template specSTslow<T1, TH>(addr, val, th);
		}
*/
	} 

	template<class M1, M1* m1, class T1, class TH>
	inline static T1 specST(volatile T1* addr, const T1& val, TH* th) {
		return specST<M1, m1, 0, T1, TH>(addr, val, th);
	}

	/************************************************/
	/**************** specLDmodRec ******************/
	/************************************************/

	template<class M1, M1* m1, const int var, class T1, class TH>
	inline static T1 specLDmodRec(
			volatile T1* addr, TH* th, const ID_WORD index,
			typename disable_if< is_same_obj<M1, MemTp, m1, m> >::type* = NULL
	) {            
		return Continuation::template specLDmodRec<M1, m1, var, T1, TH>(addr, th, index);
	}

	template<class M1, M1* m1, const int var, class T1, class TH>
	inline static T specLDmodRec(
		volatile T* addr, TH* th, const ID_WORD index,
		typename enable_if< is_same_obj<M1, MemTp, m1, m> >::type* = NULL
	) {
		return static_cast<BuffThC*>(th)->specLD(addr, th->getID(), index);   
	}

	template<class M1, M1* m1, const int var, class T1, class TH>
	inline static void specSTmodRec(
		volatile T* addr, const T& val, const ID_WORD index, TH* th, 
		typename enable_if< is_same_obj<M1, MemTp, m1, m>, BuffThC >::type* = NULL
	)  {
		static_cast<BuffThC*>(th)->specST(addr, val, th->getID(), index);    
	}

	template<class M1, M1* m1, const int var, class T1, class TH>
	inline static void specSTmodRec(
			volatile T1* addr, const T1& val, const ID_WORD index, TH* th,
			typename disable_if< is_same_obj<M1, MemTp, m1, m> >::type* = NULL
	) {            
		Continuation::template specSTmodRec<M1, m1, var, T1, TH>(addr, val, th, index);
	}
};


template<
		class M, M* m, const int TH_ATTR, class Continuation, 
		class BuffThComp = typename valid_SpMod_Buff_pair<M,m,TH_ATTR>::type 
>
class USpModel 
		//: public USpModelNever<M, m, TH_ATTR> {
		: public BaseUSpModel< 
			typename M::ElemType, M, m, 
			BuffThComp,
			Continuation
		>  {
  private:
	typedef typename enable_if< is_same_type< typename valid_SpMod_Buff_pair<M,m,TH_ATTR>::type, BuffThComp > >::type B;
};

class Dummy{}; Dummy dummy;
template<> 
class USpModel<Dummy, &dummy, 0, void*,  int>  : public USpModelNever<Dummy, &dummy, 0> {};
typedef USpModel<Dummy, &dummy, 0, void*,  int> USpModelFP;


/*

template<
	class T, SpMod_ReadOnly<T>* m, class Continuation 
>
class USpModel< SpMod_ReadOnly<T>, m, RO_ATTR , Continuation > 
: public BaseUSpModel<
				T, SpMod_ReadOnly<T>, m, 
				Buff_RO<T, SpMod_ReadOnly<T>, m>,
				Continuation
			> 
{ };


template<
	class T, class HashFct, SpMod_SCcore<HashFct>* m_core, 
	SpMod_SC<HashFct, m_core, T>* m, class Continuation 
>
class USpModel< SpMod_SC<HashFct, m_core, T>, m, ItIndAttr_HAfw, Continuation > 
: public BaseUSpModel< 
			T, SpMod_SC<HashFct, m_core, T>, m, 
			BuffSCM<T, SpMod_SC<HashFct, m_core, T>, m, ItInd_Haddr<1> >,
			Continuation
		>
{ };

template<
	class T, class HashFct, SpMod_SCcore<HashFct>* m_core, 
	SpMod_SC<HashFct, m_core, T>* m, class Continuation 
>
class USpModel< SpMod_SC<HashFct, m_core, T>, m, ItIndAttr_HAbw, Continuation > 
: public BaseUSpModel< 
			T, SpMod_SC<HashFct, m_core, T>, m, 
			BuffSCM<T, SpMod_SC<HashFct, m_core, T>, m, ItInd_Haddr<0> >,
			Continuation
		>
{ };

*/


/*
template<
	class T, class HashFct, SpMod_SCcore<HashFct>* m_core, 
	SpMod_SC<HashFct, m_core, T>* m, class Continuation 
>
class USpModel< SpMod_SC<HashFct, m_core, T>, m, ItIndAttr_SV, Continuation > 
: public BaseUSpModel< 
			T, SpMod_SC<HashFct, m_core, T>, m, 
			BuffSCM<T, SpMod_SC<HashFct, m_core, T>, m, ItInd_SV>,
			Continuation
		>
{ };
*/


/***********************************************************/
/********************** USpMselect *************************/
/***********************************************************/


/**
 * USpMselect receives a unified speculative model (UM)  
 * and a basic speculative model (m) and returns the 
 * buffarable thread component corresponding to that
 * (USpMselect<UM, M, m>::BuffThreadComponent)
 */

template<class UM, class M, M* m>
class USpMselect {};

template<class M, M* m, const int TH_ATTR, class Continuation, class M1, M1* m1>
class USpMselect< USpModel<M, m, TH_ATTR, Continuation>, M1, m1 > 
: public USpMselect<Continuation, M1, m1> {};


template<class M, M* m, const int TH_ATTR, class Continuation>
class USpMselect < 
	USpModel<M, m, TH_ATTR, Continuation>, 
	M, m
//typename valid_SpMod_Buff_pair<M,m,TH_ATTR>::type
//typename enable_if<valid_SpMod_Buff_pair<M,m,TH_ATTR>, typename valid_SpMod_Buff_pair<M,m,TH_ATTR>::type>::type
> {
  public:
	typedef typename valid_SpMod_Buff_pair<M,m,TH_ATTR>::type BuffThComp;
}; 

/***********************************************************/
/**************************** INH **************************/
/***********************************************************/

template<class UM>
class INH {};

template<> class INH<USpModelFP> {};

template<
		class M, M* m, const int TH_ATTR, class Continuation
>
class INH<USpModel<M,m,TH_ATTR, Continuation> > 
: public USpMselect< USpModel<M,m,TH_ATTR, Continuation>, M, m >::BuffThComp,
  public INH<Continuation> {};


#endif // define UN_MEM_MODEL
