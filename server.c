#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/syscall.h>

#define MAX_BUF 1024
#define PORT 6666

void handleSigInt(int);
void cleanUp();
void handlels(char*);
void clearBuffer(char*);
int fileExists(const char*);

int myListenSocket, clientSocket;
struct header hdr;
FILE *file;

struct header
{
    long    data_length;
};

int main()
{
    
    char* logName[MAX_BUF], ip[MAX_BUF], buffer[MAX_BUF] , str[MAX_BUF];
    int  port, i, addrSize, bytesRcv;
    struct sockaddr_in  myAddr, clientAddr;
    socklen_t len;
    
    port = PORT;
    
    printf("--== Server Running --==\n");
    
    printf("--== Creating Socket --==");
    myListenSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (myListenSocket < 0) {
        printf("Error: Server couldn't open socket\n");
        exit(-1);
    }
    
    
    printf("--== Setting up Server Address --==\n");
    memset(&myAddr, 0, sizeof(myAddr));
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myAddr.sin_port = htons(port);
    
    
    printf("--== Server Binding to socket --==\n");
    i = bind(myListenSocket, (struct sockaddr *) &myAddr, sizeof(myAddr));
    if (i < 0) {
        printf("Error: Server couldn't bind socket\n");
        exit(-1);
    }
    
    
    printf("--== Server Listening to Socket --==\n");
    i = listen(myListenSocket, 5);
    if (i < 0) {
        printf("Error: Server couldn't listen\n");
        exit(-1);
    }
    
    
    while (1){
    printf("--== Server waiting for connection request --==\n");
    addrSize = sizeof(clientAddr);
    clientSocket = accept(myListenSocket, (struct sockaddr *) &clientAddr,  &addrSize);
    if (clientSocket < 0) {
        printf("Error: Server couldn't accept the connection\n");
        exit(-1);
    }
    
    printf("--== Connection Successful, Server Ready to read Messages! --==\n");
    
    
    
    while ((bytesRcv = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0){
        
        if (buffer[0] == 'l' && buffer[1] == 's') {
            
            handlels(buffer);
            send(clientSocket, buffer, sizeof(buffer), 0);
            
            //handle cd
        } else if (buffer[0] == 'c' && buffer[1] == 'd') {
            printf("received cd command\n");
            
            char directory[MAX_BUF] = {0};
            int i;
            
            for (i = 3; buffer[i] != '\0'; i++){
                directory[i-3] = buffer[i];
            }
            
            if (chdir( directory) == 0 ){
                printf("cd success\n");
                send(clientSocket, "success", sizeof("success"), 0);
            } else {
                printf("cd fail\n");
                send(clientSocket, "fail\0", sizeof("fail\0"), 0);
            }
            
            //handle get
        } else if (buffer[0] == 'g' && buffer[1] == 'e' && buffer[2] == 't'){
            printf("received get command \n");
            
            char fileName[MAX_BUF] = {0};
            int i;
            
            for (i = 4; buffer[i] != '\0'; i++){
                fileName[i-4] = buffer[i];
            }
            
            struct header hdr;

            if (!fileExists(fileName)) {
                file = fopen(fileName, "rb");

                fseek(file, 0, SEEK_END);
                unsigned long filesize = ftell(file);

                // send header to client
                hdr.data_length = filesize;
                send(clientSocket, (const char*)(&hdr), sizeof(hdr), 0);

                printf("Sent size of file to client");

                // recv msg from client to begin sending file
                recv(clientSocket, buffer, sizeof(buffer),0);

                if (strcmp(buffer, "clientReady") != 0) {
                  printf("client not ready\n");
                  return -1;
                }


                // client ready to receive data
                char *buffers = (char*)malloc(sizeof(char)*filesize);
                rewind(file);
                // store read data into buffer
                fread(buffers, sizeof(char), filesize, file);
                // send buffer to client

                #ifdef DEBUG
                printf("[DEBUG] Sending file to server.\n");
                #endif

                int sent = 0, n = 0;
                while (sent < filesize) {
                n = send(clientSocket, buffers+sent, filesize-sent, 0); // error checking is done in actual code and it sends perfectly
                if (n == -1)
                    break;  // ERROR

                sent += n;
                }

                printf("Sent file to server\n");


                fclose(file);
                free(buffers);
            } else {
                hdr.data_length = -1;
                send(clientSocket, (const char*)(&hdr), sizeof(hdr), 0);
            }
            
            //handle put
        } else if ( buffer[0] == 'p' && buffer[1] == 'u' && buffer[2] == 't'){
            printf("received put command \n");
            
            
            char fileName[MAX_BUF] = {0};
            int i;
            char c;
            int k=0;
            
            for (i = 4; buffer[i] != '\0'; i++){
                fileName[i-4] = buffer[i];
            }
            
            file = fopen(fileName, "w");
            send(clientSocket, "filesize", sizeof("filesize"), 0);
            
            hdr.data_length = 0;
            // receive header
            recv(clientSocket, (const char*)(&hdr), sizeof(hdr), 0);
            printf("data_length = %d\n", hdr.data_length);
            // resize buffer
            char *tempBuffer = calloc(sizeof(char), hdr.data_length);
            // receive data
            send(clientSocket, "serverReady", sizeof("serverReady"), 0);

            int filesize = hdr.data_length;
            int received = 0, n = 0;
            while(received < filesize) {
                n = recv(clientSocket, tempBuffer+received, filesize-received, 0);
                if(n == -1)
                    break;
                received += n;
            }
            fwrite(tempBuffer, 1, sizeof(char)*hdr.data_length, file);
            
            printf("finished writing\n");
            fclose(file);
            free(tempBuffer);
            printf("finished sending\n");
            send(clientSocket, "success", sizeof("success"), 0);
            
            //handle mkdir
        } else if ( buffer[0] == 'm' && buffer[1] == 'k' && buffer[2] == 'd'&& buffer[3] == 'i'&& buffer[4] == 'r') {
            printf("received mkdir command \nq  ");
            
            char directory[MAX_BUF] = {0};
            int i;
            
            for (i = 6; buffer[i] != '\0'; i++){
                directory[i-6] = buffer[i];
            }
            
            /** making directory */
            if (mkdir(directory, 0700) == 0) {
                printf("mkdir success\n");
                send(clientSocket, "success", sizeof(buffer), 0);
            } else {
                printf("mkdir fail\n");
                send(clientSocket, "fail", sizeof(buffer), 0);
            }
        } else {
            printf("getting this string\n %s\n", buffer);
        }
        
        clearBuffer(buffer);
        }
        close(clientSocket);
    }
    
    return 0;
}


/*         Name: cleanUp
 *  Description: closes the listening socket, client socket, and the log file
 *   Parameters: none
 *       Return: void
 */
void cleanUp(){
    /* Closing sockets and log file */
    close(myListenSocket);
    close(clientSocket);
    
    printf("clean up finished, terminating process\n");
    exit(0);
}

/*         Name: ls
 *  Description: fills buffer with output of ls command
 *   Parameters: char array buffer
 *       Return: void
 */
void handlels(char* buffer){
    printf("received ls command\n");
    file = popen("ls", "r");
    char c;
    int k = 0;
    while ((c = fgetc(file)) != EOF) {
        buffer[k] = c;
        k++;
    }
    buffer[k] = 0;
    fflush(file);
    pclose(file);
}

/*         Name: clearBuffer
 *  Description: clears buffer using memset
 *   Parameters: char array buffer
 *       Return: void
 */
void clearBuffer(char* buffer){
    memset(buffer, '\0', sizeof(char)*MAX_BUF);
}

/*         Name: fileExists
 *  Description: returns an intenger based on existence of file
 *   Parameters: char*
 *       Return: int
 */
int fileExists(const char *filename) {
    return access(filename, F_OK);
}

/*         Name: handleSigInt
 *  Description: handles the SIGINT signal and calls the cleanUp function
 *   Parameters: int
 *       Return: void
 */
void handleSigInt(int i){
    printf("handling SIGINT, calling cleanUp function\n");
    cleanUp();
    exit(0);
}