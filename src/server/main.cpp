#include <iostream>
#include <fstream>
#include <sstream>

#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <sys/types.h>  /* Used for calling fork() */
#include <unistd.h>     /* for close() */


#define MAXPENDING 5                         /* Maximum outstanding connection requests */
void DieWithError(std::string errorMessage); /* Error handling function */
void HandleTCPClient(int clientSocket);      /* TCP client handling function */
void exitWithFailure(int clientSocket);

int main(int argc, char *argv[]) {    
    int servSock;                    /*Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned short echoServPort;     /* Server port */
    unsigned int clntLen;            /* Length of client address data structure */ 

    if (argc != 2) {    /* Test for correct number of arguments */
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);   
    }
    echoServPort = atoi(argv[1]);        /* First arg:  local port */
    /* Create socket for incoming connections */
    if ((servSock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
     	DieWithError("socket() failed");
    }

	/* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Local port */
    
    /* Bind to the local address */
    if (bind (servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) {
        DieWithError("bind() failed");
    }

    /* Mark the socket so it will listen for incoming connections */
    if (listen (servSock, MAXPENDING) < 0) {
    	DieWithError("listen() failed");
    }
	for (;;) { /* Run forever */
        /* Set the size of the in-out parameter */
        clntLen = sizeof(echoClntAddr);        /* Wait for a client to connect */
        if ((clntSock = accept (servSock, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0) {
            DieWithError("accept() failed");
        }

        /* clntSock is connected to a client! */
        printf("Handling client %s (%d)\n", inet_ntoa(echoClntAddr.sin_addr), clntSock);
        HandleTCPClient(clntSock);
     }
     /* NOT REACHED */
} 

void DieWithError(std::string errorMessage) {
    std::cerr << errorMessage << std::endl;
    exit(-1);
}

void exitWithFailure(int clientSocket) {
    const uint32_t returnCode = 1;
    const uint32_t size = 0;

    // Send return code as failure
    if (send (clientSocket, (char*) &returnCode, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
        DieWithError("send() sent a different number of bytes than expected (string length)");
    }    

	// Send zero length
    if (send (clientSocket, (char*) &size, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
        DieWithError("send() sent a different number of bytes than expected (string length)");
    }

    if(close(clientSocket) != 0) {
        DieWithError("Could not close client socket.");
    }
    exit(-1);
}

void HandleTCPClient(int clientSocket) {
    pid_t processID = fork();
    
    // Kill process if an error occurred with fork()
    if(processID == -1) {
        DieWithError("fork() failed when handling the incoming client");
    }
    // Continue process clients if the process is the parent
    else if(processID != 0) {
        return;
    }

    // Handle the client as the child process
    const int lenBytesNeeded = sizeof (uint32_t);
    char lenBuffer[lenBytesNeeded];   /* Buffer for receiving file length */
    int lenBytesReceived = 0;
    while (lenBytesReceived < lenBytesNeeded) {
        int bytesRcvd;
        if ((bytesRcvd = recv(clientSocket, lenBuffer + lenBytesReceived, lenBytesNeeded, 0)) <= 0) {
            exitWithFailure(clientSocket);
        }
        lenBytesReceived += bytesRcvd;   /* Keep tally of total bytes */
        
        #ifdef DEBUG
        std::cout << "Receiving file length: " << lenBytesReceived << " bytes received" << std::endl;
        #endif
    }

    // Save the file length from the received bytes buffer
    uint32_t fileLength = *(uint32_t*)lenBuffer;
    #ifdef DEBUG
    std::cout << "File length is: " << fileLength << std::endl;
    #endif

    // Then receive the file itself
    char fileBuffer[fileLength];   /* Buffer for receiving file */
    int fileBytesReceived = 0;
    while (fileBytesReceived < fileLength)
    {
        int bytesRcvd;
        if ((bytesRcvd = recv(clientSocket, fileBuffer + fileBytesReceived, fileLength, 0)) <= 0) {
            exitWithFailure(clientSocket);
        }
        fileBytesReceived += bytesRcvd;   /* Keep tally of total bytes */

        #ifdef DEBUG
        std::cout << "Receiving file data: " << fileBytesReceived << " bytes received" << std::endl;
        #endif
    }

    // Compute file name for image file
    std::stringstream fileNameStream;
    fileNameStream << "QR_Img_" << clientSocket << ".bin";
    const std::string fileName = fileNameStream.str();

    // Save file buffer to the file system
    std::ofstream fileStream;
    fileStream.open(fileName, std::ios::binary | std::ios::out);
    fileStream.write(fileBuffer, fileLength);
    fileStream.close();

    // // Convert QR code image to URL with jar file
    // std::stringstream javaCmdStream;
    // javaCmdStream << "java -cp javase.jar:core.jar com.google.zxing.client.j2se.CommandLineRunner " << fileName
    //               << " > " << fileName << ".out";
    // const std::string javaCmd = javaCmdStream.str();
    // system(javaCmd)


    const uint32_t returnCode = 0;
    uint32_t urlLength = 0;
    char* URL;

    // Send return code as success
    if (send (clientSocket, (char*) &returnCode, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
        DieWithError("send() sent a different number of bytes than expected (string length)");
    }    
	// Send URL length
    if (send (clientSocket, (char*) &urlLength, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
        DieWithError("send() sent a different number of bytes than expected (string length)");
    }
	// Send URL data
    if (send (clientSocket, (char*) URL, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
        DieWithError("send() sent a different number of bytes than expected (string length)");
    }

    // Close socket with client
    if(close(clientSocket) != 0) {
        DieWithError("Could not close client socket.");
    }

    std::cout << "Handled client (" << clientSocket << ")" << std::endl;
    exit(0);
}

