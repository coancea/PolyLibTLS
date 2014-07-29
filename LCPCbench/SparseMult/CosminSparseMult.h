	/* computes  a matrix-vector multiply with a sparse matrix
        held in compress-row format.  If the size of the matrix
        in MxN with nz nonzeros, then the val[] is the nz nonzeros,
        with its ith entry in column col[i].  The integer vector row[]
        is of size M+1 and row[i] points to the begining of the
        ith row in col[].  
    */
        //const int N = 30000; int NZ = 60000000; //cut two 0 from both
	const int N = 4000000; int NZ = 200000000;
//	const int N = 4000000; int NZ = 800000000;
	double *x, *y, *val, *x_end, *y_end, *val_end;
	int *col, *row, *col_end, *row_end;

	void SparseCompRow_matmult( int M, double *y_t, double *val_t, int *row_t,
        int *col_t, double *x_t, int start, int end)
	{
		int reps;
		int r;
		int i; 
		
		for (r=start; r<end; r++)
		{
			double sum = 0.0; 
			int rowR = row_t[r];
			int rowRp1 = row_t[r+1];
			//cout<<"Diff: "<<rowRp1-rowR<<endl;
			
			for (i=rowR; i<rowRp1; i++) 
				sum += x_t[ col_t[i] ] * val_t[i];
			
			y_t[r] = sum;
			
		}
		
	}


	void SparseCompRow_matmult( int M, double *y_t, double *val_t, int *row_t,
        int *col_t, double *x_t)
    {
		SparseCompRow_matmult( M, y_t, val_t, row_t, col_t, x_t, 0, M);
    }

	void init() {
        x = new double[N]; //RandomVector(N, R);
        y = new double[N];//(double*) malloc(sizeof(double)*N);

		for(int i=0; i<N; i++) x[i] = ((double)i)*2/3;

        double result = 0.0;
        int nr = NZ/N;      /* average number of nonzeros per row  */
        int anz = nr *N;    /* _actual_ number of nonzeros         */

            
        val = new double[anz]; //RandomVector(anz, R);

		for(int i=0; i<anz; i++) val[i] = ((double)i) * 5 / 50;

        col = new int[NZ]; //(int*) malloc(sizeof(int)*nz);
        row = new int[N+1]; //(int*) malloc(sizeof(int)*(N+1));
        int r=0;
        int cycles=1;

        row[0] = 0; 
        for (r=0; r<N; r++)
        {
            /* initialize elements for row r */

            int rowr = row[r];
            int step = r/ nr;
            int i=0;

            row[r+1] = rowr + nr;
            if (step < 1) step = 1;   /* take at least unit steps */


            for (i=0; i<nr; i++)
                col[rowr+i] = i*step;
                
        }
		x_end = x+N;
		y_end = y+N;
		val_end = val + anz;
		col_end = col + NZ;
		row_end = row + N+1;

	}

	double kernel_measureSparseMatMult()
    {
        /* initialize vector multipliers and storage for result */
        /* y = A*y;  */

        x = new double[N]; //RandomVector(N, R);
        y = new double[N];//(double*) malloc(sizeof(double)*N);

		for(int i=0; i<N; i++) x[i] = ((double)i)*2/3;

        double result = 0.0;

#if 0
        // initialize square sparse matrix
        //
        // for this test, we create a sparse matrix with M/nz nonzeros
        // per row, with spaced-out evenly between the begining of the
        // row to the main diagonal.  Thus, the resulting pattern looks
        // like
        //             +-----------------+
        //             +*                +
        //             +***              +
        //             +* * *            +
        //             +** *  *          +
        //             +**  *   *        +
        //             +* *   *   *      +
        //             +*  *   *    *    +
        //             +*   *    *    *  + 
        //             +-----------------+
        //
        // (as best reproducible with integer artihmetic)
        // Note that the first nr rows will have elements past
        // the diagonal.
#endif

        int nr = NZ/N;      /* average number of nonzeros per row  */
        int anz = nr *N;    /* _actual_ number of nonzeros         */

            
        val = new double[anz]; //RandomVector(anz, R);

		for(int i=0; i<anz; i++) val[i] = ((double)i) * 5 / 50;

        col = new int[NZ]; //(int*) malloc(sizeof(int)*nz);
        row = new int[N+1]; //(int*) malloc(sizeof(int)*(N+1));
        int r=0;
        int cycles=1;

        row[0] = 0; 
        for (r=0; r<N; r++)
        {
            /* initialize elements for row r */

            int rowr = row[r];
            int step = r/ nr;
            int i=0;

            row[r+1] = rowr + nr;
            if (step < 1) step = 1;   /* take at least unit steps */


            for (i=0; i<nr; i++)
                col[rowr+i] = i*step;
                
        }


        {
            SparseCompRow_matmult(N, y, val, row, col, x/*, cycles*/);
            //cycles *= 2;
        }
        /* approx Mflops */
        
        delete row; //free(row);
        delete col; //free(col);
        delete val; //free(val);
        delete y;   //free(y);
        delete x;   //free(x);

        return result;
    }
