CXX=g++
#CFLAGS=-O3 -Wall -fPIC
CFLAGS=-g -fPIC -std=c++11

DEFS=
INCL=-Isrc
LIBS=src/libmemcuv.a -luv
HDRS:=$(wildcard src/*.h)
SRCS:=$(wildcard src/*.cpp)
#SRCS:=$(filter-out $(EXCLUDES), $(SRCS))
OBJS:=$(patsubst %.cpp, %.o, $(SRCS))

all: subdirs

#src/libmemcuv.a:
#	make -C src libmemcuv.a

%.exe: %.cpp src 
	${CXX} ${DEFS} ${INCL} -o $@ ${CFLAGS} $< ${LIBS}

#install: all        
#	install -d ${DESTDIR}/usr/include/memcuv/        
#	cp ${HDRS} ${DESTDIR}/usr/include/memcuv/        
#	make -C gen-cpp install        
#	install -d ${DESTDIR}/usr/lib64/        
#	cp libmemcuv.so  ${DESTDIR}/usr/lib64/        
#	cp libmemcuv.a  ${DESTDIR}/usr/lib64/

SUBDIRS = src test
.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)

$(SUBDIRS):
	make -C $@

#test:
#	make -C test

clean:        
	make -C src clean
	make -C test clean
	@rm -f *.exe

var:        
	@echo HDRS ${HDRS}        
	@echo SRCS ${SRCS}        
	@echo OBJS ${OBJS}
