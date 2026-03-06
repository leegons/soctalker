# SocTalker Makefile
# 编译服务器和客户端

CXX = g++
CXXFLAGS = -std=c++11 -g -Wall
LDFLAGS = -lpthread -lreadline

.PHONY: all clean

all: server client
	@echo "Build complete: server, client"

server: server.cc
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

client: client.cc
	$(CXX) $(CXXFLAGS) -o $@ $< -lreadline

clean:
	rm -f server client
