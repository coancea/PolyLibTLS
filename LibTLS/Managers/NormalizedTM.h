using namespace std;

template<class SELF, const ID_WORD UNROLL> 
class NormalizedTM {
  public:
	typedef Speculative_Thread<SELF>	SpecThread;
	typedef NormalizedTM<SELF, UNROLL>	BASE;

	template<class TH>
	inline void setBigLapID(TH* th) {}
	
  protected:
	static const int	nr_threads_per_it;
	volatile ID_WORD	id_ended;


	BaseChunks() : id_ended(ID_WORD_MAX) {  }

	inline SELF* __attribute__((always_inline))
	getMe() const { return static_cast<SELF*>(this); }

  protected:
	ID_WORD				nr_of_liv_th_mul_unroll; 
	ID_WORD				nr_of_liv_th;

  public:

	template<class TH>
	inline void __attribute__((always_inline))
	setNextBigIterId(TH* th, const ID_WORD old_id) const {
		const ID_WORD new_id = old_id + nr_of_liv_th;
		th->setID(new_id);
		//return new_id;
	}

	const ID_WORD firstID() const { return 1; }
	const ID_WORD firstID(const int mask) const {
		return mask*BASE::nr_threads_per_it + 1 - nr_of_liv_th;
	};

	inline ID_WORD __attribute__((always_inline))
	previousID(const ID_WORD id) const {
		return id - nr_of_liv_th;
	} 

	inline ID_WORD __attribute__((always_inline))
	computeNextID(const ID_WORD old_id) const {
		return old_id;
	}

	inline ID_WORD __attribute__((always_inline)) 
	computeNextSynchFrameId(const ID_WORD id) {
		return id + BASE::nr_threads_per_it;
	}

	template<class TH>
	inline void __attribute__((always_inline))
	setNextID(TH* th) const { }


	NormalizedChunks()  {}

	void update() {
		this->master 			= firstID();
		ID_WORD M_LIVE_NR 		= this->getMe()->getLiveMasterThNum();
		nr_of_liv_th			= M_LIVE_NR*BASE::nr_threads_per_it;
		nr_of_liv_th_mul_unroll	= UNROLL*BASE::nr_threads_per_it*M_LIVE_NR;
	}

	void resetMaster(const ID_WORD roll, const ID_WORD seq_its) {
		int rem = roll % BASE::nr_threads_per_it;
		if(rem == 0) rem = BASE::nr_threads_per_it;
		rem = roll - rem + 1 - nr_of_liv_th;
		master_id = rem + seq_its*BASE::nr_threads_per_it;
		cout<<"Master: "<<rem<<endl;
	}

	inline void updateThreadsIdRollback(
		const ID_WORD 					safe_id, 
		typename BASE::SpecThread** 	Spec_Threads, 
		const ID_WORD					seq_its
	) {
		resetMaster(safe_id, seq_its);
		int j = 0;
		for(int i=0; i<getMe()->getMasterThNum(); i++) {
			typename BASE::SpecThread* th = Spec_Threads[i];
			if(!th->isEnded()) {
				th->setID(master_id+j);
				j+=BASE::nr_threads_per_it;
			}
		}
	}
  
  public:

	

	/**
	 * NEED TO HANDLE HOW TO END THE SPECULATIVE EXECUTION
	 */
	template<class IDandBuffTH>
	void acquireResources(IDandBuffTH* th) { 
		typename SELF::MemoryModel* memModel = SELF::getMemoryModel();

		const ID_WORD old_id = th->getID();
		getMe()->setNextBigIterId(th, old_id);

		while(!getMe()->shouldRollback(old_id, th)) {
			if(isMasterId(old_id)) {         // getMe()->highestNrThread == new_id;
				try { memModel->commit_at_iteration_end(th); }
				catch(Dependence_Violation v) { 
					//cout<<"Rollback: "<<v.roll_id<<" th: "<<old_id<<endl;
					this->getMe()->rollbackSTs(old_id, old_id, th); break; 
				}

				memModel->acquireResources(th, new_id);
				th->execSynchCode();
					
				computeNextMaster(old_id);   //computeNextSynchFrameId
				return; 
			}

			/** if end of speculation, return here **/
			if(old_id > this->id_ended) { pthread_exit(NULL); } 
			pthread_yield ();
		}

		new_id_continuation(th);		
	}

	template<class TH>
	void commit_ended(TH* th) { 
		typename SELF::MemoryModel* memModel = SELF::getMemoryModel();

		const ID_WORD id = th->getID();
		while(!getMe()->shouldRollback(id, th) && this->id_ended==id) {

			// if I become master then commit, set the id_ended, and finish
			if(isMasterId(id)) {
				try { memModel->commit_at_iteration_end(th); }
				catch(Dependence_Violation v) { 
					// don't throw a violation because you are master have ended!
					;
				}
				// if( NeedsRollback<typename SELF::MM>::value )
				//	 getMe()->rollbackSTs(id, id, th);	
			} 
		}		
	}
		
		 // In case of a rollback go execute 
		 // the thread still needs to acquire resources,
		 // and execute the sequential part of code in
		 // mutual exclusion!
		 //
	template<class IDandBuffTH>
	void new_id_continuation(IDandBuffTH* th) {
		typename SELF::MemoryModel* memModel = SELF::getMemoryModel();
		ID_WORD old_id = th->getID();
		getMe()->setNextBigIterId( th, old_id );

		while(!getMe()->shouldRollback(new_id, th)) {
			if(isMasterID(old_id)) {
				memModel->acquireResources(th, new_id);
				th->execSynchCode();
				computeNextMaster(old_id);
				return;
			}
			pthread_yield();	
		}
	}

};


template<class SELF, const ID_WORD UNROLL>
const int	BaseChunks<SELF,UNROLL>::nr_threads_per_it = SELF::SLAVES_PER_MASTER+1;
