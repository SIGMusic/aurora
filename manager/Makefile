# The recommended compiler flags for the Raspberry Pi
CCFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -std=c++11 -Wall

all: bin/manager.o bin/server.o bin/radio.o
	g++ ${CCFLAGS} -I../lib -lrf24-bcm -lboost_system -lpthread -o bin/manager bin/manager.o bin/server.o bin/radio.o

bin/manager.o: manager.cpp manager.h
	g++ -c ${CCFLAGS} -I../lib -lpthread -o bin/manager.o manager.cpp

bin/server.o: server.cpp server.h
	g++ -c ${CCFLAGS} -I../lib -lboost_system -lpthread -o bin/server.o server.cpp

bin/radio.o: radio.cpp radio.h
	g++ -c ${CCFLAGS} -lrf24-bcm -lpthread -o bin/radio.o radio.cpp

clean:
	rm -rf bin/*

debug: CCFLAGS += -DDEBUG -g -Og
debug: all
