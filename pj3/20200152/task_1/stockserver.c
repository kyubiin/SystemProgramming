#include "csapp.h"
#define MAXARGS 3

// 클라이언트 연결을 관리하기 위한 구조체
typedef struct
{
    int maxfd;                   // 가장 큰 디스크립터 (select 함수의 첫 번째 매개변수)
    fd_set read_set;             // 모든 활성 디스크립터 집합
    fd_set ready_set;            // 읽기 가능한 디스크립터 집합
    int nready;                  // 준비된 디스크립터 수
    int maxi;                    // clientfd의 가장 큰 인덱스
    int clientfd[FD_SETSIZE];    // 클라이언트 파일 디스크립터 배열
    rio_t clientrio[FD_SETSIZE]; // 클라이언트 RIO 구조체 배열
} ClientPool;

int byte_cnt = 0; // 서버가 받은 총 바이트 수
int listenfd;     // 전역화된 listenfd

// 함수 프로토타입 선언
void init_pool(int listenfd, ClientPool *p);
void add_client(int clientfd, ClientPool *p);
void check_client(ClientPool *p);
void echo(int connfd);

#define MAXID 101

// 주식 정보를 관리하기 위한 구조체
typedef struct Stock
{
    int id;               // 주식 ID
    int left_stock;       // 남은 주식 수
    int price;            // 주식 가격
    struct Stock *left;   // 왼쪽 자식 노드
    struct Stock *right;  // 오른쪽 자식 노드
    pthread_mutex_t lock; // 주식 정보 보호를 위한 뮤텍스
} Stock;

Stock *root = NULL; // 주식 트리의 루트 노드

// 함수 프로토타입 선언
Stock *create_stock_item(int id, int left_stock, int price);
void insert_stock(Stock *new_stock);
Stock *find_stock(int id);
void parse_command(char *buf, char **argv);
void init_stock_data();
void free_stock_tree(Stock *node);

// 명령어 처리 함수
void show_stock(int connfd);
void buy_stock(int connfd, int target_id, int quantity);
void sell_stock(int connfd, int target_id, int quantity);
void close_client_connection(int connfd, int i, ClientPool *p);
void update_stock_data();
void signal_handler(int sig);
void calculate_and_print_elapsed_time();

struct timeval start_time, end_time; // 시작 시간과 종료 시간을 저장할 구조체
#define MAX_CLIENTS 100
int client_count = 0;                                          // 현재 연결된 클라이언트 수
int timing_flag = 1;                                           // 타이밍 플래그
pthread_mutex_t client_count_lock = PTHREAD_MUTEX_INITIALIZER; // 클라이언트 수를 보호하기 위한 뮤텍스
pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;         // 파일 접근을 보호하기 위한 뮤텍스

FILE *stock_file;     // 주식 데이터를 저장할 파일 포인터
char file_path[1024]; // 파일 경로를 저장할 문자열

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    signal(SIGINT, signal_handler); // 시그널 핸들러 설정
    init_stock_data();              // 주식 데이터 초기화

    int connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    static ClientPool pool;

    listenfd = Open_listenfd(argv[1]); // 리스닝 소켓 열기
    init_pool(listenfd, &pool);        // 클라이언트 풀 초기화

    while (1)
    {
        pool.ready_set = pool.read_set;                                          // 읽기 가능한 디스크립터 집합 초기화
        pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL); // 준비된 디스크립터 수 확인

        if (timing_flag)
        {
            timing_flag = 0;
            gettimeofday(&start_time, NULL); // 시작 시간 저장
        }

        if (FD_ISSET(listenfd, &pool.ready_set))
        {
            clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 클라이언트 연결 수락
            Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE,
                        client_port, MAXLINE, 0);                            // 클라이언트 정보 가져오기
            printf("Connected to (%s, %s)\n", client_hostname, client_port); // 연결된 클라이언트 정보 출력

            pthread_mutex_lock(&client_count_lock);
            client_count++; // 클라이언트 수 증가
            pthread_mutex_unlock(&client_count_lock);

            add_client(connfd, &pool); // 클라이언트 추가
        }

        check_client(&pool); // 클라이언트 확인
    }

    free_stock_tree(root); // 주식 트리 메모리 해제
    exit(0);
}

