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

#ifndef NODE_H
#define NODE_H
#include "Node.h"
#endif

#ifndef CELL_H
#define CELL_H
#include "Cell.h"
#endif


int Node::oldSubindex(MathVector ic, int l)
{
    int i = 0;
    for (int k = 0; k < MathVector::NDIM; k++) {
      if (((int)ic.value(k) & l) != 0)
        i += Cell::NSUB >> (k + 1);
    }
    return i;
}
