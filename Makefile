CXX=g++
CFLAGS=-O3 -Wall -fpic -std=c++11
#CFLAGS=-g -fpic -std=c++11

DEFS=
INCL=-Isrc 
LIBS=src/libmemcuv.a -luv
HDRS:=$(wildcard src/*.h)
SRCS:=$(wildcard src/*.cpp)
OBJS:=$(patsubst %.cpp, %.o, $(SRCS))

all: subdirs

%.exe: %.cpp src 
	${CXX} ${DEFS} ${INCL} -o $@ ${CFLAGS} $< ${LIBS}

SUBDIRS = src test
.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)

$(SUBDIRS):
	make -C $@

clean:        
	make -C src clean
	make -C test clean
	@rm -f *.exe
