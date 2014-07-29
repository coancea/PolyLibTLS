
/****************************************************************/
/**********************   BASE_MANAGER FCTS *********************/
/****************************************************************/

template<
    class SELF, class MemModel, MemModel* memModel, 
    const int NM, const int NS
> 
void Base_Manager<SELF,MemModel,memModel,NM,NS>::commitInFront(const ID_WORD safe_thread) {
  ID_WORD max_thread = highestNrThread;
  memModel->recoverFromRollback(safe_thread, max_thread/*, NUM_MASTERS, Spec_Threads*/);
  //getMe()->cleanUpThreads();
}


template<
    class SELF, class MemModel, MemModel* memModel, 
    const int NM, const int NS
> 
template<class TH>
void Base_Manager<SELF,MemModel,memModel,NM,NS>::execute_sequential(const ID_WORD sf_id, const int nr_its, TH* th) {
  ID_WORD prev_id = th->getID();

  int seq_id = getMe()->getRollNextSeqId(sf_id);		
  th->setID(seq_id);
  th->updateInductionVars();
		
  int bound = getMe()->seq_mult() * nr_its;
  for(int i=0; i<bound && !th->isEnded(); i++) {
    th->executeNonSpecIteration();
  }

  th->setID(prev_id);
}


template<
    class SELF, class MemModel, MemModel* memModel, 
    const int NM, const int NS
> 
const int Base_Manager<SELF,MemModel,memModel,NM,NS>::Roll_Seq_Its = 1;


template<
    class SELF, class MemModel, MemModel* memModel, 
    const int NM, const int NS
> 
template<class TH>
void Base_Manager<SELF,MemModel,memModel,NM,NS>::resetThAfterRoll(TH* t) {
  for(int i=0; i<NM; i++) {
    SpecThread* th = this->Spec_Threads[i];
    MemModel::rollbackCleanUp(static_cast<TH*>(th));
  }
}




template<
    class SELF, class MemModel, MemModel* memModel, 
    const int NM, const int NS
> 
template<class TH>
int Base_Manager<SELF,MemModel,memModel,NM,NS>::treatRollback(TH* th) {
  //cout<<"ROLLBACK!!!!!!!!!! roll_id: "<<this->safe_id<<endl;

  computeHighestNrThread();	

  //cout<<"before commit in front"<<endl;

  //reinitialize the environment
  commitInFront  (this->safe_id);

  //cout<<"after commit in front"<<endl;

  memModel->reset(this->safe_id, highestNrThread);

  //cout<<"before exec seq"<<endl;

  /** execute a chunk of execution sequentially **/
  execute_sequential(this->safe_id+1, Roll_Seq_Its, th);  

  //cout<<"after exec seq"<<endl;

  this->getMe()->updateThreadsIdRollback(this->safe_id+1, Spec_Threads, Roll_Seq_Its); 
  this->safe_id      	= NO_ROLLBACK;	

  resetThAfterRoll(th);

  //the rollback has been performed, wake up the threads
  getMe()->respawnThreads();
        
  return 1;
}



template<
    class SELF, class MemModel, MemModel* memModel, 
    const int NM, const int NS
> 
void Base_Manager<SELF,MemModel,memModel,NM,NS>::updateRollbackID(const ID_WORD id) 
{
  non_blocking_synch(&synchCAS);
  if(safe_id==NO_ROLLBACK || safe_id>id) safe_id = id;
  release_synch(&synchCAS); //synchCAS = 0;
}


template<
    class SELF, class MemModel, MemModel* memModel, 
    const int NM, const int NS
> 
template<class TH>
void Base_Manager<SELF,MemModel,memModel,NM,NS>::speculate() {

  for(int i=0; i<NM; i++) {
      TH* th = (TH*)Spec_Threads[i];
      if(th) th->startSpeculativeThread();
  }

  for(int i=0; i<NM; i++) {
    TH* th = (TH*)Spec_Threads[i];
    if(th) pthread_join( *th->getPThread(), NULL );
  }
}



