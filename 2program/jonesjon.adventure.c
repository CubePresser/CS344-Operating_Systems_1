// PROGRAM 2 - OPERATING SYSTEMS I
// Jonathan Alexander Jones
// -------------------------------
// This program will read in information from the files stored in the jonesjon.rooms.PROCESS_ID to construct an adventure game.
// In this game the player will begin in the "starting room" and will win the game automatically upon entering the
// "ending room", which causes the game to exit, displaying the path taken by the player and the number of steps they took.
// The player will also be able to ask for the time at any time during gameplay. This feature will utilize threads.
// -------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

//Copy the same Room structure from jonesjon.buildrooms.c
struct Room {
    int id; //Helps to determine if room is starting room or end room
    char* name;
    int numOutboundConnections;
    struct Room* outboundConnections[6];
};

//Prototypes
void* showTime();
void showPath(struct Room*, int*, int);
void showCurrentLocation(struct Room*, int);
void game(struct Room*);
void cleanNames(struct Room*);
void printRoom(struct Room*);
char* findDirectory();
void fillStructs(struct Room*, char*);
void getConnections(struct Room*, char*);

int main() {
    //Declare array of 7 room structs that will contain the data read in from the files in the jonesjon.rooms.PROCESS_ID directory.
    struct Room rooms[7];

    //START --- GET DATA ---

    //Get the directory name
    char* dirName;
    dirName = findDirectory();

    //Get data from the files and put them into the structs. Get all info but the outbound connections, then add those individually
    fillStructs(rooms, dirName);
    getConnections(rooms, dirName);

    //Free up memory and make aesthetic changes
    free(dirName);
    cleanNames(rooms);

    //END --- GET DATA ---
    //START --- GAME ---

    game(rooms);

    //END --- GAME ---

    //Clean up memory
    int i;
    for(i=0; i<7; i++)
    {
        free(rooms[i].name);
    }
    return 0;
}

/*************************************************
 * Function: showTime
 * Description: Runs on a separate thread from the rest of the program. 
 * Gets the time for the user and writes it into a file called currentTime.txt
 * Params: mutex
 * Returns: nothing
 * Pre-conditions: pthread has been created with this routine.
 * Post-conditions: The current time has been written into currentTime.txt
 * **********************************************/
void* showTime(void* myMutex)
{
    //Lock the mutex and grant priority to this thread
    pthread_mutex_lock(myMutex);

    //Create file for writing into
    int fd = open("./currentTime.txt", O_RDWR | O_CREAT, 0600);
    close(fd);

    //Open file for writing into
    FILE* timeFile = fopen("./currentTime.txt", "w");

    //Initialize time variables
    time_t t;
    struct tm *tInfo;
    //String buffer to contain the time and be written into the file
    char buffer[64];

    time(&t);
    tInfo = localtime(&t);

    //Fills buffer with a custom time format (12Hrtime;am/pm; FullDayOfWeek; FullMonth; DayOfMonth; Year(XXXX))
    strftime(buffer, sizeof(buffer), "%I:%M%p, %A, %B %d, %Y", tInfo);

    //Replace the '0' at the beginning of a one digit hour with whitespace (EX: 01:20pm --->  1:20pm)
    if(buffer[0] == '0')
    {
        buffer[0] = ' ';
    }

    //Write the time into a file and unlock the mutex so other threads can run safely
    fwrite(buffer, sizeof(char), strlen(buffer), timeFile);
    fclose(timeFile);
    pthread_mutex_unlock(myMutex);
}

/*************************************************
 * Function: showPath
 * Description: Given an array containing the id's of the rooms visited during gameplay.
 * Uses these id's and searches through array of room structs to print out the rooms
 * in order that the user visited while adventuring.
 * Params: Array of room structs, array of room id's, number of steps taken
 * Returns: none
 * Pre-conditions: Array of room structs contains data, especially a name or function will seg fault
 * Post-conditions: Displays path to user
 * **********************************************/
void showPath(struct Room* listRooms, int* arrPath, int numSteps)
{
    int i, k;
    for(i=0; i<numSteps; i++)
    {
        for(k=0; k<7; k++)
        {
            if(listRooms[k].id == arrPath[i])
            {
                printf("%s\n", listRooms[k].name);
            }
        }
    }
}

/*************************************************
 * Function: showCurrentLocation
 * Description: Displays current location and the connections avaliable to the user.
 * Params: Array of room structs, current location
 * Returns: none
 * Pre-conditions: Array of room structs contains data, especially names otherwise function will seg fault.
 * Post-conditions: Current location and connections available is displayed.
 * **********************************************/
