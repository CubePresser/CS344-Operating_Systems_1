// PROGRAM 3 - OPERATING SYSTEMS I
// Jonathan Alexander Jones
// -------------------------------
// This program is a small version of a shell. It has three built in functions and every other function is run in a child process using unix/bash programs.
// When CTRL-Z is pressed, the shell is put into foreground only mode and will ignore requests for programs to run in the background.
// The three built in commands are exit, status and cd. Program name is smallsh.
// See readme.txt for instructions on how to compile smallsh.c
// -------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

//Function that return

//struct background process
//Contains the pid of a background process and its finished status
//If fin is 0 then it is not finished else it is finished
struct bgProcess
{
    pid_t bg_pid;
    int fin;
};

//Prototypes
void executePrompt(char *);
void prompt();
char* getUserInput();
void changeDirectory(int, char* []);
void getStatus(int);
int tooManyArguments(int, int);
int isBG(int*, char*[]);
void catchSIGINT(int);
void catchSIGTSTP(int);
void removeSymbols(char*[], int);
void expandPID(int, char*[]);

//Global to keep track of if the shell is in the foreground mode or not
//If fgState = 0, program is not in foreground state, else it is
int fgState;

int main()
{
    //Set foreground mode state = 0
    fgState = 0;

    //Define signal handlers
    struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0};
    SIGINT_action.sa_handler = catchSIGINT;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = SA_RESTART;

    SIGTSTP_action.sa_handler = catchSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;

    //Sigaction for signal handlers
    sigaction(SIGINT, &SIGINT_action, NULL);
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    prompt();
    return 0;
}

/*************************************************
 * Function: getUserInput
 * Description: Obtains user input from the command line and returns it as a whole string
 * Params: none
 * Returns: Character Array
 * Pre-conditions: none
 * Post-conditions: Returns string
 * **********************************************/
char* getUserInput()
{
    //How many characters were returned from getline
    int numChars;
    while(1)
    {
        char *line = NULL;
        size_t size;
        //Get user input using get line and store in string literal line
        numChars = getline(&line, &size, stdin);

        //If the number of chars is an error value of -1 then clear errors and restart getting of user input (For case where ^C is invoked while waiting for input)
        if(numChars == -1)
        {
            clearerr(stdin);
        }
        else
        {
            return line;
        }
    }
}
/*************************************************
 * Function: prompt
 * Description: The BIG one. I meant to modularize this more but due to time constraints I was unable to. What is important is that it gets the job done!
 * Runs an infinite loop to continue recieving user input until user exits by typing exit. Tokenates user input by white space and then stores each individual string
 * into an array argv (much like main). Then checks the first argument of this input for a built in function call or valid program name. Runs the code corresponding
 * to each case. This function has so many if statements it may as well be an AI.
 * Params: none
 * Returns: none
 * Pre-conditions: Signal handlers are set up. Global variable fgState is set.
 * Post-conditions: Exited process with an exit call.
 * **********************************************/
