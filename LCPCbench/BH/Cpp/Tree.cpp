
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




void Tree::createTestData(int nbody)
{
    MathVector cmr; // = new MathVector();
    MathVector cmv; // = new MathVector();

    Body* head = new Body();
    Body* prev = head;

    double rsc  = 3.0 * PI / 16.0;
    double vsc  = sqrt(1.0 / rsc);
    double seed = 123.0;

    for (int i = 0; i < nbody; i++) {
      Body* p = new Body();

      prev->setNext(p);
      prev = p;
      p->mass = 1.0/(double)nbody;
      
      seed      = myRand(seed);
      double t1 = xRand(0.0, 0.999, seed);
      t1        = pow(t1, (-2.0/3.0)) - 1.0;
      double r  = 1.0 / sqrt(t1);

      double coeff = 4.0;
      for (int k = 0; k < MathVector::NDIM; k++) {
        seed = myRand(seed);
        r = xRand(0.0, 0.999, seed);
        p->pos.value(k, coeff*r);
      }
      
      cmr.addition(p->pos);

      double x, y;
      do {
        seed = myRand(seed);
        x    = xRand(0.0, 1.0, seed);
        seed = myRand(seed);
        y    = xRand(0.0, 0.1, seed);
      } while (y > x*x * pow(1.0 - x*x, 3.5));

      double v = sqrt(2.0) * x / pow(1 + r*r, 0.25);

      double rad = vsc * v;
      double rsq;
      do {
        for (int k = 0; k < MathVector::NDIM; k++) {
          seed     = myRand(seed);
          p->vel.value(k, xRand(-1.0, 1.0, seed));
        }
        rsq = p->vel.dotProduct();
      } while (rsq > 1.0);
      double rsc1 = rad / sqrt(rsq);
      p->vel.multScalar(rsc1);
      cmv.addition(p->vel);
    }

    // mark end of list
    prev->setNext(NULL);
    // toss the dummy node at the beginning and set a reference to the first element
    bodyTab = head->getNext();

    cmr.divScalar((double)nbody);
    cmv.divScalar((double)nbody);

    prev = NULL;
    
    for (Enumerate e = bodyTab->elements(); e.hasMoreElements(); ) {
      Body* b = e.nextElement();
      b->pos.subtraction(cmr);
      b->vel.subtraction(cmv);
      b->setProcNext(prev);
      prev = b;
    }
    
    // set the reference to the last element
    bodyTabRev = prev;
}



void Tree::stepSystem(int nstep)
{
    // free the tree
    root = NULL;

    makeTree(nstep);

    // compute the gravity for all the particles
    for (Enumerate e = bodyTabRev->elementsRev(); e.hasMoreElements(); ) {
      Body* b = e.nextElement();
      b->hackGravity(rsize, root);
    }

    vp(bodyTabRev, nstep);

}



void Tree::makeTree(int nstep)
{
    for (Enumerate e = bodiesRev(); e.hasMoreElements(); ) {
      Body* q = e.nextElement();
      if (q->mass != 0.0) {
        q->expandBox(this, nstep);
        MathVector xqic = intcoord(q->pos);
        if (root == NULL) {
          root = q;
        } else {
          root = root->loadTree(q, xqic, Node::IMAX >> 1, this);
        }
      }
    }
    root->hackcofm();
}



void Tree::vp(Body* p, int nstep)
{
    MathVector dacc; // = new MathVector();
    MathVector dvel; // = new MathVector();
    double dthf = 0.5 * DTIME;

    for (Enumerate e = p->elementsRev(); e.hasMoreElements(); ) {
      Body* b = e.nextElement();
      MathVector acc1 = b->newAcc;
      if (nstep > 0) {
        dacc.subtraction(acc1, b->acc);
        dvel.multScalar(dacc, dthf);
        dvel.addition(b->vel);
        b->vel = dvel;
      }
      b->acc = acc1;
      dvel.multScalar(b->acc, dthf);

      MathVector vel1 = b->vel;
      vel1.addition(dvel);
      MathVector dpos = vel1;
      dpos.multScalar(DTIME);
      dpos.addition(b->pos);
      b->pos = dpos;
      vel1.addition(dvel);
      b->vel = vel1;
    }
}
