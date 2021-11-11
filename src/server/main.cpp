#include <iostream>
#include <fstream>
#include <sstream>
#include <exception>

#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <sys/types.h>  /* Used for calling fork() */
#include <sys/mman.h>   /* Used for mmap()/munmap() */
#include <sys/stat.h>   /* Used for fstat() */
#include <sys/wait.h>   /* Used for waitpid() */
#include <sys/select.h> /* Used for select() */
#include <unistd.h>     /* for close() */
#include <fcntl.h>      /* Needed for mmap()/munmap() */
#include <ctime>        /*get current time for log*/

void DieWithError(std::string errorMessage); /* Error handling function */
void exitWithFailure(int clientSocket, const int returnCode = 1, const std::string msg = ""); /* Terminate process and send error to client */
void HandleTCPClient(int clientSocket, int serverSocket, char* clientIP, int connectTime, int rateReq, int rateTime); /* TCP client handling function */
void ReceiveQRSendURL(int clientSocket, char* clientIP, int connectTime, int rateReq, int rateTime);

#define MAXPENDING 5                         /* Maximum outstanding connection requests */

#define MAX_BYTES_FILE 4000000 //allow files up to 4 megabytes, anything larger is an error

//Default parameter values
#define PORT_DEFAULT 2012
#define NUM_REQ_DEFAULT 3
#define NUM_SEC_DEFAULT 60
#define NUM_USER_DEFAULT 3
#define TIMEOUT_DEFAULT 80

