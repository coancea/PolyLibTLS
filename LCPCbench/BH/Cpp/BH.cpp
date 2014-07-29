using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include<time.h>
#include<iostream>
#include<signal.h>
#include<sys/time.h>

class Thread_BH_HG_TLS;

#ifndef MATH_VECT_H
#define MATH_VECT_H
#include "MathVector.h"
#endif

#ifndef CELL_H
#define CELL_H
#include "Cell.h"
#endif

#ifndef BODY_H
#define BODY_H
#include "Body.h"
#endif

#ifndef TREE_H
#define TREE_H
#include "Tree.h"
#endif

#ifndef NODE_H
#define NODE_H
#include "Node.h"
#endif


#include "BH_ParTLS.h"

#ifndef ALL_METHODS_H
#define ALL_METHODS_H
#include "AllMethods.h"
#endif

//import java.util.Enumeration;
//import java.lang.Math;

/**
 * A Cpp implementation of the <tt>bh</tt> Olden benchmark.
 * The Olden benchmark implements the Barnes-Hut benchmark
 * that is decribed in :
 * <p><cite>
 * J. Barnes and P. Hut, "A hierarchical o(NlogN) force-calculation algorithm",
 * Nature, 324:446-449, Dec. 1986
 * </cite>
 **/

  /**
   * The user specified number of bodies to create.
   **/
  int nbody = 0;

  /**
   * The maximum number of time steps to take in the simulation
   **/
  int nsteps = 10;

  /**
   * Should we print information messsages
   **/
  bool printMsgs = false;
  /**
   * Should we print detailed results
   **/
  bool printResults = false;



  /**
   * The usage routine which describes the program options.
   **/
  void usage()
  {
    cout<<"usage: BH -b <size> [-s <steps>] [-p] [-m] [-h]"<<endl;
    cout<<"    -b the number of bodies"<<endl;
    cout<<"    -e type of exec: 1==sequential, 2==HAND PARALLEL, 3==Thread-Level Speculation (TLS)"<<endl;
    cout<<"    -s the max. number of time steps (default=10)"<<endl;
    cout<<"    -p (print detailed results)"<<endl;
    cout<<"    -m (print information messages"<<endl;
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
    int i = 1; // or 0?
    char* arg;

    while (i < argc && args[i][0] == '-') {
      arg = args[i++];

      // check for options that require arguments
      if (arg[1] == 'b') {
	    if (i < argc) {
	      nbody = atoi(args[i++]);
	    } else {
	      cout<<"-b requires the number of bodies!!!"<<endl;
          exit(0);
	    }
      } else if (arg[1] == 's') {
	    if (i < argc) {
	      nsteps = atoi(args[i++]);
	    } else {
	      cout<<"-l requires the number of levels"<<endl;
          exit(0);
	    }
      } else if (arg[1] == 'm') {
	    printMsgs  = true;
      } else if (arg[1] == 'p') {
	    printResults = true;
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
    if (nbody == 0) usage();
  }



  timeval start_time, end_time;
  unsigned long running_time = 0;


  int main(int argc, char** args)
  {
    unsigned int time_init, time_alg;
    parseCmdLine(argc, args);

    if (printMsgs)
      cout<<"nbody = "<<nbody<<endl;

    gettimeofday ( &start_time, 0 );                  //// MODIFY!!!
    Tree* root = new Tree();
    root->createTestData(nbody);
    gettimeofday( &end_time, 0 );

    time_init = DiffTimeClone(&end_time,&start_time);
    if (printMsgs)
      cout<<"Bodies created."<<endl;

    gettimeofday ( &start_time, 0 );  
    double tnow = 0.0;
    int i = 0;
    //while ((tnow < TSTOP + 0.1*DTIME) && (i < nsteps)) {

    {
      if(exec_mode == TLS_M) 
        root->stepSystemTLS(i++);
      else if(exec_mode == PAR_M)
        root->stepSystemPAR(i++);
      else if(exec_mode = SEQ_M)
        root->stepSystem(i++);
    }

      tnow += DTIME;
    //}
    gettimeofday( &end_time, 0 );

    time_alg = DiffTimeClone(&end_time,&start_time);

    if (printResults) {
      int j = 0;
      for (Enumerate e = root->bodies(); e.hasMoreElements(); ) {    //// MODIFY!!!
        Body* b = e.nextElement();
        cout<<"body "<<j++<<" -- "; b->pos.toPrint(); cout<<endl;
      }
    }

    if (printMsgs) {
      cout<<"Build Time "<<time_init<<endl;
      cout<<"Compute Time "<<time_alg<<endl;
      cout<<"Total Time "<<time_init+time_alg<<endl;
    }
    cout<<"Done!"<<endl;
  }
