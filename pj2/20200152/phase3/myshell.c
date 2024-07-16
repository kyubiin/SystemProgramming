#include "csapp.h"
#include <errno.h>
#define MAXARGS 128
#define MAX_CMD_LINE 100
#define FG 1
#define BG 2
#define R 1
#define S 0

// job 구조체 정의
struct job_t
{
    pid_t pid;                  // Process ID
    int job_idx;                // Job index
    int state;                  // FG or BG
    int r_state;                // Running(R) or Stopped(S)
    char cmdline[MAX_CMD_LINE]; // Command line
    struct job_t *next;         // Next job in the list
};

struct job_t *jobs = NULL; // job 목록을 가리키는 포인터
int next_job_index = 1;    // 다음 job에 할당할 인덱스

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
void execute_commands(char **argv, int *cmd_indices, int cmd_count);
char *format_cmdline(char *cmdline);
/* Job */
struct job_t *getjob(struct job_t *jobs, int job_idx);
void waitfg(pid_t pid);
/* Signal Handler */
void schld_handler(int sig);
void sint_handler(int sig);
void ststp_handler(int sig);

// job 목록 초기화 함수
void initjobs(struct job_t *jobs)
{
    jobs->pid = 0;
    jobs->job_idx = 0;
    jobs->state = 0;
    jobs->r_state = -1;
    jobs->next = NULL;
}

// 현재 포어그라운드에서 실행 중인 job의 PID 반환
pid_t fgpid(struct job_t *jobs)
{
    struct job_t *current = jobs->next;
    while (current != NULL)
    {
        if (current->state == FG)
        {
            return current->pid;
        }
        current = current->next;
    }
    return 0; // 포어그라운드 job이 없는 경우 0 반환
}

// pid로부터 jid 반환
int pid2jid(pid_t pid)
{
    struct job_t *current = jobs->next;
    while (current != NULL)
    {
        if (current->pid == pid)
        {
            return current->job_idx;
        }
        current = current->next;
    }
    return 0; // 해당 PID를 가진 job이 없는 경우 0 반환
}

// 새로운 백그라운드 job 추가
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline)
{
    if (pid < 1)
        return 0;

    struct job_t *new_job = malloc(sizeof(struct job_t));
    if (!new_job)
    {
        perror("malloc error");
        return 0;
    }

    new_job->pid = pid;
    new_job->state = state;
    new_job->job_idx = next_job_index++;
    strcpy(new_job->cmdline, cmdline);
    new_job->next = NULL;
    // 백그라운드 job이 실행 상태로 시작되어야 함
    new_job->r_state = R; // 여기에서 running 상태로 설정

    struct job_t *current = jobs;
    while (current->next != NULL)
    {
        current = current->next;
    }
    current->next = new_job;

    return 1;
}

// 지정된 PID를 가진 job을 삭제
int deletejob(struct job_t *jobs, pid_t pid)
{
    struct job_t *current = jobs;
    struct job_t *prev = NULL;

    while (current != NULL && current->pid != pid)
    {
        prev = current;
        current = current->next;
    }

    if (current == NULL)
    {
        printf("Job with PID %d not found.\n", pid);
        return 0;
    }

    if (prev == NULL)
    { // 첫 번째 job 삭제
        jobs->next = current->next;
    }
    else
    { // 중간 또는 마지막 job 삭제
        prev->next = current->next;
    }

    free(current);
    return 1;
}

// 현재 job 목록 출력
void listjobs(struct job_t *jobs)
{
    struct job_t *current = jobs->next; // 첫 번째 job부터 시작

    if (current == NULL)
    {
        printf("No jobs currently.\n");
    }
    else
    {
        while (current != NULL)
        {
            char *r_state_str = current->r_state == R ? "running" : "suspended";
            char *formatted_cmd = format_cmdline(current->cmdline);
            if (formatted_cmd == NULL)
            {
                perror("Failed to duplicate string");
                return;
            }

            printf("[%d] %s %s\n", current->job_idx, r_state_str, formatted_cmd);
            free(formatted_cmd);     // 메모리 해제
            current = current->next; // 다음 job으로 이동
        }
    }
}

