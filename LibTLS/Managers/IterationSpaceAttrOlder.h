using namespace std;

/************************************************************************/
/******************** BASE ITER_SPACE ATTRIBUTE *************************/
/************************************************************************/

template<class SELF, const ID_WORD UNROLL> 
class BaseChunks {
  public:
	typedef Speculative_Thread<SELF>		SpecThread;
	typedef BaseChunks<SELF, UNROLL>		BASE;

	template<class TH>
	inline void setBigLapID(TH* th) {}

	volatile ID_WORD	master_id;


  protected:
	static const int	nr_threads_per_it;
	volatile ID_WORD	id_ended;
	
	inline SELF* __attribute__((always_inline))
	getMe() { return static_cast<SELF*>(this); }

	inline const ID_WORD __attribute__((always_inline)) 
	firstID() const { return 1; } 

	template<class TH>
	inline ID_WORD __attribute__((always_inline))
	setNextBigIterId(TH* th, const ID_WORD old_id) const {
		return old_id;
	}

  public:

	BaseChunks() { id_ended = ID_WORD_MAX;  }

	inline bool isMasterId(const ID_WORD id) {
		return (master_id == id);
	}

	/**
	 * NEED TO HANDLE HOW TO END THE SPECULATIVE EXECUTION
	 */
	template<class IDandBuffTH>
	/*inline*/ void /*__attribute__((always_inline))*/ acquireResources(IDandBuffTH* th) { 

		typename SELF::MemoryModel* memModel = SELF::getMemoryModel();

		const ID_WORD old_id = th->getID();
		const ID_WORD new_id = getMe()->setNextBigIterId(th, old_id);

		while(!getMe()->shouldRollback(old_id, th)) {
			//cout<<"old_id: "<<old_id<<" master: "<<master_id<<endl;
			if(isMasterId(old_id)) {         // getMe()->highestNrThread == new_id;
				try { memModel->commit_at_iteration_end(th); }
				catch(Dependence_Violation v) { 
					//cout<<"Rollback: "<<v.roll_id<<" th: "<<old_id<<endl;
					this->getMe()->rollbackSTs(old_id, old_id, th); break; 
				}

				memModel->acquireResources(th, new_id);
				th->execSynchCode();
					
				getMe()->computeNextMaster();   //computeNextSynchFrameId
				//cout<<"New id: "<<new_id<<endl;
				return; 
			}

			/** if end of speculation, return here **/
			if(old_id > this->id_ended) { pthread_exit(NULL); } 
			//pthread_yield ();
		}
		//cout<<"Rollback!!"<<endl;
		new_id_continuation(th);		
	}

	template<class TH>
	void commit_ended(TH* th) { 
		typename SELF::MemoryModel* memModel = SELF::getMemoryModel();
		const ID_WORD id = th->getID();

		//cout<<"In commit ended master: "<<master_id<<" id: "<<id<<endl;

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
				return;
			} 
		}		
	}
		
	// In case of a rollback go execute 
	// the thread still needs to acquire resources,
	// and execute the sequential part of code in
	// mutual exclusion!
	template<class IDandBuffTH>
	void new_id_continuation(IDandBuffTH* th) {
		typename SELF::MemoryModel* memModel = SELF::getMemoryModel();
		const ID_WORD old_id = th->getID();
		const ID_WORD new_id = getMe()->setNextBigIterId( th, old_id );

		while(!getMe()->shouldRollback(new_id, th)) {
			if(isMasterId(old_id)) {
				memModel->acquireResources(th, new_id);
				th->execSynchCode();
				getMe()->computeNextMaster();
				return;
			}
			if(old_id > this->id_ended) { pthread_exit(NULL); }
			pthread_yield();	
		}
	}

};

template<class SELF, const ID_WORD UNROLL>
const int	BaseChunks<SELF,UNROLL>::nr_threads_per_it = SELF::SLAVES_PER_MASTER+1;


/************************************************************************/
/******************** INTERLEAVED ATTRIBUTE *****************************/
/************************************************************************/


template<class SELF, const ID_WORD UNROLL> 
class InterleavedChunks : public BaseChunks<SELF, UNROLL> {
  private:
	typedef BaseChunks<SELF, UNROLL>	BASE;
	unsigned int 		new_jump;
	

	ID_WORD				nr_of_mas_th;
	ID_WORD				nr_of_liv_th; 
	ID_WORD				nr_of_liv_th_mul_unroll; 
	ID_WORD				nr_of_liv_th_mul_unroll_1; 

