using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include<time.h>
#include<iostream>
#include<signal.h>
#include<sys/time.h>

#ifndef THREAD_MANAGERS
#define THREAD_MANAGERS
#include "Managers/Thread_Manager.h"
#endif  

#include "BiGraph.h"
#include "EM3D_TLS.h"
#include "TLSmethods.h"

//   g++ -O3 -Wno-deprecated -pthread -I../../../../PolyLibTLSniagra/ -o em3d.exe em3d.cpp
//   ./em3d.exe -n 2097152 -d 32 -m -e 3


/** 
 * Java implementation of the <tt>em3d</tt> Olden benchmark.  This Olden
 * benchmark models the propagation of electromagnetic waves through
 * objects in 3 dimensions. It is a simple computation on an irregular
 * bipartite graph containing nodes representing electric and magnetic
 * field values.
 *
 * <p><cite>
 * D. Culler, A. Dusseau, S. Goldstein, A. Krishnamurthy, S. Lumetta, T. von 
 * Eicken and K. Yelick. "Parallel Programming in Split-C".  Supercomputing
 * 1993, pages 262-273.
 * </cite>
 **/

  /**
   * The number of nodes (E and H) 
   **/
  static int numNodes = 0;
  /**
   * The out-degree of each node.
   **/
  static int numDegree = 0;
  /**
   * The number of compute iterations 
   **/
  static int numIter = 1;
  /**
   * Should we print the results and other runtime messages
   **/
  static bool printResult = false;
  /**
   * Print information messages?
   **/
  static bool printMsgs = false;


  /**
   * The usage routine which describes the program options.
   **/
  void usage()
  {
    cout<<"usage: java Em3d -n <nodes> -d <degree> [-p] [-m] [-h] [-e]"<<endl;
    cout<<"    -n the number of nodes"<<endl;
    cout<<"    -d the out-degree of each node"<<endl;
    cout<<"    -i the number of iterations"<<endl;
    cout<<"    -p (print detailed results)"<<endl;
    cout<<"    -e type of exec: 1==sequential, 2==HAND PARALLEL, 3==Thread-Level Speculation (TLS)"<<endl;
    cout<<"    -m (print informative messages)"<<endl;
    cout<<"    -h (this message)"<<endl;
    exit(0);
  }

  const int SEQ_M = 1;
  const int PAR_M = 2;
  const int TLS_M = 3;
  int exec_mode = 1;

  /**
   * Parse the command line options.
   * @param args the command line options.
   **/
  void parseCmdLine(int argc, char** args)
  {
    int i = 1;
    char* arg;

	while (i < argc && args[i][0] == '-') {
      arg = args[i++];
      
      // check for options that require arguments
      if (arg[1] == 'n') {
	    if (i < argc) {
	      numNodes = atoi(args[i++]);
	    } else {
	      cout<<"-n requires the number of nodes!!!"<<endl;
          exit(0);
	    }
      } else if (arg[1] == 'd') {
	    if (i < argc) {
	      numDegree = atoi(args[i++]);
	    } else {
	      cout<<"-d requires the out degree!!!"<<endl;
          exit(0);
	    }
      } else if (arg[1] == 'i') {
	    if (i < argc) {
	      numIter = atoi(args[i++]);
	    } else {
	      cout<<"-i requires the number of iterations!!!"<<endl;
          exit(0);
	    }
      } else if (arg[1] == 'm') {
	    printMsgs  = true;
      } else if (arg[1] == 'p') {
	    printResult = true;
      } else if (arg[1] == 'h') {
	    usage();
      } else if (arg[1] == 'e') {
        /** 
         * specify the execution model: 
         * sequential, static parallel, TLS parallel
         */
        bool valid = true;
        if (i < argc) {
	      exec_mode = atoi(args[i++]);
          if(exec_mode>3 && exec_mode<1) valid = false;
	    } else {
          valid = false;
        }
        if(!valid) {
	      cout<<"-e requires execution mode: \n 1 for sequential \n 2 for static parallel, \n 3 for TLS parallel"<<endl;
          exit(0);
	    }
      }
    }
    if (numNodes == 0 || numDegree == 0) usage();
  }


  void runAlgCore(BiGraph* graph) {
    if(exec_mode == TLS_M) { 

      for (int i = 0; i < numIter; i++) {
        graph->compute_TLS();
      }

    } else if(exec_mode == PAR_M) {

      for (int i = 0; i < numIter; i++) {
        graph->compute_TLS();
      }

    } else if(exec_mode == SEQ_M) {

      for (int i = 0; i < numIter; i++) {
        graph->compute();
      }

    }
  }


  timeval start_time, end_time;
  unsigned long running_time = 0;

  /**
   * The main roitine that creates the irregular, linked data structure
   * that represents the electric and magnetic fields and propagates the
   * waves through the graph.
   * @param args the command line arguments
   **/
  int main(int argc, char** args)
  {
    unsigned int time_init, time_alg;

    parseCmdLine(argc, args);

    if (printMsgs) 
      cout<<"Initializing em3d random graph..."<<endl;

    gettimeofday ( &start_time, 0 );                 
    BiGraph* graph = BiGraph::create(numNodes, numDegree, printResult);
    gettimeofday( &end_time, 0 );

    time_init = DiffTime(&end_time,&start_time);

    // compute a single iteration of electro-magnetic propagation
    if (printMsgs) 
      cout<<"Propagating field values for "<<numIter<<" iteration(s)..."<<endl;

    gettimeofday ( &start_time, 0 ); 
    runAlgCore(graph);
    gettimeofday( &end_time, 0 );

	time_alg = DiffTime(&end_time,&start_time);

    // print current field values
    if (printResult)
      graph->toPrint();

    if (printMsgs) {
      cout<<"EM3D build time "<<time_init<<endl;
      cout<<"EM3D compute time "<<time_alg<<endl;
      cout<<"EM3D total time "<<(time_init + time_alg)<<endl;
    }
    cout<<"Done!"<<endl;
  }