void prompt()
{
    
    //Define variables
    char* argv[512];
    char cmdInput[2048];    //Maximum size for each user input string

    //Ints to keep track of certain bvalues
    int i, k, childExitMethod, tempChildExitMethod, inputF, outputF, pidNum, tempArgc;

    //Give childExitMethod an initial value
    wait(childExitMethod);

    //spawnPid for all children
    pid_t spawnPid;


    //Create a signalSet that contains only SIGINT and SIGTSTP so that processes will ignore these when specified since they are built in signals for us
    sigset_t signalSet;
    sigaddset(&signalSet, SIGINT);
    sigaddset(&signalSet, SIGTSTP);

    //Array of bgProcess id's to help with cleanup of background processes. (Max of 256 background processes allowed)
    struct bgProcess pidArr[256];

    //Number of background processes currently stored in array (They will not be removed)
    pidNum = 0;

    //Infinite loop
    while(1)
    {
        //clean cmdInput
        memset(cmdInput, '\0', sizeof(cmdInput));
        for(i=0;i<pidNum;i++)
        {
            //If background process is not completed
            if(!pidArr[i].fin)
            {
                //Check to see if child has terminated, if so, clean it up
                if(waitpid(pidArr[i].bg_pid, &tempChildExitMethod, WNOHANG))
                {
                    //Give PIF of background process when it terminates
                    printf("background pid %d is done: ", pidArr[i].bg_pid);
                    //Exit status?
                    getStatus(tempChildExitMethod);
                    //Set bg PID finished value to true
                    pidArr[i].fin = 1;
                }
            }
        }
        i = 0;

        //Prompt user for input
        printf(": ");
        fflush(stdout);
        strcpy(cmdInput, getUserInput());

        //Tokenate first argument of string by whitespace and store into index 0 of arguments array
        argv[0] = strtok(cmdInput, " \n");

        //Make sure that the user actually entered a command
        if(argv[0] != NULL)
        {
            //Move to next index for strtok storage into argv
            i=1;
            //Tokenate string by whitespace and store into an index of argv while the strtok does not get a NULL value from the string indicating the end of the string
            while((argv[i] = strtok(NULL, " \n")) != NULL)
            {
                i++;
            }

            //Expand any $$ in the user input
            expandPID(i, argv);

            //===================BUILT IN FUNCTIONS===========================//
            if(strcmp(argv[0], "cd") == 0)
            {
                changeDirectory(i, argv);
            }
            else if(strcmp(argv[0], "exit") == 0)
            {
                //Check to see if there were too many arguments given to exit or not
                if(!tooManyArguments(i, 1))
                {
                    //If the number of background processes is not zero...
                    for(i=0;i<pidNum;i++)
                    {
                        //If background process is not completed
                        if(!pidArr[i].fin)
                        {
                            kill(pidArr[i].bg_pid, SIGTERM);
                            //Check to see if child has terminated, if so, clean it up
                            waitpid(pidArr[i].bg_pid, &tempChildExitMethod, 0);
                            printf("background pid %d is done: ", pidArr[i].bg_pid);
                            getStatus(tempChildExitMethod);
                            pidArr[i].fin = 1;
                        }
                    }
                    exit(0);
                }
            }
            else if(strcmp(argv[0], "status") == 0)
            {
                getStatus(childExitMethod);
            }
            //====================END OF BUILT IN FUNCTIONS====================//

            //If the first argument is a # then treat it as a comment.
            else if(argv[0][0] == '#')
            {
                //Dummy action
                sleep(0);
            }

            //If the user input was neither a built in function or a comment...
            else
            {

                //Create new process
                spawnPid = fork();
                
                //Abort if bad process!!!
                if(spawnPid == -1)
                {
                    perror("Bad Process!");
                    fflush(stdout);
                }

                //=============================CHILD FUNCTIONS============================
                else if(spawnPid == 0)
                {

                    //If an & was detected at the end of the command line then make a background process
                    if(isBG(&i, argv))
                    {
                        //Have background child ignore signals
                        sigprocmask(SIG_SETMASK, &signalSet, NULL);

                        //Close stdin
                        close(0);

                        //Redirect stdin to /dev/null
                        inputF = open("/dev/null", O_RDONLY, 0644);
                        dup2(inputF,0);
    
                        //Close stdout
                        close(1);

                        //Redirect stdout to /dev/null
                        outputF = open("/dev/null", O_WRONLY, 0644);
                        dup2(outputF, 1);

                        //Set pointer to address containing the string & to NULL so that it is ignored (Basically removing & from argv)
                        argv[i-1] = NULL;

                        //Decrement number of items in argv (argc) by 1
                        i--;
                    }

                    //Redirect STDIN and STDOUT if needed and remove the symbols associated with those redirects
                    removeSymbols(argv, i);

                    //Switch the program that this process is executing to the one specified by the user
                    execvp(argv[0], argv);

                    //If exec fails then print this message and exit with value 1
                    printf("%s: no such file or directory\n", argv[0]);
                    fflush(stdout);
                    exit(1);
                }

                //==================================PARENT================================
                //Is the child that we just created a background process?
                if(!isBG(&i, argv))
                {
                    //If not, just wait for the foreground child process to complete
                    waitpid(spawnPid, &childExitMethod, 0);

                    //If child was signaled, print that signal
                    if(WIFSIGNALED(childExitMethod))
                    {
                        getStatus(childExitMethod);
                    }
                }

                //If child is a background process
                else
                {
                    //Enter the information of the child background process into the background process data array
                    pidArr[pidNum].bg_pid = spawnPid;
                    pidArr[pidNum].fin = 0;
                    pidNum++;

                    //Indicate that a background process was created
                    printf("background pid is %d\n", spawnPid);
                    fflush(stdout);
                }
            }
        }
    }
}

