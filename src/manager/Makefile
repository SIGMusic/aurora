CXX=g++
CXXFLAGS=-std=c++11 -Wall -I/opt/local/include
LDLIBS=-lpthread -lrt -lssl -lcrypto -lrf24-bcm
LD=g++
EXE=manager
OBJS=main.o server.o radio.o wstransport.o udptransport.o

# Check if Raspberry Pi
ifneq (,$(findstring arm, $(shell uname -m)))
	# Add the recommended compiler flags for the Raspberry Pi
	CXXFLAGS+=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s
endif

all: $(EXE)

$(EXE): $(OBJS)
	$(LD) $(LDLIBS) -o $(EXE) $^

main.o: main.cpp server.h radio.h common.h
	${CXX} -c ${CXXFLAGS} $< -o $@

server.o: server.cpp server.h common.h transport.h wstransport.h udptransport.h
	${CXX} -c ${CXXFLAGS} $< -o $@

radio.o: radio.cpp radio.h ../network.h common.h
	${CXX} -c ${CXXFLAGS} $< -o $@

wstransport.o: wstransport.cpp wstransport.h transport.h
	${CXX} -c ${CXXFLAGS} $< -o $@

udptransport.o: udptransport.cpp udptransport.h transport.h
	${CXX} -c ${CXXFLAGS} $< -o $@

.PHONY: all clean debug

clean:
	rm -f $(EXE) $(OBJS)

debug: CXXFLAGS+=-DDEBUG -g -Og
debug: all
