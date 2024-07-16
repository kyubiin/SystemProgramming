#include "csapp.h"

#define MAXARGS 3
#define MAXID 101     // 최대 ID 정의
#define NTHREADS 100  // 스레드 개수 정의
#define SBUFSIZE 100  // 버퍼 크기 정의
#define CLIENTNUM 100 // 클라이언트 수 정의

typedef struct
{
    int *buf;    // 버퍼 포인터
    int n;       // 버퍼 크기
    int front;   // 버퍼 앞쪽 인덱스
    int rear;    // 버퍼 뒷쪽 인덱스
    sem_t mutex; // mutex semaphore
    sem_t slots; // 슬롯 semaphore
    sem_t items; // 아이템 semaphore
} sbuf_t;

sbuf_t sbuf; // 공유 버퍼 구조체 선언

void sbuf_init(sbuf_t *sp, int n);        // 공유 버퍼 초기화 함수 선언
void sbuf_insert(sbuf_t *sp, int connfd); // 공유 버퍼에 삽입 함수 선언
int sbuf_remove(sbuf_t *sp);              // 공유 버퍼에서 제거 함수 선언

int byte_count = 0; // 바이트 카운터 초기화
int listenfd;       // listen 소켓 파일 디스크립터

void handle_client(int connfd); // 클라이언트 처리 함수 선언

typedef struct Stock
{
    int id;              // 재고 ID
    int left_stock;      // 남은 재고 수량
    int price;           // 가격
    struct Stock *left;  // 왼쪽 자식 노드 포인터
    struct Stock *right; // 오른쪽 자식 노드 포인터
    int read_count;      // 읽기 카운트
    sem_t mutex;         // mutex semaphore
    sem_t write;         // 쓰기 semaphore
} Stock;

Stock *root = NULL; // 루트 노드 초기화

Stock *create_stock_item(int id, int left_stock, int price); // 재고 항목 생성 함수 선언
void parse_command(char *buf, char **argv);                  // 명령어 파싱 함수 선언
void init_server();                                          // 서버 초기화 함수 선언

FILE *stock_file;           // 파일 포인터
char file_path[1024];       // 파일 경로
pthread_mutex_t file_mutex; // 파일 잠금용 mutex

void insert_stock(Stock *new_stock);                      // 재고 삽입 함수 선언
Stock *find_stock(int id);                                // 재고 찾기 함수 선언
void show_stock(int connfd);                              // 재고 표시 함수 선언
void buy_stock(int connfd, int target_id, int quantity);  // 재고 구매 함수 선언
void sell_stock(int connfd, int target_id, int quantity); // 재고 판매 함수 선언
void update_stock_data();                                 // 재고 데이터 업데이트 함수 선언

void *thread(void *vargp); // 스레드 함수 선언

struct timeval start_time, end_time; // 시작 및 종료 시간 변수
sem_t time_mutex;                    // 시간 관련 mutex
int client_count = 0;                // 클라이언트 카운터 초기화
int timing_flag = 1;                 // 타이밍 플래그 초기화

