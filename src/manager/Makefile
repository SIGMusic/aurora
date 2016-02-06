# The recommended compiler flags for the Raspberry Pi
CC=g++
CCFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -std=c++11 -Wall -I../../lib -lpthread

all: bin/manager.o bin/server.o bin/radio.o
	${CC} ${CCFLAGS} -lrf24-bcm -lboost_system -o bin/manager bin/manager.o bin/server.o bin/radio.o

bin/manager.o: manager.cpp manager.h
	${CC} -c ${CCFLAGS} -o bin/manager.o manager.cpp

bin/server.o: server.cpp server.h
	${CC} -c ${CCFLAGS} -lboost_system -o bin/server.o server.cpp

bin/radio.o: radio.cpp radio.h
	${CC} -c ${CCFLAGS} -lrf24-bcm -o bin/radio.o radio.cpp

clean:
	rm -rf bin/*

debug: CCFLAGS += -DDEBUG -g -Og
debug: all
