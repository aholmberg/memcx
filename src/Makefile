CXX=g++
CFLAGS=-O3 -Wall -fpic -std=c++11
#CFLAGS=-g -fpic -std=c++11

DEFS=
INCL=-I.
LIBS=-luv
SRCS:=$(wildcard *.cpp)
OBJS:=$(patsubst %.cpp, %.o, $(SRCS))

all: libmemcuv.so libmemcuv.a

libmemcuv.so: ${OBJS}        
	${CXX} -shared -o $@ ${TARG} ${OBJS} ${LIBS}

libmemcuv.a: ${OBJS}        
	ar -r $@ ${OBJS}

%.o: %.cpp ${HDRS}
	${CXX} -c -o $@ ${CFLAGS} ${DEFS} ${INCL} $<

clean:        
	@rm -f *.o *.a *.so
