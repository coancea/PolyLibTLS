


class Body;

class Enumerate {
 private:
  Body* current;
  const bool  forw;
 public:
  Enumerate(bool f, Body* b) : forw(f) { this->current = b; }

  bool hasMoreElements() const; // { return (current != NULL); }
  Body* nextElement();

  bool hasMoreElements_TLS(Thread_BH_HG_TLS* th) const;
  Body* nextElement_TLS   (Thread_BH_HG_TLS* th);
};



class Cell;

#ifndef NODE_H
#define NODE_H
#include "Node.h"
#endif


/**
 * A class used to representing particles in the N-body simulation.
 **/
class Body : public Node
{
 public:
  MathVector vel;
  MathVector acc;
  MathVector newAcc;
  double     phi;

  Body* next;
  Body* procNext;

  /**
   * Create an empty body.
   **/
  Body()
  {
    //vel      = new MathVector();
    //acc      = new MathVector();
    //newAcc   = new MathVector();
    phi      = 0.0;
    next     = NULL;
    procNext = NULL;
  }

  /**
   * Set the next body in the list.
   * @param n the body
   **/
  void setNext(Body* n)
  {
    next = n;
  }

  /**
   * Get the next body in the list.
   * @return the next body
   **/
  Body* getNext() const
  {
    return next;
  }

  /**
   * Set the next body in the list.
   * @param n the body
   **/
  void setProcNext(Body* n)
  {
    procNext = n;
  }

  /**
   * Get the next body in the list.
   * @return the next body
   **/
  Body* getProcNext() const
  {
    return procNext;
  }

  /**
   * Enlarge cubical "box", salvaging existing tree structure.
   * @param tree the root of the tree.
   * @param nsteps the current time step
   **/
  void expandBox(Tree* tree, int nsteps);

  /**
   * Check the bounds of the body and return true if it isn't in the
   * correct bounds.
   **/
  bool icTest(Tree* tree);

  /**
   * Descend Tree and insert particle.  We're at a body so we need to
   * create a cell and attach this body to the cell.
   * @param p the body to insert
   * @param xpic
   * @param l 
   * @param tree the root of the data structure
   * @return the subtree with the new body inserted
   **/
  virtual Node* loadTree(Body* p, MathVector xpic, int l, Tree* tree);

  /**
   * Descend tree finding center of mass coordinates
   * @return the mass of this node
   **/
  virtual double hackcofm();

  /**
   * Return an enumeration of the bodies
   * @return an enumeration of the bodies
   **/
  Enumerate elements()
  {
    // a local class that implements the enumerator
    Enumerate e(true, this);
    return e;
  }

  Enumerate elementsRev()
  {
    Enumerate e(false, this);
    return e;
  }

  /**
   * Determine which subcell to select.
   * Combination of intcoord and oldSubindex.
   * @param t the root of the tree
   **/
  int subindex(Tree* tree, int l);

  /**
   * Evaluate gravitational field on the body.
   * The original olden version calls a routine named "walkscan",
   * but we use the same name that is in the Barnes code.
   **/
  void hackGravity    (double rsize, Node* root);
  void hackGravity_TLS(double rsize, Node* root, Thread_BH_HG_TLS* th);

  /**
   * Recursively walk the tree to do hackwalk calculation
   **/
  virtual HG walkSubTree    (double dsq, HG hg);
  virtual HG walkSubTree_TLS(double dsq, HG hg, Thread_BH_HG_TLS* th);

  /**
   * Return a string represenation of a body.
   * @return a string represenation of a body.
   **/
  void toPrint()
  {
    cout<<"Body ";
    this->Node::toPrint();
  }

};











