TARGET = lib/libcoroutine.a
CXX = g++
CFLAGS = -g -O2 -Wall -fPIC -Wno-deprecated

SRC = ./src
INC = -I./include

OBJS = $(addsuffix .o, $(basename $(wildcard $(SRC)/*.cc)))

$(TARGET) : $(OBJS)
	@mkdir -p lib 
	@ar cqs $@ $^

%.o : %.cc 
	@$(CXX) $(CFLAGS) -c -o $@ $< $(INC)

.PHONY: clean

clean :
	rm -f src/*.o 