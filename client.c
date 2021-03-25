#include <stdio.h>        // For printf() and fprintf().
#include <sys/socket.h>   // For socket(), connect(), send(), recv().
#include <arpa/inet.h>    // For sockaddr_in, inet_addr().
#include <stdlib.h>       // For atoi().
#include <string.h>       // For memset(), strstr().
#include <unistd.h>       // For close(), access(), exec().
#include <errno.h>

#define BUFSIZE 1024    // Buffer size.
//#define DEBUG 0         // If defined, print statements will be enabled for debugging.

// For the size of files being sent or received.
struct header
{
  long  data_length;
};


/////////////////////////////////////////////////////////////////////
// Function protoypes.
/////////////////////////////////////////////////////////////////////

// Exits the program with error an message
void Die(const char *message) { perror(message); exit(1); }

// Returns 0 if a string starts with a specfified substring.
int StartsWith(const char *string, const char *substring);

// Prints a message of available commands.
void HelpMessage();

// Returns 0 if the file name specifed exists in the current diretory.
int FileExists(const char *filename);

// Returns the number of substrings in a string.
int NumInputs(char *string);

char *SecondSubstring(char* string);

// Returns 0 if message is sucessfully sent.
int SendMessage(int socket, char *buffer);

// Returns the number of bytes received.
int ReceiveMessage(int socket, char *buffer);

// Handles the request for the ls command.
int HandleRequestLs(int socket, char *cmdbuffer, char *msgbuffer);

// Sends a message to the server and expects a single message back 
// from the server with the result of the command.
int HandleRequest(int socket, char *cmdbuffer, char *msgbuffer);

// Sends a file from the current directory to the server.
int HandleRequestPut(int socket, char *cmdbuffer, char *msgbuffer);

// Gets a file from the server if it exists.
int HandleRequestGet(int socket, char *cmdbuffer, char *msgbuffer);