void sigint_handler(int sig)
{
    printf("Caught SIGINT, updating stock data and exiting\n"); // SIGINT 수신 시 메시지 출력
    update_stock_data();                                        // 재고 데이터 업데이트
    exit(0);                                                    // 프로그램 종료
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

int main(int argc, char **argv)
{ // 메인 함수
    if (argc != 2)
    {                                                   // 인자 개수가 2개가 아니면
        fprintf(stderr, "usage: %s <port>\n", argv[0]); // 사용법 출력
        exit(0);                                        // 종료
    }

    init_server(); // 서버 초기화

    int *connfd;                                         // 연결 소켓 파일 디스크립터 포인터
    socklen_t clientlen;                                 // 클라이언트 주소 길이
    struct sockaddr_storage clientaddr;                  // 클라이언트 주소 구조체
    char client_hostname[MAXLINE], client_port[MAXLINE]; // 클라이언트 호스트 이름과 포트

    listenfd = Open_listenfd(argv[1]); // listen 소켓 열기
    if (listenfd < 0)
    {                                                        // 소켓 열기 실패 시
        fprintf(stderr, "Error opening listening socket\n"); // 오류 메시지 출력
        exit(1);                                             // 종료
    }

    while (1)
    {                                                // 무한 루프
        clientlen = sizeof(struct sockaddr_storage); // 클라이언트 주소 길이 초기화
        connfd = Malloc(sizeof(int));                // 연결 소켓 파일 디스크립터 메모리 할당
        if (connfd == NULL)
        {                                      // 메모리 할당 실패 시
            fprintf(stderr, "Malloc error\n"); // 오류 메시지 출력
            continue;                          // 루프 계속
        }
        *connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); // 연결 수락
        if (*connfd < 0)
        {                                      // 연결 수락 실패 시
            fprintf(stderr, "Accept error\n"); // 오류 메시지 출력
            Free(connfd);                      // 메모리 해제
            continue;                          // 루프 계속
        }

        if (timing_flag)
        {                                    // 타이밍 플래그가 설정되어 있으면
            timing_flag = 0;                 // 타이밍 플래그 해제
            client_count = CLIENTNUM;        // 클라이언트 수 초기화
            gettimeofday(&start_time, NULL); // 시작 시간 기록
        }

        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0); // 클라이언트 정보 얻기
        printf("Connected to (%s, %s)\n", client_hostname, client_port);                              // 연결된 클라이언트 정보 출력
        sbuf_insert(&sbuf, *connfd);                                                                  // 공유 버퍼에 삽입
    }
    exit(0); // 종료
}

void *thread(void *vargp)
{                                   // 스레드 함수
    Pthread_detach(pthread_self()); // 스레드 분리
    Free(vargp);                    // 메모리 해제

    while (1)
    {                                    // 무한 루프
        int connfd = sbuf_remove(&sbuf); // 공유 버퍼에서 제거
        handle_client(connfd);           // 클라이언트 처리
        Close(connfd);                   // 소켓 닫기

        P(&time_mutex); // mutex 잠금
        client_count--; // 클라이언트 수 감소
        if (client_count == 0 && !timing_flag)
        {                    // 모든 클라이언트가 처리되었고 타이밍 플래그가 해제되었으면
            timing_flag = 1; // 타이밍 플래그 설정
            calculate_and_print_elapsed_time();
        }
        V(&time_mutex); // mutex 해제
    }

    return NULL; // NULL 반환
}

void handle_client(int connfd)
{                                // 클라이언트 처리 함수
    int n;                       // 읽은 바이트 수
    char buf[MAXLINE];           // 버퍼
    rio_t rio;                   // RIO 구조체
    Rio_readinitb(&rio, connfd); // RIO 구조체 초기화
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0)
    {                                                                                    // 클라이언트로부터 데이터 읽기
        P(&time_mutex);                                                                  // mutex 잠금
        byte_count += n;                                                                 // 바이트 카운터 증가
        V(&time_mutex);                                                                  // mutex 해제
        printf("Server received %d (%d total) bytes on fd %d\n", n, byte_count, connfd); // 수신 바이트 수 출력

        char *argv[MAXARGS];      // 명령어 인자 배열
        parse_command(buf, argv); // 명령어 파싱

        if (!strcmp(argv[0], "show"))
        {                       // show 명령어 처리
            show_stock(connfd); // 재고 표시
        }
        else if (!strcmp(argv[0], "buy"))
        {                                                    // buy 명령어 처리
            buy_stock(connfd, atoi(argv[1]), atoi(argv[2])); // 재고 구매
        }
        else if (!strcmp(argv[0], "sell"))
        {                                                     // sell 명령어 처리
            sell_stock(connfd, atoi(argv[1]), atoi(argv[2])); // 재고 판매
        }
        else if (!strcmp(argv[0], "exit"))
        {          // exit 명령어 처리
            break; // 루프 탈출
        }
        else
        {                                     // 잘못된 명령어 처리
            char str[] = "Invalid Command\n"; // 오류 메시지
            Rio_writen(connfd, str, MAXLINE); // 클라이언트에 오류 메시지 전송
        }
    }
}