void init_pool(int listenfd, ClientPool *p)
{
    p->maxi = -1;
    for (int i = 0; i < FD_SETSIZE; i++)
        p->clientfd[i] = -1; // 모든 클라이언트 파일 디스크립터 초기화
    FD_ZERO(&p->read_set);   // 읽기 집합 초기화

    p->maxfd = listenfd;            // 최대 파일 디스크립터 설정
    FD_SET(listenfd, &p->read_set); // 리스닝 소켓을 읽기 집합에 추가
}

void add_client(int clientfd, ClientPool *p)
{
    int i;
    for (i = 0; i < FD_SETSIZE; i++)
    {
        if (p->clientfd[i] < 0)
        {
            p->clientfd[i] = clientfd;                 // 클라이언트 파일 디스크립터 저장
            FD_SET(clientfd, &p->read_set);            // 읽기 집합에 추가
            Rio_readinitb(&p->clientrio[i], clientfd); // RIO 초기화

            if (clientfd > p->maxfd)
                p->maxfd = clientfd; // 최대 파일 디스크립터 업데이트
            if (i > p->maxi)
                p->maxi = i; // 최대 인덱스 업데이트
            break;
        }
    }

    if (i == FD_SETSIZE)
        app_error("add_client error: Too many clients"); // 클라이언트 초과 시 에러 처리

    p->nready--; // 준비된 디스크립터 수 감소
}

void check_client(ClientPool *p)
{
    int i, connfd, n;
    rio_t rio;
    char buf[MAXLINE] = {0};

    for (i = 0; (p->nready > 0) && (i <= p->maxi); i++)
    {
        if (p->clientfd[i] < 0)
            continue;

        connfd = p->clientfd[i];
        rio = p->clientrio[i];
        if (FD_ISSET(connfd, &p->ready_set))
        {
            p->nready--;

            if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
            {
                byte_cnt += n; // 받은 바이트 수 증가
                printf("Server received %d (%d total) bytes on fd %d\n", n, byte_cnt, connfd);

                char *argv[MAXARGS];
                parse_command(buf, argv); // 명령어 파싱

                if (!strcmp(argv[0], "show"))
                    show_stock(connfd); // 주식 정보 출력
                else if (!strcmp(argv[0], "buy"))
                    buy_stock(connfd, atoi(argv[1]), atoi(argv[2])); // 주식 구매
                else if (!strcmp(argv[0], "sell"))
                    sell_stock(connfd, atoi(argv[1]), atoi(argv[2])); // 주식 판매
                else if (!strcmp(argv[0], "exit"))
                    close_client_connection(connfd, i, p); // 클라이언트 연결 종료
                else
                {
                    char str[] = "Invalid Command\n";
                    Rio_writen(connfd, str, MAXLINE); // 잘못된 명령어 처리
                }
            }
            else
            {
                close_client_connection(connfd, i, p); // 클라이언트 연결 종료
            }
        }
    }

    pthread_mutex_lock(&client_count_lock);
    if (client_count == 0 && !timing_flag)
    {
        timing_flag = 1;
        calculate_and_print_elapsed_time(); // 경과 시간 출력
    }
    pthread_mutex_unlock(&client_count_lock);
}

