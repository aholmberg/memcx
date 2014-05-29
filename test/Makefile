CXX=g++
CFLAGS=-O3 -Wall -fpic -std=c++11
#CFLAGS=-g -fpic -std=c++11

DEFS=
INCL=-I../src
LIBS=-luv -lcppunit
EXCLUDES:=tput.cpp
SRCS:=$(wildcard *.cpp)
TESTS_SRC:=$(filter-out $(EXCLUDES), $(SRCS))
ALL_TESTS:=$(patsubst %.cpp, %.exe, $(TESTS_SRC))

all: $(ALL_TESTS)
	@for t in $(ALL_TESTS); do \
        echo "#########################################\n$$t\n#########################################"; \
		./$$t; \
	done

.PHONY: ../src/libmemcuv.a
../src/libmemcuv.a:
	make -C ../src libmemcuv.a

clean:        
	@rm -Rf *.exe *.dSYM

%.exe: %.cpp ../src/libmemcuv.a
	${CXX} ${DEFS} ${INCL} -o $@ ${CFLAGS} $^ ${LIBS}