template<
    class SELF, class MemModel, MemModel* memModel, 
    const int NM, const int NS
> 
void Base_Manager<SELF,MemModel,memModel,NM,NS>::registerSpecThread(SpecThread* th, uint pos) {
  if(thCount >= NM) thCount = 0;

  th->setTM(this->getMe());
  //th->setMask(pos);
  th->id_orig = thCount;
  th->setID(this->getMe()->firstID(thCount));
  Spec_Threads[thCount] = th;
  thCount++;
}



template<
    class SELF, class MemModel, MemModel* memModel, 
    const int NM, const int NS
> 
template<class TH>
void Base_Manager<SELF,MemModel,memModel,NM,NS>::registerSpecThreads() {
  thCount = 0;
  while(thCount<NM) {
    TH* th = allocateThread<TH>();
    th->setTM(this->getMe());
    //th->setMask(thCount);
    th->id_orig = thCount;
    th->setID(this->getMe()->firstID(thCount));
    Spec_Threads[thCount] = th;
    thCount ++;
  }
}



/******************************************************************/
/**********************   THREAD_MANAGER FCTS *********************/
/******************************************************************/

template<
		class SELF, class MemModel, MemModel* memModel, 
		const int NUM_MASTERS, const int NUM_SLAVES, class EXEC_TYPE_ATTR
> 
template<class TH>
void Master_AbstractTM<SELF,MemModel,memModel,NUM_MASTERS,NUM_SLAVES,EXEC_TYPE_ATTR>::
cleanUpAll() { 
    this->init();
    for(int i=0; i<NUM_MASTERS; i++) {
        TH* th = (TH*)this->Spec_Threads[i];
        if(th) MemModel::rollbackCleanUp(th);
        th->init(); //th->setToRunning();
        th->id_orig = i;
        th->setID(this->firstID(i));
    }
    this->thCount = NUM_MASTERS;
}


template<
		class SELF, class MemModel, MemModel* memModel, 
		const int NUM_MASTERS, const int NUM_SLAVES, class EXEC_TYPE_ATTR
> 
int Master_AbstractTM<SELF,MemModel,memModel,NUM_MASTERS,NUM_SLAVES,EXEC_TYPE_ATTR>::
updateRollbackID(const ID_WORD rollid, const ID_WORD thid) {
  int changed = 0;
  non_blocking_synch(&this->synchCAS);
  unsigned long barrier_id = this->getRollbackId();
  if(barrier_id==NO_ROLLBACK || barrier_id>rollid) {
    this->setRollbackId(rollid);
    thread_barrier_id = thid;
    changed = 1;
  }
  release_synch(& this->synchCAS); //this->synchCAS = 0;
  return changed;
}



template<
		class SELF, class MemModel, MemModel* memModel, 
		const int NUM_MASTERS, const int NUM_SLAVES, class EXEC_TYPE_ATTR
> 
int Master_AbstractTM<SELF,MemModel,memModel,NUM_MASTERS,NUM_SLAVES,EXEC_TYPE_ATTR>::
set_barrier(const int roll_id, const int thread_id) {
    /**
	if there is another thread with a lower id already 
	  trying to perform a rollback exit the function and wait
	else set the safe_id
    **/ 
    if(!updateRollbackID(roll_id, thread_id)) {
        return 0; 
    }
					
    //cout<<"set_barrier: "<<thread_id<<" roll id: "<<roll_id<<endl;
	
    /** wait for all the other running threads to reach the barrier **/
    int goOn = 1;
    while(goOn) {
        int all = NUM_MASTERS - nr_of_waiting_threads - nr_of_dead_threads;
        if(all <= 1) {
            //cout<<"Service rollback: waiting: "<<nr_of_waiting_threads<<
            //	" dead: "<<nr_of_dead_threads<<" MASTERS: "<<NUM_MASTERS<<endl;
            goOn = 0;
        }

        /**
            if another thread with a lower id is trying to do the 
                rollback then exit the function and wait
            else yield the processor to another thread so that it 
                can reach the barrier
            There might be two threads with the same barrier_id 
                hence the check for thread_barrier_id
        **/ 
        unsigned long barrier_id = this->getRollbackId();
        if(barrier_id!=roll_id || thread_id!=thread_barrier_id) 
            { return 0; }
			
    }
    return 1;
}


