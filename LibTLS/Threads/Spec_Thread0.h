#ifndef SPEC_THREAD_HELP
#define SPEC_THREAD_HELP

using namespace std;

class Spec_Thread0 {
	public:
		volatile ID_WORD   		id_orig;
	protected:
		// the thread's unique identifier
		volatile ID_WORD		id;

		// 0 if not rollback, -1 otherwise
		volatile ID_WORD		rollback;
		
		// 0 - waiting; 1 - running; 2 - ended
		volatile int			state;

		int 				mask;
		int		 		mybarrier;


	public:

		/** getters **/
		inline ID_WORD __attribute__((always_inline)) 
			getID		() const { return id; 		}
		inline int __attribute__((always_inline)) 
			isWaiting	() const { return state == 0;	}
		inline int __attribute__((always_inline)) 
			isEnded		() const { return state == 2; 	}
		inline int __attribute__((always_inline)) 
			isRunning	() const { return state == 1; 	}
		inline int __attribute__((always_inline)) 
			getMask		() const { return mask;  	}	
		inline int __attribute__((always_inline)) 
			getRollback	() const { return rollback;	}	
		
		
		/** setter **/
		inline void __attribute__((always_inline)) 
			setID       (const ID_WORD i)  	{ id = i; 	}
		inline void __attribute__((always_inline)) 
			setToWaiting()			{ state = 0; 	}
		inline void __attribute__((always_inline)) 
			setToEnded	() 		{ state = 2; 	}
		inline void __attribute__((always_inline)) 
			setToRunning()			{ state = 1;	}
		inline void __attribute__((always_inline)) 
			setMask		(const int pow)	{ mask= pow/*1<<pow*/; }	
		inline void __attribute__((always_inline)) 
			setRollback	(const ID_WORD roll){ if(roll>rollback) rollback = roll; }	


		
		/** init **/
		inline void __attribute__((always_inline)) 
		init( const ID_WORD id = ID_WORD_MAX, const int mask = 1 ) {
			this->id = id; 
			this->id_orig = id;
			this->mybarrier = 0; 
			this->state = 1; 
			this->rollback = 0;
			this->mask = mask; 
		}

		virtual void rollbackCleanUp() { }

		inline static void __attribute__((always_inline)) setFIFOthread(
			pthread_attr_t* thread_attr, 
			struct sched_param* thread_param
		) {
			pthread_attr_init( thread_attr );
			pthread_attr_setscope( thread_attr, PTHREAD_SCOPE_SYSTEM );

			pthread_attr_setschedpolicy (thread_attr, SCHED_FIFO);
			int max_priority = sched_get_priority_max (SCHED_FIFO);
			int min_priority = sched_get_priority_min (SCHED_FIFO);
			thread_param->sched_priority = (max_priority+min_priority)/2;
			
			pthread_attr_setschedparam (thread_attr, thread_param);
			pthread_attr_setinheritsched (thread_attr, PTHREAD_EXPLICIT_SCHED);
		} 

		inline static void setUSUALthread(
			pthread_attr_t* thread_attr
		) {
			pthread_attr_init( thread_attr );
			pthread_attr_setscope( thread_attr, PTHREAD_SCOPE_SYSTEM );
		} 
};


#endif // define SPEC_THREAD_HELP
