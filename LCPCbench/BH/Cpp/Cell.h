
#ifndef MATH_VECT_H
#define MATH_VECT_H
#include "MathVector.h"
#endif


#ifndef NODE_H
#define NODE_H
#include "Node.h"
#endif

class Body;
class Tree;


/**
 * A class used to represent internal nodes in the tree
 **/
class Cell : public Node
{
 public:
  // subcells per cell
  static const int NSUB = 8;  // 1 << NDIM

  /**
   * The children of this cell node.  Each entry may contain either 
   * another cell or a body.
   **/
  Node*  subp[NSUB];
  Cell*  next;

  Cell()
  {
    //subp = new Node[NSUB];
    for(int i=0; i<NSUB; i++) subp[i] = NULL;
    next = NULL;
  }
  
  /**
   * Descend Tree and insert particle.  We're at a cell so 
   * we need to move down the tree.
   * @param p the body to insert into the tree
   * @param xpic
   * @param l
   * @param tree the root of the tree
   * @return the subtree with the new body inserted
   **/
  virtual Node* loadTree(Body* p, MathVector xpic, int l, Tree* tree);

  /**
   * Descend tree finding center of mass coordinates
   * @return the mass of this node
   **/
  virtual double hackcofm();

  /**
   * Recursively walk the tree to do hackwalk calculation
   **/
  virtual HG walkSubTree    (double dsq, HG hg);
  virtual HG walkSubTree_TLS(double dsq, HG hg, Thread_BH_HG_TLS* th);

  /**
   * Decide if the cell is too close to accept as a single term.
   * @return true if the cell is too close.
   **/
  bool subdivp    (double dsq, HG hg);
  bool subdivp_TLS(double dsq, HG hg, Thread_BH_HG_TLS* th);

  /**
   * Return a string represenation of a cell.
   * @return a string represenation of a cell.
   **/
  void toPrint();

};