void showCurrentLocation(struct Room* arrRooms, int loc)
{
    int i, j;
    for(i=0; i<7; i++)
    {
        //Find a room in the array of room structs who's id is equal to our location (which is an id)
        if(arrRooms[i].id == loc)
        {
            printf("\nCURRENT LOCATION: %s\nPOSSIBLE CONNECTIONS: ", arrRooms[i].name);

            //Print possible connections by accessing the name of each connection in the found room struct
            for(j=0; j<arrRooms[i].numOutboundConnections; j++)
            {
                if(j==0)
                {
                    //Print the first room without a comma behind it (aesthetic)
                    printf(arrRooms[i].outboundConnections[j]->name);
                }
                else
                {
                    printf(", %s", arrRooms[i].outboundConnections[j]->name);
                }
            }
            //Add a period and a newline at the end of the connections list
            printf(".\n");
            break;
        }
    }
}

/*************************************************
 * Function: game
 * Description: Runs all the features of the game.
 * By this point, all the data has already been read into the structs.
 * This function pulls together many other functions and takes care of the looping and thread set up.
 * Params: Array of room structs
 * Returns: none
 * Pre-conditions: Array of Room structs contains data
 * Post-conditions: Game is run and is completed.
 * **********************************************/
void game(struct Room* arrRooms)
{
    //Initialize Variables
    int i;
    int location = 0;
    int arrLocation = 0;
    int roomPath[100];
    int steps = 0;
    char *buffer = NULL;
    char buffer2[64];
    size_t size;

    //Initialize mutex
    pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&myMutex);

    //Initialize thread for showTime function
    int resultInt;
    pthread_t myThread;
    pthread_create(&myThread, NULL, showTime, &myMutex);

    //Set the first room in the path equal to the start room (not read anyway)
    roomPath[0] = 0;

    //Print out current location
    showCurrentLocation(arrRooms, location);

    //While the location isn't the room with id=1 (the end room) ask the user for navigation instructions
    while(location != 1)
    {
        //Get user input
        int goodInput = 0;
        printf("WHERE TO? >");
        getline(&buffer, &size, stdin);
        buffer[strlen(buffer)-1] = '\0';

        //Check to see if user asked for time
        if(strcmp(buffer, "time") == 0)
        {
            //Time run with thread
            pthread_mutex_unlock(&myMutex);
            pthread_join(myThread, NULL);
            pthread_mutex_lock(&myMutex);

            //Print contents of time file to the screen
            FILE* fileRead = fopen("currentTime.txt", "r");
            while(fgets(buffer2, sizeof(buffer2), fileRead) != NULL)
            {
                printf("\n%s\n\n", buffer2);
            }
            fclose(fileRead);

            //Create another thread in case the user wants to get the time again
            pthread_create(&myThread, NULL, showTime, &myMutex);
        }
        else
        {
            //Get the actual array location since the ID does not correspond to the location in the rooms array
            for(i=0; i<7; i++)
            {
                if(arrRooms[i].id == location)
                {
                    arrLocation = i;
                }
            }
            //For the number of outbound connections at the our location
            for(i=0; i<arrRooms[arrLocation].numOutboundConnections; i++)
            {
                //Check to see if the user inputted a string that is equal to the name of any one of these connections
                if(strcmp(buffer, arrRooms[arrLocation].outboundConnections[i]->name) == 0)
                {
                    //Move location and record step
                    location = arrRooms[arrLocation].outboundConnections[i]->id;
                    goodInput = 1;
                    roomPath[steps] = arrRooms[arrLocation].outboundConnections[i]->id;
                    steps++;

                    //If we haven't reached the end, display the new location
                    if(location != 1)
                    {
                        showCurrentLocation(arrRooms, location);
                    }
                }
            }
        
            //If user entered bad input then output error message and re-loop
            if(!goodInput)
            {
                printf("\nHUH? I DON\'T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
            }
        }
    }

    //End messages and show the path
    printf("\nYOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\nYOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", steps);
    showPath(arrRooms, roomPath, steps);

    //Free memory
    free(buffer);

    //Unlock the mutex and join the thread to prevent memory leaks (will overwrite currentTime.txt file with new time)
    //pthread_mutex_unlock(&myMutex);
    //pthread_join(myThread, NULL);
}

////////////////////////////////////////////////////////////////////////DATA SEGMENT///////////////////////////////////////////////////////////////////////////////////


/*************************************************
 * Function: cleanNames
 * Description: Removes the newline that is read in from my alternative to strtoken in the room names.
 * Params: Array of room structs
 * Returns: none
 * Pre-conditions: Array of room structs contains a name else this will seg fault
 * Post-conditions: Room names do not contain a new line
 * **********************************************/
