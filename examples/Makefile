CXX ?= g++
CXXFLAGS = -std=c++11 -O2 -Wall -pthread

all: sender receiver

sender: sender.cpp messages.h ../udpcomm.hpp
	$(CXX) $(CXXFLAGS) sender.cpp -o sender

receiver: receiver.cpp messages.h ../udpcomm.hpp
	$(CXX) $(CXXFLAGS) receiver.cpp -o receiver

clean:
	rm -f sender receiver

.PHONY: all clean
