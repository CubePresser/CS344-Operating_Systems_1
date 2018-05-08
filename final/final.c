#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include <string.h>

int main()
{
    char threeStr[3] = "ab";
    strcpy(threeStr, "abc");
    printf(threeStr);

    return 0;
}