template<
		class SELF, class MemModel, MemModel* memModel, 
		const int NUM_MASTERS, const int NUM_SLAVES, class EXEC_TYPE_ATTR
> 
const int Master_AbstractTM<SELF,MemModel,memModel,NUM_MASTERS,NUM_SLAVES,EXEC_TYPE_ATTR>::SLAVES_PER_MASTER = NUM_SLAVES;



template<
		class SELF, class MemModel, MemModel* memModel, 
		const int NUM_MASTERS, const int NUM_SLAVES, class EXEC_TYPE_ATTR
> 
template<class TH>
void Master_AbstractTM<SELF,MemModel,memModel,NUM_MASTERS,NUM_SLAVES,EXEC_TYPE_ATTR>::
ended_roll(TH* th) {
    nr_of_dead_threads = NUM_MASTERS; 
    for(int i=0; i<NUM_MASTERS; i++) {
        SpecThread* thd = this->Spec_Threads[i];
        if(thd!=th) thd->kill();
    }	
    respawnThreads();
    pthread_exit(PTHREAD_CANCELED);
}



template<
		class SELF, class MemModel, MemModel* memModel, 
		const int NUM_MASTERS, const int NUM_SLAVES, class EXEC_TYPE_ATTR
> 
template<class TH>
void Master_AbstractTM<SELF,MemModel,memModel,NUM_MASTERS,NUM_SLAVES,EXEC_TYPE_ATTR>::
goWait(TH* th) {
    if(HARD_WAIT){
        hardWait(this, nr_of_waiting_threads++);
    } else {
        non_blocking_synch(&this->synchCAS); //pthread_mutex_lock( &this->mutex );
        nr_of_waiting_threads++;
        release_synch(& this->synchCAS); //this->synchCAS = 0;//pthread_mutex_unlock( &this->mutex );
        th->goWait();
    }
}



template<
		class SELF, class MemModel, MemModel* memModel, 
		const int NUM_MASTERS, const int NUM_SLAVES, class EXEC_TYPE_ATTR
> 
void Master_AbstractTM<SELF,MemModel,memModel,NUM_MASTERS,NUM_SLAVES,EXEC_TYPE_ATTR>::
respawnThreads() {
    if(HARD_WAIT) {
        nr_of_waiting_threads = 0;			
        pthread_mutex_lock( &this->mutex );
        pthread_cond_broadcast( &this->cond_mutex );
        pthread_mutex_unlock( &this->mutex );
    } else {
        nr_of_waiting_threads = 0;
        for(int i=0; i<NUM_MASTERS; i++) {
            SpecThread* th = this->Spec_Threads[i];
            if(th->isWaiting()) th->setToRunning();
        }	
    }
}



template<
		class SELF, class MemModel, MemModel* memModel, 
		const int NUM_MASTERS, const int NUM_SLAVES, class EXEC_TYPE_ATTR
> 
template<class TH>
int Master_AbstractTM<SELF,MemModel,memModel,NUM_MASTERS,NUM_SLAVES,EXEC_TYPE_ATTR>::
rollbackSTs(const int rollback_id, const int thread_id, TH* th) { 
    //wait for all threads to reach the barrier
    if(!set_barrier(rollback_id, thread_id)) {
        return this->shouldRollback(thread_id, th);
    }

    nr_of_rollbacks++;
    //cout<<"Start rollback! "<<rollback_id<<" "<<thread_id<<endl;
    this->treatRollback(th);
    //cout<<"Rollback ended!"<<endl;
    return 1;
}


