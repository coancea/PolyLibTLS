#ifndef MATH_VECT_H
#define MATH_VECT_H
#include "MathVector.h"
#endif


class Body;
class Tree;

//class MathVector;


/**
   * A class which is used to compute and save information during the 
   * gravity computation phse.
   **/
class HG
{
 public:
  /**
   * Body to skip in force evaluation
   **/
  Body*       pskip;
  /**
   * Point at which to evaluate field
   **/
  MathVector  pos0;  
  /**
   * Computed potential at pos0
   **/
  double      phi0;  
  /** 
   * computed acceleration at pos0
   **/
  MathVector  acc0;  
   
  /**
   * Create a HG  object.
   * @param b the body object
   * @param p a vector that represents the body
   **/
  HG(Body* b, MathVector p)
  {
    pskip = b;
    pos0  = p; //p.clone();
    phi0  = 0.0;
    //acc0  = new MathVector();
  }
};




/**
 * A class that represents the common fields of a cell or body
 * data structure.
 **/
class Node
{
 public:
  /**
   * Mass of the node.
   **/
  double     mass;
  /**
   * Position of the node
   **/
  MathVector pos;

  // highest bit of int coord
  static const int IMAX =  1073741824;

  // potential softening parameter
  static const double EPS = 0.05;

  /**
   * Construct an empty node
   **/
  Node()
  {
    mass = 0.0;
  }

  virtual Node*   loadTree(Body* p, MathVector xpic, int l, Tree* root) = 0;
  virtual double  hackcofm() = 0;
  virtual HG      walkSubTree(double dsq, HG hg) = 0;
  virtual HG      walkSubTree_TLS(double dsq, HG hg, Thread_BH_HG_TLS* th) = 0;

  static int oldSubindex(MathVector ic, int l);

  /**
   * Return a string representation of a node.
   * @return a string representation of a node.
   **/
  void toPrint()
  {
    cout<<mass<<" : ";
    pos.toPrint();
    cout<<endl;
  }

  /**
   * Compute a single body-body or body-cell interaction
   **/
  HG gravSub(HG hg);     // SHOULD CHANGE THE SIGNATURE TO WORK WITH POINTERS???
  HG gravSub_TLS(HG hg, Thread_BH_HG_TLS* th); 

};
