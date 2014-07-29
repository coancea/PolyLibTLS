#ifndef SPEC_THREAD
#define SPEC_THREAD

using namespace std;

#ifndef PTHREAD
#define PTHREAD
#include <pthread.h>
#endif


#include "Util/AtomicOps.h"
#include "MemModels/UnifiedMemModel.h"
#include "Threads/Spec_Thread0.h"
#include "Threads/Bufferable_Threads.h"


/**
 * It would be nice to be able to parameterize this class under the SELF type!
 * Unfortunately this is not possible since it is used by the Thread Manager
 * and therefore the SELF parameter will introduce a circular type dependency
 * that cannot be expressed with templates
 */
//template<class SELF, class TM>					
template<class TM>
class Speculative_Thread : public Spec_Thread0 {
	protected:
		/**
		 * the thread manager who is conducting the speculation
		 */
		TM* 				tm;
		

		/**
		 * the pthread params
		 */
		pthread_t 		pthread;
		pthread_attr_t 		thread_attr;
		struct sched_param 	thread_param;
		struct sched_param 	thread_param1;
		
	public:
		
		inline pthread_t* 		 __attribute__((always_inline))
			getPThread() 		{ return &pthread;	}

		inline pthread_attr_t*	         __attribute__((always_inline))
			getPThreadAttr() 	{ return &thread_attr;	}

		inline struct sched_param* 
			getPThreadSched() 	{ return &thread_param;	}
		
		inline  TM*  			__attribute__((always_inline))
			getTM() const		{ return tm; 		}

		inline void  
			setTM(TM* tm)		{ this->tm 	= tm; 	}

		// this does not do the job -- crashes the program.
		void kill();

		/** no constructors since this is an abstract class **/

		inline void init(ID_WORD i=0, const int msk = 1, TM* tm = NULL) {
			this->Spec_Thread0::init(i/*, msk*/);
		}
	
		void startSpeculativeThread();

		virtual void run() = 0;
		virtual void executeNonSpecIteration() = 0;

	 	void goWait();
		static void* spec_fct(void* v_th);
};


template<class SELF, class UM>
class ThLDSTcomp : public INH<UM> {
  public:

	inline SELF* __attribute__((always_inline))
	getMe() { return static_cast<SELF*>(this); }


	inline void __attribute__((always_inline))
	resetThComp() {  UM::resetThComp(getMe()); }

	inline void setParent() {
		UM::setThCompParent(getMe());
	}

	template<class M, M* m, class T>
	inline T __attribute__((always_inline))
	specLD(volatile T* addr, typename enable_if< is_SpRO_model<M> >::type* = NULL) 
	throw(Dependence_Violation) {
		try {
			m->checkBounds((T*)addr/*, getMe()->getID()*/);
			return *addr;
		} catch (IntervalExc exc) {
			return UM::template specLDslow<T,SELF>(addr, getMe(), NULL);
 		}
	}

	template<class M, M* m, class T>
	inline T __attribute__((always_inline))
	specLD(volatile T* addr, typename disable_if< is_SpRO_model<M> >::type* = NULL) 
	throw(Dependence_Violation) {

		try {
			const ID_WORD ind = m->checkBounds((T*)addr/*, getMe()->getID()*/);
			typedef typename USpMselect<UM, M, m>::BuffThComp TH;
			TH* th = static_cast<TH*>(this);
			return th->specLD( addr, getMe()->getID(), ind );  
		} catch (IntervalExc exc) {
			return UM::template specLDslow<T,SELF>(addr, getMe(), NULL);
 		}


	}

	template<class M, M* m, class T>
	inline void __attribute__((always_inline))
	specST(volatile T* addr, const T& val) {

		try {
			const ID_WORD ind = m->checkBounds((T*)addr/*, getMe()->getID()*/);
			typedef typename USpMselect<UM, M, m>::BuffThComp TH;
			TH* th = static_cast<TH*>(this);
			th->template specST(addr, val, getMe()->getID(), ind);
		} catch (IntervalExc exc) {
			return UM::template specSTslow<T,SELF>(addr, val, getMe(), NULL);			
 		}

/*
		try {
			const ID_WORD ind = m->checkBounds((T*)addr);
			typedef typename USpMselect<UM, M, m>::BuffThComp TH;
			TH* th = static_cast<TH*>(this);			
			th->template specST(addr, val, getMe()->getID(), ind);
		} catch (IntervalExc exc) {
			void (*slowST)(volatile T*, const T&, SELF*, void*) = UM::template specSTslow<T,SELF>;
			slowST(addr, val, getMe(), NULL);			
		}
*/			
	}
};

