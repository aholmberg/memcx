CXX=g++
#CFLAGS=-O3 -Wall -fPIC
CFLAGS=-g -fPIC -std=c++11

DEFS=
INCL=-I.
LIBS=-luv
EXCLUDES:=
HDRS:=$(wildcard *.h)
SRCS:=$(wildcard *.cpp)
SRCS:=$(filter-out $(EXCLUDES), $(SRCS))
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

var:        
	@echo HDRS ${HDRS}        
	@echo SRCS ${SRCS}        
	@echo OBJS ${OBJS}
