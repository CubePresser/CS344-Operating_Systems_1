//THIS PROGRAM WILL GIVE DATA TO THE SERVER THROUGH STDIN TO BE ENCRYPTED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

//Prototypes
void error(const char*, int);
void fillAddrStruct(struct sockaddr_in*, struct hostent*, int*, char*[]);
void setSocket(int*, struct sockaddr_in*);
void connectServer(struct sockaddr_in*, int*);
void getInput();
void sendMessage(int*, FILE**);
void getMessage(int*);
void openFiles(FILE**, FILE**, char*[]);
void closeFiles(FILE**, FILE**);
void checkFiles(FILE**, FILE**, char*[]);

int main(int argc, char* argv[])
{
    //Initialize necessary variables
    int socketFD, portNumber;
    struct sockaddr_in serverAddress;
    struct hostent* serverHostInfo;
    FILE *inputFD, *keyFD;

    //Check usage
    if(argc < 4)
    {
        fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]);
        exit(0);
    }
    else
    {
        //Open plaintext and key file to check if they are valid files for this program
        openFiles(&inputFD, &keyFD, argv);
        //Check validity
        checkFiles(&inputFD, &keyFD, argv);
        //Close files so that file pointer is reset
        closeFiles(&inputFD, &keyFD);

        //Set up the server address struct
        fillAddrStruct(&serverAddress, serverHostInfo, &portNumber, argv);

        //Set up socket
        setSocket(&socketFD, &serverAddress);

        //Connect to server
        connectServer(&serverAddress, &socketFD);

        //Open plaintext and key for reading from
        openFiles(&inputFD, &keyFD, argv);
        sendMessage(&socketFD, &inputFD); //Send the plaintext file
        sendMessage(&socketFD, &keyFD); //Send the key file

        //Get message from the server
        getMessage(&socketFD);

        //Close socket
        closeFiles(&inputFD, &keyFD);
        close(socketFD);

    }

    return 0;
}

/*************************************************
 * Function: error
 * Description: prints specified error message and exits with specified code
 * Params: error message, exit code
 * Returns: none
 * Pre-conditions: valid exit code given
 * Post-conditions: exits process and prints error message to stderr
 * **********************************************/
void error(const char *msg, int n)
{
    perror(msg);
    exit(n);
}

/*************************************************
 * Function: fillAddrStruct
 * Description: Sets up the server address struct and fills other information like the port number
 * Params: address of sockaddr_in struct, hostent struct, address of portnumber integer, command line args
 * Returns: none
 * Pre-conditions: proper addresses and arguments are passed in
 * Post-conditions: server address struct is filled and port number is given. Exit if errors.
 * **********************************************/
void fillAddrStruct(struct sockaddr_in* serverAddress, struct hostent* serverHostInfo, int* portNumber, char* argv[])
{
    //Clear out address struct and obtain port number from command line
    memset((char*)serverAddress, '\0', sizeof(serverAddress));
    *portNumber = atoi(argv[3]);

    //Create network capable socket
    serverAddress->sin_family = AF_INET;
    //Store port number and convert from LSB to MSB form
    serverAddress->sin_port = htons(*portNumber);
    //Convert the machine name into a special form of address
    serverHostInfo = gethostbyname("localhost");

    //Copy in the address
    if(serverHostInfo == NULL)
    {
        fprintf(stderr, "otp_enc error: no such host\n");
        exit(0);
    }
    //save data
    memcpy((char*)&serverAddress->sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);
}

/*************************************************
 * Function: setSocket
 * Description: sets up socket for communication with server
 * Params: address of socket file descriptor var, address of server address struct
 * Returns: none
 * Pre-conditions: correct arguments passed in
 * Post-conditions: socket file descriptor is set and exits on error
 * **********************************************/
void setSocket(int* socketFD, struct sockaddr_in* serverAddress)
{
    //Fill socketFD
    *socketFD = socket(AF_INET, SOCK_STREAM, 0);
    //If a bad file descriptor is given, exit with an error
    if(*socketFD < 0)
    {
        close(*socketFD);
        error("otp_enc error: opening socket", 1);
    }
}

/*************************************************
 * Function: connectServer
 * Description: Establishes a connection to a server (otp_enc_d) and checks if the connection is allowed via indentification bits 
 * communicated between this client and that server.
 * Params: address of serverAddress struct, address of socket file descriptor
 * Returns: none
 * Pre-conditions: server address and socket file descriptor are correctly filled
 * Post-conditions: Client either establishes connection with server or terminates connection due to invalid identification bits recieved
 * **********************************************/
void connectServer(struct sockaddr_in* serverAddress, int* socketFD)
{
    int charsWritten, charsRead;
    char buffer[1];

    //Establish connection, exit with status 2 if there was an error connecting
    if(connect(*socketFD, (struct sockaddr*)serverAddress, sizeof(*serverAddress)) < 0)
    {
        error("otp_enc error: connecting", 2);
    }
    //Send indicator bit 0 that says, I am encryption to the server
    charsWritten = send(*socketFD, "0", 1, 0);
    if(charsWritten < 0)
    {
        close(*socketFD);
        error("otp_enc error: sending identifier bit", 1);
    }
    else if(charsWritten == 0)
    {
        close(*socketFD);
        error("otp_enc error: charsWritten 0 when sending identifier bit", 1);
    }
    //Recieve identifier bit from server
    charsRead = recv(*socketFD, buffer, sizeof(buffer), 0);
    if(charsRead < 0)
    {
        close(*socketFD);
        error("otp_enc error: recieving identifier bit", 1);
    }
    else if(charsRead == 0)
    {
        close(*socketFD);
        error("otp_enc error: charsRead 0 when recieving identifier bit", 1);
    }

    //Check if identifier bit recieved is valid or not
    //If invalid then state invalid connection attempt and exit with code 1
    if(buffer[0] == '1')
    {
        close(*socketFD);
        fprintf(stderr, "otp_enc error: tried to connect to otp_dec_d\n");
        exit(1);
    }

}