/////////////////////////////////////////////////////////////////////
// Main.
/////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
  int sockfd;                         // Socket file descripter.
  struct sockaddr_in serveraddress;   // Server address.
  unsigned short serverport;          // Server port.
  char cmdbuffer[BUFSIZE];            // Buffer for command inputs.
  char msgbuffer[BUFSIZE];            // Buffer for send and receive.
  char *serverip;                     // Server IP address (dotted).

  // check for correct # of arguments (1 or 2)
  if ((argc < 2) || (argc > 3)) {
    fprintf(stderr, "Usage: %s <Server IP> [<Port>]\n", argv[0]);
    return -1;
  }

  // Server IP.
  serverip = argv[1];

  // Check if port is specified.
  if (argc == 3) {
    serverport = atoi(argv[2]);
  } else {
    serverport = 7; // 7 is a well know port for echo service.
  }

  #ifdef DEBUG
  printf("[DEBUG] Creating TCP socket...\n");
  #endif

  // Create a TCP socket.
  if ((sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
    Die("Failed to create socket");
  }

  #ifdef DEBUG
  printf("[DEBUG] Created a TCP socket...\n");
  #endif

  // Construct the server address structure.
  memset(&serveraddress, 0, sizeof(serveraddress));
  serveraddress.sin_family      = AF_INET;
  serveraddress.sin_addr.s_addr = inet_addr(serverip);  // Convert to network byte.
  serveraddress.sin_port        = htons(serverport);    // Convert to port.

  #ifdef DEBUG
  printf("[DEBUG] Connecting to server %s...\n", serverip);
  #endif

  // Connect to the server
  if (connect(sockfd, (struct sockaddr *) &serveraddress, sizeof(serveraddress)) < 0) {
    Die("Failed to connect to server");
  }

  #ifdef DEBUG
  printf("[DEBUG] Connected to server... connect()\n");
  #endif

  // Return value variable for functions.
  int rv = 0;
  int loop = 1;

  // Main logic code.
  while (loop) {
    printf("ft> ");
    fgets(cmdbuffer, BUFSIZE, stdin);

    cmdbuffer[strlen(cmdbuffer)-1] = 0;

    // NumInputs replaces the buffer used, so create a new buffer.
    char pstring[BUFSIZE];
    strcpy(pstring, cmdbuffer);

    // Check the number of inputs given by the user.
    rv = NumInputs(pstring);

    #ifdef DEBUG
    printf("[DEBUG] cmdbuffer = '%s'\n", cmdbuffer);
    #endif

    // This big if/else could be changed.
    // 1 command input
    if (rv == 1) {

      #ifdef DEBUG
      printf("[DEBUG] 1 word input\n");
      #endif

      if (strcmp(cmdbuffer, "quit") == 0) {

        #ifdef DEBUG
        printf("--== Sending message '%s' to the server --==\n", cmdbuffer);
        #endif

        // Send the 'quit' message to the server.
        SendMessage(sockfd,cmdbuffer);

        #ifdef DEBUG
        printf("--== Sent message '%s' to the server --==\n", cmdbuffer);
        #endif

        // Exit program.
        loop = 0;
      } else if (strcmp(cmdbuffer, "help") == 0) {
        HelpMessage();
      } else if (strcmp(cmdbuffer, "ls") == 0) {

        #ifdef DEBUG
        printf("[DEBUG] ls command\n");
        #endif

        HandleRequestLs(sockfd, cmdbuffer, msgbuffer);
      } else if (strcmp(cmdbuffer, "clear") == 0) {
        system("clear");
      } else {
        printf("Invalid command.\n");
        HelpMessage();
      } // End of 1 command input.
    } else if (rv == 2) { // 2 command inputs.
      // Basic format validation for option and 1 argument.
      if ((StartsWith(cmdbuffer, "get ") == 0) && strstr(cmdbuffer, " ")) {

        #ifdef DEBUG
        printf("[DEBUG] get <remote-file> command\n");
        #endif

        HandleRequestGet(sockfd, cmdbuffer, msgbuffer);

      } else if ((StartsWith(cmdbuffer, "put ") == 0) && strstr(cmdbuffer, " ")) {

        #ifdef DEBUG
        printf("[DEBUG] put <file-name> command\n");
        #endif

        HandleRequestPut(sockfd, cmdbuffer, msgbuffer);

      } else if ((StartsWith(cmdbuffer, "cd") == 0) && strstr(cmdbuffer, " ")) {

        #ifdef DEBUG
        printf("[DEBUG] cd <directory> command\n");
        #endif

        HandleRequest(sockfd, cmdbuffer, msgbuffer);
      } else if ((StartsWith(cmdbuffer, "mkdir") == 0) && strstr(cmdbuffer, " ")) {

        #ifdef DEBUG
        printf("[DEBUG] mkdir <directory-name> command\n");
        #endif

        HandleRequest(sockfd, cmdbuffer, msgbuffer);
      } else {
        printf("Invalid command.\n");
        HelpMessage();
      } // End of 2 command input.
    } else {
      printf("Invalid command.\n");
      HelpMessage();
    }
  } // End while loop.
} // End main.


/////////////////////////////////////////////////////////////////////
// Function implementations.
/////////////////////////////////////////////////////////////////////

int StartsWith(const char *string, const char *substring) {
  return strncmp(string, substring, strlen(substring));
}

void HelpMessage() {
  printf("Commands are:\n\n");
  printf("ls:\t\t\t\t print a listing of the contents of the current directory\n");
  printf("get <remote-file>:\t\t retrieve the <remote-file> on the server and store it in the current directory\n");
  printf("put <file-name>:\t\t put and store the file from the client machine to the server machine.\n");
  printf("cd <directory-name>:\t\t change the directory on the server\n");
  printf("mkdir <directory-name>:\t\t create a new sub-directory named <directory-name>\n");
}

int FileExists(const char *filename) {
  return access(filename, F_OK);
}

int NumInputs(char *string) {
  // send ls command
  #ifdef DEBUG
  printf("[DEBUG] inside NumInputs. String passed was: '%s'\n", string);
  #endif

  int numsubstrings = 0;
  char *token = strtok(string, " ");

  while (token) {

    #ifdef DEBUG
    printf("[DEBUG] token: %s\n", token);
    #endif

    numsubstrings++;
    token = strtok(NULL, " ");
  }

  return numsubstrings;
}

char *SecondSubstring(char* string) {
  char *token = strtok(string, " ");
  return strtok(NULL, " ");
}

int SendMessage(int socket, char *buffer) {
  #ifdef DEBUG
  printf("Inside SendMessage: '%s'\n", buffer);
  #endif

  // Send command to server
  if (send(socket, buffer, sizeof(char)*BUFSIZE, 0) < 0) {
    Die("Failed to send message");
  }

  #ifdef DEBUG
  printf("Sent message: '%s', successfully\n", buffer);
  #endif

  return 0;
}

