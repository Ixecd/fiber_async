TARGET = test_assert
CXX = g++
CFLAGS = -g -O2 -Wall -fPIC -Wno-deprecated

SRC = ./
INC = -I../../include
LIB = -L../../lib -lcoroutine -lpthread

OBJS = $(addsuffix .o, $(basename $(wildcard *.cc)))

all:
	$(CXX) -o fiber $(CFLAGS)  test_fiber.cc $(INC) $(LIB)

clean:
	-rm -f *.o fiber
