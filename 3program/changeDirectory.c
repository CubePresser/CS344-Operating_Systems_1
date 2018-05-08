#include <stdio.h>
#include <string.h>
#include <stdlib.h>



int main(int argc, char* argv[])
{
    printf("Argc = %d\n", argc);
    if(argc > 2)
    {
        printf("Error: Too many arguments.");
        exit(1);
    }
    printf("%s\n", argv[1]);
    chdir(argv[1]);
    system("pwd");
    return 0;
}