void close_client_connection(int connfd, int i, ClientPool *p)
{
    pthread_mutex_lock(&client_count_lock);
    client_count--; // 클라이언트 수 감소
    pthread_mutex_unlock(&client_count_lock);

    Close(connfd);                // 클라이언트 연결 닫기
    FD_CLR(connfd, &p->read_set); // 읽기 집합에서 제거
    p->clientfd[i] = -1;          // 파일 디스크립터 초기화

    if (i == p->maxi)
        p->maxi--; // 최대 인덱스 업데이트
    if (connfd == p->maxfd)
    {
        p->maxfd = listenfd; // 최대 파일 디스크립터 업데이트
        for (int j = 0; j <= p->maxi; j++)
        {
            if (p->clientfd[j] > p->maxfd)
                p->maxfd = p->clientfd[j]; // 최대 파일 디스크립터 찾기
        }
    }
}

Stock *create_stock_item(int id, int left_stock, int price)
{
    Stock *new_stock = (Stock *)malloc(sizeof(Stock));
    new_stock->id = id;                 // 주식 ID 설정
    new_stock->left_stock = left_stock; // 남은 주식 수 설정
    new_stock->price = price;           // 주식 가격 설정
    new_stock->left = NULL;
    new_stock->right = NULL;
    pthread_mutex_init(&new_stock->lock, NULL); // 뮤텍스 초기화
    return new_stock;
}

void free_stock_tree(Stock *node)
{
    if (node == NULL)
        return;

    free_stock_tree(node->left);  // 왼쪽 서브트리 메모리 해제
    free_stock_tree(node->right); // 오른쪽 서브트리 메모리 해제

    pthread_mutex_destroy(&node->lock); // 뮤텍스 파괴
    free(node);                         // 노드 메모리 해제
}

void init_stock_data()
{
    if (getcwd(file_path, sizeof(file_path)) == NULL)
    {
        perror("getcwd error");
        exit(EXIT_FAILURE);
    }
    strcat(file_path, "/stock.txt"); // 파일 경로 설정

    stock_file = fopen(file_path, "a");
    if (stock_file == NULL)
    {
        perror("Failed to open file for appending");
        exit(EXIT_FAILURE);
    }
    fclose(stock_file);

    stock_file = fopen(file_path, "r");
    if (stock_file == NULL)
    {
        perror("Failed to open file for reading");
        exit(EXIT_FAILURE);
    }
    while (!feof(stock_file))
    {
        char stock_data[1024];
        if (!fgets(stock_data, MAXLINE, stock_file))
            break;

        char *endptr;
        char *token = strtok(stock_data, " ");
        int id = strtol(token, &endptr, 10);

        token = strtok(NULL, " ");
        int stock = strtol(token, &endptr, 10);

        token = strtok(NULL, " ");
        int price = strtol(token, &endptr, 10);

        Stock *new_stock = create_stock_item(id, stock, price); // 새 주식 항목 생성
        insert_stock(new_stock);                                // 주식 트리에 삽입
    }
    fclose(stock_file);
}

void insert_stock(Stock *new_stock)
{
    if (root == NULL)
        root = new_stock; // 루트가 비어 있으면 새로운 주식 항목을 루트로 설정
    else
    {
        Stock *parent = NULL, *current = root;
        while (current != NULL)
        {
            parent = current;
            if (new_stock->id < current->id)
                current = current->left;
            else
                current = current->right;
        }
        if (new_stock->id < parent->id)
            parent->left = new_stock;
        else
            parent->right = new_stock;
    }
}

Stock *find_stock(int id)
{
    Stock *current = root;
    while (current != NULL)
    {
        if (id == current->id)
            return current; // 주식 ID가 일치하면 해당 주식 항목 반환
        else if (id > current->id)
            current = current->right;
        else
            current = current->left;
    }
    return NULL; // 찾지 못하면 NULL 반환
}

void show_stock(int connfd)
{
    if (root == NULL)
        return;

    Stock *stack[MAXID];
    int top = -1;
    stack[++top] = root;

    char str[MAXLINE] = {0};
    while (top != -1)
    {
        Stock *iter = stack[top--];
        char buf[RIO_BUFSIZE];
        sprintf(buf, "%d %d %d\n", iter->id, iter->left_stock, iter->price);
        strcat(str, buf);

        if (iter->left != NULL)
            stack[++top] = iter->left;

        if (iter->right != NULL)
            stack[++top] = iter->right;
    }

    Rio_writen(connfd, str, MAXLINE); // 주식 정보를 클라이언트에 전송
}