#define ROLL 0

template<class SELF, class UM, class TM>
class SpecMasterThread : 
	public Speculative_Thread<TM>, 
	public ThLDSTcomp<SELF, UM> {
	/**
	 * SELF is supposed to provide implementation for the following methods:
	 * 	updateInductionVars()
	 * 	end_condition()
	 * 	iteration_body()
         *      non_spec_iteration_body()
	 *	
	 * TM is supposed to provide implementation for the following methods:
	 * 	acquire()
	 * 	release()
	 *	rollbackSTs(ULong, ULong, Speculative_Thread<TM>*);
	 *	ended()
	 */

	private:
		const unsigned long* padding;
		const unsigned long  unroll;
		int                  make_roll;

	public:
		static volatile int  synch_barrier_start;

		inline void __attribute__((always_inline)) staticCheckTM() {
			// not clear how to implement this, regarding that the TM
			// receives as template parameters the memory model dispatcher and 		
			// a pointer to the memory model dispatcher ...
			;
		}

		inline void reset() {
			UM::rollbackCleanUp(this->getMe());
			this->setToRunning();
		}

		inline SpecMasterThread(const unsigned long uroll, const unsigned long* dummy) 
			: unroll(uroll), padding(dummy) 
		{ 
			this->Speculative_Thread<TM>::init(); this->setParent();
		}

		inline SpecMasterThread(
				const unsigned long uroll, 
				const unsigned long i, 
				const unsigned long* dummy
		) : unroll(uroll), padding(dummy)
		{
			this->Speculative_Thread<TM>::init(i); this->setParent();
		}

		inline ~SpecMasterThread() { delete[] padding; }

		inline void __attribute__((always_inline)) execSynchCode() { }

		inline void __attribute__((always_inline)) iteration_init() {
			this->getTM()->acquireResources(this->getMe());
			this->resetThComp();
		}

		inline void __attribute__((always_inline)) iteration_ender() {
			this->getTM()->setBigLapID(this->getMe());
		}

		inline void __attribute__((always_inline)) executeNonSpecIteration() {
			SELF* me = this->getMe();

			//me->updateInductionVars();

			/**
			 * unroll the loop so that the expensive operations of
			 * getting a new id and access to the lock-free data
			 * structures are amortized
			 */
			for(int i=0; i<this->unroll; i++) {

				/** 
				 * if the end_condition is met, then the
				 * speculative thread has ended his job
				 * will return or wait ...
				 */
				if( me->end_condition_non_spec() ) { 
					//cout<<"Program ends here! "<<this->id<<endl;
					this->setToEnded(); 
					//this->tm->ended/*_roll*/( me ); 
					return;  
				}

				/**
				 * execute the body of the iteration where
				 * all the variable access is speculative
				 */
				me->non_spec_iteration_body();
			}
		}

		inline bool end_condition_non_spec() {
			return this->getMe()->end_condition();
		}


		/**
		 * The meta-body of the model when there is no master thread
		 * (predictor) executing the sequential part of the code and
		 * feeding the speculative threads with a new id and induction
		 * variables. In this model each speculative thread is doing
		 * all this!
		 */
		inline void run() {
			//pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
			make_roll = 0;
			SELF* me = this->getMe();

			while( true ) {
				try 
				{
					/** 
					 * get a new id, get access to the lock-free data 
					 * structure, check if the thread has to wait due
					 * to a rollback
					 */
					this->iteration_init();
 
					/**
					 * update the induction variables with respect to 
					 * number of the iteration the thread is executing
					 */
					me->updateInductionVars();


					/**
					 * unroll the loop so that the expensive operations of
					 * getting a new id and access to the lock-free data
					 * structures are amortized
					 */
					for(int i=0; i<this->unroll; i++) {

						/** 
						 * if the end_condition is met, then the
						 * speculative thread has ended his job
						 * will return or wait ...
						 */
						if( me->end_condition() ) { 
							this->setToEnded(); 

							bool may_return = true;
							while(! this->tm->allEnded(me) ) {
								if(this->tm->shouldRollback(this->id, me)) { may_return = false; break; }
								pthread_yield();
							}  

							if(may_return) return;
							else break;  
						}
						/**
						 * execute the body of the iteration where
						 * all the variable access is speculative
						 */
						me->iteration_body();
						
						this->getTM()->setNextID(this->getMe());
					}
					
#if (ROLL!=0)			
						// I use this to add "artificial" dependency violations
						make_roll++;
						//cout<<"make_roll: "<<make_roll<<endl;
						if( (make_roll % ROLL) == 0) {
							//cout<<"make_roll: "<<make_roll<<endl;
							throw Dependence_Violation(this->id-1, this->id,
		 							Dependence_Violation::LOAD_TP
							);
						}
#endif

					/**
					 * release ownership of the lock-free data structure it
					 * has acquired at the beginning of the iteration if necessary
					 * and possibly get a new id (depending on the traversal strategy)
					 */
					this->iteration_ender();
				} catch (Dependence_Violation v) {
					/** 
					 * if the current thread has discovered a dependence
					 * violation, start the rollback routine!
					 */
					me->tm->rollbackSTs(v.roll_id, v.thread_id, this->getMe());
					//cout<<"THREAD "<<(this->id%8)<<" continues from rollback"<<endl;
				}
			}
		}
};