void init_server()
{ // 서버 초기화 함수
    if (getcwd(file_path, sizeof(file_path)) == NULL)
    {                           // 현재 작업 디렉토리 얻기 실패 시
        perror("getcwd error"); // 오류 메시지 출력
        exit(EXIT_FAILURE);     // 종료
    }
    strcat(file_path, "/stock.txt"); // 파일 경로 설정

    root = NULL;                        // 루트 노드 초기화
    stock_file = fopen(file_path, "a"); // 파일 열기 (추가 모드)
    if (stock_file != NULL)
        fclose(stock_file); // 파일 닫기

    stock_file = fopen(file_path, "r"); // 파일 열기 (읽기 모드)
    if (stock_file != NULL)
    {
        while (!feof(stock_file))
        {                          // 파일 끝까지 읽기
            char stock_data[1024]; // 재고 데이터 버퍼
            if (!fgets(stock_data, MAXLINE, stock_file))
                break; // 파일 읽기 실패 시 루프 탈출

            char *endptr;                          // 문자열 끝 포인터
            char *token = strtok(stock_data, " "); // 공백을 기준으로 문자열 자르기
            int id = strtol(token, &endptr, 10);   // 문자열을 정수로 변환

            token = strtok(NULL, " ");
            int stock = strtol(token, &endptr, 10);

            token = strtok(NULL, " ");
            int price = strtol(token, &endptr, 10);

            Stock *new_stock = create_stock_item(id, stock, price); // 새 재고 항목 생성
            insert_stock(new_stock);                                // 재고 삽입
        }
        fclose(stock_file); // 파일 닫기
    }

    signal(SIGINT, sigint_handler); // SIGINT 시그널 핸들러 설정

    pthread_t tid; // 스레드 ID
    for (int i = 0; i < NTHREADS; i++)
    { // 스레드 생성
        Pthread_create(&tid, NULL, thread, NULL);
    }

    sbuf_init(&sbuf, SBUFSIZE); // 공유 버퍼 초기화

    Sem_init(&time_mutex, 0, 1);           // 시간 관련 mutex 초기화
    pthread_mutex_init(&file_mutex, NULL); // 파일 잠금용 mutex 초기화
}

void sbuf_init(sbuf_t *sp, int n)
{                                     // 공유 버퍼 초기화 함수
    sp->buf = Calloc(n, sizeof(int)); // 버퍼 메모리 할당 및 초기화
    sp->n = n;                        // 버퍼 크기 설정
    sp->front = sp->rear = 0;         // 앞쪽과 뒷쪽 인덱스 초기화
    Sem_init(&sp->mutex, 0, 1);       // mutex semaphore 초기화
    Sem_init(&sp->slots, 0, n);       // 슬롯 semaphore 초기화
    Sem_init(&sp->items, 0, 0);       // 아이템 semaphore 초기화
}

void sbuf_insert(sbuf_t *sp, int connfd)
{                                           // 공유 버퍼에 삽입 함수
    P(&sp->slots);                          // 슬롯 semaphore 잠금
    P(&sp->mutex);                          // mutex semaphore 잠금
    sp->buf[(sp->rear++) % sp->n] = connfd; // 연결 소켓 파일 디스크립터 삽입
    V(&sp->mutex);                          // mutex semaphore 해제
    V(&sp->items);                          // 아이템 semaphore 해제
}

int sbuf_remove(sbuf_t *sp)
{                                                // 공유 버퍼에서 제거 함수
    P(&sp->items);                               // 아이템 semaphore 잠금
    P(&sp->mutex);                               // mutex semaphore 잠금
    int connfd = sp->buf[(sp->front++) % sp->n]; // 연결 소켓 파일 디스크립터 제거
    V(&sp->mutex);                               // mutex semaphore 해제
    V(&sp->slots);                               // 슬롯 semaphore 해제
    return connfd;                               // 제거한 연결 소켓 파일 디스크립터 반환
}

