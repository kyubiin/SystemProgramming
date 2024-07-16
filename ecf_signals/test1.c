#include <stdio.h>
#include "csapp.h"

int main()
{
    int pid, pid1;
    pid = fork();
    if (pid < 0)
    {
        printf("Fork Failed\n");
        return 1;
    }
    else if (pid == 0)
    {
        pid1 = getpid();              /* Get pid of current process */
        printf("%d %d\n", pid, pid1); /* Line A */
    }
    else
    {
        pid1 = getpid();              /* Get pid of current process */
        printf("%d %d\n", pid, pid1); /* Line B */
        wait(NULL);
    }
    return 0;
}