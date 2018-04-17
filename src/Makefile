# Compiler variables
CCFLAGS = -ansi -Wall -g -std=c++11

all: simulate

# Rule to link object code files to create executable file
simulate: simulate.o RPCClient.o
	g++ $(CCFLAGS) -o simulate simulate.o RPCClient.o

simulate.o: simulate.cpp
	g++ $(CCFLAGS) -c simulate.cpp
	
RPCClient.o: RPCClient.cpp RPCClient.h
	g++ $(CCFLAGS) -c RPCClient.cpp

# Pseudo-target to remove object code and executale files
clean:
	-rm *.o simulate