Stock *create_stock_item(int id, int left_stock, int price)
{                                                      // 재고 항목 생성 함수
    Stock *new_stock = (Stock *)malloc(sizeof(Stock)); // 재고 항목 메모리 할당
    new_stock->id = id;                                // ID 설정
    new_stock->left_stock = left_stock;                // 남은 재고 수량 설정
    new_stock->price = price;                          // 가격 설정
    new_stock->left = NULL;                            // 왼쪽 자식 노드 초기화
    new_stock->right = NULL;                           // 오른쪽 자식 노드 초기화
    new_stock->read_count = 0;                         // 읽기 카운트 초기화
    Sem_init(&new_stock->mutex, 0, 1);                 // mutex semaphore 초기화
    Sem_init(&new_stock->write, 0, 1);                 // 쓰기 semaphore 초기화
    return new_stock;                                  // 새 재고 항목 반환
}

void insert_stock(Stock *new_stock)
{ // 재고 삽입 함수
    if (root == NULL)
        root = new_stock; // 루트 노드가 비어있으면 루트로 설정
    else
    {
        Stock *parent = NULL, *current = root; // 부모와 현재 노드 포인터 초기화
        while (current != NULL)
        {
            parent = current; // 부모 노드 설정
            if (new_stock->id < current->id)
                current = current->left; // 새로운 재고 ID가 작으면 왼쪽으로 이동
            else
                current = current->right; // 새로운 재고 ID가 크면 오른쪽으로 이동
        }
        if (new_stock->id < parent->id)
            parent->left = new_stock; // 새로운 재고 ID가 부모보다 작으면 왼쪽 자식으로 설정
        else
            parent->right = new_stock; // 새로운 재고 ID가 부모보다 크면 오른쪽 자식으로 설정
    }
}

Stock *find_stock(int id)
{                          // 재고 찾기 함수
    Stock *current = root; // 현재 노드를 루트로 설정
    while (current != NULL)
    {
        if (id == current->id)
            return current; // 찾는 ID와 현재 노드 ID가 같으면 반환
        else if (id > current->id)
            current = current->right; // 찾는 ID가 크면 오른쪽으로 이동
        else
            current = current->left; // 찾는 ID가 작으면 왼쪽으로 이동
    }
    return NULL; // 찾는 재고가 없으면 NULL 반환
}

void show_stock(int connfd)
{ // 재고 표시 함수
    if (root == NULL)
        return; // 루트가 비어있으면 반환

    Stock *stack[MAXID]; // 스택 배열
    int top = -1;        // 스택 탑 초기화
    stack[++top] = root; // 루트 노드를 스택에 추가

    char str[MAXLINE] = {0}; // 문자열 버퍼 초기화
    while (top != -1)
    {
        Stock *iter = stack[top--]; // 스택에서 노드 꺼내기

        P(&iter->mutex);    // mutex 잠금
        iter->read_count++; // 읽기 카운트 증가
        if (iter->read_count == 1)
            P(&iter->write); // 첫 번째 읽기 시 쓰기 잠금
        V(&iter->mutex);     // mutex 해제

        char buf[RIO_BUFSIZE];                                               // 버퍼
        sprintf(buf, "%d %d %d\n", iter->id, iter->left_stock, iter->price); // 재고 정보 포맷팅
        strcat(str, buf);                                                    // 문자열에 추가

        P(&iter->mutex);    // mutex 잠금
        iter->read_count--; // 읽기 카운트 감소
        if (iter->read_count == 0)
            V(&iter->write); // 마지막 읽기 시 쓰기 잠금 해제
        V(&iter->mutex);     // mutex 해제

        if (iter->left != NULL)
            stack[++top] = iter->left; // 왼쪽 자식 노드 스택에 추가

        if (iter->right != NULL)
            stack[++top] = iter->right; // 오른쪽 자식 노드 스택에 추가
    }

    Rio_writen(connfd, str, MAXLINE); // 클라이언트에 재고 정보 전송
}