int ReceiveMessage(int socket, char *buffer) {

  #ifdef DEBUG
  printf("Inside ReceiveMessage.\n");
  #endif

  int bytesRecvd = 0;

  // Send command to server
  if ((bytesRecvd = recv(socket, buffer, sizeof(char)*BUFSIZE, 0)) <= 0) {
    if(errno == 0) {
      printf("Server is closed, shutting off client.\n");
      exit(1);
    }
    Die("recv() failed.");
  }

  buffer[bytesRecvd] = '\0';

  #ifdef DEBUG
  printf("Received message from server: '%s'\n", buffer);
  #endif

  return bytesRecvd;
}

int HandleRequestLs(int socket, char *cmdbuffer, char *msgbuffer) {

  #ifdef DEBUG
  printf("--== Sent message '%s' to the server --==\n", cmdbuffer);
  #endif

  // Send command to server.
  SendMessage(socket, cmdbuffer);


  #ifdef DEBUG
  printf("[DEBUG] Handling '%s' request to server\n", cmdbuffer);
  #endif

  int bytesRecvd = 0;

  // Read results from server until a null terminator is found.
  bytesRecvd = ReceiveMessage(socket, msgbuffer);

  msgbuffer[bytesRecvd] = '\0';

  // Output the server results.
  printf("%s", msgbuffer);

  // Clear msgbuffer.
  memset(msgbuffer, 0, sizeof(char)*BUFSIZE);

  #ifdef DEBUG
  printf("--== Received entire result of '%s' from the server --==\n", cmdbuffer);
  #endif

  return 0;
}

int HandleRequest(int socket, char *cmdbuffer, char *msgbuffer) {

  #ifdef DEBUG
  printf("--== Sent message '%s' to the server --==\n", cmdbuffer);
  #endif

  // Send command to server.
  SendMessage(socket,cmdbuffer);

  #ifdef DEBUG
  printf("[DEBUG] Handling '%s' request to server\n", cmdbuffer);
  #endif

  int bytesRecvd = 0;

  // Read result from server.
  bytesRecvd = ReceiveMessage(socket, msgbuffer);

  if ((strcmp(msgbuffer, "success") != 0)) {
    // Output the server results.
    printf("%s", msgbuffer);
  }

  // clear msgbuffer
  memset(msgbuffer, 0, sizeof(char)*BUFSIZE);

  #ifdef DEBUG
  printf("--== Received entire result of '%s' from the server --==\n", cmdbuffer);
  #endif

  return 0;
}

