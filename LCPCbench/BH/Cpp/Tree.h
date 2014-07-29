#ifndef MATH_VECT_H
#define MATH_VECT_H
#include "MathVector.h"
#endif

#ifndef BODY_H
#define BODY_H
#include "Body.h"
#endif



class Enumerate;
class Node;
class Cell;
class Body;

//import java.util.Enumeration;


/**
 * A class that represents the root of the data structure used
 * to represent the N-bodies in the Barnes-Hut algorithm.
 **/
class Tree
{
 public:
  MathVector  rmin;
  double      rsize;
  /**
   * A reference to the root node.
   **/
  Node*        root;
  /**
   * The complete list of bodies that have been created.
   **/
  Body*        bodyTab;
  /**
   * The complete list of bodies that have been created - in reverse.
   **/
  Body*        bodyTabRev;

 public:  
  /**
   * Construct the root of the data structure that represents the N-bodies.
   **/
  Tree()
  {
    //rmin       = new MathVector(); // MODIFY
    rsize      = -2.0 * -2.0;
    root       = NULL;
    bodyTab    = NULL;
    bodyTabRev = NULL;

    rmin.value(0, -2.0);
    rmin.value(1, -2.0);
    rmin.value(2, -2.0);
  }
  
  /**
   * Return an enumeration of the bodies.
   * @return an enumeration of the bodies.
   **/
  Enumerate bodies() const
  {
    return bodyTab->elements();
  }

  /**
   * Return an enumeration of the bodies - in reverse.
   * @return an enumeration of the bodies - in reverse.
   **/
  Enumerate bodiesRev() const
  {
    return bodyTabRev->elementsRev();
  }

  /**
   * Create the testdata used in the benchmark.
   * @param nbody the number of bodies to create
   **/
  void createTestData(int nbody);


  /**
   * Advance the N-body system one time-step.
   * @param nstep the current time step
   **/
  void stepSystem   (int nstep);
  void stepSystemPAR(int nstep);
  void stepSystemTLS(int nstep);

  static void vp(Body* p, int nstep);
  static void vpPAR(Body* p, int nstep);
  static void vpTLS(Body* p, int nstep);

  static void vp_iterPAR(Body* b, int nstep);


  /**
   * Initialize the tree structure for hack force calculation.
   * @param nsteps the current time step
   **/
  void makeTree(int nstep);

  /**
   * Compute integerized coordinates.
   * @return the coordinates or null if rp is out of bounds
   **/
  MathVector intcoord(MathVector vp)
  {
    MathVector xp; // = new MathVector();
    
    double xsc = (vp.value(0) - rmin.value(0)) / rsize;
    if (0.0 <= xsc && xsc < 1.0) {
      xp.value(0, floor(Node::IMAX * xsc));
    } else {
      cout<<"Value is out of bounds. Exiting!!!"<<endl; exit(0);
    }

    xsc = (vp.value(1) - rmin.value(1)) / rsize;
    if (0.0 <= xsc && xsc < 1.0) {
      xp.value(1, floor(Node::IMAX * xsc));
    } else {
      cout<<"Value is out of bounds. Exiting!!!"<<endl; exit(0);
    }

    xsc = (vp.value(2) - rmin.value(2)) / rsize;
    if (0.0 <= xsc && xsc < 1.0) {
      xp.value(2, floor(Node::IMAX * xsc));
    } else {
      cout<<"Value is out of bounds. Exiting!!!"<<endl; exit(0);
    }
    return xp;
  }
};






