// PROGRAM 2 - OPERATING SYSTEMS I
// Jonathan Alexander Jones
// -------------------------------
// This program will generate 7 different room files. Each room has a room
// name, at least 3 outgoing connections from this room to other rooms
// and a room type. The connections from one room to the others is 
// randomly assigned. Possible room type entires are: START_ROOM,
// END-ROOM, and MID-ROOM. The program is generating rooms for the
// game program within this same directory.
// -------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#define NUM_ROOMS 10

//Set up room structure
//------------------------
//A room with ID 0 is the start room while a room with the ID 1 is the end room. 
//Randomization is still preserved since the graph structure is always different
//and every room gets a different name regardless of ID.
struct Room {
    int id;
    char* name;
    int numOutboundConnections;
    struct Room* outboundConnections[6];
};

//Prototypes
void createFiles(struct Room*);
void createDirectory();
void initRooms(struct Room*);

//For testing purposes
//void printRoom(struct Room*);

void CreateFiles();
bool IsGraphFull(struct Room*);
void AddRandomConnection(struct Room*);
bool CanAddConnectionRoom();
struct Room* GetRandomRoom(struct Room*);
bool CanAddConnectionRoom(struct Room*);
bool ConnectionAlreadyExists(struct Room*, struct Room*);
void ConnectRoom(struct Room*, struct Room*);
bool IsSameRoom(struct Room*, struct Room*);


int main()
{
    //Initialize array of 7 room structs and obtain a seed for randomization from the system clock
    struct Room rooms[7];
    int i;
    srand(time(NULL));

    //Assign each room a name, an id and clean up all possible garbage values in variables
    initRooms(rooms);

    //Create all connections in graph and fill room structs with information necessary to generate files
    while (IsGraphFull(rooms) == false)
    {
        AddRandomConnection(rooms);
    }

    //Create the directory "./jonesjon.rooms.PROCESS_ID" and use information stored in array of room structs to generate a file for each individual room
    createDirectory();
    createFiles(rooms);

    //Free the memory that was allocated for each room's name in the initRooms function 
    for(i=0; i<7; i++)
    {
        free(rooms[i].name);
    }
    return 0;
}

/*************************************************
 * Function: createFiles
 * Description: Uses data stored in room structs to generate a file for each of the 7 individual rooms.
 *     Creates a file using open() then closes it and generates a file stream for writing to this file.
 *     Stores this file in the "./jonesjon.rooms.PROCESS_ID" directory.
 * Params: Array of room structs.
 * Returns: none
 * Pre-conditions: Each room must have a unique id, name and a number of outbound connections plus pointers to the structs that they are connected to.
 * Post-conditions: File is created for each individual room in the correct directory.
 * **********************************************/

void createFiles(struct Room* arrRooms)
{
    int i, j;
    char fileName[40];
    char content[200];

    for(i=0; i<7; i++)
    {
        //Clean fileName and write into it the proper filename for the room plus the path to the rooms directory
        memset(fileName, '\0', 40);
        sprintf(fileName, "./jonesjon.rooms.%d/jonesjon%d%s", getpid(), arrRooms[i].id ,arrRooms[i].name);

        //Initialize file pointers/descriptors and create the file
        int fd = open(fileName, O_RDWR | O_CREAT, 0600);
        close(fd);
        FILE* myFile = fopen(fileName, "w");


        //Add content to file by extracting info from room structs
        sprintf(content, "ROOM NAME: %s\n", arrRooms[i].name);
        for(j=0; j<arrRooms[i].numOutboundConnections; j++)
        {
            sprintf(content + strlen(content), "CONNECTION %d: %s\n", j+1, arrRooms[i].outboundConnections[j]->name);
        }

        //Check ID for room type. If ID is 0 then START_ROOM. If ID is 1 then END_ROOM. Else MID_ROOM
        if(arrRooms[i].id == 0)
        {
            sprintf(content + strlen(content), "ROOM TYPE: START_ROOM\n");
        }
        else if(arrRooms[i].id == 1)
        {
            sprintf(content + strlen(content), "ROOM TYPE: END_ROOM\n");
        }
        else
        {
            sprintf(content + strlen(content), "ROOM TYPE: MID_ROOM\n");
        }

        //Write info to created file and close it since the file operations are in a loop and the program won't close it for us. Avoids losing track of bytes.
        fwrite(content, sizeof(char), strlen(content), myFile);
        fclose(myFile);

    }
}

/*************************************************
 * Function: createDirectory
 * Description: In the current directory, generates a directory called "jonesjon.rooms.PROCESS_ID"
 * Params: none
 * Returns: none
 * Pre-conditions: none
 * Post-conditions: Directory called "jonesjon.rooms.PROCESS_ID" is created in the current directory
 * **********************************************/
void createDirectory()
{
    char fileName[30];
    memset(fileName, '\0', 30);
    sprintf(fileName, "./jonesjon.rooms.%d", getpid());
    int result = mkdir(fileName, 0755);
}

/*************************************************
 * Function: initRooms
 * Description: Randomly assings names to rooms in the rooms array and sets ID numbers plus cleans struct variables.
 * Params: Array of room structs.
 * Returns: none
 * Pre-conditions: Array of 7 room structs has been passed in.
 * Post-conditions: Rooms have been randomly assigned names, ID numbers are set and struct variables are clean.
 * **********************************************/
