#ifndef SPEC_THREAD_NOTINLINED
#define SPEC_THREAD_NOTINLINED

template<class TM>
void Speculative_Thread<TM>::kill() { 
	int succ = 1; 
	do{
		succ = pthread_cancel(pthread);
	} while (succ!=0);	
}

template<class TM>
void Speculative_Thread<TM>::startSpeculativeThread() {
	Spec_Thread0::setUSUALthread(&this->thread_attr);
	int success = pthread_create( &pthread, &thread_attr, &spec_fct, this );			 
}

template<class TM>
void Speculative_Thread<TM>::goWait() {
	this->setToWaiting();
	while(this->isWaiting()) { 
		if(HARD_WAIT) pthread_testcancel();
		pthread_yield();
	}
}

template<class TM>
void* Speculative_Thread<TM>::spec_fct(void* v_th) {
	Speculative_Thread<TM>* th = (Speculative_Thread<TM>*)v_th;
	th->run();
}

/************************************************************************/
/********** Methods belonging to Buffarable Thread Components ***********/
/************************************************************************/

//const int WAR_SIZE = 2048*4;
void ItInd_SV::initWARvct(int sz) {
	len = pow2(sz*32)-1;    //*32
	buffer2 = new ID_WORD[len+1];
	for(unsigned int i=0; i<=len; i++)
		buffer2[i] = ID_WORD_MAX;
}


template<
	class T, class M, M* m, 
	class ATTR_INDDEP, 
	class ATTR_CACHABLE
> 
void BuffSCM<T, M, m, ATTR_INDDEP, ATTR_CACHABLE>::init(const ID_WORD buffer_size) {
	buffer      = new SavedTupleSC<T>[buffer_size]; 
	pos 	    = &buffer[0];
	end         = &buffer[buffer_size]; 
	this->ATTR_INDDEP::initWARvct(buffer_size);
	//hash 		= m.getHashObject();
} 

template<
	class T, class M, M* m, 
	class ATTR_INDDEP, 
	class ATTR_CACHABLE
> 
void BuffSCM<T, M, m, ATTR_INDDEP, ATTR_CACHABLE>::resize() {
  unsigned long long  buffer_size = this->end - this->buffer;

  SavedTupleSC<T>* arrnew = new SavedTupleSC<T>[2*buffer_size];
  for(unsigned long i=0; i<buffer_size; i++) {
    arrnew[i] = buffer[i];
  }
  delete[] buffer;
  buffer = arrnew;
}


template<
	class T, class M, M* m, 
	class ATTR_INDDEP, 
	class ATTR_CACHABLE
> 
T BuffSCM<T, M, m, ATTR_INDDEP, ATTR_CACHABLE>::getLoopIndWrite(volatile T* address) {
	SavedTupleSC<T>* beg = pos-1;
	while(buffer<=beg) {
		//if( ((unsigned long)address) == beg->addr ) return beg->val;
		if( address == beg->addr ) return beg->val;
		beg--;
	}
	return *((T*)address);
}


#endif // define SPEC_THREAD_NOTINLINED
