# Project 1: Quick Response Codes

## By Will Engdahl & Matthew Spofford

The project can be compiled using `make` or `make all`. There are also other Makefile rules for debugging purposes that can be found in `Makefile`.

To run the project, the `server.out` and `client.out` executables can be located in the `bin` directory that is created during the compilation process. The necessary .JAR files are also copied into the `bin` directory for convenience in running the server. For testing purposes, please use the QR code images provided in the `qr-codes` directory.

Options can be entered in any order, enter keyword (PORT,RATE MAX_USERS, or TIMEOUT) followed by the values. Example: `./server.out RATE 4 50 PORT 3021` sets port to 3021 and rate to 4 requests per 50 seconds.

The admin log is saved in the local directory as adminlog.txt.

Current TODOS (I take the paramters, but nothing else is done with them yet):
- Rate limiting
- Connection timeout
- Admin log (method created, must call it where needed)
- Return codes