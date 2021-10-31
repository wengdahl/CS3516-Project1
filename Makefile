SRC_CLIENT = src/client/*
SRC_SERVER = src/server/*
CCPP=g++
CPLUS_INCLUDE_PATH = includes/

debug:
	mkdir -p bin
	mkdir -p bin/Debug
	echo $(SRC)

# Debug rules define DEBUG macro for deubbing purposes.

# Addem rules define ADDEM macro
# This changes functionality of main/worker threads to fit functionality of addem
addem:
	$(CCPP) -o2 -DADDEM -I $(CPLUS_INCLUDE_PATH) $(SRC) -o addem $(LIB)
cleanaddem:
	rm -f addem
debug-addem:
	make debug
	$(CCPP) -g -DADDEM -DDEBUG $(SRC) -I $(CPLUS_INCLUDE_PATH) -o bin/Debug/addem $(LIB)
cleandebug-addem:
	rm -f bin/Debug/addem

# Life rules define LIFE macro
# This changes functionality of main/worker threads to fit functionality of Game of Life
life:
	$(CCPP) -o2 -DLIFE -I $(CPLUS_INCLUDE_PATH) $(SRC) -o life $(LIB)
cleanlife:
	rm -f life
debug-life:
	make debug
	$(CCPP) -g -DLIFE -DDEBUG -I $(CPLUS_INCLUDE_PATH) $(SRC) -o bin/Debug/life $(LIB)
cleandebug-life:
	rm -f bin/Debug/life

debug:
	mkdir -p bin
	mkdir -p bin/Debug

client-debug:
	make debug
	$(CCPP) -g -DDEBUG -I $(CPLUS_INCLUDE_PATH) $(SRC_CLIENT) -o bin/Debug/client.out

server-debug:
	make debug
	$(CCPP) -g -DDEBUG -I $(CPLUS_INCLUDE_PATH) $(SRC_SERVER) -o bin/Debug/server.out
	

client:
	$(CCPP) -o2 -I $(CPLUS_INCLUDE_PATH) $(SRC_CLIENT) -o bin/client.out

server:
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