  public:

	inline const int seq_mult() const { return nr_of_mas_th; }

	const ID_WORD firstID(const int mask) const {
		return mask*BASE::nr_threads_per_it+1;
	};
	
	inline ID_WORD __attribute__((always_inline))
	previousID(const ID_WORD id) const {
		return id - nr_of_liv_th; 
	} 

	inline ID_WORD __attribute__((always_inline))
	computeNextID(const ID_WORD old_id) const {
		return old_id + nr_of_liv_th;
	}

	template<class TH>
	void __attribute__((always_inline))
	setNextID(TH* th) {
		th->setID( th->getID() + nr_of_liv_th );
	}

  public:
	InterleavedChunks() : new_jump(0) { }  

	void init() {
		update();
		this->master_id = this->firstID(0);
	}

	void update() {
		nr_of_mas_th 				= this->getMe()->getLiveMasterThNum();
		nr_of_liv_th 				= nr_of_mas_th * BASE::nr_threads_per_it; 
		nr_of_liv_th_mul_unroll		= nr_of_liv_th * UNROLL; 		
		nr_of_liv_th_mul_unroll_1	= nr_of_liv_th_mul_unroll - nr_of_liv_th; 
	}

	inline ID_WORD __attribute__((always_inline)) 
	computeNextMaster() {
		new_jump ++;
		this->master_id += BASE::nr_threads_per_it;
		if(new_jump == nr_of_mas_th) {
			this->master_id += nr_of_liv_th_mul_unroll_1;
			new_jump = 0;
		}
	}

	inline ID_WORD getRollNextSeqId(const ID_WORD safe_id) {
		ID_WORD rem0 = safe_id % nr_of_liv_th_mul_unroll;
		ID_WORD ret_id  = safe_id - rem0 + 1;
		return ret_id;
	}

	inline void resetMaster(const ID_WORD roll, const ID_WORD seq_its) {
		ID_WORD rem0 = roll % nr_of_liv_th_mul_unroll;
		ID_WORD ret_id  = roll - rem0 + 1;
		this->master_id = ret_id + seq_its*nr_of_liv_th_mul_unroll;
	}	

	inline void updateThreadsIdRollback(
			const ID_WORD 				safe_id, 
			typename BASE::SpecThread** Spec_Threads, 
			const int 					seq_its
	) {
		resetMaster(safe_id, seq_its);
		int j = 0;
		for(int i=0; i<this->getMe()->getMasterThNum(); i++) {
			typename BASE::SpecThread* th = Spec_Threads[i];
			if(!th->isEnded()) {
				th->setID(this->master_id+j);
				j+=BASE::nr_threads_per_it;
			}
		}
	}
};



/************************************************************************/
/******************** SEQUENTIAL ATTRIBUTE *****************************/
/************************************************************************/



template<class SELF, const ID_WORD UNROLL> 
class SequentialChunks : public BaseChunks<SELF, UNROLL> {
  private:
	typedef BaseChunks<SELF, UNROLL>	BASE;

	ID_WORD				nr_of_liv_th_1_mul_unroll; 
	ID_WORD				nr_of_th_per_it_mul_unroll;
	
  public:

	inline const int seq_mult() const { return 1; }
	
	// 0->1; 1->UNROLL*nr_threads_per_it+1; ...
	const ID_WORD firstID(const int mask) const {
		return mask*nr_of_th_per_it_mul_unroll+1;
	};


	inline ID_WORD __attribute__((always_inline))
	previousID(const ID_WORD id) const {
		if( id%UNROLL != this->firstID() ) { return id - 1; }
		else {
			const ID_WORD NUM_M_TH = this->getMe()->getLiveMasterThNum();
			return id - nr_of_liv_th_1_mul_unroll;
		} 
	} 

	inline ID_WORD __attribute__((always_inline))
	computeNextID(const ID_WORD old_id) const {
		return old_id + BASE::nr_threads_per_it;
	}

	template<class TH>
	void __attribute__((always_inline))
	setNextID(TH* th) {
		th->setID( th->getID() + BASE::nr_threads_per_it );
	}

	
	SequentialChunks() {  }

	void init() {
		update();
		this->master_id = this->firstID(0);
	}

