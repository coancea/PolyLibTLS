PolyLibTLS
==========

Short Description: A Family of Lightweight TLS implementations


1. Remember to set the path to the TLS library:
    $ export LIB_TLS_ROOT=full_path_to_LibTLS


###############################
########### REMARKS ###########
###############################

2. Folder "LibTLS" contains the PolyLibTLS library.
   Folder "Demo"  contains a (simple) motivating example.
   Folder "TestIDEA" contains the IDEA benchmark.
   Folder "LCPCbench" contains several benchmarks that were parallelize by 
     hand under TLS (IDEA, NeuralNet, FFT, BH, EM3D, TSP). Each bench
     has a very simple "Makefile" and further (short) explanation is
     given in "RunHint". 

3. The library was tested mainly on simple examples that did not require
separate compilation of 'cpp' files.   

4. For good performance most functions need to be inlined -- i.e. 
`specLD' and `specST'. 

5. In some cases the serial-commit model performs poorly, mainly because:
     a) it uses an `mfence' instruction to guarantee sequential consistency;
        this can be eliminated in the same way as with the In-Place model.

     b) in the presence of iteration-independent RAW dependencies:
          -- we are currently betting that there are no iteration-independent,
             hence we provide some light-weight ways to test that:
               (i)   maintain the highest written address and bet that 
                     the next read addresses is higher
               (ii)  similar for the lowest
               (iii) keep a vector maintaining the written addresses;
                     if a vector location is not set then that address has
                     not been written and its value is returned from memory.

   In the cases when iteration-independent RAW dependencies are often
   we would need (to implement) a new Serial-Commit model which maintains 
   its updates in a hash structure (instead of a buffer) and hence can
   provide the requested value in O(1) time.

 
6. To run speculation inside a loop you need to do smth like:

for(int j=0; j<4; j++) {
	ttmm.speculate <SpecTh>();
	ttmm.cleanUpAll<SpecTh>();
}  

