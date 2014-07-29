IDEAkey : ${OBJS}
	${CXX} -O3 -pthread -I$(LIB_TLS_ROOT) -I. -o $@ TestIDEA.cpp -DVER=1

IDEAcipher : ${OBJS}
	${CXX} -O3 -pthread -I$(LIB_TLS_ROOT) -I. -o $@ TestIDEA.cpp -DVER=2

clean:
	rm IDEAkey IDEAcipher
