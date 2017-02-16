.PHONY: all
all: server client
	@echo build $@

.PHONY: clean
clean:
	rm -f server
	rm -f client 

server: server.cc $(OBJS)
	gcc -std=c++11 -lstdc++ -lpthread -lreadline -lhistory -g -o $@ $< $(OBJS)

client: client.cc $(OBJS)
	gcc -std=c++11 -lstdc++ -g -o $@ $< $(OBJS)