template<class SELF, class UM, class TM>
volatile int SpecMasterThread<SELF, UM, TM>::synch_barrier_start = 0;

/**
 * This type of thread is suppose to work with a MasterThread_Manager
 * Therefore, both MasterThread_Manager and SpecSlaveThread needs to
 * be extended and the communication mechanism needs to be implemented!
 * THIS IS NOT MPLEMENTED YET!
 */
template<class SELF, class TM>
class SpecSlaveThread : public Speculative_Thread<TM> {
	/**
	 * a refinment of SpecSlaveThread needs to implement:
	 *		setInputOutputIndVars(TM::InductionVarsStruct*, TM::InductionVarsStruct*)
	 *		boolean checkPredictor()
	 *		void	iteration_body()
	 */

	protected:
		SELF* 				me;
		const unsigned int 	unroll;
	public:
		inline void __attribute__((always_inline)) staticCheckTM() {
			// not clear how to implement this, regarding that the TM
			// receives as template parameters the memory model dispatcher and 		
			// a pointer to the memory model dispatcher ...
			;
		}

		inline void init(SELF* me, const int uroll, unsigned long i=0, const int msk = 1, TM* tm = NULL) {
			this->me = me;
			this->unroll = uroll;
			this->Speculative_Thread<TM>::init(i, msk, tm);

			/** static check of TM???  **/
			me->staticCheckTM();
		}

		inline SpecSlaveThread(SELF* me, const unsigned int uroll): unroll(uroll) { 
			this->init(me, uroll); 
		}
		inline SpecSlaveThread(SELF* me, const unsigned int uroll, unsigned long i, const int msk, TM* tm) 
				: unroll(uroll) 
		{
			this->init(me, uroll, i, msk, tm);
		}	

		inline void __attribute__((always_inline)) iteration_ender() {
			this->getTM()->release(me);
		}	

		/**
		 * The meta-body of the model for the slave thread. The thread
		 * manager is now a master thread acting like a predictor --
		 * which is executing the sequential part of the loop and feeding
		 * the slave threads with the necessary induction variables.
		 */
		inline void run() {
			while( true ) {
				try {
					/** 
					 * wait untile the master thread has given you a
					 * new id and a new set of induction variables;
					 */
					while(this->isWaiting()) {
						pthread_yield();
					}
					if( this->isEnded() ) return;

					/**
					 * unroll the loop so that the expensive operations of
					 * getting a new id and access to the lock-free data
					 * structures are amortized
					 */
					for(int i=0; i<unroll; i++) {
						/**
						 * execute the body of the iteration where
						 * all the variable access is speculative
						 */
						me->iteration_body();
					}

					/**
					 * release ownership of the lock-free data structure it
					 * has acquired at the beginning of the iteration
					 */
					this->iteration_ender();

					if(!me->checkPredictor()) 
						throw Dependence_Violation(this->id, this->id, Dependence_Violation::PREDICTOR_TP);
				} catch (Dependence_Violation v) {
					/** 
					 * if the current thread has discovered a dependence
					 * violation, start the rollback routine!
					 */
					this->tm->rollbackSTs(v.roll_id, v.thread_id, this);
				}  
			}
		}
};

#include "Threads/SpecThreads.cpp"

#endif // define SPEC_THREAD
