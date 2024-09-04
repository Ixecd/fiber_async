TARGET = test_assert
CXX = g++
CFLAGS = -g -O2 -Wall -fPIC -Wno-deprecated

SRC = ./
INC = -I../../include
LIB = -L../../lib -lcoroutine -lpthread

OBJS = $(addsuffix .o, $(basename $(wildcard *.cc)))

all:
	$(CXX) -o iomanager $(CFLAGS)  test_iomanager.cc $(INC) $(LIB)
	$(CXX) -o server	$(CFLAGS)  server.cc		 $(INC) $(LIB)
clean:
	-rm -f *.o iomanager
