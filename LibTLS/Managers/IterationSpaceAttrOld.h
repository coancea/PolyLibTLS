using namespace std;

/************************************************************************/
/******************** BASE ITER_SPACE ATTRIBUTE *************************/
/************************************************************************/

template<class SELF, const ID_WORD UNROLL> 
class BaseChunks {
  public:
	typedef Speculative_Thread<SELF>		SpecThread;
	typedef BaseChunks<SELF, UNROLL>	BASE;

	template<class TH>
	inline void setBigLapID(TH* th) {}
	
  protected:
	static const int	nr_threads_per_it;
	volatile ID_WORD	id_ended;


	BaseChunks() : id_ended(ID_WORD_MAX) {  }

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

	/**
	 * NEED TO HANDLE HOW TO END THE SPECULATIVE EXECUTION
	 */
	template<class IDandBuffTH>
	/*inline*/ void /*__attribute__((always_inline))*/ acquireResources(IDandBuffTH* th) { 
		typename SELF::MemoryModel* memModel = SELF::getMemoryModel();
		const ID_WORD old_id = th->getID();
		const ID_WORD new_id = getMe()->setNextBigIterId(th, old_id);

		volatile ID_WORD* high_ptr = &this->getMe()->highestNrThread;

		int count = 0;

		// while no rollbacks -- check if it safe to acquire the buffer entry 
		while(!this->getMe()->shouldRollback(new_id, th)) {
			count++;

			if(*high_ptr == new_id) {
				// then also lowestNrThread == old_id 
				//cout<<*high_ptr<<endl;
				try { memModel->commit_at_iteration_end(th); }
				catch(Dependence_Violation v) { 
					cout<<"Rollback: "<<v.roll_id<<" th: "<<old_id<<endl;
					this->getMe()->rollbackSTs(v.roll_id, old_id, th); break; 
				}

				memModel->acquireResources(th, new_id);
				th->execSynchCode();
					
				*high_ptr = getMe()->computeNextSynchFrameId(new_id);
				return; 
			}

			if(old_id > this->id_ended) { pthread_exit(NULL); } 

			pthread_yield ();
		}

		new_id_continuation(th);
	}