// 명령어 수정
char *format_cmdline(char *cmdline)
{
    // 입력된 명령어 문자열을 복사하여 새로운 메모리 공간에 할당
    char *formatted = strdup(cmdline);
    if (!formatted)
    {
        // 메모리 할당에 실패한 경우 오류 메시지를 출력하고 NULL을 반환
        perror("Failed to duplicate string");
        return NULL;
    }

    // 앞쪽 공백을 제거하기 위한 포인터를 초기화
    char *start = formatted;
    char *end;

    // 문자열의 시작 부분에서 공백 문자를 건너뛰기
    while (*start && isspace((unsigned char)*start))
        start++;

    // 문자열의 시작 부분을 앞으로 이동시켜 공백이 제거된 새로운 문자열을 만들기
    memmove(formatted, start, strlen(start) + 1);

    // 문자열의 끝 부분에서 공백이나 특수 문자('&')를 제거
    end = formatted + strlen(formatted) - 1;
    while (end > formatted && (isspace((unsigned char)*end) || *end == '&'))
        *end-- = '\0';

    // 문자열 내의 중복 공백을 하나의 공백으로 축소
    char *src = formatted, *dest = formatted;
    while (*src)
    {
        *dest++ = *src;
        if (isspace((unsigned char)*src))              // 현재 문자가 공백일 경우
            while (isspace((unsigned char)*(src + 1))) // 다음 문자도 공백이면 건너뛰기
                src++;
        src++;
    }
    *dest = '\0'; // 문자열의 끝을 나타내는 널 문자를 설정

    return formatted; // 정리된 문자열을 반환
}

// job 인덱스로 job 구조체 반환
struct job_t *getjob(struct job_t *jobs, int job_idx)
{
    struct job_t *current = jobs->next; // 첫 번째 job부터 시작

    while (current != NULL)
    {
        if (current->job_idx == job_idx)
        {
            return current; // 해당 job 인덱스를 가진 job을 찾으면 반환
        }
        current = current->next; // 다음 job으로 이동
    }
    return NULL; // 해당 job 인덱스를 가진 job이 없으면 NULL 반환
}

// pid로 job 구조체 반환
struct job_t *getjobpid(struct job_t *jobs, pid_t pid)
{
    struct job_t *current = jobs->next;

    while (current != NULL)
    {
        if (current->pid == pid)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// 포어그라운드 job이 종료될 때까지 대기
void waitfg(pid_t pid)
{
    sigset_t mask, prev_mask;

    // 모든 시그널을 블록하고 SIGCHLD, SIGINT, SIGTSTP만 허용
    Sigfillset(&mask);         // 모든 시그널을 블록
    Sigdelset(&mask, SIGCHLD); // SIGCHLD 처리를 위해 삭제
    Sigdelset(&mask, SIGINT);  // SIGINT 처리를 위해 삭제
    Sigdelset(&mask, SIGTSTP); // SIGTSTP 처리를 위해 삭제

    // 이전 시그널 마스크 저장 및 새 마스크 설정
    Sigprocmask(SIG_BLOCK, &mask, &prev_mask);

    // 포어그라운드 job이 실행 중인 동안 대기
    while (fgpid(jobs) == pid)
    {
        Sigsuspend(&prev_mask);
    }

    // 원래의 시그널 마스크로 복원
    Sigprocmask(SIG_SETMASK, &prev_mask, NULL);
}

// SIGCHLD 시그널 처리
void schld_handler(int sig)
{
    int olderrno = errno;
    sigset_t mask_all, prev_all;
    pid_t pid;
    int status;

    Sigfillset(&mask_all);
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0)
    {
        Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);

        struct job_t *job = getjobpid(jobs, pid);
        if (job == NULL)
        {
            Sigprocmask(SIG_SETMASK, &prev_all, NULL);
            continue;
        }

        if (WIFEXITED(status) || WIFSIGNALED(status))
        {
            deletejob(jobs, pid); // 종료된 프로세스 삭제
        }
        else if (WIFSTOPPED(status))
        {
            job->r_state = S; // job 실행 상태를 중지로 변경
            // printf("Job [%d] (%d) stopped by signal %d\n", job->job_idx, pid, WSTOPSIG(status));
        }
        else if (WIFCONTINUED(status))
        {
            job->r_state = R; // job 실행 상태를 실행 중으로 변경
        }

        Sigprocmask(SIG_SETMASK, &prev_all, NULL);
    }

    errno = olderrno;
}

// SIGINT 시그널 처리
void sint_handler(int sig)
{
    int olderrno = errno;
    pid_t pid = fgpid(jobs); // 포어그라운드 job의 PID를 가져옴

    if (pid != 0)
    {
        Kill(-pid, SIGINT); // 해당 프로세스 그룹에 SIGINT 시그널 전송
    }

    errno = olderrno;
}

