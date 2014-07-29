
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

#include "Tree.h"
#include "TSP_TLS.h"
#include "AllMethods.h"


//   g++ -O3 -Wno-deprecated -pthread -I../../../../PolyLibTLSniagra/ -o tsp.exe TSP.cpp
//   ./tsp.exe -c 20000 -m -e 0



/**
 * A Java implementation of the <tt>tsp</tt> Olden benchmark, the traveling
 * salesman problem.
 * <p>
 * <cite>
 * R. Karp, "Probabilistic analysis of partitioning algorithms for the 
 * traveling-salesman problem in the plane."  Mathematics of Operations Research 
 * 2(3):209-224, August 1977
 * </cite>
 **/

  /**
   * Number of cities in the problem.
   **/
  int cities;
  /**
   * Set to true if the result should be printed
   **/
  bool printResult = false;
  /**
   * Set to true to print informative messages
   **/
  bool printMsgs = false;


  /**
   * The usage routine which describes the program options.
   **/
  void usage()
  {
    cout<<"usage: java TSP -c <num> [-p] [-m] [-h] [-e]"<<endl;
    cout<<"    -c number of cities (rounds up to the next power of 2 minus 1)"<<endl;
    cout<<"    -p (print the final result)"<<endl;
    cout<<"    -m (print informative messages)"<<endl;
    cout<<"    -e type of exec: 0==sequential original, 1==sequential, 2==HAND PARALLEL, 3==Thread-Level Speculation (TLS)"<<endl;
    cout<<"    -h (this message)"<<endl;
    exit(0);
  }


  const int SEQ_M_ORIG = 0;
  const int SEQ_M      = 1;
  const int PAR_M      = 2;
  const int TLS_M      = 3;

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
      if (arg[1] == 'c') {
	    if (i < argc) {
	      cities = atoi(args[i++]);
	    } else {
	      cout<<"-c requires the size of tree!!!"<<endl;
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
          if(exec_mode>3 && exec_mode<0) valid = false;
	    } else {
          valid = false;
        }
        if(!valid) {
	      cout<<"-e requires execution mode: \n 0 for original sequential \n 1 for sequential with iterator"
              <<"\n 2 for static parallel, \n 3 for TLS parallel"<<endl;
          exit(0);
	    }
      }
    }
    if (cities == 0) usage();
  }


  void runAlgCore(Tree* graph) {
    if(exec_mode == TLS_M || exec_mode == PAR_M) { 
        graph->tsp_TLS(150);
    } else if(exec_mode == SEQ_M) {
        graph->tsp(150);
    } else if(exec_mode == SEQ_M_ORIG) {
        graph->tsp_orig(150);
    }
  }


  timeval start_time, end_time;
  unsigned long running_time = 0;



  /**
   * The main routine which creates a tree and traverses it.
   * @param args the arguments to the program
   **/
  int main(int argc, char** args)
  {
    unsigned int time_init, time_alg;
    srand(7.2523); 

    parseCmdLine(argc, args);

    if (printMsgs) 
      cout<<"Building tree of size "<<cities<<endl;

    gettimeofday ( &start_time, 0 );                 
    Tree*  graph = Tree::buildTree(cities, false, 0.0, 1.0, 0.0, 1.0);
    gettimeofday( &end_time, 0 );

    time_init = DiffTime(&end_time,&start_time);
    
    gettimeofday ( &start_time, 0 ); 
    runAlgCore(graph);
    gettimeofday( &end_time, 0 );

	time_alg = DiffTime(&end_time,&start_time);

    if (printResult)
      graph->printVisitOrder();

    if (printMsgs) {
      cout<<"Tsp build time "<<time_init<<endl;
      cout<<"Tsp time "<<time_alg<<endl;
      cout<<"Tsp total time "<<(time_init + time_alg)<<endl;
    }
    cout<<"Done!"<<endl;
  }


/*

Tree* Tree::conquer_TLS(SpTh* th) 
{
    // create the list of nodes
    Tree* t = makeList();
 
    // Create initial cycle 
    Tree* cycle = t;
    t = t->next;
    cycle->next = cycle;
    cycle->prev = cycle;

    // Loop over remaining points 
    Tree* donext;
    for (; t != NULL; t = donext) {
      donext = t->next; 
      Tree* min = cycle;
      double mindist = t->distance(cycle);
      for (Tree* tmp = cycle->next; tmp != cycle; tmp=tmp->next) {
        double test = tmp->distance(t);
        if (test < mindist) {
          mindist = test;
          min = tmp;
        } 
      } 

      Tree* next = min->next;
      Tree* prev  = min->prev;

      double mintonext = min->distance(next);
      double mintoprev = min->distance(prev);
      double ttonext = t->distance(next);
      double ttoprev = t->distance(prev);

      if ((ttoprev - mintoprev) < (ttonext - mintonext)) {
        
        prev->next = t;
        t->next = min;
        t->prev = prev;
        min->prev = t;
      } else {
        next->prev = t;
        t->next = next;
        min->next = t;
        t->prev = min;
      }
    } 

    return cycle;
}
*/






