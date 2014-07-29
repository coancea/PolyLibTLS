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






Node* Cell::loadTree(Body* p, MathVector xpic, int l, Tree* tree)
{
    // move down one level
    int   si = oldSubindex(xpic, l);
    Node* rt = subp[si];
    if (rt != NULL) 
      subp[si] = rt->loadTree(p, xpic, l >> 1, tree);
    else 
      subp[si] = p;
    return this;
}



double Cell::hackcofm()
{
    double mq = 0.0;
    MathVector tmpPos; // = new MathVector();
    MathVector tmpv;   //   = new MathVector();
    for (int i=0; i < NSUB; i++) {
      Node* r = subp[i];
      if (r != NULL) {
        double mr = r->hackcofm();
        mq = mr + mq;
        tmpv.multScalar(r->pos, mr);
        tmpPos.addition(tmpv);
      }
    }
    mass = mq;
    pos = tmpPos;
    pos.divScalar(mass);

    return mq;
}


HG Cell::walkSubTree(double dsq, HG hg)
{
    if (subdivp(dsq, hg)) {
      for (int k = 0; k < Cell::NSUB; k++) {
        Node* r = subp[k];
        if (r != NULL)
          hg = r->walkSubTree(dsq / 4.0, hg);
      }
    } else {
      hg = gravSub(hg);   
    }
    return hg;
}


bool Cell::subdivp(double dsq, HG hg)
{
    MathVector dr; //= new MathVector();
    dr.subtraction(pos, hg.pos0);
    double drsq = dr.dotProduct();

    // in the original olden version drsp is multiplied by 1.0
    return (drsq < dsq);
}

void Cell::toPrint()
{
    cout<<"Cell ";
    this->Node::toPrint();
}