int HandleRequestPut(int socket, char *cmdbuffer, char *msgbuffer) {
  // Getting pointer to the file name, which is the 2nd substring
  char *filename = strchr(cmdbuffer, ' ') + 1;

  #ifdef DEBUG
  printf("[DEBUG] Checking if file exists.\n");
  #endif

  if (FileExists(filename) == 0) {
    #ifdef DEBUG
    printf("[DEBUG] File '%s' exists.\n", filename);
    printf("--== Sending message '%s' to the server --==\n", cmdbuffer);
    #endif

    // Send command to server.
    SendMessage(socket, cmdbuffer);

    #ifdef DEBUG
    printf("--== Sent message '%s' to the server --==\n", cmdbuffer);
    printf("[DEBUG] Waiting on recv() for message 'filesize' from server\n");
    #endif

    int bytesRecvd = 0;
    // Check for message "Server ready to receive file" message from server.
    ReceiveMessage(socket, msgbuffer);

    FILE *file;

    if (strcmp(msgbuffer, "filesize") == 0) {
      file = fopen(filename, "rb");

      #ifdef DEBUG
      printf("[DEBUG] Received message from server: '%s'\n", msgbuffer);
      printf("[DEBUG] Server is ready to receive file.\n");
      #endif

      if (file == NULL) {
        printf("Unable to open file '%s'\n", filename);
        return -1;
      }

      fseek(file, 0, SEEK_END);
      unsigned long filesize = ftell(file);

      #ifdef DEBUG
      printf("[DEBUG] %s has size: %lu\n", filename, filesize);
      printf("[DEBUG] Sending filesize to server\n");
      #endif

      // Send file size to server.
      struct header hdr;
      hdr.data_length = filesize;
      if (send(socket, (const char*)(&hdr), sizeof(hdr), 0) < 0) {
        Die("send() failed.\n");
      }

      #ifdef DEBUG
      printf("[DEBUG] Sent filesize to server\n");
      printf("[DEBUG] Waiting for server to be ready to receive messages.\n");
      #endif

      ReceiveMessage(socket, msgbuffer);

      if (strcmp(msgbuffer, "serverReady") != 0) {
          printf("Server not ready\n");
          return -1;
      }

      #ifdef DEBUG
      printf("[DEBUG] Server ready to receive messages\n");
      #endif

      char *buffers = (char*)malloc(sizeof(char)*filesize);
      rewind(file);

      // Store read data into buffer.
      fread(buffers, sizeof(char), filesize, file);

      #ifdef DEBUG
      printf("[DEBUG] Sending file to server.\n");
      #endif

      int sent = 0, n = 0;
      while (sent < filesize) {
        if ((n = send(socket, buffers+sent, filesize-sent, 0)) < 0) {
          Die("send() failed.");
        }

        if (n == -1)
            break;  // ERROR

        sent += n;
      }

      #ifdef DEBUG
      printf("[DEBUG] Sent file to server\n");
      #endif

      // Clean up data.
      fclose(file);
      free(buffers);

      // Receive a message from server indicating the server has succesfully received the file.
      bytesRecvd = ReceiveMessage(socket, msgbuffer);

      // Output result from server
      printf("%s\n", msgbuffer);
    } else {
      printf("Received incorrect message from server. Expected 'filesize' but instead received: '%s'\n", msgbuffer);
      return -1;
    }
  } else {
    printf("File '%s' does not exist in current directory\n", filename);
    return -1;
  }

  // clear msgbuffer
  memset(msgbuffer, 0, sizeof(char)*BUFSIZE);

  #ifdef DEBUG
  printf("--== Server received entire file. --==\n");
  #endif

  return 0;
}

int HandleRequestGet(int socket, char *cmdbuffer, char *msgbuffer) {
  // Send message "Get <file name>"
  #ifdef DEBUG
  printf("[DEBUG] Sending message '%s' to server.\n", cmdbuffer);
  #endif

  // Sending command 'get <file name>'
  SendMessage(socket,cmdbuffer);

  #ifdef DEBUG
  printf("[DEBUG] Sent message '%s' to server.\n", cmdbuffer);
  printf("[DEBUG] Waiting for filesize from server.\n");
  #endif


  struct header hdr;
  hdr.data_length = 0;

  // Receive the size of data from server.
  if(recv(socket, (const char*)(&hdr), sizeof(hdr), 0) <= 0) {
    if(errno == 0) {
      printf("Server is closed, shutting off client.\n");
      exit(1);
    }
    Die("error");
  }

  int filesize = hdr.data_length;

  if (filesize == -1) {
    printf("File does not exist on server. Please try again.\n");
    return 0;
  }

  #ifdef DEBUG
  printf("[DEBUG] Received filesize from server: '%s'\n", msgbuffer);
  #endif

  // File exists on the server. 
  // Send a message to the server to begin sending the file.

  int received = 0, n = 0;
  FILE *file;
  char *filename = strchr(cmdbuffer, ' ') + 1;

  file = fopen(filename, "w");

  // Resize buffer
  char *tempBuffer = calloc(sizeof(char), filesize);

  // Tell server we are ready to receive the file
  if (send(socket, "clientReady", sizeof("clientReady"), 0) < 0) {
    Die("send() failed");
  }

  #ifdef DEBUG
  printf("[DEBUG] Sending clientReady to the server.\n");
  #endif

  while(received < filesize) {
      if((n = recv(socket, tempBuffer+received, filesize-received, 0)) <= 0) {
        if(errno == 0) {
          printf("Server is closed, shutting off client.\n");
          exit(1);
        }
        Die("error");
      }

      if(n == -1)
          break;
      received += n;
  }

  #ifdef DEBUG
  printf("[DEBUG] Received the file from the server.\n");
  #endif

  fwrite(tempBuffer, 1, sizeof(char)*filesize, file);

  // Clean up data.
  fclose(file);
  free(tempBuffer);

  return 0;
}