/*************************************************
 * Function: changeDirectory
 * Description: Change the current working directory of the shell process
 * Params: num CMD arguments, CMD arguments
 * Returns: none
 * Pre-conditions: argc must be an accurate value otherwise segfault is possible
 * Post-conditions: 
 * **********************************************/
void changeDirectory(int argc, char* argv[])
{
    //Make sure that the number of arguments is less than 3
    if(tooManyArguments(argc, 2))
    {
        return;
    }

    //If only cd was specified and no arguments, go to the home directory specified in the ENV variable HOME
    else if(argc == 1)
    {
        chdir(getenv("HOME"));
    }
    else 
    {
        //If ~ was specified as an argument go to HOME else, attempt to change into the directory specified by the user
        if(strcmp(argv[1], "~") == 0)
        {
            chdir(getenv("HOME"));
        }
        chdir(argv[1]);
    }

}

/*************************************************
 * Function: getStatus 
 * Description: Checks to the exit method of a process to see if it was terminated by signal or by exit. Prints out the result.
 * Params: process exit method integer
 * Returns: none
 * Pre-conditions: must recieve valid process exit method integer
 * Post-conditions: 
 * **********************************************/
void getStatus(int exitMethod)
{
    //If terminated via exit
    if(WIFEXITED(exitMethod))
    {
        printf("exit value %d\n", WEXITSTATUS(exitMethod));  
        fflush(stdout);
    }
    //If terminated via signal
    else if(WIFSIGNALED(exitMethod))
    {
        printf("terminated by signal %d\n", WTERMSIG(exitMethod));
        fflush(stdout);
    }
}

/*************************************************
 * Function: tooManyArguments 
 * Description: Checks to see if user entered too many arguments
 * Params: num arguments, num allowed arguments
 * Returns: 1 if too many arguments entered else 0
 * Pre-conditions: none
 * Post-conditions: truth value returned
 * **********************************************/
int tooManyArguments(int argc, int num)
{
    if(argc > num)
    {
        printf("Too many arguments!\n");
        fflush(stdout);
        return 1;
    }
    return 0;
}

/*************************************************
 * Function: isBG 
 * Description: Checks for an & at the end of a set of command line arguments. Also checks if program is in foreground only mode
 * Params: int pointer to num CMD arguments, CMD arguments
 * Returns: 0 if not background, 1 if background
 * Pre-conditions: Number of command line arguments is not zero
 * Post-conditions: Returns correct truth value
 * **********************************************/
int isBG(int* argc, char* argv[])
{
    int i;
    i = *argc;
    //Is the last argument an &?
    if(strcmp(argv[i-1], "&") == 0)
    {
        //Is the shell in foreground only mode?
        if(!fgState)
        {
            return 1;
        }
        //Since the shell is in foreground only mode, remove the & at the end of the cmd line arguments and decrement the number of CMD arguments by one
        else
        {
            argv[i-1] = NULL;
            i--;
            *argc = i;
        }
    }
    return 0;
}

/*************************************************
 * Function: catchSIGINT
 * Description: Holds SIGINT hostage and makes sure that the situation ends poorly for it.
 * Params: signo 
 * Returns: none
 * Pre-conditions: none
 * Post-conditions: none
 * **********************************************/
void catchSIGINT(int signo)
{
    //char* message = "\n: ";
    //write(STDOUT_FILENO, message, 2);
}

/*************************************************
 * Function: catchSIGTSTP
 * Description: Catches the SIGTSTP signal and changes the state of the shell to enter or exit foreground only mode
 * Params: signo 
 * Returns: none
 * Pre-conditions: fgState is either 0 or some other int
 * Post-conditions: fgState is changed to either 0 or 1
 * **********************************************/
void catchSIGTSTP(int signo)
{
    //If we're not in fgState then enter it and notify the user
    if(!fgState)
    {
        fgState = 1;
        char* message = "\nEntering foreground-only mode (& is now ignored)\n: ";
        write(STDOUT_FILENO, message, 52);
    }
    //Else leave foreground state and notify user
    else
    {
        fgState = 0;
        char* message = "\nExiting foreground-only mode\n: ";
        write(STDOUT_FILENO, message, 32);
    }
}