void cleanNames(struct Room* arrRooms)
{
    int i;
    for(i=0; i<7; i++)
    {
        //Get the newline character at the end of the string and replace it with a null terminator
        arrRooms[i].name[strlen(arrRooms[i].name)-1] = '\0';
    }
}


/*************************************************
 * Function: printRoom
 * Description: [Dev Tool] - Prints the rooms and their data out to the screen
 * Params: Array of room structs
 * Returns: none
 * Pre-conditions: Array of room structs contains all data else will seg fault
 * Post-conditions: Prints room data to screen
 * **********************************************/
/*
void printRoom(struct Room* arrRooms)
{
    int i, j;
    printf("------------------------\n");
    for(i=0; i<7; i++)
    {
        printf("id=%d\nname=%s\nnumOutbound=%d\nConnections=", arrRooms[i].id, arrRooms[i].name, arrRooms[i].numOutboundConnections);
        
        for(j=0; j<arrRooms[i].numOutboundConnections; j++)
        {
            printf(" %s,", arrRooms[i].outboundConnections[j]->name);
        }
        
        printf("\n------------------------\n");
    }
}
*/

/*************************************************
 * Function: getConnections
 * Description: Runs after fillStructs to obtain data from files about the connections that each room has and
 * assign this information to each respective room struct in this program.
 * Params: Array of room structs, rooms directory name
 * Returns: none
 * Pre-conditions: fillStructs must have been run before this!
 * Post-conditions: All room structs contain their respective outbound connection information
 * **********************************************/
void getConnections(struct Room* arrRooms, char* roomDir)
{
    //initialize variables
    int i, j, k;
    i=0;
    j=0;
    k=0;

    //initialize strings
    char filePath[64];
    //prefix to search for in filenames
    char targetDirPrefix[32] = "jonesjon";
    char buffer[32];

    FILE* roomToRead;

    //Open the rooms directory
    DIR* dirRoom;
    struct dirent *roomFile;
    dirRoom = opendir(roomDir);

    //While we iterate through the directory, build up a filename string to use a file stream with then extract connection information from it
    while((roomFile = readdir(dirRoom)) !=NULL)
    {
        //Clean the filePath string
        memset(filePath, '\0', 64);

        //Begin building the filePath string
        sprintf(filePath, "./%s/",roomDir);

        //If the name of the file we're currently looking at matches the target prefix specified, open it to read.
        if(strstr(roomFile->d_name, targetDirPrefix) !=NULL)
        {
            //Finish building the filepath string with the name of the file we found and open the file for reading + double check initialization of variable k and clean
            strcat(filePath, roomFile->d_name);
            roomToRead = fopen(filePath, "r");
            memset(buffer, '\0', 64);
            k=0;

            //Read in each line individually from the file until there are no more lines to read
            while(fgets(buffer, sizeof(buffer), roomToRead) != NULL)
            {
                //Does the line have the prefix "CONNECTION"? If so, get connection information from that line
                if(strstr(buffer, "CONNECTION") !=NULL)
                {
                    //The &buffer[#] is saying "Give me the address of your string buffer and start reading it from this byte". This allows me to bypass usage
                    //of the strtok. If I am aware of the byte postion in this line where the data I care about starts, I will start reading there.
                    //Overwrite buffer with the last few bytes of buffer (The data we care about)
                    sprintf(buffer, "%s", &buffer[14]);

                    //Iterate through the entire rooms array to find the room name that matches the name of the connection we found and add that connection
                    for(j=0; j<7; j++)
                    {
                        if(strcmp(arrRooms[j].name, buffer) == 0)
                        {
                           //Add address of room we want to connect to the array of outboundConnections
                           arrRooms[i].outboundConnections[k] = &arrRooms[j];
                        }
                    }
                    //Step to the next connection location
                    k++;
                }
            }

            //Close file and step to next location in arrRooms
            fclose(roomToRead);
            i++;
        }
    }
    //Close directory
    closedir(dirRoom);

}

/*************************************************
 * Function: fillStructs
 * Description: Reads through each rooms file in the rooms directory and enters that information into the array of room structs to each respective room
 * Params: Array of room structs
 * Returns: none
 * Pre-conditions: Files must contain correct format and data
 * Post-conditions: Array of room structs contains data for each respective room
 * **********************************************/