void ststp_handler(int sig)
{
    static volatile int inside_handler = 0; // 중복 호출 방지를 위한 플래그
    if (inside_handler)
        return; // 이미 핸들러 내부에 있으면 바로 반환
    inside_handler = 1;

    int olderrno = errno;
    sigset_t mask_all, prev_all;
    pid_t pid = fgpid(jobs);

    Sigfillset(&mask_all);
    Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);

    if (pid != 0)
    {
        struct job_t *job = getjobpid(jobs, pid);
        if (job != NULL)
        {
            Kill(pid, SIGTSTP); // 해당 프로세스에 SIGTSTP 시그널 전송
            job->r_state = S;   // 작업 상태를 중지로 변경
            job->state = BG;    // 작업 상태를 백그라운드로 변경
            // printf("Job [%d] (%d) stopped by signal %d\n", job->job_idx, pid, sig);
        }
    }

    Sigprocmask(SIG_SETMASK, &prev_all, NULL);
    errno = olderrno;
    inside_handler = 0; // 핸들러 완료 후 플래그 초기화
}

int main()
{
    char cmdline[MAXLINE]; /* Command line */
    char *ret;
    Signal(SIGCHLD, schld_handler);
    Signal(SIGINT, sint_handler);
    Signal(SIGTSTP, ststp_handler);

    jobs = malloc(sizeof(struct job_t)); // job 목록 할당
    initjobs(jobs);                      // job 목록 초기화

    while (1)
    {
        /* Read */
        printf("CSE4100-SP-P2> ");
        ret = fgets(cmdline, MAXLINE, stdin);
        //
        if (feof(stdin))
            exit(0);

        /* Evaluate */
        eval(cmdline);
    }
    return (0);
}

