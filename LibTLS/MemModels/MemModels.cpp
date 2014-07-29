
using namespace std;

template< class HashFct, SpMod_SCcore<HashFct>* LSmem, class T > 
ID_WORD SpMod_SC<HashFct, LSmem, T>::checkBoundsSlow(T* addr, const ID_WORD th_num) {
      ID_WORD diff = (ID_WORD)(addr-(T*)this->addr_beg);
      if(diff<=this->A_m_a) return LSmem->getIndex( diff );
      else return ID_WORD_MAX;
}


template< class HashFct, SpMod_SCcore<HashFct>* LSmem, class T > 
ID_WORD SpMod_SCbw<HashFct, LSmem, T>::checkBoundsSlow(T* addr, const ID_WORD th_num) {
      ID_WORD diff = (ID_WORD)((T*)this->addr_end - addr);
      if(diff<=this->A_m_a) return LSmem->getIndex( diff );
      else return ID_WORD_MAX;
}



/***************************************************************************/


template< class HashFct, SpMod_IPcore<HashFct>* LSmem, class T > 
void SpMod_IP0<HashFct, LSmem, T>::recoverFromRollback  ( 
            const ID_WORD safe_thread, const ID_WORD highest_id_thread ) {
  
  {  /** clean the address vector -- hopefully this does not recaps it **/
    address_vect.clear();
  }

  /** sort based on the thread number **/
  sort(aliased_buff, aliased_buff+SZ, resPtr_less_then<T>() );

  /** fill the address vector **/
  for(int i=0; i<SZ; i++) {
    BuffIP_Resource<T>* rec = aliased_buff[i];
    if( rec->id > safe_thread ) {
      SavedTupleIP<T>* cur  = &rec->buffer[0];
      SavedTupleIP<T>* pos  = rec->pos;
      while(cur<pos) { address_vect.push_back(cur++); }
    }
  }

  /** sort the address vector **/
  std::sort(address_vect.begin(), address_vect.end(), tuple_less_then<T>() );


  /** rollback computation **/
  { 
    typedef typename  std::vector< SavedTupleIP<T>* >::iterator my_iterator;
    my_iterator beg = address_vect.begin(), cur = beg, end = address_vect.end();

    while(cur < end) {
      T* addr = (*cur)->addr;
      *addr   = (*cur)->val;
      cur++;
      while(cur < end && (*cur)->addr == addr) cur++;
    }
/*
    unsigned long vect_sz = address_vect.size(); 
    for(int i=0; i<vect_sz; ) {
      SavedTupleIP<T>* tup = address_vect[i];
      *(tup->addr) = tup->val; i++;
      while( i<vect_sz && tup->addr == address_vect[i]->addr ) i++;
    }
*/
  }
}


/***************************************************************************************************/


template< class HashFct, SpMod_IPcore<HashFct>* LSmem, class T > 
ID_WORD SpMod_IP<HashFct, LSmem, T>::checkBoundsSlow(T* addr, const ID_WORD th_num) {
      ID_WORD diff = (ID_WORD)(addr-(T*)this->addr_beg);
      if(diff<=this->A_m_a) return LSmem->getIndex( diff );
      else return ID_WORD_MAX;
}
    
template< class HashFct, SpMod_IPcore<HashFct>* LSmem, class T > 
void SpMod_IP<HashFct, LSmem, T>::printIndexForAddr(T* addr) {
      ID_WORD diff = (ID_WORD)(addr-(T*)this->addr_beg);
      cout<<"Addr: "<<(void*)addr<<" index: "<<LSmem->getIndex( diff )<<endl;
}

/***************************************************************************************************/

template< class HashFct, SpMod_IPcore<HashFct>* LSmem, class T > 
ID_WORD SpMod_IPbw<HashFct, LSmem, T>::checkBoundsSlow(T* addr, const ID_WORD th_num) {
      ID_WORD diff = (ID_WORD)((T*)this->addr_end - addr);
      if(diff<=this->A_m_a) return LSmem->getIndex( diff );
      else return ID_WORD_MAX;
}

 
