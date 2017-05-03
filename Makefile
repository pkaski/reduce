
NAUTY_PATH=./nauty/nauty26r7

GMP_PATH=./gmp/gmp-6.1.2

NAUTY_OBJS=$(NAUTY_PATH)/naugraph.o $(NAUTY_PATH)/naugroup.o $(NAUTY_PATH)/naurng.o $(NAUTY_PATH)/nausparse.o $(NAUTY_PATH)/nautil.o $(NAUTY_PATH)/nautinv.o $(NAUTY_PATH)/naututil.o $(NAUTY_PATH)/nauty.o $(NAUTY_PATH)/schreier.o

GMP_A=$(GMP_PATH)/.libs/libgmp.a

all: libgraph.a reduce

CFLAGS=-O3 -std=c99 -Wall -I$(NAUTY_PATH) -I$(GMP_PATH)

COMMITID=$(shell git rev-parse HEAD)

graph.o: graph.c graph.h

common.o: common.c common.h

libgraph.a: graph.o common.o $(NAUTY_OBJS) 
	ar -r libgraph.a common.o graph.o $(NAUTY_OBJS)

reduce: reduce.c graph.h libgraph.a
	$(CC) $(CFLAGS) -DCOMMITID=\"$(COMMITID)\" -o reduce reduce.c libgraph.a $(GMP_A)

clean:
	rm -f reduce libgraph.a *.o *~ *.log 

