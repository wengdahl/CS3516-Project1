SRC_CLIENT = src/client/*
SRC_SERVER = src/server/*
CCPP=g++
CPLUS_INCLUDE_PATH = includes/

init-bin:
	mkdir -p bin
	mkdir -p bin/Debug

client-debug:
	make init-bin
	$(CCPP) -g -DDEBUG -I $(CPLUS_INCLUDE_PATH) $(SRC_CLIENT) -o bin/Debug/client.out

server-debug:
	make init-bin
	$(CCPP) -g -DDEBUG -I $(CPLUS_INCLUDE_PATH) $(SRC_SERVER) -o bin/Debug/server.out
	

client:
	make init-bin
	$(CCPP) -o2 -I $(CPLUS_INCLUDE_PATH) $(SRC_CLIENT) -o bin/client.out

server:
	make init-bin
	$(CCPP) -o2 -I $(CPLUS_INCLUDE_PATH) $(SRC_SERVER) -o bin/server.out

all-debug:
	make client-debug
	make server-debug

all:
	make client
	make server

clean:
	rm -f 
	rm -f -r bin/
