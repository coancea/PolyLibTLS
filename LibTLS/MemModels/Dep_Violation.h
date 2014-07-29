#ifndef DEPENDENCE_VIOLATION
#define DEPENDENCE_VIOLATION

using namespace std;

class Dependence_Violation {
public:
	static const int LOAD_TP  = 1;
	static const int STORE_TP = 2;
	static const int SYNCH_TP = 3;
	static const int PREDICTOR_TP = 4;

	unsigned long roll_id;
	unsigned long thread_id;
	unsigned long load_entry;

	/* 1 -> load; 2 -> store; 3 -> synch */ 
	int type;

	inline Dependence_Violation(unsigned long id) { 
		roll_id = id; 
		thread_id = 0; 
		type = 0;  
	}
	
	inline Dependence_Violation(unsigned long id, unsigned long th) { 
		roll_id = id; 
		thread_id = th; 
		type = 0;  
	}


	inline Dependence_Violation(unsigned long id, unsigned long th, unsigned long tp) { 
		roll_id = id; 
		thread_id = th; 
		type = tp; 
	}

	inline Dependence_Violation(unsigned long id, unsigned long th, int tp, unsigned long ld) { 
		roll_id = id; 
		thread_id = th; 
		type = tp; 
		load_entry = ld; 
	}
};

#endif // define DEPENDENCE_VIOLATION

