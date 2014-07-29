#ifndef TH_MAN
#define TH_MAN 


//#ifndef STL_VECTOR
//#include<vector>
//#endif  STL_VECTOR


#include "Util/AtomicOps.h"
#include "MemModels/UnifiedMemModel.h"
#include "Threads/SpeculativeThread.h"
#include "Managers/BaseManager.h"
#include "Managers/IterationSpaceAttr.h" 

template<
		class SELF, class MemModel, MemModel* memModel, 
		const int NUM_MASTERS, const int NUM_SLAVES, class EXEC_TYPE_ATTR
> 
class Master_AbstractTM : 
	public Base_Manager<
		SELF, MemModel, memModel, 
		NUM_MASTERS, NUM_SLAVES
	>, public EXEC_TYPE_ATTR {
  private:
	typedef Base_Manager<SELF, MemModel, memModel, NUM_MASTERS, NUM_SLAVES> 	PARENT;
	typedef typename PARENT::SpecThread						SpecThread;

	//volatile ID_WORD	id_ended;
	volatile ID_WORD	thread_barrier_id;

	volatile int		nr_of_waiting_threads;
	volatile int		nr_of_dead_threads;

	int updateRollbackID(const ID_WORD rollid, const ID_WORD thid);

	int set_barrier(const int roll_id, const int thread_id);

  public:
	typedef MemModel			MM;
	static const int SLAVES_PER_MASTER; // = NUM_SLAVES;

	inline static int getSlavesPerMaster() {
		return NUM_SLAVES;
	}
	inline static int getNrMasterThs() {
		return NUM_MASTERS;
	}


	inline void __attribute__((always_inline)) init(/*SELF* sf*/) {
		this->PARENT::init(/*sf*/);
		thread_barrier_id     = ID_WORD_MAX;
		//this->id_ended        = ID_WORD_MAX;
		nr_of_waiting_threads = 0;
		nr_of_dead_threads    = 0;
		nr_of_rollbacks       = 0;
		EXEC_TYPE_ATTR::init();
		memModel->reset(0,0); /** this is not needed for init **/
		MM::setNumProc(NUM_MASTERS);
	}

	template<class TH>
	void cleanUpAll();

	template<class TH>
	void ended_roll(TH* th); 

	template<class TH>
	inline bool allEnded(TH* th) {
		for(int i=0; i<NUM_MASTERS; i++) {
			SpecThread*	thd = this->Spec_Threads[i];
			if(!thd->isEnded()) return false; 
		}	
		return true;
	}


/*
	void reset() {
		this->highestNrThread = ID_WORD_MAX; 
		this->lowestNrThread  = 0; 
		this->safe_id      	  = NO_ROLLBACK;
		this->synchCAS        = 0;
		memModel->reset(0,0);
		this->thCount	= 0;
		this->EXEC_TYPE_ATTR::init();
	}
*/


/*
	template<class TH>
	void ended(TH* th) { 
		non_blocking_synch(&this->synchCAS);
			nr_of_dead_threads++; 
			this->NUM_LIVE_MASTERS--;
			ID_WORD id = th->getID();
			if( this->id_ended > id )
				this->id_ended = id;
		release_synch(& this->synchCAS); //this->synchCAS = 0;
		this->commit_ended(th);
	}
*/		


	/*******************************************************************************/
	/*******************************************************************************/
	template<class TH>
	void goWait(TH* th);

        /*******************************************************************************/
        /*******************************************************************************/

	void respawnThreads();

    /*******************************************************************************/
    /*******************************************************************************/
	int nr_of_rollbacks;
	template<class TH>
	int rollbackSTs(const int rollback_id, const int thread_id, TH* th);


    /********************************************************************************/
    /********************************************************************************/
/*
	template<class TH>
	inline unsigned long __attribute__((always_inline)) acquire(TH* th) {
		return this->getNewID(th);
	}

	template<class TH>
	inline void __attribute__((always_inline)) release(TH* th) {
		memModel->releaseLock(th);
	}
*/

};

#include "Managers/ManagerFcts.cpp"

#endif
