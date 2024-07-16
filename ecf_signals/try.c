#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main()
{
    pid_t pid1, pid2;

    // 첫 번째 fork 호출
    pid1 = fork();

    if (pid1 == 0)
    {
        // 첫 번째 자식 프로세스
        printf("First child process, PID: %d, Parent PID: %d\n", getpid(), getppid());

        // 첫 번째 자식 프로세스에서 두 번째 fork 호출
        pid2 = fork();
        if (pid2 == 0)
        {
            // 첫 번째 자식의 자식 프로세스
            printf("Child of first child process, PID: %d, Parent PID: %d\n", getpid(), getppid());
        }
        else
        {
            printf("First child process after second fork, Child PID: %d\n", pid2);
        }
    }
    else
    {
        printf("Parent process, PID: %d, First child PID: %d\n", getpid(), pid1);

        // 부모 프로세스에서 두 번째 fork 호출
        pid2 = fork();
        if (pid2 == 0)
        {
            // 부모의 두 번째 자식 프로세스
            printf("Second child process, PID: %d, Parent PID: %d\n", getpid(), getppid());
        }
        else
        {
            printf("Parent process after second fork, Second child PID: %d\n", pid2);
        }
    }

    return 0;
}