void buy_stock(int connfd, int target_id, int quantity)
{                                         // 재고 구매 함수
    Stock *stock = find_stock(target_id); // 재고 찾기
    if (stock == NULL)
    {
        char str[] = "Stock not found\n"; // 재고를 찾을 수 없는 경우
        Rio_writen(connfd, str, MAXLINE); // 클라이언트에 메시지 전송
        return;
    }

    P(&stock->write); // 쓰기 잠금
    if (stock->left_stock >= quantity)
    {
        stock->left_stock -= quantity;    // 남은 재고 감소
        char str[] = "[buy] success\n";   // 성공 메시지
        Rio_writen(connfd, str, MAXLINE); // 클라이언트에 메시지 전송
        update_stock_data();              // 재고 데이터 업데이트
    }
    else
    {
        char str[] = "Not enough left stock\n"; // 재고가 부족한 경우
        Rio_writen(connfd, str, MAXLINE);       // 클라이언트에 메시지 전송
    }
    V(&stock->write); // 쓰기 잠금 해제
}

void sell_stock(int connfd, int target_id, int quantity)
{                                         // 재고 판매 함수
    Stock *stock = find_stock(target_id); // 재고 찾기
    if (stock == NULL)
    {
        char str[] = "Stock not found\n"; // 재고를 찾을 수 없는 경우
        Rio_writen(connfd, str, MAXLINE); // 클라이언트에 메시지 전송
        return;
    }

    P(&stock->write);                 // 쓰기 잠금
    stock->left_stock += quantity;    // 남은 재고 증가
    char str[] = "[sell] success\n";  // 성공 메시지
    Rio_writen(connfd, str, MAXLINE); // 클라이언트에 메시지 전송
    update_stock_data();              // 재고 데이터 업데이트
    V(&stock->write);                 // 쓰기 잠금 해제
}

void update_stock_data()
{                                    // 재고 데이터 업데이트 함수
    pthread_mutex_lock(&file_mutex); // 파일 잠금

    stock_file = fopen(file_path, "w"); // 파일 열기 (쓰기 모드)
    if (stock_file == NULL)
    {
        perror("Failed to open file for writing"); // 파일 열기 실패 시 오류 메시지 출력
        pthread_mutex_unlock(&file_mutex);         // 파일 잠금 해제
        return;
    }

    if (root != NULL)
    {
        Stock *stack[MAXID]; // 스택 배열
        int top = -1;        // 스택 탑 초기화
        stack[++top] = root; // 루트 노드를 스택에 추가

        while (top != -1)
        {
            Stock *iter = stack[top--];                                          // 스택에서 노드 꺼내기
            char buf[RIO_BUFSIZE];                                               // 버퍼
            sprintf(buf, "%d %d %d\n", iter->id, iter->left_stock, iter->price); // 재고 정보 포맷팅
            fputs(buf, stock_file);                                              // 파일에 쓰기

            if (iter->left != NULL)
                stack[++top] = iter->left; // 왼쪽 자식 노드 스택에 추가

            if (iter->right != NULL)
                stack[++top] = iter->right; // 오른쪽 자식 노드 스택에 추가
        }
    }
    fclose(stock_file);                // 파일 닫기
    pthread_mutex_unlock(&file_mutex); // 파일 잠금 해제
}

void parse_command(char *buf, char **argv)
{                // 명령어 파싱 함수
    char *delim; // 구분자 포인터
    int argc;    // 인자 개수

    buf[strlen(buf) - 1] = ' '; // 마지막 문자를 공백으로 변경
    while (*buf && (*buf == ' '))
        buf++; // 공백 건너뛰기

    argc = 0;
    while ((delim = strchr(buf, ' ')))
    {
        argv[argc++] = buf; // 인자 추가
        *delim = '\0';      // 구분자를 널 문자로 변경
        buf = delim + 1;    // 다음 위치로 이동
        while (*buf && (*buf == ' '))
            buf++; // 공백 건너뛰기
    }
    argv[argc] = NULL; // 마지막 인자 널 포인터
}