/*************************************************
 * Function: sendMessage 
 * Description: sends content of file given via the socket to the server and adds a control character to the stream of info so the server knows when to stop recieving
 * Params: address of socket file descriptor, address of the pointer of the file descriptor for the file given by the user
 * Returns: none
 * Pre-conditions: socket file descriptor is valid and the input file descriptor is open for reading. File contains only one line of characters.
 * Post-conditions: Contents of file are sent or program exits with error message
 * **********************************************/
void sendMessage(int* socketFD, FILE** inputFD)
{
    int charsWritten, pos;
    char* fileContent = NULL;
    size_t sizeFile;

    //Get contents of file and store in file content buffer
    getline(&fileContent, &sizeFile, *inputFD);

    //Remove newline at end of file content and replace it with a control character and null terminator
    pos = strcspn(fileContent, "\n");
    strcpy(&fileContent[pos], "0\0");

    //Send file contents
    charsWritten = send(*socketFD, fileContent, strlen(fileContent), 0);
    if(charsWritten < 0)
    {
        error("otp_enc error: writing to socket", 1);
    }
    if(charsWritten < strlen(fileContent))
    {
        error("otp_enc warning: Not all data written to socket!\n", 1);
    }
}

/*************************************************
 * Function: getMessage
 * Description: Recieves encrypted text from the server and outputs it to stdout
 * Params: address of socket file descriptor
 * Returns: none
 * Pre-conditions: socket file descriptor is open and correct
 * Post-conditions: message has been fully recieved and sent to stdout
 * **********************************************/
void getMessage(int *socketFD)
{
    //Initialize buffer large enough to hold 80000 bytes if needed and a readbuffer of size 2 to read the message byte by byte
    char fileMessage[80000], readBuffer[2];
    int charsRead;

    //Clean file message
    memset(fileMessage, '\0', 80000);

    //While the control character '0' has not been found, continue calling recieve and filling fileMessage one byte at a time
    while(strstr(fileMessage, "0") == NULL)
    {
        //Clean readBuffer
        memset(readBuffer, '\0', sizeof(readBuffer));
        //Read one character in from recv at a time
        charsRead = recv(*socketFD, readBuffer, sizeof(readBuffer)-1, 0);
        //Concatenate the readbuffer onto fileMessage
        strcat(fileMessage, readBuffer);

        //Check if there was an error reading characters or there were not enough characters read and break from loop
        if (charsRead == -1) break;
        if (charsRead == 0) break;
    }

    //Clean fileMessage of the 0 character
    fileMessage[strcspn(fileMessage, "0")] = '\n';
    printf(fileMessage);
}

/*************************************************
 * Function: openFiles
 * Description: Opens the plaintext and key file for reading and fills the file descriptors
 * Params: address of plaintext file descriptor, address of key file descriptor, command line args
 * Returns: none
 * Pre-conditions: proper command line args are given
 * Post-conditions: File descriptors are opened for reading
 * **********************************************/
void openFiles(FILE **inputFD, FILE **keyFD, char* argv[])
{
    //Open plaintext file descriptor for reading
    *inputFD = fopen(argv[1], "r"); 
    if(*inputFD == NULL)
    {
        fprintf(stderr, "otp_enc error: failed to open plaintext for reading", argv[1]);
        exit(1);
    }
    //Open key file descriptor for reading
    *keyFD = fopen(argv[2], "r");
    if(*keyFD == NULL)
    {
        fprintf(stderr, "otp_enc error: failed to open %s for reading", argv[2]);
        exit(1);
    }
    
}

/*************************************************
 * Function: closeFiles
 * Description: closes plaintext and key file descriptors
 * Params: address of plaintext file descriptor, address of key file descriptor
 * Returns: none
 * Pre-conditions: file descriptors passed are already open
 * Post-conditions: file descriptors passed are closed
 * **********************************************/
void closeFiles(FILE **inputFD, FILE **keyFD)
{ 
    fclose(*inputFD); 
    fclose(*keyFD);
}

/*************************************************
 * Function: checkFiles
 * Description: Checks the plaintext file sent for bad characters and compares the size of the plaintext and key files to see if there is a size error
 * Params: Address of plaintext file descriptor, address of key file descriptor, command line args
 * Returns: none
 * Pre-conditions: file descriptors passed are open for reading, command line args are valid
 * Post-conditions: Exits with error if bad characters are found or keyfile is shorter than plaintext
 * **********************************************/
void checkFiles(FILE **inputFD, FILE **keyFD, char* argv[])
{
    //Check if keyFD length is shorter than inputFD length
    int charsRead1, charsRead2, c;
    charsRead1 = 0;
    charsRead2 = 0; 

    //Get number of chars in inputFD and check for bad characters
    while((c = fgetc (*inputFD)))
    {
        if(c == EOF)
        { 
            break;
        }
        //Check for bad characters
        else if((c < 65 || c > 90) && c != 32 && c != 10)
        {
            closeFiles(inputFD, keyFD);
            fprintf(stderr, "otp_enc error: input contains bad characters");
            exit(1);
        }
        charsRead1++;
    }
    while((c = fgetc(*keyFD)))
    {
        if(c == EOF)
        {
            break;
        }
        charsRead2++;
    }

    //Check if key is shorter than file
    if(charsRead2 < charsRead1)
    {
        closeFiles(inputFD, keyFD);
        fprintf(stderr, "Error: key \'%s\' is too short\n", argv[2]);
        exit(1);
    }
}
