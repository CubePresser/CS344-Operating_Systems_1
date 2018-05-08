//Program 4 - JONATHAN A JONES
//This program creates a key of specified length.
//The characters in the file generated will be any of the 27 allowed characters, generated using the standard UNIX randomization methods.
//The last character outputted is a newline.
//Takes in a command line argument for the length of the key and outputs to stdout

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>

//Prototypes
void generateKey();
int getRandomNumber();
char convertToChar();

int main(int argc, char* argv[])
{
    //Check number of arguments
    if(argc > 2)
    {
        fprintf(stderr, "too many arguments were entered\n");
        exit(1);
    }
    else if(argc == 1)
    {
        fprintf(stderr, "not enough arguments entered\n");
        exit(1);
    }

    //Get length of key
    int len;
    len = atoi(argv[1]);

    srand(time(NULL));

    generateKey(len);
    return 0;
}

/*************************************************
 * Function: generateKey
 * Description: Generates a key of the length specified by the user and sends to stdout character by character
 * Params: int length of key 
 * Returns: none
 * Pre-conditions: none
 * Post-conditions: Has printed a key of length len of Capital letters/whitespace to stdout and a newline character
 * **********************************************/
void generateKey(int len)
{
    //For the length specified by the user, get a random capital letter or whitespace and output to stdout
    int i;
    for(i=0;i<len;i++)
    {
        //Call convertToChar and getRandom number to get random character
        printf("%c", convertToChar(getRandomNumber()));
    }
    //Add a new line at the end
    printf("\n");
}

/*************************************************
 * Function: convertToChar
 * Description: Converts an integer to a character
 * Params: integer to be converted to a char
 * Returns: character converted from integer
 * Pre-conditions: none
 * Post-conditions: integer converted to character
 * **********************************************/
char convertToChar(int n)
{
    char result;
    result = n;
    return result;
    
}

//Whitespace ASCII = 32
//Charcters 65-90
/*************************************************
 * Function: getRandomNumber
 * Description: Generates a random number corresponding to ASCII for a capital letter or whitespace
 * Params: none
 * Returns: integer ASCII value
 * Pre-conditions: none
 * Post-conditions: returns an ASCII value between 65-90 or 32
 * **********************************************/
int getRandomNumber()
{
    //Generate a random number corresponding to an ASCII character
    int result;
    result = rand()%27;
    //If random number is 27, convert to ASCII decimal equivalent of whitespace
    if(result == 26)
    {
        result = 32;
    }
    //If random number is not 27 then return ASCII decimal equivalent of a capital letter
    else
    {
        result += 65;
    }
    return result;
}

