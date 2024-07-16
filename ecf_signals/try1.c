#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "csapp.h"

int main()
{
    int a = 9;
    if (Fork() == 0)
        printf("p1 : a=%d\n", a--);
    printf("p2 : %d\n", a++);
    exit(0);
}