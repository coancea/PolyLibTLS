#include "IndArrTLS.h"

/*
static unsigned int DiffTime(timeval* time2, timeval* time1) {
	uint diff = (time2->tv_sec-time1->tv_sec) * 1000000 + (time2->tv_usec-time1->tv_usec);
	return diff;
}
*/

void initDataSet() {
    std::cout << "Initializing value and indirect arrays ..." << std::endl;

    srand(33);

    for (int i = 0; i < N; i++) {
        //X[i] = i%2; // use something like this for many rollbacks!!! 
        X[i] = (unsigned) ( rand() % N );
        for(int j = 0; j < M; j++) {
            float r    = ((float) rand()) /  RAND_MAX;
            nums[i*M + j] = r;
        }

        //save original nums[i][0] in res_seq
        res_seq[i] = nums[i*M];
    }
}

void run_seq(){

    std::cout << "Starting Sequential Execution ..." << std::endl;

	struct timeval start, end;
	unsigned long running_time = 0;
	gettimeofday(&start, NULL);

    for(int i=0; i<N; i++) {

        float* row = &nums[ X[i]*M ];
        float  sum = 0.0;

        for(int j=0; j<M; j++) {
            sum += (j < M-32) ? (row[j] + row[j + 32]) / (row[j + 32] + 1.0) : 
                                (row[j] + row[j - 32]) / (row[j - 32] + 1.0) ;
        }
        row[0] = sum;
    }

	gettimeofday(&end, NULL);
	running_time = DiffTime(&end, &start);
	printf("Time = %lu\n", running_time);

    //save results in res_seq and revert to
    //  the original nums
    for(int i=0; i<N; i++) {
        float tmp  = res_seq[i];
        res_seq[i] = nums[i*M + 0];
        nums[i*M + 0] = tmp;
    }
}


int main() {
    initDataSet();

    run_seq();

    run_tls();

    // check that sequential and TLS version give
    //    the same result
    bool success = true;
    for(int i=0; i<N; i++) {
        if (res_seq[i] != nums[i*M]) { 
            cout << "It: " << i << " seq: " << res_seq[i] << " != tls: " << nums[i*M] << endl; 
            success = false; 
            break; 
        }
    }

    if(success) {
        cout << "VALID  : Sequential and TLS Execution yield the same result!" << endl;
    } else {
        cout << "INVALID: Sequential and TLS Execution yield the different results!" << endl;
    }

    return 0;
}
