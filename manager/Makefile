# The recommended compiler flags for the Raspberry Pi
CCFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s

all: manager server
	g++ -o bin/manager bin/manager.o bin/server.o

manager: manager.cpp
	g++ -c ${CCFLAGS} -std=c++11 -Wall -I../lib -lrf24-bcm -lboost_system -lpthread -o bin/$@.o $@.cpp

server: server.cpp server.h
	g++ -c ${CCFLAGS} -std=c++11 -Wall -I../lib -lrf24-bcm -lboost_system -lpthread -o bin/$@.o $@.cpp

clean:
	rm -rf bin/*
