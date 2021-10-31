#include <iostream>

#include <stdio.h>          /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>   /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>       /* for atoi() and exit() */
#include <string.h>       /* for memset() */
#include <sys/types.h> /* Used for calling fork() */
#include <unistd.h>      /* for close() */

#define RCVBUFSIZE 32   /* Size of receive buffer */

#define MAXPENDING 5             /* Maximum outstanding connection requests */
void DieWithError(std::string errorMessage);     /* Error handling function */
void HandleTCPClient(int clientSocket);       /* TCP client handling function */

int main(int argc, char *argv[])
{    int servSock;                                   /*Socket descriptor for server */
      int clntSock;                                   /* Socket descriptor for client */
      struct sockaddr_in echoServAddr; /* Local address */
      struct sockaddr_in echoClntAddr; /* Client address */
      unsigned short echoServPort;        /* Server port */
      unsigned int clntLen;                     /* Length of client address data structure */ 

      if (argc != 2)     /* Test for correct number of arguments */
     { 
           fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
            exit(1);   
     }
     echoServPort = atoi(argv[1]);        /* First arg:  local port */
    /* Create socket for incoming connections */
      if ((servSock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) 
     	DieWithError("socket() failed");   
	/* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));         /* Zero out structure */
    echoServAddr.sin_family         = AF_INET;                      /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port             = htons(echoServPort);     /* Local port */
    
    /* Bind to the local address */
   if (bind (servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
	DieWithError("bind() failed");

    /* Mark the socket so it will listen for incoming connections */
    if (listen (servSock, MAXPENDING) < 0)
    	DieWithError("listen() failed");
	for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        clntLen = sizeof(echoClntAddr);        /* Wait for a client to connect */
        if ((clntSock = accept (servSock, (struct sockaddr *) &echoClntAddr, &clntLen))
             < 0)            	DieWithError("accept() failed");

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

void HandleTCPClient(int clientSocket) {
    pid_t processID = fork();
    
    if(processID == -1) {
        DieWithError("fork() failed when handling the incoming client");
    }

    // Continue process clients if the process is the parent
    else if(processID != 0) {
        return; // 
    }

    // Handle the client as the child process
    char echoBuffer[RCVBUFSIZE];      /* Buffer for echo string */

    // First receive the string length
    int lenBytesNeeded = sizeof (uint32_t);

    int lenBytesReceived = 0;
    while (lenBytesReceived < lenBytesNeeded)
    {
        int bytesRcvd;
        if ((bytesRcvd = recv(clientSocket, echoBuffer + lenBytesReceived, lenBytesNeeded, 0)) <= 0)
            DieWithError("recv() failed or connection closed prematurely"); 
        lenBytesReceived += bytesRcvd;   /* Keep tally of total bytes */
        
        #ifdef DEBUG
        std::cout << "Receiving str length: " << lenBytesReceived << " bytes received" << std::endl;
        #endif
    }

    uint32_t strLength;
    memcpy(&strLength, echoBuffer, lenBytesNeeded);

    #ifdef DEBUG
    std::cout << "Str length is: " << strLength << std::endl;
    #endif

    // Then receive the string itself
    int strBytesReceived = 0;
    while (strBytesReceived < strLength)
    {
        int bytesRcvd;
        if ((bytesRcvd = recv(clientSocket, echoBuffer + strBytesReceived, strLength, 0)) <= 0)
            DieWithError("recv() failed or connection closed prematurely"); 
        strBytesReceived += bytesRcvd;   /* Keep tally of total bytes */
        echoBuffer[bytesRcvd] = '\0';  /* Terminate the string! */

        #ifdef DEBUG
        std::cout << "Receiving str bytes: " << strBytesReceived << " bytes received" << std::endl;
        #endif
    }

    #ifdef DEBUG
    std::cout << "Receiving str: " << echoBuffer << std::endl;
    #endif

    // Then send the string back to the client
    /* Send the string to the server */
    if (send (clientSocket, echoBuffer, strLength, 0) != strLength)
        DieWithError("send() sent a different number of bytes than expected (string)");

    std::cout << "Handled client (" << clientSocket << ")" << std::endl;
    exit(0);
}