void initRooms(struct Room* arrRooms)
{
    //Hard coded room names
    char roomNames[NUM_ROOMS][10] = {"Alpha", "Bravo", "Charlie", "Delta", "Echo", "Foxtrot", "Golf", "Hotel", "India", "Juliet"};
    int i, rnum;

    for(i=0; i<7; i++)
    {
        //Get a random number between 0 and 9
        rnum = rand()%10;
        arrRooms[i].id=i;
        arrRooms[i].name = calloc(10, sizeof(char));
        memset(arrRooms[i].name, '\0', 10);

        //Check for a "xxx" tombstone in the array of names. If found, try another random name.
        while(strcmp("xxx", roomNames[rnum]) == 0)
        {
            rnum=rand()%8;
        }

        strcpy(arrRooms[i].name, roomNames[rnum]);
        //Set a tombstone after a name has been used to prevent duplicate names from appearing in rooms.
        strcpy(roomNames[rnum], "xxx");

        arrRooms[i].numOutboundConnections = 0;
    }


}

/*************************************************
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
*************************************************/


/*************************************************
 * Function: IsGraphFull
 * Description: Checks to see if all rooms have at least 3 outbound connections.
 * Params: Array of room structs
 * Returns: Returns true if all rooms have 3 to 6 outbound connections
 * Pre-conditions: none
 * Post-conditions: none
 * **********************************************/
bool IsGraphFull(struct Room* arrRooms)
{
    int i;
    for(i=0; i<7; i++)
    {
        if(arrRooms[i].numOutboundConnections < 3)
        {
            return false;
        }
    }
    return true;
}

/*************************************************
 * Function: AddRandomConnection
 * Description: Adds a random, valid outbound connection from a Room to another Room
 * Params: Array of room structs
 * Returns: none
 * Pre-conditions: none
 * Post-conditions: Randomly chosen room must have a new valid random connection.
 * **********************************************/
void AddRandomConnection(struct Room* arrRooms)
{
    //Placeholder room pointers
    struct Room* A;
    struct Room* B;

    //Find a room that needs a connection
    while(true)
    {
        A = GetRandomRoom(arrRooms);

        if(CanAddConnectionRoom(A) == true)
            break;
    }

    //Find another room that needs a connection and connect the two if it is not the same room and that connection does not already exist.
    do
    {
        B = GetRandomRoom(arrRooms); 
    }
    while(CanAddConnectionRoom(B) == false || IsSameRoom(A, B) == true || ConnectionAlreadyExists(A, B) == true);

    ConnectRoom(A, B);
    ConnectRoom(B, A);

}

/*************************************************
 * Function: GetRandomRoom
 * Description: Gets a random room from the rooms array and does NOT validate if a connection can be added
 * Params: Array of room structs.
 * Returns: Returns a random room.
 * Pre-conditions: none
 * Post-conditions: Random room is returned.
 * **********************************************/
struct Room* GetRandomRoom(struct Room* listRooms)
{
    int i, rnum;
    rnum = rand()%7;

    //Make sure that the placeholder rooms get the actual ADDRESS of the memory we want to change.
    return &listRooms[rnum];
}

/*************************************************
 * Function: CanAddConnectionRoom
 * Description: Returns true if a connection can be added from Room x (< 6 outbound connections), false otherwise
 * Params: Room pointer
 * Returns: boolean
 * Pre-conditions: none
 * Post-conditions: none
 * **********************************************/
bool CanAddConnectionRoom(struct Room* x)
{
    if(x->numOutboundConnections < 6)
    {
        return true;
    }
    return false;
}

/*************************************************
 * Function: ConnectionAlreadyExists
 * Description: Returns true if a connection from Room x to Room y already exists, false otherwise.
 * Params: Two room pointers
 * Returns: boolean
 * Pre-conditions: none
 * Post-conditions: none
 * **********************************************/
bool ConnectionAlreadyExists(struct Room* x, struct Room* y)
{
    int i;
    for(i=0; i<x->numOutboundConnections; i++)
    {
        if(y == x->outboundConnections[i])
        {
            return true;
        }
    }
    return false;
}

//Connects Rooms x and y together, does not check if this connection is valid
/*************************************************
 * Function: ConnectRoom
 * Description: Connects Rooms x and y together, does not check if this connection is valid.
 * Params: Two room pointers
 * Returns: none
 * Pre-conditions: none
 * Post-conditions: none
 * **********************************************/
void ConnectRoom(struct Room* x, struct Room* y)
{
    x->outboundConnections[x->numOutboundConnections] = y;
    x->numOutboundConnections = x->numOutboundConnections + 1;
    //printf("%d\n", x.numOutboundConnections);
}

/*************************************************
 * Function: IsSameRoom
 * Description: Returns true if Rooms x and y are the same Room, false otherwise.
 * Params: Two room pointers
 * Returns: boolean
 * Pre-conditions: none
 * Post-conditions: none
 * **********************************************/
bool IsSameRoom(struct Room* x, struct Room* y)
{
    if(x->id == y->id)
    {
        return true;
    }
    return false;
}
