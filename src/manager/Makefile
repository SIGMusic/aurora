# The recommended compiler flags for the Raspberry Pi
CXX=g++
CXXFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -std=c++11 -Wall -I../../lib
LD=g++
LDLIBS=-lpthread -lrf24-bcm -lboost_system
EXE=manager
OBJS=main.o server.o radio.o

all: $(OBJS)
	$(LD) $(LDLIBS) -o $(EXE) $^

main.o: main.cpp server.h radio.h common.h
	${CXX} -c ${CXXFLAGS} $< -o $@

server.o: server.cpp server.h common.h
	${CXX} -c ${CXXFLAGS} $< -o $@

radio.o: radio.cpp radio.h ../network.h common.h
	${CXX} -c ${CXXFLAGS} $< -o $@

.PHONY: all clean debug

clean:
	rm -f $(EXE) $(OBJS)

debug: CXXFLAGS+=-DDEBUG -g -Og
debug: all