void eval(char *cmdline)
{
    char *argv[MAXARGS];      // Argument list for execve()
    char buf[MAXLINE];        // Holds modified command line
    int bg;                   // Should the job run in bg or fg?
    pid_t pid;                // Process id
    int cmd_indices[MAXARGS]; // Command indices for pipe separation
    int cmd_count = 0;        // Number of commands separated by pipes

    strcpy(buf, cmdline);      // cmdline을 buf로 복사
    bg = parseline(buf, argv); // 파싱 함수를 호출하여 백그라운드 실행 여부 확인

    if (argv[0] == NULL)
        return; // Ignore empty lines

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
        if (cmd_count > 1)
        {                            /* If there are pipes, handle with execute_commands */
            if ((pid = Fork()) == 0) // 자식 프로세스
            {
                execute_commands(argv, cmd_indices, cmd_count); // 파이프를 포함한 명령어 실행
                exit(0);
            }
        }
        else
        { // Handle single commands without pipes
            if ((pid = Fork()) == 0)
            {                  // Child runs user job
                Setpgid(0, 0); // Ensure the child is in a new process group
                if (execvp(argv[0], argv) < 0)
                {
                    printf("%s: command not found\n", argv[0]);
                    exit(0);
                }
            }
        }

        // 포어그라운드 job의 경우, 종료까지 대기
        if (!bg)
        {
            addjob(jobs, pid, FG, cmdline); // Add foreground job to jobs list
            waitfg(pid);                    // Wait for foreground job to terminate
        }
        else
        {
            addjob(jobs, pid, BG, cmdline); // Add background job to jobs list
            // printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline); // Print background job info
        }
    }
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
                close(fd[0]); // 읽기용 파일 디스크립터는 닫기
                if (fd[1] != STDOUT_FILENO)
                {
                    dup2(fd[1], STDOUT_FILENO); // 표준 출력을 fd[1]로 복제
                    close(fd[1]);               // 복제 후 원본 디스크립터를 닫음
                }
            }
            // 커맨드 실행
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

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv)
{
    if (!strcmp(argv[0], "exit")) /* quit command */
        exit(0);

    if (!strcmp(argv[0], "&")) /* Ignore singleton & */
        return 1;

    if (strcmp(argv[0], "cd") == 0) /* cd */
    {
        int ret = -1; // 오류 처리를 위해 기본적으로 -1로 설정
        if (argv[1])
        {
            if (strcmp(argv[1], "~") == 0) // '~' 입력 시 홈 디렉토리로 이동
            {
                ret = chdir(getenv("HOME"));
            }
            else
            {
                ret = chdir(argv[1]); // 입력된 경로로 디렉토리 변경을 시도
            }
        }
        else
        {
            ret = chdir(getenv("HOME")); // 경로 입력이 없을 때 홈 디렉토리로 이동
        }

        if (ret != 0) // 'cd' 명령 실패 시 오류 메시지를 출력
        {
            perror("chdir");
        }

        return 1; // 'cd' 명령어를 처리하였으므로 1을 반환하여 더 이상의 명령어 처리가 필요없음을 나타냄
    }

    if (!strcmp(argv[0], "jobs")) /* jobs */
    {
        listjobs(jobs); // 등록된 모든 job을 리스트로 출력
        return 1;
    }

    if (!strcmp(argv[0], "kill")) /* kill */
    {
        if (argv[1] == NULL || argv[1][0] != '%') // job 번호가 제대로 입력되지 않은 경우 사용법 출력
        {
            printf("Usage: kill %%jobnumber\n");
            return 1;
        }
        int job_id = atoi(argv[1] + 1); // '%' 다음에 오는 job 번호를 정수로 변환
        struct job_t *job = getjob(jobs, job_id);
        if (job == NULL) // 지정된 job이 존재하지 않으면 메시지를 출력
        {
            printf("No Such Job\n");
            return 1;
        }
        Kill(-job->pid, SIGKILL); // 해당 job을 kill
        return 1;
    }

    // "bg" 명령어를 처리
    if (!strcmp(argv[0], "bg"))
    {
        if (argv[1] == NULL) // job 번호가 제대로 입력되지 않은 경우 사용법 출력
        {
            printf("%s command requires a job id argument\n", argv[0]);
            return 1;
        }
        int job_id = atoi(argv[1] + 1); // '%' 다음에 오는 job 번호를 정수로 변환
        struct job_t *job = getjob(jobs, job_id);
        if (job == NULL) // 지정된 job이 존재하지 않으면 메시지를 출력
        {
            printf("No Such Job\n");
            return 1;
        }

        Kill(-job->pid, SIGCONT); // 해당 job에 계속 실행(SIGCONT) 시그널을 send
        job->state = BG;          // job의 상태를 백그라운드로 설정
        job->r_state = R;         // job의 실행 상태를 Running으로 설정

        char *formatted_cmd = format_cmdline(job->cmdline);             // 명령어를 포맷팅
        printf("[%d] %s %s\n", job->job_idx, "running", formatted_cmd); // 포맷된 명령어와 상태를 출력
        free(formatted_cmd);                                            // 할당된 메모리를 해제
        return 1;
    }

    if (!strcmp(argv[0], "fg")) /* fg */
    {
        if (argv[1] == NULL) // job 번호가 제대로 입력되지 않은 경우 사용법을 출력
        {
            printf("%s command requires a job id argument\n", argv[0]);
            return 1;
        }
        int job_id = atoi(argv[1] + 1); // '%' 다음에 오는 job 번호를 정수로 변환
        struct job_t *job = getjob(jobs, job_id);
        if (job == NULL) // 지정된 job이 존재하지 않으면 메시지를 출력
        {
            printf("No Such Job\n");
            return 1;
        }

        Kill(-job->pid, SIGCONT); // 해당 job에 계속 실행(SIGCONT) 시그널을 send
        job->state = FG;          // job의 상태를 포어그라운드로 설정
        job->r_state = R;         // job의 실행 상태를 Running으로 설정

        char *formatted_cmd = format_cmdline(job->cmdline);             // 명령어를 포맷팅
        printf("[%d] %s %s\n", job->job_idx, "running", formatted_cmd); // 포맷된 명령어와 상태를 출력
        free(formatted_cmd);                                            // 할당된 메모리를 해제

        waitfg(job->pid); // 포어그라운드에서 실행 중인 job이 종료될 때까지 wait
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
    int bg = 0;  /* Background job? */

    buf[strlen(buf) - 1] = '\0'; // 줄 끝의 개행 문자를 널 문자로 대체

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

    if (*buf != '\0') // 버퍼의 마지막 부분이 널 문자가 아니라면, 마지막 인수 추가
    {
        argv[argc++] = buf;
    }

    /* Should the job run in the background? */
    if (argc > 0 && (bg = (argv[argc - 1][strlen(argv[argc - 1]) - 1] == '&')))
    {
        argv[argc - 1][strlen(argv[argc - 1]) - 1] = '\0'; // '&' 문자 제거
        if (strlen(argv[argc - 1]) == 0)                   // 인수가 '&' 문자만 있었다면
            argc--;                                        // 마지막 인수 제거
    }

    argv[argc] = NULL; // 인수 배열의 끝을 널로 설정

    return bg;
}
/* $end parseline */