	void update() {
		ID_WORD M_LIVE_NR 		= this->getMe()->getLiveMasterThNum();
		nr_of_th_per_it_mul_unroll = UNROLL * BASE::nr_threads_per_it;
		nr_of_liv_th_1_mul_unroll	= nr_of_th_per_it_mul_unroll*(M_LIVE_NR-1);
	}

	template<class TH>
	inline void setBigLapID(TH* th) {
		const ID_WORD id = th->getID() + nr_of_liv_th_1_mul_unroll;
		th->setID(id);
	}

	inline ID_WORD __attribute__((always_inline)) 
	computeNextMaster() {
		this->master_id += nr_of_th_per_it_mul_unroll;
	}

	inline ID_WORD getRollNextSeqId(const ID_WORD safe_id) {
		ID_WORD rem1 = safe_id % nr_of_th_per_it_mul_unroll;
		ID_WORD ret_id = safe_id - rem1 + 1;
		return ret_id;
	}


	inline void resetMaster(const ID_WORD roll, const ID_WORD seq_its) {
		ID_WORD rem1 = roll % nr_of_th_per_it_mul_unroll;
		ID_WORD ret_id = roll - rem1 + 1;
			//- nr_of_th_per_it_mul_unroll * this->getMe()->getLiveMasterThNum();
		this->master_id = ret_id + seq_its*nr_of_th_per_it_mul_unroll;
	}

	inline void updateThreadsIdRollback(
			const ID_WORD 					safe_id, 
			typename BASE::SpecThread** 	Spec_Threads, 
			const int 						seq_its
	) {
		resetMaster(safe_id, seq_its);
		int j = 0;
		for(int i=0; i<this->getMe()->getMasterThNum(); i++) {
			typename BASE::SpecThread* th = Spec_Threads[i];
			if(!th->isEnded()) {
				th->setID(this->master_id+j);
				j+=nr_of_th_per_it_mul_unroll;
			}
		}
	}
};


/************************************************************************/
/******************** NORMALIZED ATTRIBUTE ******************************/
/************************************************************************/

template<class SELF, const ID_WORD UNROLL> 
class NormalizedChunks : public BaseChunks<SELF, UNROLL> {
  protected:
	typedef BaseChunks<SELF, UNROLL>	BASE;

	ID_WORD				nr_of_liv_th_mul_unroll; 
	ID_WORD				nr_of_liv_th;

  public:

	inline const int seq_mult() const { return 1; }

	template<class TH>
	inline ID_WORD __attribute__((always_inline))
	setNextBigIterId(TH* th, const ID_WORD old_id) const {
		const ID_WORD new_id = old_id + nr_of_liv_th;
		th->setID(new_id);
		return new_id;
	}

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

	template<class TH>
	inline void __attribute__((always_inline))
	setNextID(TH* th) const { }


	NormalizedChunks() { }

	void init() {
		update();
		this->master_id = this->firstID(0);
	}

	void update() {
		ID_WORD M_LIVE_NR 		= this->getMe()->getLiveMasterThNum();
		nr_of_liv_th			= M_LIVE_NR*BASE::nr_threads_per_it;
		nr_of_liv_th_mul_unroll	= UNROLL*BASE::nr_threads_per_it*M_LIVE_NR;
	}

	inline void computeNextMaster() {
		this->master_id += BASE::nr_threads_per_it;
	}

	inline ID_WORD getRollNextSeqId(const ID_WORD safe_id) {
		int rem = safe_id % BASE::nr_threads_per_it;
		if(rem == 0) rem = BASE::nr_threads_per_it;
		rem = safe_id - rem + 1;
		return rem;
	}

	inline void resetMaster(const ID_WORD roll, const ID_WORD seq_its) {
		int rem = roll % BASE::nr_threads_per_it;
		if(rem == 0) rem = BASE::nr_threads_per_it;
		rem = roll - rem + 1 - nr_of_liv_th; //BASE::nr_threads_per_it;
		this->master_id = rem + seq_its*BASE::nr_threads_per_it;
	}

	inline void updateThreadsIdRollback(
		const ID_WORD 					safe_id, 
		typename BASE::SpecThread** 	Spec_Threads, 
		const ID_WORD					seq_its
	) {
		resetMaster(safe_id, seq_its);
		int j = 0;

		for(int i=0; i<this->getMe()->getMasterThNum(); i++) {
			typename BASE::SpecThread* th = Spec_Threads[i];
			if(!th->isEnded()) {
				th->setID(this->master_id+j);
				j+=BASE::nr_threads_per_it;
			}
		}
	}
};