void fillStructs(struct Room* arrRooms, char* roomDir)
{
    //Initialize variables
    int i, j, k;
    i=0;
    j=2;
    k=0;

    //Initialize strings
    char filePath[64];
    //Prefix to search for in fileName
    char targetDirPrefix[32] = "jonesjon";
    char buffer[32];

    FILE* roomToRead;

    //Open directory
    DIR* dirRoom;
    struct dirent *roomFile;
    dirRoom = opendir(roomDir);

    //Iterate through each file in the directory
    while((roomFile = readdir(dirRoom)) !=NULL)
    {
        //Clean filepath and begin to build filePath for later usage in opening a file stream
        memset(filePath, '\0', 64);
        sprintf(filePath, "./%s/",roomDir);

        //If the name of the file we are currently on has the correct prefix then obtain data from this file
        if(strstr(roomFile->d_name, targetDirPrefix) !=NULL)
        {
            //Finish building filepath name, open file for reading from and clean a string
            strcat(filePath, roomFile->d_name);
            roomToRead = fopen(filePath, "r");
            memset(buffer, '\0', 64);

            //Get rid of garbage values for outbound connections
            arrRooms[i].numOutboundConnections = 0;

            //Read each line in from the file
            while(fgets(buffer, sizeof(buffer), roomToRead) != NULL)
            {
                //Run a series of if statements that check the prefixes for each line in the file.
                //Then based on this, assign data to the room structs
                if(strstr(buffer, "ROOM NAME") != NULL)
                {
                    arrRooms[i].name = calloc(10, sizeof(char));
                    memset(arrRooms[i].name, '\0', 10);


                    //Starts to read line from byte 11 to get the data we need
                    //The &buffer[#] is saying "Give me the address of your string buffer and start reading it from this byte". This allows me to bypass usage
                    //of the strtok. If I am aware of the byte postion in this line where the data I care about starts, I will start reading there.
                    //Overwrite buffer with the last few bytes of buffer (The data we care about)
                    sprintf(arrRooms[i].name, "%s", &buffer[11]);
                }
                //Only counts the number of connection lines
                else if(strstr(buffer, "CONNECTION") !=NULL)
                {
                    arrRooms[i].numOutboundConnections++;
                }
                //If START_ROOM is read in, set that room id equal to zero
                else if(strstr(buffer, "ROOM TYPE: START_ROOM") !=NULL)
                {
                    arrRooms[i].id = 0;
                }
                //If END_ROOM is read in, set that room id equal to one
                else if(strstr(buffer, "ROOM TYPE: END_ROOM") !=NULL)
                {
                    arrRooms[i].id = 1;
                }
                else if(strstr(buffer, "ROOM TYPE: MID_ROOM") !=NULL)
                {
                    arrRooms[i].id = j;
                    //j here is keeping track of the current number of id's used (The next available unique id..)
                    j++;
                }
            }

            //Close file and iterate to next room struct in array
            fclose(roomToRead);
            i++;
        }
    }
    //Close directory
    closedir(dirRoom);
}

/*************************************************
 * Function: findDirectory
 * Description: Returns name of the most recently created jonesjon.rooms.PROCESS_ID directory
 * Params: none
 * Returns: Name of most recently created jonesjon.rooms.PROCESS_ID directory
 * Pre-conditions: none (will handle no directory being present)
 * Post-conditions: Directory name is returned or error message is presented to user with exit code 1
 * **********************************************/
char* findDirectory()
{
    //Initialize variables
    int newestDirTime = -1;
    //Specify prefix for directory name so we avoid other directories
    char targetDirPrefix[32] = "jonesjon.rooms.";
    char newestDirName[256];
    //Clean string
    memset(newestDirName, '\0', sizeof(newestDirName));

    //Open current directory for reading
    DIR* dirToCheck;
    struct dirent *fileInDir;
    struct stat dirAttributes;

    dirToCheck = opendir(".");

    //If we successfully open our current directory
    if(dirToCheck > 0)
    {
        //Read all the directory names until ther are none
        while((fileInDir = readdir(dirToCheck)) !=NULL)
        {
            //Check to see that directory has correct prefix
            if(strstr(fileInDir->d_name, targetDirPrefix) != NULL)
            {
                //Fill stats with attributes of directory found
                stat(fileInDir->d_name, &dirAttributes);
                
                //Compare the time of this directory with the current new time
                if((int)dirAttributes.st_mtime > newestDirTime)
                {
                    //If this directory is newest, make the directory name this directory name!
                    newestDirTime = (int)dirAttributes.st_mtime;
                    memset(newestDirName, '\0', sizeof(newestDirName));
                    strcpy(newestDirName, fileInDir->d_name);
                }
            }
        }
    }

    //Close directory
    closedir(dirToCheck);


    //Did we even find a directory at all? If not, complain about it
    if(newestDirName[0] == '\0')
    {
        fprintf(stderr, "ERROR: \"jonesjon.rooms.*\" directory not detected! Exit status: 1\n");
        exit(1);
    }

    //Allocate space for the string literal we are returning
    char* buffer = calloc(256, sizeof(char));
    strcpy(buffer, newestDirName);
    return buffer;

}