void log(std::string msg, std::string IPaddr){
    //Open file in append mode
    std::ofstream logfile ("adminlog.txt",std::ios_base::app);

    //Write current time
    std::time_t t = std::time(0);
    std::tm* now = std::localtime(&t);

    logfile << (now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' <<  now->tm_mday << " "
     << now->tm_hour <<":"<< now->tm_min <<":"<<now->tm_sec <<" ";

    //Write IP and message
    logfile << IPaddr << " " << msg << std::endl;
    //Close file to prevent issues with threads
    logfile.close();
}
//Logs a server message without associated IP
void logMessage(std::string msg) {
    log(msg,"");
}

int main(int argc, char *argv[]) {    
    int servSock;                    /*Socket descriptor for server */
    int clntSock;                /* Socket descriptor for client */
    sockaddr_in echoServAddr; /* Local address */
    sockaddr_in echoClntAddr; /* Client address */
    unsigned short echoServPort;     /* Server port */
    unsigned int clntLen;            /* Length of client address data structure */ 

    //Parameters
    unsigned int portNum = PORT_DEFAULT; //PORT
    unsigned int reqNum = NUM_REQ_DEFAULT; //RATE
    unsigned int reqSecs = NUM_SEC_DEFAULT;
    unsigned int maxNumUsers = NUM_USER_DEFAULT;// MAX_USERS
    unsigned int connectTime = TIMEOUT_DEFAULT;// TIME_OUT

    //Check each argument for parameters
    int i = 1;
    while(i<argc){
        if(!strcmp(argv[i], "PORT")){
            if(portNum == PORT_DEFAULT){
                portNum = atoi(argv[i+1]);
            }
            i+=2;
        }else if(!strcmp(argv[i], "RATE")){
            if(reqNum == NUM_REQ_DEFAULT){
                reqNum = atoi(argv[i+1]);
            }
            if(reqSecs == NUM_SEC_DEFAULT){
                reqSecs = atoi(argv[i+2]);
            }
            i+=3;
        }else if(!strcmp(argv[i], "MAX_USERS")){
            if(maxNumUsers == NUM_USER_DEFAULT){
                maxNumUsers = atoi(argv[i+1]);
            }
            i+=2;
        }else if(!strcmp(argv[i], "TIME_OUT")){
            if(connectTime == TIMEOUT_DEFAULT){
                connectTime = atoi(argv[i+1]);
            }
            i+=2;
        }else{
            DieWithError("Invalid parameters");
        }
    }
    logMessage("Server startup");

    #ifdef DEBUG
        std::cout << "Created Server:"  <<std::endl;
        std::cout << "Port: " << portNum <<std::endl;
        std::cout << "Rate: " << reqNum << " requests per "<< reqSecs << " seconds" <<std::endl;
        std::cout << "User: " << maxNumUsers << std::endl;
        std::cout << "Timeout: "  << connectTime <<std::endl;
    #endif

    /* Create socket for incoming connections */
    if ((servSock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
     	DieWithError("socket() failed");
    }

	/* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(portNum);      /* Local port */
    
    /* Bind to the local address */
    if (bind (servSock, (sockaddr*) &echoServAddr, sizeof(echoServAddr)) < 0) {
        DieWithError("bind() failed");
    }

    /* Mark the socket so it will listen for incoming connections */
    if (listen (servSock, maxNumUsers + 1) < 0) {
    	DieWithError("listen() failed");
    }

    unsigned int totalClients = 0;
	for (;;) { /* Run forever */

        #ifdef DEBUG
        printf("Begin accepting clients.\n");
        #endif

        /* Set the size of the in-out parameter */
        clntLen = sizeof(echoClntAddr);        /* Wait for a client to connect */
        if ((clntSock = accept(servSock, (sockaddr*) &echoClntAddr, &clntLen)) < 0) {
            DieWithError("accept() failed");
        }

        #ifdef DEBUG
        printf("Accpetted client %s (%d)\n", inet_ntoa(echoClntAddr.sin_addr), clntSock);
        fflush(stdout);
        #endif

        // Check to see if any child processes have terminated
        int status;
        pid_t waitOutput = waitpid(-1, &status, WNOHANG);

        // Output error if waiting failed
        if(totalClients > 0 && waitOutput == -1) {
            std::cerr << "waitpid() failed." << std::endl;
            exit(-1);
        }

        // Decrease client counter if the child process exited
        if(WIFEXITED(status)) {
            totalClients--;
        }

        // If the maximum user limit has not been reached
        // Handle the new client
        if(totalClients < maxNumUsers) {
            /* clntSock is connected to a client! */
            printf("Handling client %s (%d)\n", inet_ntoa(echoClntAddr.sin_addr), clntSock);
            log("Connected to client",inet_ntoa(echoClntAddr.sin_addr));
            HandleTCPClient(clntSock, servSock, inet_ntoa(echoClntAddr.sin_addr), connectTime, reqNum, reqSecs);
            totalClients++;
        }
        // Otherwise, send the client a failure code
        else {
            std::cerr << "Maximum users reached." << std::endl;
            int returnCode = 1;
            int size = 0;
            // Send return code as failure
            if (send (clntSock, (char*) &returnCode, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
                DieWithError("send() sent a different number of bytes than expected (failure return code)");
            }    
            // Send zero length
            if (send (clntSock, (char*) &size, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
                DieWithError("send() sent a different number of bytes than expected (failure str length)");
            }
            if(close(clntSock) != 0) {
                DieWithError("Could not close client socket.");
            }
        }
     }
     /* NOT REACHED */
} 

void DieWithError(std::string errorMessage) {
    std::cerr << errorMessage << std::endl;
    exit(-1);
}

void exitWithFailure(int clientSocket, const int returnCode, const std::string msg) {
    const uint32_t size = msg.size();

    std::cerr << "Exiting with failure: " << msg << std::endl;

    // Send return code as failure
    if (send (clientSocket, (char*) &returnCode, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
        DieWithError("send() sent a different number of bytes than expected (failure return code)");
    }    

	// Send size of message
    if (send (clientSocket, (char*) &size, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
        DieWithError("send() sent a different number of bytes than expected (failure str length)");
    }

    // Send message
    if (send (clientSocket, msg.c_str(), size, 0) != size) {
        DieWithError("send() sent a different number of bytes than expected (failure str length)");
    }


    if(close(clientSocket) != 0) {
        DieWithError("Could not close client socket.");
    }
    exit(-1);
}

void DisconnectClientThread() {

}

void HandleTCPClient(int clientSocket, int serverSocket, char* clientIP, int connectTime, int rateReq, int rateTime) {
    pid_t processID = fork();
    //Used for disconnecting clients 
    time_t timer = time(NULL);
    
    // Kill process if an error occurred with fork()
    if(processID == -1) {
        DieWithError("fork() failed when handling the incoming client");
    }
    // Continue process clients if the process is the parent
    else if(processID != 0) {
        // Close client socket on parent process
        if(close(clientSocket) != 0) {
            DieWithError("Could not close client socket on parent.");
        }
        return;
    }
    // Close parent socket on child process
    if(close(serverSocket) != 0) {
        DieWithError("Could not close server socket on parent.");
    }

    // Catch a TerminateClientExcept just in case:
    //  a timeout occurs
    //  the user has closed their sockets ( which means recv() has return 0 I believe)
    //  Other conditions that require leaving the while loop

    int currRecvInRate;
    std::time_t prevRecvTime = std::time(0);

    while(true) {    
        // Setup a timeval for the timeout settings
        timeval timeoutSettings;
        timeoutSettings.tv_sec = connectTime;
        timeoutSettings.tv_usec = 0;

        fd_set socketSet;
        FD_ZERO(&socketSet);
        // Add client socket to the socket set
        FD_SET(clientSocket, &socketSet);

        // Handle any events that occur on the socket set
        // Consider if any timeout has occurred while waiting for the client
        int eventOutput = select(clientSocket+1, &socketSet, NULL, NULL, &timeoutSettings);

        if(!FD_ISSET(clientSocket, &socketSet)) {
            log("Client timed out", clientIP);
            exitWithFailure(clientSocket, 2, "Timeout occured, disconnected for inactivity.");
        }

        // Confirm that a rate limit is not occurring
        // TODO Still need to work on rate limiting
        std::time_t currRecvTime = std::time(0);

        // Handle the client as the child process
        const int lenBytesNeeded = sizeof(uint32_t);
        char lenBuffer[lenBytesNeeded];   /* Buffer for receiving file length */
        int lenBytesReceived = 0;
        while (lenBytesReceived < lenBytesNeeded) {
            int bytesRcvd;
            if ((bytesRcvd = recv(clientSocket, lenBuffer + lenBytesReceived, lenBytesNeeded, 0)) < 0) {
                std::cout << "Failed to recv() " << bytesRcvd << std::endl;
                exitWithFailure(clientSocket);
            }
            // Socket closed on the other end
            else if(bytesRcvd == 0) {
                // TODO client has disconnected
                std::cout << "Client disconnected!@!!!" << std::endl;
                exit(1);
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

        //Process failure if file is too large
        if(fileLength>MAX_BYTES_FILE){
            #ifdef DEBUG
            std::cout << "File length too long" << std::endl;
            #endif
            log("Client request file exceeds allowable size", clientIP);
            exitWithFailure(clientSocket);
        }

        // Then receive the file itself
        char fileBuffer[fileLength];   /* Buffer for receiving file */
        int fileBytesReceived = 0;
        while (fileBytesReceived < fileLength)
        {
            int bytesRcvd;
            if ((bytesRcvd = recv(clientSocket, fileBuffer + fileBytesReceived, fileLength, 0)) < 0) {
                std::cout << "Failed to recv() again" << std::endl;
                exitWithFailure(clientSocket);
            }
            // 
            else if(bytesRcvd == 0) {
                // TODO client has disconnected
                std::cout << "Client disconnected again!@!!!" << std::endl;
                exit(1);
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
        // Compute file name for the URL output file
        fileNameStream << ".out";
        const std::string outputFileName = fileNameStream.str();

        // Save file buffer to the file system
        std::ofstream fileStream;
        fileStream.open(fileName, std::ios::binary | std::ios::out);
        fileStream.write(fileBuffer, fileLength);
        fileStream.close();

        // Convert QR code image to URL with jar file
        std::stringstream javaCmdStream;
        javaCmdStream << "java -cp javase.jar:core.jar com.google.zxing.client.j2se.CommandLineRunner " << fileName
                    << " > " << outputFileName;
        const std::string javaCmd = javaCmdStream.str();
        system(javaCmd.c_str());

        // Load URL conversion output file descriptor
        int outputFileDesc;
        if ((outputFileDesc = open(outputFileName.c_str(), O_RDONLY)) < 0) {
            std::cout << "Failed to open output file" << std::endl;
            exitWithFailure(clientSocket);
        }
        // Load output file stats
        struct stat outputFileStats;
        if(fstat(outputFileDesc, &outputFileStats) < 0) {
            std::cout << "Failed to open output file desc" << std::endl;
            exitWithFailure(clientSocket);
        }
        // Load output file into memory
        const uint32_t outFileSize = outputFileStats.st_size;
        char* outFileBuffer;
        if ((outFileBuffer = (char *) mmap(NULL, outFileSize, PROT_READ, MAP_SHARED, outputFileDesc, 0)) == (char *) -1) {
            std::cout << "Failed to map file to memory" << std::endl;
            exitWithFailure(clientSocket);
        }

        #ifdef DEBUG
        std::cout << "Output File Size: " << outFileSize << std::endl;
        std::cout << "Output File: " << std::endl << outFileBuffer << std::endl;
        #endif

        // Send return code as success
        const uint32_t returnCode = 0;
        if (send(clientSocket, (char*) &returnCode, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
            DieWithError("send() sent a different number of bytes than expected (return code)");
        }    
        // Send output file length
        if (send(clientSocket, (char*) &outFileSize, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
            DieWithError("send() sent a different number of bytes than expected (string length)");
        }
        // Send URL data
        if (send(clientSocket, outFileBuffer, outFileSize, 0) != outFileSize) {
            DieWithError("send() sent a different number of bytes than expected (URL data)");
        }

        log("QR code processed",clientIP);

        // Unmap output file from memory
        if(munmap(outFileBuffer, outFileSize) < 0) {
            DieWithError("Could not unmap output file.");
        }
        // Close output file descriptor
        if(close(outputFileDesc) != 0) {
            DieWithError("Could not close output file.");
        }

        // Delete QR code image file
        if(std::remove(fileName.c_str()) != 0) {
            DieWithError("Could not delete QR code image.");
        }
        // Delete URL output file
        if(std::remove(outputFileName.c_str()) != 0) {
            DieWithError("Could not delete saved QR code image.");
        }
    }

    // Close socket with client
    if(close(clientSocket) != 0) {
        DieWithError("Could not close client socket.");
    }

    std::cout << "Terminated client process (" << clientSocket << ")" << std::endl;
    exit(0);
}

void ReceiveQRSendURL(int clientSocket, char* clientIP, int connectTime, int rateReq, int rateTime) {

}