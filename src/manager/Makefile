# The recommended compiler flags for the Raspberry Pi
CXX=g++
CXXFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s -std=c++11 -Wall -I../../lib
LD=g++
LDLIBS=-lpthread -lrf24-bcm -lboost_system
OBJS=bin/main.o bin/server.o bin/radio.o bin/utils.o

all: $(OBJS)
	$(LD) $(LDLIBS) -o bin/manager $^

# Generic, but does not take into account dependencies
# bin/%.o: %.cpp
# 	$(CXX) -c $(CXXFLAGS) $< -o $@

bin/main.o: main.cpp server.h radio.h common.h
	${CXX} -c ${CXXFLAGS} $< -o $@

bin/server.o: server.cpp server.h common.h
	${CXX} -c ${CXXFLAGS} $< -o $@

bin/radio.o: radio.cpp radio.h ../network.h common.h
	${CXX} -c ${CXXFLAGS} $< -o $@

bin/utils.o: utils.cpp common.h
	${CXX} -c ${CXXFLAGS} $< -o $@

.PHONY: all clean debug

clean:
	rm -f bin/manager $(OBJS)

debug: CXXFLAGS+=-DDEBUG -g -Og
debug: all