	template<class TH>
	void commit_ended(TH* th) { 
		typename SELF::MemoryModel* memModel = SELF::getMemoryModel();

		const ID_WORD id = th->getID();
		while(!this->getMe()->shouldRollback(id, th) && this->id_ended>id) {

			// ok, so if I am a thread that was supposed to finish, then finish
			if(id >= this->id_ended) { pthread_exit(NULL); } 

			// if I am the first to finish and need to do the commit, do it!
			if(this->getMe()->highestNrThread >= id) {

				//cout<<"committing the last iteration"<<endl;

				try { memModel->commit_at_iteration_end(th); }
				catch(Dependence_Violation v) { 
					cout<<"Rollback: "<<v.roll_id<<" th: "<<id<<endl;
					this->getMe()->rollbackSTs(v.roll_id, id, th); break; 
				}

				this->id_ended = id;
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
		while(true) {
			const ID_WORD new_id = getMe()->setNextBigIterId( th, th->getID() );
			
			while(!this->getMe()->shouldRollback(new_id, th)) {
				if(this->getMe()->highestNrThread == new_id) {			
					memModel->acquireResources(th, new_id);
					th->execSynchCode();
					this->getMe()->highestNrThread = getMe()->computeNextSynchFrameId(new_id);
					return;
				}
				pthread_yield();	
			}
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
	unsigned int 		new_jump;
	

	ID_WORD				nr_of_mas_th;
	ID_WORD				nr_of_liv_th; //multiply_x<BASE::nr_threads_per_it>(NUM_M_TH); 
	ID_WORD				nr_of_liv_th_mul_unroll; // nr_of_liv_th * UNROLL
	ID_WORD				nr_of_liv_th_mul_unroll_1; // nr_of_liv_th * (UNROLL - 1)

  public:

	//th1.setID(1); th2.setID(3); sl1.setID(2); sl2.setID(4);	
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
	}typedef BaseChunks<SELF, UNROLL>	BASE;

	inline ID_WORD __attribute__((always_inline)) 
	computeNextSynchFrameId(const ID_WORD id) {
		new_jump ++;
		ID_WORD highest = id+BASE::nr_threads_per_it;
		if(new_jump == nr_of_mas_th) {
			highest += nr_of_liv_th_mul_unroll_1;
			new_jump = 0;
		}
		return highest; 
	}

	template<class TH>
	void __attribute__((always_inline))
	setNextID(TH* th) {
		th->setID( th->getID() + nr_of_liv_th );
	}

  public:
	InterleavedChunks() : new_jump(0) {}  

	void update() {
		this->getMe()->highestNrThread 	= 1;
		nr_of_mas_th 				= this->getMe()->getLiveMasterThNum();
		nr_of_liv_th 				= nr_of_mas_th * BASE::nr_threads_per_it; //multiply_x<nr_threads_per_it>(NUM_M_TH); 
		nr_of_liv_th_mul_unroll		= nr_of_liv_th * UNROLL; 		// nr_of_liv_th * UNROLL
		nr_of_liv_th_mul_unroll_1	= nr_of_liv_th_mul_unroll - nr_of_liv_th; // nr_of_liv_th * (UNROLL - 1)
	}

	void resetHighest(const ID_WORD roll) {
		ID_WORD rem0 = roll % nr_of_liv_th_mul_unroll;
		ID_WORD ret_id  = roll - rem0;
		this->getMe()->highestNrThread = ret_id+1;
	}	

	inline void updateThreadsIdRollback(
			const ID_WORD 				safe_id, 
			typename BASE::SpecThread** Spec_Threads, 
			const int 					NUM_MASTERS
	) {
		int j = 0;
		unsigned int highest = this->getMe()->highestNrThread;
		for(int i=0; i<NUM_MASTERS; i++) {
			typename BASE::SpecThread* th = Spec_Threads[i];
			if(!th->isEnded()) {
				th->setID(highest+j);
				j+=BASE::nr_threads_per_it;
			}
		}
	}

/*
	ID_WORD whichBigIteration(const ID_WORD roll) {
		ID_WORD rem0 = roll % nr_of_liv_th;
		if(rem0 == 0) rem0 = nr_of_liv_th;
		ID_WORD rem1 = roll % nr_of_liv_th_mul_unroll;
		ID_WORD ret_id  = roll - rem1 + rem0;
		return ret_id;
	}
*/
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

	inline ID_WORD __attribute__((always_inline)) 
	computeNextSynchFrameId(const ID_WORD id) {
		return id + nr_of_th_per_it_mul_unroll;
	}

	template<class TH>
	void __attribute__((always_inline))
	setNextID(TH* th) {
		th->setID( th->getID() + BASE::nr_threads_per_it );
	}

	
	SequentialChunks()  { }

	void update() {
		cout<<"S-a apelat update!!!"<<endl;
		this->getMe()->highestNrThread = 1;
		ID_WORD M_LIVE_NR 		= this->getMe()->getLiveMasterThNum();
		nr_of_th_per_it_mul_unroll = UNROLL * BASE::nr_threads_per_it;
		nr_of_liv_th_1_mul_unroll	= nr_of_th_per_it_mul_unroll*(M_LIVE_NR-1);
	}

	template<class TH>
	inline void setBigLapID(TH* th) {
		const ID_WORD id = th->getID() + nr_of_liv_th_1_mul_unroll;
		th->setID(id);
	}

	void resetHighest(const ID_WORD roll) {
		ID_WORD rem1 = roll % nr_of_th_per_it_mul_unroll;
		ID_WORD ret_id = roll - rem1 + 1;
		this->getMe()->highestNrThread = ret_id;
	}

	inline void updateThreadsIdRollback(
			const ID_WORD 					safe_id, 
			typename BASE::SpecThread** 	Spec_Threads, 
			const int 						NUM_MASTERS
	) {
		int j = 0;
		unsigned int highest = this->getMe()->highestNrThread;
		for(int i=0; i<NUM_MASTERS; i++) {
			typename BASE::SpecThread* th = Spec_Threads[i];
			if(!th->isEnded()) {
				th->setID(highest+j);
				j+=nr_of_th_per_it_mul_unroll;
			}
		}
	}
/*
	//th1.setID(1); th2.setID(2*UNROLL+1); sl1.setID(2); sl2.setID(2*UNROLL+2);
	ID_WORD whichBigIteration(const ID_WORD roll) {
		ID_WORD rem0 = roll % BASE::nr_threads_per_it;
		if(rem0 == 0) rem0 = BASE::nr_threads_per_it;
		ID_WORD rem1 = roll % nr_of_th_per_it_mul_unroll;
		ID_WORD ret_id = roll - rem1 + rem0;
		return ret_id;
	}
*/
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

	inline ID_WORD __attribute__((always_inline)) 
	computeNextSynchFrameId(const ID_WORD id) {
		return id + BASE::nr_threads_per_it;
	}

	template<class TH>
	inline void __attribute__((always_inline))
	setNextID(TH* th) const { }


	NormalizedChunks()  {}

	void update() {
		this->getMe()->highestNrThread = 1;
		ID_WORD M_LIVE_NR 		= this->getMe()->getLiveMasterThNum();
		nr_of_liv_th			= M_LIVE_NR*BASE::nr_threads_per_it;
		nr_of_liv_th_mul_unroll	= UNROLL*BASE::nr_threads_per_it*M_LIVE_NR;
	}

	void resetHighest(const ID_WORD roll) {
		int rem = roll % BASE::nr_threads_per_it;
		if(rem == 0) rem = BASE::nr_threads_per_it;
		rem = roll - rem + 1;
		this->getMe()->highestNrThread = rem;
		//cout<<"Highest: "<<rem<<endl;
	}

	inline void updateThreadsIdRollback(
			const ID_WORD 					safe_id, 
			typename BASE::SpecThread** 	Spec_Threads, 
			const int 						NUM_MASTERS
	) {
		int j = 0;
		unsigned int highest = this->getMe()->highestNrThread;
		unsigned int base    = highest-nr_of_liv_th;
		for(int i=0; i<NUM_MASTERS; i++) {
			typename BASE::SpecThread* th = Spec_Threads[i];
			if(!th->isEnded()) {
				th->setID(base+j);
				j+=BASE::nr_threads_per_it;
			}
		}
	}
/*
	ID_WORD whichBigIteration(const ID_WORD roll) {	
		cout<<"Go back to: "<<roll - nr_of_liv_th<<endl;
		return roll - nr_of_liv_th;
	}
*/
};


/*
template<class ATTR, const int NUM_MASTERS, const int NUM_SLAVES>
is_valid_exec_model {
	static const bool value = false;
};

template<const int UNROLL, const int NUM_MASTERS, const int NUM_SLAVES>
is_valid_exec_model<InterleavedChunks<UNROLL, NUM_SLAVES> > {
	static const bool value = true;
};SLAVES_TH_NUM



template<const int x>
inline ID_WORD __attribute__((always_inline)) multiply_x(const ID_WORD r) {
	return x*r;
}
template<>
inline ID_WORD __attribute__((always_inline)) multiply_x<1>(const ID_WORD r) {
	return r;
}
template<>
inline ID_WORD __attribute__((always_inline)) multiply_x<2>(const ID_WORD r) {
	return r<<1;
}
template<>
inline ID_WORD __attribute__((always_inline)) multiply_x<4>(const ID_WORD r) {
	return r<<2;
}
*/

/*

public:

	// the acquireResources function can now be moved in the Thread_Manager ...

	template<class IDandBuffTH>
	inline void __attribute__((always_inline)) acquireResources(IDandBuffTH* th) { 
		typename SELF::MemoryModel* memModel = SELF::getMemoryModel();
		const ID_WORD old_id = th->getID();

		const ID_WORD new_id = getMe()->setNextBigIterId(th, old_id);

		// while no rollbacks -- check if it safe to acquire the buffer entry 
		while(!this->getMe()->shouldRollback(new_id, th)) {
			if(this->getMe()->highestNrThread == new_id) {
				// then also lowestNrThread == old_id 
				try { memModel->commit_at_iteration_end(th, old_id); }
				catch(Dependence_Violation v) { 
					cout<<"Rollback: "<<v.roll_id<<" th: "<<old_id<<endl;
					this->getMe()->rollbackSTs(v.roll_id, old_id, th); break; 
				}
					
				memModel->demandResource(th, new_id);
				th->execSynchCode();
					
				this->getMe()->highestNrThread = getMe()->computeNextSynchFrameId(new_id);
				return; //new_id;
			}
			pthread_yield ();
		}

		new_id_continuation(th);
	}

  private:

		
		 // In case of a rollback go execute 
		 // the thread still needs to acquire resources,
		 // and execute the sequential part of code in
		 // mutual exclusion!
		 //
	template<class IDandBuffTH>
	void new_id_continuation(IDandBuffTH* th) {
		typename SELF::MemoryModel* memModel = SELF::getMemoryModel();
		while(true) {
			const ID_WORD new_id = getMe()->setNextBigIterId( th, th->getID() );
			
			while(!this->getMe()->shouldRollback(new_id, th)) {
				if(this->getMe()->highestNrThread == new_id) {			
					memModel->demandResource(th, new_id);
					th->execSynchCode();
					this->getMe()->highestNrThread = getMe()->computeNextSynchFrameId(new_id);
					return;
				}
				pthread_yield();	
			}
		}
	}

*/
