#include <iostream>

#include <stdio.h>          /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>   /* for sockaddr_in and inet_addr() */
#include <stdlib.h>        /* for atoi() and exit() */
#include <string.h>       /* for memset() */
#include <unistd.h>      /* for close() */ 
#include <vector>		/*Holds file data*/
#include <iostream>
#include <fstream>
 
#define RCVBUFSIZE 32   /* Size of receive buffer */

void DieWithError(std::string errorMessage);  /* Error handling function */ 

int main(int argc, char *argv[])
{
    int sock;                         /* Socket descriptor */
    struct sockaddr_in echoServAddr;  /* Echo server address */
    unsigned short echoServPort;       /* Echo server port */
    char *servIP;                                        /* Server IP address (dotted quad) */
    char *fileName;                                 /* String representing file name */
    char echoBuffer[RCVBUFSIZE];      /* Buffer for echo string */
    uint32_t echoStringLen;               /* Length of string to echo - 4 byte int*/
    int bytesRcvd, totalBytesRcvd;          /* Bytes read in single recv()                                         						and total bytes read */

    if ((argc < 3) || (argc > 4))    /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage: %s <Server IP> <File Name> [<Echo Port>]\n",
                argv[0]);
        exit(1);
    }
    servIP = argv[1];             /* First arg: server IP address (dotted quad) */
    fileName = argv[2];         /* Second arg: file name */

	if (argc == 4)
        echoServPort = atoi(argv[3]);    /* Use given port, if any */ 
    else
        echoServPort = 7;       /* 7 is the well-known port for the echo service */

	///// Read file size and data into file_size_in_byte and data //// Some code taken from https://stackoverflow.com/questions/4373047/read-text-file-into-char-array-c-ifstream
	std::ifstream infile;
	infile.open(fileName, std::ios::binary); //open the file
	//End method if no valid file found
	if(!infile.is_open())
		DieWithError("Could not read file");
	
	//Process file
	infile.seekg(0, std::ios::end);
	uint32_t file_size_in_byte = infile.tellg();
	std::vector<char> data; // used to store text data
	data.resize(file_size_in_byte);
	infile.seekg(0, std::ios::beg);
	//Read into data
	infile.read(&data[0], file_size_in_byte);
	//close the file
	infile.close();
	/////////

    /* Create a reliable, stream socket using TCP */
    if ((sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)   
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));        /* Zero out structure */     
    echoServAddr.sin_family         = AF_INET;                     /* Internet address family */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);        /* Server IP address */
    echoServAddr.sin_port             = htons(echoServPort);   /* Server port */ 
    /* Establish the connection to the echo server */
    if (connect (sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("connect() failed");

	/* Send the file length to the server, 4 bytes long*/
    if (send (sock, &file_size_in_byte, 4, 0) != 4)
        DieWithError("send() sent a different number of bytes than expected (file length)");

    /* Send the string to the server */
    if (send (sock, (char *)(&data[0]), file_size_in_byte, 0) != file_size_in_byte)
        DieWithError("send() sent a different number of bytes than expected (file)");

    /* Receive the same string back from the server */
    totalBytesRcvd = 0;	      /* Count of total bytes received     */
                   /* Setup to print the echoed string */ 

	// First receive the string length of the return msg
    int lenBytesNeeded = sizeof(uint32_t);

	int lenBytesReceived = 0;
    while (lenBytesReceived < lenBytesNeeded)
    {
        int bytesRcvd;
        if ((bytesRcvd = recv(sock, echoBuffer + lenBytesReceived, lenBytesNeeded, 0)) <= 0)
            DieWithError("recv() failed or connection closed prematurely"); 
        lenBytesReceived += bytesRcvd;   /* Keep tally of total bytes */ 
    }

    uint32_t code;
    memcpy(&code, echoBuffer, lenBytesNeeded);
	#ifdef DEBUG
	printf("Received server code: \n"); 
	printf("%u",code);
	#endif

    lenBytesReceived = 0;
    while (lenBytesReceived < lenBytesNeeded)
    {
        int bytesRcvd;
        if ((bytesRcvd = recv(sock, echoBuffer + lenBytesReceived, lenBytesNeeded, 0)) <= 0)
            DieWithError("recv() failed or connection closed prematurely"); 
        lenBytesReceived += bytesRcvd;   /* Keep tally of total bytes */ 
    }

    uint32_t strLength;
    memcpy(&strLength, echoBuffer, lenBytesNeeded);
    char urlBuffer[strLength + 1];
	#ifdef DEBUG
	printf("\nReceived file length: \n"); 
	printf("%u\n",strLength);
    #endif

    totalBytesRcvd = 0;
    while (totalBytesRcvd < strLength)
    {
        /* Receive up to the buffer size bytes from the sender */
        if ((bytesRcvd = recv(sock, urlBuffer + bytesRcvd, strLength, 0)) <= 0) {
            DieWithError("recv() failed or connection closed prematurely");
        }
        totalBytesRcvd += bytesRcvd;   /* Keep tally of total bytes */ 
    }

    // Null terminate the URL buffer string
    urlBuffer[strLength] = '\0';
    
    // Output URL file
    std::cout << urlBuffer << std::endl;

    close(sock);
    exit(0);
 } 

void DieWithError(std::string errorMessage) {
    std::cerr << errorMessage << std::endl;
    exit(-1);
} 
