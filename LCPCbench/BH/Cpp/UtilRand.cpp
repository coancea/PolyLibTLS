#include <math.h>

  /**
   * Random number generator used by the orignal BH benchmark.
   * @param seed the seed to the generator
   * @return a random number
   **/
double myRand(double seed)
{
    double t = 16807.0*seed + 1;
    
    seed = t - (2147483647.0 * floor(t / 2147483647.0));
    return seed;
}

  /**
   * Generate a floating point random number.  Used by
   * the original BH benchmark.
   *
   * @param xl lower bound
   * @param xh upper bound
   * @param r seed
   * @return a floating point randon number
   **/
double xRand(double xl, double xh, double r)
{
    double res = xl + (xh-xl)*r/2147483647.0;
    return res;
}
