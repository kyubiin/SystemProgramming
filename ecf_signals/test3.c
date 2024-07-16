#include <stdio.h>
#include "csapp.h"
#define SIZE 5
int nums[SIZE] = {1, 2, 3, 4, 5};

int main()
{
    int i, total = 0;
    pid_t pid;
    pid = fork();
    if (pid == 0)
    {
        for (i = 0; i < SIZE; i++)
            nums[i] += i;
    }
    else if (pid > 0)
    {
        wait(NULL);
        for (i = 0; i < SIZE; i++)
            total += nums[i];
        printf("Total is % d\n", total);
    }
    return 0;
}