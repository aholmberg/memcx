CXX=g++
#CFLAGS=-O3 -Wall -fPIC
CFLAGS=-g -fPIC -std=c++11

DEFS=
INCL=-I../src
LIBS=-luv -lcppunit
EXCLUDES:=
HDRS:=$(wildcard *.h)
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

var:        
	@echo HDRS ${HDRS}        
	@echo SRCS ${SRCS}        
	@echo OBJS ${OBJS}
	@echo ALL_TESTS ${ALL_TESTS}
	@echo LIBS ${LIBS}
	@echo INCL ${INCL}

%.exe: %.cpp ../src/libmemcuv.a
	${CXX} ${DEFS} ${INCL} -o $@ ${CFLAGS} $^ ${LIBS}
