#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    printf("p");
    fflush(stdout);

    if (fork() != 0)
    {
        printf("q");
        fflush(stdout);
        return 0; // 부모 프로세스 종료
    }
    else
    {
        printf("r");
        fflush(stdout);
        waitpid(-1, NULL, 0); // 자식 프로세스가 종료될 때까지 대기
    }
    return 0;
}
