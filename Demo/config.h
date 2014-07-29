#ifndef MAX_CONFIG
#define MAX_CONFIG

#include <stdio.h>
//#include <iostream>
#include <stdlib.h>
#include <cstdlib>
#include <sys/time.h>
#include<time.h>
#include<signal.h>


#define logN                14//10 //14
#define logM                14 //14
#define N                   (1<<logN)
#define M                   (1<<logM)
#define NUMBERS_COUNT       (1<<(logN+logM))
#define NUM_THREADS         6


static float     nums[N*M];    //[N][M];
static float     res_seq[N];
static unsigned  X      [N];

#endif  // MAX_CONFIG
