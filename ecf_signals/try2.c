#include "csapp.h"

void end(void)
{
    printf("2");
    fflush(stdout);
}

int main()
{
    // 첫 번째 자식 프로세스 생성
    if (Fork() == 0)
    {
        atexit(end); // 프로세스 종료 시 end 함수 호출
    }

    // 두 번째 자식 프로세스 생성
    if (Fork() == 0)
    {
        printf("0");
        fflush(stdout);
    }
    else
    {
        printf("1");
        fflush(stdout);
        exit(0); // 부모 프로세스 종료
    }
}
