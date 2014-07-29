NNBW : ${OBJS}
	${CXX} -O3 -pthread -I$(LIB_TLS_ROOT) -I. -o $@ TestNNbw.cpp -DVER=1

NNFW : ${OBJS}
	${CXX} -O3 -pthread -I$(LIB_TLS_ROOT) -I. -o $@ TestNNfw.cpp -DVER=2

clean :
	rm NNBW NNFW
