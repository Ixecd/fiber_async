TARGET = test_assert
CXX = g++
CFLAGS = -g -O2 -Wall -fPIC -Wno-deprecated

SRC = ./
INC = -I../../include
LIB = -L../../lib -lcoroutine -lpthread

OBJS = $(addsuffix .o, $(basename $(wildcard *.cc)))

all:
	$(CXX) -o scheduler $(CFLAGS)  test_scheduler.cc $(INC) $(LIB)

clean:
	-rm -f *.o scheduler
