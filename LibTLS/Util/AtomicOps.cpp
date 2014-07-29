#include "Util/AtomicOps.h"

template<class T>
void BuffIP_Resource<T>::resize() {
  unsigned long long  buffer_size = this->end - this->buffer;

  SavedTupleIP<T>* arrnew = new SavedTupleIP<T>[2*buffer_size];
  for(unsigned long long i=0; i<buffer_size; i++) {
    arrnew[i] = buffer[i];
  }
  delete[] this->buffer;
  this->buffer = arrnew;
}

template <class TH>
inline TH* allocateThread(const unsigned long id = 0, const int padding = 32) {
	unsigned long* dummy = new unsigned long[padding];
	TH* th = new TH(id, dummy);
	return th;
}