/*************************************************
 * Function: removeSymbols
 * Description: Checks for "<" and ">" in command line arguments and takes necessary steps for redirection of STDOUT and STDIN. Also like its name,
 * removes those symbols associated with redirection and the file names to be redirected to
 * Params: CMD arguments, num CMD arguments
 * Returns: none 
 * Pre-conditions: argc is an accurate value to the number of arguments in argv
 * Post-conditions: Proper redirections made and symbols/fileNames associated with those redirections have been removed from argv. Argc has been decremented accordingly 
 * **********************************************/
void removeSymbols(char* argv[], int argc)
{
    //input and output file pointers, could have used just one but this is easier to read
    int inputF, outputF;

    //Temporary storage when remving symbols, can hold 512 arguments 
    char* tempArgv[512];
    int i, k;
    k = 0;

    //For the number of arguments..
    for(i=0; i<argc; i++)
    {
        //If we are at the LAST argument
        if(i == argc-1)
        {
            //is there a redirection symbol here? That would be an error since the symbol is already the last argument
            if(strcmp(argv[i], "<") == 0 || strcmp(argv[i], ">") == 0)
            {
                printf("no file specificed for redirection\n");
                exit(1);
            }
        }

        // ========================== INPUT ================================
        // If there is an input redirection symbol and the next argument is not empty
        else if(strcmp(argv[i], "<") == 0 && argv[i+1] != NULL)
        {
            //Open file pointer for file specified by redirect
            inputF = open(argv[i+1], O_RDONLY, 0644);

            //If bad open then print error message
            if(inputF == -1)
            {
                printf("cannot open %s for input\n", argv[i+1]);
                exit(1);
            }
            else
            {
                //Close STDIN just in case and redirect
                close(0);
                dup2(inputF, 0);

                //Put X's in place of the symbols in argv as a sort of tombstone for later when we remove them
                strcpy(argv[i], "X");
                strcpy(argv[i+1], "X");
            }

        } 
        // ========================== OUTPUT ============================
        // If there is an output redirection sumbol and the next argument is not empty
        else if(strcmp(argv[i], ">") == 0 && argv[i+1] != NULL)
        {
            //Open file pointer for file specified by redirect
            outputF = open(argv[i+1], O_WRONLY | O_TRUNC, 0644);

            //If bad open then attempt to create a new file
            if(outputF == -1)
            {
                outputF = open(argv[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);

                //If the file creation failed, print an error message
                if(outputF == -1)
                {
                    printf("cannot open %s for writing\n", argv[i+1]);
                    exit(1);
                }
                else
                {
                    //Close STDOUT just in case and redirect
                    close(1);
                    dup2(outputF, 1); 
                    
                    //Put X's in place of the symbols in argv as a sort of tombstone for later on when we remove them
                    strcpy(argv[i], "X");
                    strcpy(argv[i+1], "X");

                }
            }
            //If we didn't create a new file and instead opened one for writing...
            else
            {
                close(1);
                dup2(outputF, 1);
                strcpy(argv[i], "X");
                strcpy(argv[i+1], "X");

            }

        }
    }
    //Get tombstones and replace the pointers to them with null pointers then replace that part with next available non-null pointer in the argv
    for(i=0; i<argc; i++)
    {
        if(strcmp(argv[i], "X") != 0)
        {
            argv[k] = argv[i];
            k++;
        }
        else
        {
            argv[i] = NULL;
        }
    }
}

/*************************************************
 * Function: expandPID
 * Description: Finds an instance of && in a string and expands it into the PID
 * Params: num CMD arguments, CMD arguments
 * Returns: none
 * Pre-conditions: correct number of arguments passed in
 * Post-conditions: any instance of $$ in the CMD arguments is expanded into the process id
 * **********************************************/
void expandPID(int argc, char* argv[])
{
    //Create some temps and clean them
    int i;
    char processID[16];
    memset(processID, '\0', sizeof(processID));
    char newCommand[256];
    memset(newCommand, '\0', sizeof(processID));

    //Loop through every argument
    for(i=0; i<argc; i++)
    {
        //If there is an instance of $$ in a string
        if(strstr(argv[i], "$$") != NULL)
        {
            //Get the PID and store it
            sprintf(processID, "%d", getpid());

            //Get characters that appear after the $$ and store them
            strcpy(newCommand, strstr(argv[i], "$$")+2);
            //Copy the process id into the string starting at the point where the $$ appeared
            strcpy(strstr(argv[i], "$$"), processID);
            //Concatenate the newCommand string onto the expanded process ID string
            strcat(argv[i], newCommand);

        }
    }
}
