TARGET = test_assert
CXX = g++
CFLAGS = -g -O2 -Wall -fPIC -Wno-deprecated

SRC = ./
INC = -I../../include
LIB = -L../../lib -lcoroutine 

OBJS = $(addsuffix .o, $(basename $(wildcard *.cc)))

all:
	$(CXX) -o mutex $(CFLAGS)  test_mutex.cc $(INC) $(LIB)

clean:
	-rm -f *.o mutex
