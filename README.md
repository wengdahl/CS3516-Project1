# Project 1: Quick Response Codes

## By Will Engdahl & Matthew Spofford

The project can be compiled using `make` or `make all`. There are also other Makefile rules for debugging purposes that can be found in `Makefile`.

To run the project, the `server.out` and `client.out` executables can be located in the `bin` directory that is created during the compilation process. The necessary .JAR files are also copied into the `bin` directory for convenience in running the server. For testing purposes, please use the QR code images provided in the `qr-codes` directory.

Options can be entered in any order, enter keyword (PORT,RATE MAX_USERS, or TIMEOUT) follwoed by the values. Example: `./server.out RATE 4 50 PORT 3021` sets port to 3021 and rate to 4 requests per 50 seconds. 

Current TODOS (I take the paramters, but nothing else is done with them yet):
- Rate limiting
- Max users
- Connection timeout
- Admin log
- Return codes
- Size limit for file (return code error) - DONE
- Only process specified size (I think this might be done already)
- Threads for specified number of conenctions
