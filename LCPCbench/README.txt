# set the path to the TLS library:
export LIB_TLS_ROOT=path_to_LibTLS


###############################
########### REMARKS ###########
###############################

1. The library was tested mainly on simple examples that did not require
separate compilation of 'cpp' files.   Some work is needed to make it
work in this more general case. (I tried to split the functionality
into `.h' and `.cpp' but as it is now the `.cpp' files are included from
the `.h' files.)

2. For good performance most functions need to be inlined -- i.e. 
`specLD' and `specST'. 

3. In some cases the serial-commit model performs poorly, mainly because:
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

 
4. To run speculation inside a loop you need to do smth like:

for(int j=0; j<4; j++) {
	ttmm.speculate <SpecTh>();
	ttmm.cleanUpAll<SpecTh>();
}  

