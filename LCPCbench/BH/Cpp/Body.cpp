#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#include<time.h>
#include<iostream.h>
#include<signal.h>
#include<sys/time.h>


#ifndef MATH_VECT_H
#define MATH_VECT_H
#include "MathVector.h"
#endif

#ifndef TREE_H
#define TREE_H
#include "Tree.h"
#endif

#ifndef NODE_H
#define NODE_H
#include "Node.h"
#endif

#ifndef CELL_H
#define CELL_H
#include "Cell.h"
#endif

#ifndef BODY_H
#define BODY_H
#include "Body.h"
#endif



Body* Enumerate::nextElement() {
    Body* retval = current;

    if(this->forw)   current = current->next;
    else             current = current->procNext;

    return retval;
}


void Body::expandBox(Tree* tree, int nsteps)
{
    MathVector rmid; // = new MathVector();

    bool inbox = icTest(tree);
    while (!inbox) {
      double rsize = tree->rsize;
      rmid.addScalar(tree->rmin, 0.5 * rsize);
      
      for (int k = 0; k < MathVector::NDIM; k++) {
        if (pos.value(k) < rmid.value(k)) {
          double rmin = tree->rmin.value(k);
          tree->rmin.value(k, rmin - rsize);
        }
      }
      tree->rsize = 2.0 * rsize;
      if (tree->root != NULL) {
        MathVector ic = tree->intcoord(rmid);
        int k = oldSubindex(ic, IMAX >> 1);
        Cell* newt = new Cell();
        newt->subp[k] = tree->root;
        tree->root = newt;
        inbox = icTest(tree);
      }
    }
}


bool Body::icTest(Tree* tree)
{
    double pos0 = pos.value(0);
    double pos1 = pos.value(1);
    double pos2 = pos.value(2);
    
    // by default, it is in bounds
    bool result = true;

    double xsc = (pos0 - tree->rmin.value(0)) / tree->rsize;
    if (!(0.0 < xsc && xsc < 1.0)) {
      result = false;
    }

    xsc = (pos1 - tree->rmin.value(1)) / tree->rsize;
    if (!(0.0 < xsc && xsc < 1.0)) {
      result = false;
    }

    xsc = (pos2 - tree->rmin.value(2)) / tree->rsize;
    if (!(0.0 < xsc && xsc < 1.0)) {
      result = false;
    }

    return result;
}


Node* Body::loadTree(Body* p, MathVector xpic, int l, Tree* tree)
{
    // create a Cell
    Cell* retval = new Cell();
    int si = subindex(tree, l);
    // attach this Body node to the cell
    retval->subp[si] = this;

    // move down one level
    si = oldSubindex(xpic, l);
    Node* rt = retval->subp[si];
    if (rt != NULL) 
      retval->subp[si] = rt->loadTree(p, xpic, l >> 1, tree);
    else 
      retval->subp[si] = p;
    return retval;
}



int Body::subindex(Tree* tree, int l)
{
    MathVector xp; //= new MathVector();

    double xsc = (pos.value(0) - tree->rmin.value(0)) / tree->rsize;
    xp.value(0, floor(IMAX * xsc));

    xsc = (pos.value(1) - tree->rmin.value(1)) / tree->rsize;
    xp.value(1, floor(IMAX * xsc));

    xsc = (pos.value(2) - tree->rmin.value(2)) / tree->rsize;
    xp.value(2, floor(IMAX * xsc));

    int i = 0;
    for (int k = 0; k < MathVector::NDIM; k++) {
      if (((int)xp.value(k) & l) != 0) {
        i += Cell::NSUB >> (k + 1);
      }
    }
    return i;
}


void Body::hackGravity(double rsize, Node* root)
{
    MathVector pos0 = pos;

    HG hg(this, pos);                    
    hg = root->walkSubTree(rsize * rsize, hg);
    phi = hg.phi0;
    newAcc = hg.acc0;
}

HG Body::walkSubTree(double dsq, HG hg)
{
    if (this != hg.pskip)
      hg = gravSub(hg);                          
    return hg;
}