/************************************************************************/
/******************** ITERATION_SPACE_ATTRIBUTE *************************/
/************************************************************************/

template<class SELF, const ID_WORD UNROLL> 
template<class IDandBuffTH>
/*inline*/ void /*__attribute__((always_inline))*/ 
BaseChunks<SELF,UNROLL>::acquireResources(IDandBuffTH* th) { 

    typename SELF::MemoryModel* memModel = SELF::getMemoryModel();

    while(true) {
        const ID_WORD old_id = th->getID();
        const ID_WORD new_id = getMe()->setNextBigIterId(th, old_id);

        //cout<<"Master: "<<this->master_id<<endl;

        if( getMe()->shouldRollback(old_id, th) ) continue;

        //cout<<"old_id: "<<old_id<<" master: "<<master_id<<endl;
        if(isMasterId(old_id)) {         // getMe()->highestNrThread == new_id;
            bool was_roll = false;

            //cout<<"Before receiving new id, old id: "<<old_id<<" new_id: "<<new_id<<endl;

            try { memModel->commit_at_iteration_end(th); }
            catch(Dependence_Violation v) { 
                was_roll = true;
                //cout<<"Rollback in commit at ITERATION END!!!!!!: "<<v.roll_id<<" th: "<<old_id<<endl;
                this->getMe()->rollbackSTs(old_id, old_id, th); //continue; //break; 
                //cout<<"After rollback in commit at ITERATION END!!!!"<<endl;
            }
            if(was_roll) continue;

            th->setID(new_id);   // was part of setNextBigIterId 
            memModel->acquireResources(th, new_id);

            try {   th->execSynchCode();   }
            catch(Dependence_Violation v) { 
                was_roll = true;
                //cout<<"Rollback in execSynchCode!!!!!!: "<<v.roll_id<<" th: "<<old_id<<endl;
                getMe()->computeNextMaster();
                this->getMe()->rollbackSTs(v.roll_id, new_id, th); //continue; //break; 
            }
                
            if(was_roll) continue;

            getMe()->computeNextMaster();   //computeNextSynchFrameId
				
            return; 
        }

        /** if end of speculation, return here **/
        if(old_id > this->id_ended) { pthread_exit(NULL); } 
        //cout<<"In new id: "<<old_id<<" "<<new_id<<" master: "<<master_id<<endl;
        pthread_yield ();
    }		
}


template<class SELF, const ID_WORD UNROLL> 
template<class TH>
void BaseChunks<SELF,UNROLL>::commit_ended(TH* th) { 

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
            //    getMe()->rollbackSTs(id, id, th);	
            return;
        } 
    }		
}

template<class SELF, const ID_WORD UNROLL>
const int	BaseChunks<SELF,UNROLL>::nr_threads_per_it = SELF::SLAVES_PER_MASTER+1;


/************************************************************************/
/******************** INTERLEAVED ATTRIBUTE *****************************/
/************************************************************************/




/*
	inline void BaseManager::updateThreadsIdRollback() {

		//ID_WORD rem = highestNrThread % NUM_LIVE_MASTERS;
		//ID_WORD hh  = highestNrThread - rem;
			
		//for(int i=0; i<=rem; i++)
		//	Spec_Threads[i]->setID(hh+i);

		//int count = 1;
		//for(int j=rem+1; j<NUM_LIVE_MASTERS; j++) {
		//	Spec_Threads[j]->setID(highestNrThread-NUM_LIVE_MASTERS+count);
		//	count ++;
		//}


		int count = 0;
		for(int i=0; i<NUM_MASTERS; i++) {
			SpecThread* th = Spec_Threads[i];
			if(!th->isEnded()) {
				th->setID(this->highestNrThread-count);
				count++;
			}
		}
	}
*/

