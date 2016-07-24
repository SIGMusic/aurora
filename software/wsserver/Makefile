CXX=g++
CXXFLAGS=-std=c++11 -Wall -I../../include -I/opt/local/include
LDLIBS=-lpthread -lrt -lssl -lcrypto -lrf24-bcm
LD=g++
EXE=wsserver
OBJS=server.o wstransport.o

# Check if Raspberry Pi
ifneq (,$(findstring arm, $(shell uname -m)))
	# Add the recommended compiler flags for the Raspberry Pi
	CXXFLAGS+=-O0 -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s
endif

all: $(EXE)

$(EXE): $(OBJS)
	$(LD) $(LDLIBS) -o $(EXE) $^

server.o: server.cpp server.h transport.h wstransport.h
	${CXX} -c ${CXXFLAGS} $< -o $@

wstransport.o: wstransport.cpp wstransport.h transport.h
	${CXX} -c ${CXXFLAGS} $< -o $@

.PHONY: all clean debug

clean:
	rm -f $(EXE) $(OBJS)

debug: CXXFLAGS+=-DDEBUG -g -Og
debug: all
