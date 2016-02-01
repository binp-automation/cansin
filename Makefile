XENO_CFLAGS=-g -D__XENO__ $(shell xeno-config --skin=posix --cflags)
XENO_LDFLAGS=-g $(shell xeno-config --skin=posix --ldflags)

CF=-Isources ${XENO_CFLAGS}
LF=-lm ${XENO_LDFLAGS}

.phony: all clean

all: cansin

cansin: build/main.o build/candev/node_init.o build/candev/node_use.o 
	gcc ${LF} -o $@ $^

build/main.o: sources/main.c sources/candev/node.h
	gcc ${CF} -c -o $@ $<

build/candev/node_init.o: sources/candev/node_init.c
	gcc ${CF} -c -o $@ $<
	
build/candev/node_use.o: sources/candev/node_use.c
	gcc ${CF} -c -o $@ $<
