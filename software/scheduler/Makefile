CXX=g++
CXXFLAGS=-std=c++11 -Wall -I../../include -I/opt/local/include
LDLIBS=-lpthread -lrt -lssl -lcrypto -lrf24-bcm
LD=g++
EXE=scheduler
OBJS=main.o radio.o scheduler.o

# Check if Raspberry Pi
ifneq (,$(findstring arm, $(shell uname -m)))
	# Add the recommended compiler flags for the Raspberry Pi
	CXXFLAGS+=-O0 -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s
endif

all: $(EXE)

$(EXE): $(OBJS)
	$(LD) $(LDLIBS) -o $(EXE) $^

main.o: main.cpp radio.h scheduler.h
	${CXX} -c ${CXXFLAGS} $< -o $@

radio.o: radio.cpp radio.h
	${CXX} -c ${CXXFLAGS} $< -o $@

scheduler.o: scheduler.cpp scheduler.h
	${CXX} -c ${CXXFLAGS} $< -o $@

.PHONY: all clean debug

clean:
	rm -f $(EXE) $(OBJS)

debug: CXXFLAGS+=-DDEBUG -g -Og
debug: all
