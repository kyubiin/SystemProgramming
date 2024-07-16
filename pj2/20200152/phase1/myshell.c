#include "csapp.h"
#include <errno.h>

#define MAXARGS 128 // 최대 인수 개수를 128로 정의

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);

int main()
{
    char cmdline[MAXLINE]; /* Command line */

    while (1)
    {
        /* Read */
        printf("CSE4100-SP-P2> ");
        Fgets(cmdline, MAXLINE, stdin);
        if (feof(stdin))
            exit(0); // 파일 끝이면 종료

        /* Evaluate */
        eval(cmdline);
    }
}
/* $end shellmain */

/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline)
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    int status;          /* Status return value for waitpid */
    pid_t pid;           /* Process id */

    strcpy(buf, cmdline);      // cmdline을 buf로 복사
    bg = parseline(buf, argv); // 파싱 함수를 호출하여 백그라운드 실행 여부 확인
    if (argv[0] == NULL)
        return; /* Ignore empty lines */

    if (!builtin_command(argv))  // 내장 명령어가 아니면 실행
    {                            // quit -> exit(0), & -> ignore, other -> run
        if ((pid = Fork()) == 0) // 자식 프로세스
        {
            if (execvp(argv[0], argv) < 0)
            {
                printf("%s: command not found\n", argv[0]); // 명령어 찾기 실패
            }
            exit(0);
        }

        /* Parent waits for foreground job to terminate */
        if (!bg)
        {
            if (waitpid(pid, &status, 0) < 0) // 자식 프로세스의 종료를 기다림
            {
                if (errno != ECHILD)
                {
                    fprintf(stderr, "Error waiting for process %d: %s\n", pid, strerror(errno));
                }
            }
        }
        else
        {                                  /* When there is a background process */
            printf("%d %s", pid, cmdline); // pid와 cmdline 출력
        }
    }
    return;
}

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv)
{
    char *delim; /* Points to first space delimiter */
    int argc;    /* Number of args */
    int bg;      /* Background job? */

    /*deal with "!!" and "!#"*/

    buf[strlen(buf) - 1] = ' '; // 줄 끝의 개행 문자를 공백으로 대체

    // 시작 부분의 공백 무시
    while (*buf && (*buf == ' '))
        buf++;

    /* Build the argv list */
    argc = 0;
    while (*buf)
    {
        // 시작하는 공백 건너뛰기
        while (*buf && (*buf == ' '))
            buf++;
        if (!*buf)
            break; // Exit loop if end of buffer reached
        argv[argc++] = buf;
        // 다음 인수로 이동
        while (*buf && (*buf != ' '))
            buf++;
        if (!*buf)
            break;     // Exit loop if end of buffer reached
        *buf++ = '\0'; // 인수를 널로 종료
    }
    argv[argc] = NULL;

    if (argc == 0) /* Ignore blank line */
        return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0)
        argv[--argc] = NULL; // '&' 제거

    return bg;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv)
{
    if (!strcmp(argv[0], "exit")) /* quit command */
    {
        exit(0);
    }
    if (!strcmp(argv[0], "&")) /* Ignore singleton & */
        return 1;

    if (!strcmp(argv[0], "cd")) /* cd */
    {
        char *dir = argv[1] ? argv[1] : getenv("HOME"); // 다음 인자가 없으면 HOME으로 이동
        if (dir == NULL)
        {
            fprintf(stderr, "cd: HOME environment variable not set\n");
        }
        else if (chdir(dir) != 0)
        {
            // chdir 실패 시 오류 메시지 출력
            fprintf(stderr, "cd: %s: %s\n", dir, strerror(errno));
        }
        return 1;
    }

    return 0; /* Not a builtin command */
}