void buy_stock(int connfd, int target_id, int quantity)
{
    Stock *stock = find_stock(target_id);
    if (stock == NULL)
    {
        char str[] = "Stock not found\n";
        Rio_writen(connfd, str, MAXLINE); // 주식이 없으면 에러 메시지 전송
        return;
    }

    pthread_mutex_lock(&stock->lock);
    if (stock->left_stock >= quantity)
    {
        stock->left_stock -= quantity; // 주식 구매 처리
        char str[] = "[buy] success\n";
        Rio_writen(connfd, str, MAXLINE); // 성공 메시지 전송
    }
    else
    {
        char str[] = "Not enough left stock\n";
        Rio_writen(connfd, str, MAXLINE); // 남은 주식이 부족하면 에러 메시지 전송
    }
    pthread_mutex_unlock(&stock->lock);
    update_stock_data(); // 주식 데이터 업데이트
}

void sell_stock(int connfd, int target_id, int quantity)
{
    Stock *stock = find_stock(target_id);
    if (stock == NULL)
    {
        char str[] = "Stock not found\n";
        Rio_writen(connfd, str, MAXLINE); // 주식이 없으면 에러 메시지 전송
        return;
    }

    pthread_mutex_lock(&stock->lock);
    stock->left_stock += quantity; // 주식 판매 처리
    char str[] = "[sell] success\n";
    Rio_writen(connfd, str, MAXLINE); // 성공 메시지 전송
    pthread_mutex_unlock(&stock->lock);
    update_stock_data(); // 주식 데이터 업데이트
}

void update_stock_data()
{
    pthread_mutex_lock(&file_lock);

    stock_file = fopen(file_path, "w");
    if (stock_file == NULL)
    {
        perror("Failed to open file for writing");
        pthread_mutex_unlock(&file_lock);
        return;
    }
    if (root != NULL)
    {
        Stock *stack[MAXID];
        int top = -1;
        stack[++top] = root;

        while (top != -1)
        {
            Stock *iter = stack[top--];
            char buf[RIO_BUFSIZE];
            sprintf(buf, "%d %d %d\n", iter->id, iter->left_stock, iter->price);
            fputs(buf, stock_file); // 주식 데이터를 파일에 저장

            if (iter->left != NULL)
                stack[++top] = iter->left;

            if (iter->right != NULL)
                stack[++top] = iter->right;
        }
    }
    fclose(stock_file);

    pthread_mutex_unlock(&file_lock);
}

void parse_command(char *buf, char **argv)
{
    char *delim;
    int argc;

    buf[strlen(buf) - 1] = ' ';
    while (*buf && (*buf == ' '))
        buf++;

    argc = 0;
    while ((delim = strchr(buf, ' ')))
    {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' '))
            buf++;
    }
    argv[argc] = NULL;
}

void signal_handler(int sig)
{
    if (sig == SIGINT)
    {
        printf("Received SIGINT, saving stock data\n");
        update_stock_data();   // 주식 데이터 저장
        free_stock_tree(root); // 주식 트리 메모리 해제
        exit(0);
    }
}

void calculate_and_print_elapsed_time()
{
    gettimeofday(&end_time, NULL);                                                  // 종료 시간 기록
    long elapsed_seconds = end_time.tv_sec - start_time.tv_sec;                     // 초 단위 시간 계산
    long elapsed_microseconds = end_time.tv_usec - start_time.tv_usec;              // 마이크로초 단위 시간 계산
    double total_elapsed_time = elapsed_seconds + elapsed_microseconds / 1000000.0; // 총 경과 시간 계산
    printf("Elapsed Time: %.6f seconds\n", total_elapsed_time);                     // 경과 시간 출력
    fflush(stdout);                                                                 // 출력 버퍼 비우기
}
