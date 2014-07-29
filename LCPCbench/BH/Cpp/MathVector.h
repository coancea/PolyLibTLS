
const double DTIME = 0.0125;
const double TSTOP = 2.0;
#define PI  3.1415926535897932

//double myRand(double seed);
//double xRand(double xl, double xh, double r);


/**
 * A class representing a three dimensional vector that implements
 * several math operations.  To improve speed we implement the
 * vector as an array of doubles rather than use the exising
 * code in the java.util.Vector class.
 **/
class MathVector
{
 public:
  /**
   * The number of dimensions in the vector
   **/
  const static int NDIM = 3;
  /**
   * An array containing the values in the vector.
   **/
  double data[NDIM];

  /**
   * Construct an empty 3 dimensional vector for use in Barnes-Hut algorithm.
   **/
  MathVector()
  {
    //data = new double[NDIM];
    for (int i=0; i < NDIM; i++) {
      data[i] = 0.0;
    }
  }

  /**
   * Create a copy of the vector.
   * @return a clone of the math vector
   **/
  MathVector clone() 
  {
    MathVector v;
    for (int i = 0; i < NDIM; i++) {
      v.data[i] = data[i];
    }
    return v;
  }
/*
  MathVector(volatile MathVector& v) {
    for (int i = 0; i < NDIM; i++) {
      data[i] = v.data[i];
    }
  }

  MathVector(const MathVector& v) {
    for (int i = 0; i < NDIM; i++) {
      data[i] = v.data[i];
    }
  }

  void operator=(const MathVector& v) {
    for (int i = 0; i < NDIM; i++) {
      data[i] = v.data[i];
    }
  }

  void operator=(volatile MathVector& v) {
    for (int i = 0; i < NDIM; i++) {
      data[i] = v.data[i];
    }
  }
*/
  /**
   * Return the value at the i'th index of the vector.
   * @param i the vector index 
   * @return the value at the i'th index of the vector.
   **/
  double value(int i) const
  {
    return data[i];
  }

  /**
   * Set the value of the i'th index of the vector.
   * @param i the vector index
   * @param v the value to store
   **/
  void value(int i, double v)
  {
    data[i] = v;
  }

  /**
   * Set one of the dimensions of the vector to 1.0
   * param j the dimension to set.
   **/
  void unit(int j)
  {
    for (int i=0; i < NDIM; i++) {
      data[i] = (i == j ? 1.0 : 0.0);
    }
  }

  /**
   * Add two vectors and the result is placed in this vector.
   * @param u the other operand of the addition
   **/
  void addition(MathVector u)
  {
    for (int i=0; i < NDIM; i++) {
      data[i] += u.data[i];
    }
  }

  /**
   * Subtract two vectors and the result is placed in this vector.
   * This vector contain the first operand.
   * @param u the other operand of the subtraction.
   **/
  void subtraction(MathVector u)
  {
    for (int i=0; i < NDIM; i++) {
      data[i] -= u.data[i];
    }
  }

  /**
   * Subtract two vectors and the result is placed in this vector.
   * @param u the first operand of the subtraction.
   * @param v the second opernd of the subtraction
   **/
  void subtraction(MathVector u, MathVector v)
  {
    for (int i=0; i < NDIM; i++) {
      data[i] = u.data[i] - v.data[i];
    }
  }

  /**
   * Multiply the vector times a scalar.
   * @param s the scalar value
   **/
  void multScalar(double s)
  {
    for (int i=0; i < NDIM; i++) {
      data[i] *= s;
    }
  }

  /**
   * Multiply the vector times a scalar and place the result in this vector.
   * @param u the vector
   * @param s the scalar value
   **/
  void multScalar(MathVector u, double s)
  {
    for (int i=0; i < NDIM; i++) {
      data[i] = u.data[i] * s;
    }
  }

  /**
   * Divide each element of the vector by a scalar value.
   * @param s the scalar value.
   **/
  void divScalar(double s)
  {
    for (int i=0; i < NDIM; i++) {
      data[i] /= s;
    }
  }

  /**
   * Return the dot product of a vector.
   * @return the dot product of a vector.
   **/
  double dotProduct()
  {
    double s = 0.0;
    for (int i=0; i < NDIM; i++) {
      s += data[i] * data[i];
    }
    return s;
  }

  double absolute()
  {
    double tmp = 0.0;
    for (int i = 0; i < NDIM; i++) {
      tmp += data[i] * data[i];
    }
    return sqrt(tmp);
  }

  double distance(MathVector v)
  {
    double tmp = 0.0;
    for (int i = 0; i < NDIM; i++) {
      tmp += ((data[i] - v.data[i]) * (data[i] - v.data[i]));
    }
    return sqrt(tmp);
  }

  void crossProduct(MathVector u, MathVector w)
  {
    data[0] = u.data[1] * w.data[2] - u.data[2]*w.data[1];
    data[1] = u.data[2] * w.data[0] - u.data[0]*w.data[2];
    data[2] = u.data[0] * w.data[1] - u.data[1]*w.data[0];
  }

  void incrementalAdd(MathVector u)
  {
    for (int i = 0; i < NDIM; i++) {
      data[i] += u.data[i];
    }
  }

  void incrementalSub(MathVector u)
  {
    for (int i = 0; i < NDIM; i++) {
      data[i] -= u.data[i];
    }
  }

  void incrementalMultScalar(double s) 
  {
    for (int i=0; i < NDIM; i++) {
      data[i] *= s;
    }
  }

  void incrementalDivScalar(double s)
  {
    for (int i=0; i < NDIM; i++) {
      data[i] /= s;
    }
  }

  /**
   * Add a scalar to each element in the vector and put the
   * result in this vector.
   * @param u a vector
   * @param s the scalar
   **/
  void addScalar(MathVector u, double s) 
  {
    for (int i = 0; i < NDIM; i++) {
      data[i] = u.data[i] + s;
    }
  }


  /**
   * Return the string representation of the vector
   **/
  void toPrint()
  {
    for (int i = 0; i < NDIM; i++) {
      cout<<data[i]<<" ";
    }
    cout<<endl;
  }
};
