#include "csapp.h"
#include <errno.h>

#define MAXARGS 128 // 명령어의 최대 인수 개수 정의

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
void execute_commands(char **argv, int *cmd_indices, int cmd_count);

int main()
{
    char cmdline[MAXLINE]; /* Command line */
    char *ret;             // fgets의 반환 값을 저장할 포인터
    while (1)
    {
        /* Read */
        printf("CSE4100-SP-P2> ");
        ret = fgets(cmdline, MAXLINE, stdin); // 표준 입력에서 한 줄을 읽음

        if (feof(stdin)) // 입력의 끝이면 프로그램 종료
            exit(0);

        /* Evaluate */
        eval(cmdline);
    }
    return (0);
}

void execute_commands(char **argv, int *cmd_indices, int cmd_count)
{
    int i, fd[2], in_fd = 0; // i, 파일 디스크립터 배열, 입력 파일 디스크립터 초기화

    for (i = 0; i < cmd_count; i++)
    {
        // 마지막 커맨드가 아니면 파이프 생성
        if (i < cmd_count - 1)
        {
            if (pipe(fd) < 0)
            {
                perror("pipe"); // 파이프 생성 실패 시 에러 메시지 출력
                exit(1);
            }
        }

        switch (fork())
        {
        case -1:
            perror("fork"); // 포크 실패 시 에러 메시지 출력
            exit(1);
        case 0: // 자식 프로세스
            if (in_fd != STDIN_FILENO)
            {
                dup2(in_fd, STDIN_FILENO); // 표준 입력을 in_fd로 복제
                close(in_fd);              // 복제 후 원본 디스크립터를 닫음
            }
            if (i < cmd_count - 1)
            {
                close(fd[0]); // 읽기용 파일 디스크립터 닫기
                if (fd[1] != STDOUT_FILENO)
                {
                    dup2(fd[1], STDOUT_FILENO); // 표준 출력을 fd[1]로 복제
                    close(fd[1]);               // 복제 후 원본 디스크립터를 닫음
                }
            }
            execvp(argv[cmd_indices[i]], &argv[cmd_indices[i]]); // 명령어 실행
            perror("execvp");                                    // 실행 실패 시 에러 메시지 출력
            exit(1);
        default: // 부모 프로세스
            if (in_fd != STDIN_FILENO)
                close(in_fd); // 사용이 끝난 입력 파일 디스크립터 닫기
            if (i < cmd_count - 1)
            {
                close(fd[1]);  // 쓰기용 파일 디스크립터는 닫기
                in_fd = fd[0]; // 다음 명령의 입력으로 사용할 디스크립터 설정
            }
            break;
        }
    }

    // 모든 자식 프로세스가 종료될 때까지 대기
    while (wait(NULL) > 0)
        ;
}

/* eval - Evaluate a command line */
void eval(char *cmdline)
{
    char *argv[MAXARGS];      /* Argument list for execve() */
    char buf[MAXLINE];        /* Holds modified command line */
    int bg;                   /* Should the job run in bg or fg? */
    pid_t pid;                /* Process id for foreground job */
    int status;               /* Status return value for waitpid */
    int cmd_indices[MAXARGS]; /* Command indices for pipe separation */
    int cmd_count = 0;        /* Number of commands separated by pipes */

    strcpy(buf, cmdline);      // cmdline을 buf로 복사
    bg = parseline(buf, argv); // 파싱 함수를 호출하여 백그라운드 실행 여부 확인

    if (argv[0] == NULL)
        return; /* Ignore empty lines */

    cmd_indices[0] = 0; // 첫 번째 명령은 항상 인덱스 0에서 시작
    for (int i = 0; argv[i] != NULL; i++)
    {
        if (strcmp(argv[i], "|") == 0)
        {
            argv[i] = NULL;                   // '|'를 NULL로 대체하여 execvp 사용 가능하게 함
            cmd_indices[++cmd_count] = i + 1; // 다음 명령 시작 인덱스 설정
        }
    }
    cmd_count++; // 전체 명령 수 증가

    if (!builtin_command(argv)) // 내장 명령어가 아닐 경우
    {
        /* If there are pipes, handle with execute_commands */
        if (cmd_count > 1)
        {
            execute_commands(argv, cmd_indices, cmd_count); // 파이프를 포함한 명령어 실행
        }
        else
        {
            /* No pipes, simple command, can run directly */
            if ((pid = Fork()) == 0) // 자식 프로세스
            {
                if (execvp(argv[0], argv) < 0) // 명령어 실행
                {
                    printf("%s: command not found\n", argv[0]); // 명령을 찾을 수 없음
                    exit(0);
                }
            }
        }

        /* Parent waits for foreground job to terminate */
        if (!bg)
        {
            int wait_result;
            while (((wait_result = waitpid(pid, &status, 0)) == -1) && (errno == EINTR))
                continue; // 인터럽트 시 재시도

            if (wait_result == -1 && errno != ECHILD)
            {
                fprintf(stderr, "Error waiting for process %d: %s\n", pid, strerror(errno)); // 오류 메시지 출력
            }
        }
        else /* When there is a background process */
        {
            printf("%d %s", pid, cmdline); // pid와 cmdline 출력
        }
    }

    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv)
{
    if (!strcmp(argv[0], "exit")) /* quit command */
        exit(0);
    if (!strcmp(argv[0], "&")) /* Ignore singleton & */
        return 1;

    if (strcmp(argv[0], "cd") == 0) /* cd */
    {
        int ret = -1; // 오류 기본 설정
        if (argv[1])
        {
            if (strcmp(argv[1], "~") == 0)
            {
                ret = chdir(getenv("HOME"));
            }
            else
            {
                ret = chdir(argv[1]); // 주어진 경로로 디렉토리 변경
            }
        }
        else
        {
            ret = chdir(getenv("HOME"));
        }

        if (ret != 0) // 'cd' 실패 시
        {
            perror("chdir"); // 오류 메시지 출력
        }

        return 1;
    }

    return 0; /* Not a builtin command */
}

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv)
{
    char *delim; /* Points to first space delimiter */
    int argc;    /* Number of args */
    int bg;      /* Background job? */

    buf[strlen(buf) - 1] = ' '; // 줄 끝의 개행 문자를 공백으로 대체

    // 시작 부분의 공백 무시
    while (*buf && (*buf == ' '))
        buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' ')))
    {
        argv[argc++] = buf; // 인수 추가
        *delim = '\0';      // 공백 위치를 NULL로 설정하여 인수 분리
        buf = delim + 1;
        while (*buf && (*buf == ' ')) // 이어지는 공백 무시
            buf++;
    }
    argv[argc] = NULL;

    if (argc == 0) /* Ignore blank line */
        return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0)
        argv[--argc] = NULL; // '&' 제거

    return bg;
}
/* $end parseline */