This demo shows the TLS parallelization of the following
contrived loop:

    for(int i=0; i<N; i++) {

        float* row = &nums[ X[i]*M ];
        float  sum = 0.0;

        for(int j=0; j<M; j++) {
            sum += (j < M-32) ? (row[j] + row[j + 32]) / (row[j + 32] + 1.0) : 
                                (row[j] + row[j - 32]) / (row[j - 32] + 1.0) ;
        }
        row[0] = sum;
    }

Where `nums' is a NxM matrix, and `X' is an indirect array
of size `N' that contains elements in {0..N-1} and is used
to select one of the rows of matrix `nums' in the outermost
loop.

Depending on the values stored in `X' the loop can exhibit:
  (i) no (cross-iteration) dependencies, e.g., if `X' is a permutation of {0..N-1}, or 
 (ii) the ocasional dependency, e.g., if the elements of `X' are random in {0..N-1} or
(iii) frequent dependencies, e.g., if `X' has two distinct elements.

Various constant values are set in file `config.h', for example
    the values of `N' and `M', the number of speculative threads, etc. 
The values of indirect array `X' are set in file `IndArr.cpp', 
    function `initDataSet'.
The TLS implementation is given (and can be tuned) in file `IndArrTLS.h'.
    For example, ``#define MACHINE_X86_64'' refers to an optimized TLS 
    implementation that is safe for X86 machines and is about 3x faster 
    than the ones using memory fences or locks, i.e., the ones corresponding
    to ``#define MODEL_FENCE'' or ``#define MODEL_LOCK'', respectively. 

To run the demo, simply type
    $ make clean; make; ./IndArr

The application reports the sequential and speculative runtime,
and whether the results of the two executions are identical.

