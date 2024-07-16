/*
 * mm.c - A memory-efficient malloc package.
 *
 * This implementation uses an implicit free list with segregated free lists
 * and best-fit placement. Blocks are coalesced immediately upon freeing and
 * allocated blocks are split if the remaining part is large enough to be a
 * valid free block.
 *
 * This implementation balances memory efficiency and allocation speed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * Team Information
 ********************************************************/
team_t team = {
    /* Your student ID */
    "20200152",
    /* Your full name*/
    "Kyubin Kim",
    /* Your email address */
    "kyubin0314@sogang.ac.kr",
};

/* Basic constants and macros */
#define ALIGNMENT 8  // Align size
#define WORD_SIZE 4  // Word size
#define DWORD_SIZE 8 // Double word size

/* Chunk sizes */
#define INITIAL_CHUNK_SIZE (1 << 12)
#define MIN_CHUNK_SIZE (1 << 6)

/* Maximum and minimum macros */
#define MAX_VALUE(x, y) ((x) > (y) ? (x) : (y))
#define MIN_VALUE(x, y) ((x) < (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK_SIZE_AND_ALLOC(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define WRITE_WORD(p, val) (*(unsigned int *)(p) = (unsigned int)(val))

/* Read the size and allocated fields from address p */
#define READ_SIZE(p) ((*(unsigned int *)(p)) & ~0x7)
#define READ_ALLOC(p) ((*(unsigned int *)(p)) & 0x1)
#define READ_WORD(p) (*(unsigned int *)(p))
#define READ_TAG(p) (READ_WORD(p) & 0x2)

/* Given block ptr bp, compute address of its header and footer */
#define GET_HEADER(bp) ((char *)(bp) - WORD_SIZE)
#define GET_FOOTER(bp) ((char *)(bp) + READ_SIZE(GET_HEADER(bp)) - DWORD_SIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define GET_NEXT_BLOCK(bp) ((char *)(bp) + READ_SIZE(GET_HEADER(bp)))
#define GET_PREV_BLOCK(bp) ((char *)(bp) - READ_SIZE(((char *)(bp) - DWORD_SIZE)))

/* Free list macros */
#define GET_NEXT_FREE_PTR(ptr) ((char *)(ptr))
#define GET_NEXT_FREE_REF(ptr) (*(char **)(ptr))
#define GET_PREV_FREE_PTR(ptr) ((char *)(ptr) + WORD_SIZE)
#define GET_PREV_FREE_REF(ptr) (*(char **)(GET_PREV_FREE_PTR(ptr)))

#define NUM_FREE_LISTS 20

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN_SIZE(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

void **segregated_free_lists; // 분리된 free 목록을 위한 포인터 배열
void *prologue_block;         // Prologue block을 위한 포인터

/* heap 확장 및 메모리 할당/free */
static void *extend_heap(size_t size); // heap을 확장하는 함수
void mm_free(void *ptr);
void *mm_malloc(size_t size);
void *mm_realloc(void *ptr, size_t size);

/* 초기화 */
static void initialize_free_list(void);      // free 목록을 초기화하는 함수
static void initialize_prologue_block(void); // Prologue block을 설정하는 함수
int mm_init(void);

/* block 병합 및 배치 */
static void coalesce_with_prev(void *ptr, size_t *size, void **new_ptr); // 이전 block과 합치는 함수
static void coalesce_with_next(void *ptr, size_t *size);                 // 다음 block과 합치는 함수
static void *coalesce(void *ptr);                                        // block을 병합하는 함수
static void *place(void *ptr, size_t asize);                             // block을 배치하는 함수

/* block 삽입 및 삭제 */
static void insert_free_block(void *ptr, size_t size); // free block을 목록에 삽입하는 함수
static void delete_free_block(void *ptr);              // free block을 목록에서 삭제하는 함수
static int find_list_index(size_t size);               // 크기에 맞는 목록 인덱스를 찾는 함수

static void *find_fit(size_t newsize); // 적절한 block을 찾는 함수

/* free 목록을 초기화 */
static void initialize_free_list(void)
{
    segregated_free_lists = (void **)malloc(sizeof(void *) * NUM_FREE_LISTS); // free 목록을 위한 메모리 할당
    if (segregated_free_lists == NULL)
    {
        fprintf(stderr, "Error: Could not initialize free list.\n"); // 오류 메시지 출력
        exit(EXIT_FAILURE);                                          // 프로그램 종료
    }
    for (int index = 0; index < NUM_FREE_LISTS; index++)
    {
        segregated_free_lists[index] = NULL; // 각 목록을 NULL로 초기화
    }
}

/* Prologue block을 설정 */
static void initialize_prologue_block(void)
{
    if ((long)(prologue_block = mem_sbrk(4 * WORD_SIZE)) == -1) // 메모리 할당 요청
    {
        fprintf(stderr, "Error: Could not set prologue.\n"); // 오류 메시지 출력
        exit(EXIT_FAILURE);                                  // 프로그램 종료
    }
    WRITE_WORD(prologue_block, 0);                                                  // 패딩
    WRITE_WORD(prologue_block + 1 * WORD_SIZE, PACK_SIZE_AND_ALLOC(DWORD_SIZE, 1)); // Prologue header
    WRITE_WORD(prologue_block + 2 * WORD_SIZE, PACK_SIZE_AND_ALLOC(DWORD_SIZE, 1)); // Prologue footer
    WRITE_WORD(prologue_block + 3 * WORD_SIZE, PACK_SIZE_AND_ALLOC(0, 1));          // Epilogue header
}

int mm_init(void)
{
    initialize_free_list();                  // free 목록 초기화
    initialize_prologue_block();             // Prologue block 설정
    if (extend_heap(MIN_CHUNK_SIZE) == NULL) // heap 확장
    {
        return -1; // 오류 발생 시 -1 반환
    }
    return 0; // 성공 시 0 반환
}

/* 적절한 목록 인덱스를 결정 */
static int find_list_index(size_t size)
{
    int index = 0;                                 // 인덱스 초기화
    while (size > 1 && index < NUM_FREE_LISTS - 1) // 크기가 1보다 크고 인덱스가 NUM_FREE_LISTS-1보다 작은 동안 반복
    {
        size >>= 1; // 크기를 오른쪽으로 1 비트 시프트
        index++;    // 인덱스 증가
    }
    return index; // 인덱스 반환
}

/* free block을 목록에 삽입 */
static void insert_free_block(void *ptr, size_t size)
{
    int index = find_list_index(size);         // 크기에 맞는 목록 인덱스 찾기
    void *next = segregated_free_lists[index]; // 다음 block
    void *prev = NULL;                         // 이전 block

    // free 목록을 순회하며 적절한 위치 찾기
    while (next != NULL && size > READ_SIZE(GET_HEADER(next)))
    {
        prev = next;                    // 이전 block 업데이트
        next = GET_NEXT_FREE_REF(next); // 다음 block으로 이동
    }

    // block 삽입
    if (next != NULL)
    {
        if (prev != NULL)
        {
            WRITE_WORD(GET_NEXT_FREE_PTR(prev), (unsigned int)ptr); // 이전 block의 다음 포인터 설정
            WRITE_WORD(GET_PREV_FREE_PTR(next), (unsigned int)ptr); // 다음 block의 이전 포인터 설정
            WRITE_WORD(GET_NEXT_FREE_PTR(ptr), (unsigned int)next); // 현재 block의 다음 포인터 설정
            WRITE_WORD(GET_PREV_FREE_PTR(ptr), (unsigned int)prev); // 현재 block의 이전 포인터 설정
        }
        else
        {
            WRITE_WORD(GET_PREV_FREE_PTR(next), (unsigned int)ptr); // 다음 block의 이전 포인터 설정
            WRITE_WORD(GET_NEXT_FREE_PTR(ptr), (unsigned int)next); // 현재 block의 다음 포인터 설정
            WRITE_WORD(GET_PREV_FREE_PTR(ptr), 0);                  // 현재 block의 이전 포인터 설정
            segregated_free_lists[index] = ptr;                     // free 목록의 인덱스 설정
        }
    }
    else
    {
        if (prev != NULL)
        {
            WRITE_WORD(GET_NEXT_FREE_PTR(prev), (unsigned int)ptr); // 이전 block의 다음 포인터 설정
            WRITE_WORD(GET_PREV_FREE_PTR(ptr), (unsigned int)prev); // 현재 block의 이전 포인터 설정
            WRITE_WORD(GET_NEXT_FREE_PTR(ptr), 0);                  // 현재 block의 다음 포인터 설정
        }
        else
        {
            WRITE_WORD(GET_NEXT_FREE_PTR(ptr), 0); // 현재 block의 다음 포인터 설정
            WRITE_WORD(GET_PREV_FREE_PTR(ptr), 0); // 현재 block의 이전 포인터 설정
            segregated_free_lists[index] = ptr;    // free 목록의 인덱스 설정
        }
    }
}

/* free block을 목록에서 삭제 */
static void delete_free_block(void *ptr)
{
    int index = find_list_index(READ_SIZE(GET_HEADER(ptr))); // 크기에 맞는 목록 인덱스 찾기
    void *next = (void *)READ_WORD(GET_NEXT_FREE_PTR(ptr));  // 다음 block
    void *prev = (void *)READ_WORD(GET_PREV_FREE_PTR(ptr));  // 이전 block

    if (next != NULL)
    {
        WRITE_WORD(GET_PREV_FREE_PTR(next), (unsigned int)prev); // 다음 block의 이전 포인터 설정
    }

    if (prev != NULL)
    {
        WRITE_WORD(GET_NEXT_FREE_PTR(prev), (unsigned int)next); // 이전 block의 다음 포인터 설정
    }
    else
    {
        segregated_free_lists[index] = next; // free 목록의 인덱스 설정
    }
}

/* 인접한 free block 병합 */
static void *coalesce(void *ptr)
{
    size_t size = READ_SIZE(GET_HEADER(ptr));                     // 현재 block의 크기
    int prev_alloc = READ_ALLOC(GET_HEADER(GET_PREV_BLOCK(ptr))); // 이전 block의 할당 여부
    int next_alloc = READ_ALLOC(GET_HEADER(GET_NEXT_BLOCK(ptr))); // 다음 block의 할당 여부

    if (READ_TAG(GET_HEADER(GET_PREV_BLOCK(ptr)))) // 이전 block에 태그가 있는지 확인
        prev_alloc = 1;                            // 태그가 있으면 할당된 것으로 간주

    if (prev_alloc == 1 && next_alloc == 1) // 이전과 다음 block 모두 할당된 경우
    {
        return ptr; // 현재 block 반환
    }

    void *new_ptr = ptr; // 새로운 block 포인터 초기화

    if (prev_alloc == 1 && !next_alloc) // 이전 block이 할당되고 다음 block이 free인 경우
    {
        coalesce_with_next(ptr, &size); // 다음 block과 병합
    }
    else if (!prev_alloc && next_alloc == 1) // 이전 block이 free이고 다음 block이 할당된 경우
    {
        coalesce_with_prev(ptr, &size, &new_ptr); // 이전 block과 병합
    }
    else if (!prev_alloc && !next_alloc) // 이전과 다음 block 모두 free인 경우
    {
        delete_free_block(ptr);                                                                          // 현재 block 삭제
        delete_free_block(GET_PREV_BLOCK(ptr));                                                          // 이전 block 삭제
        delete_free_block(GET_NEXT_BLOCK(ptr));                                                          // 다음 block 삭제
        size += READ_SIZE(GET_HEADER(GET_PREV_BLOCK(ptr))) + READ_SIZE(GET_HEADER(GET_NEXT_BLOCK(ptr))); // 크기 업데이트
        new_ptr = GET_PREV_BLOCK(ptr);                                                                   // 새로운 block 포인터 업데이트
        WRITE_WORD(GET_HEADER(new_ptr), PACK_SIZE_AND_ALLOC(size, 0));                                   // header 업데이트
        WRITE_WORD(GET_FOOTER(new_ptr), PACK_SIZE_AND_ALLOC(size, 0));                                   // footer 업데이트
    }

    insert_free_block(new_ptr, size); // 새로운 block 삽입
    return new_ptr;                   // 새로운 block 포인터 반환
}

// free block을 확장하고 분리된 free 목록에 입력
static void coalesce_with_next(void *ptr, size_t *size)
{
    if (ptr == NULL || size == NULL)
    {
        fprintf(stderr, "Error: Invalid pointer or size in coalesce_with_next.\n"); // 오류 메시지 출력
        return;                                                                     // 함수 종료
    }

    void *next_ptr = GET_NEXT_BLOCK(ptr); // 다음 block 포인터
    if (next_ptr == NULL)
    {
        fprintf(stderr, "Error: Next block is null in coalesce_with_next.\n"); // 오류 메시지 출력
        return;                                                                // 함수 종료
    }

    delete_free_block(ptr);                                     // 현재 block 삭제
    delete_free_block(next_ptr);                                // 다음 block 삭제
    *size += READ_SIZE(GET_HEADER(next_ptr));                   // 크기 업데이트
    WRITE_WORD(GET_HEADER(ptr), PACK_SIZE_AND_ALLOC(*size, 0)); // header 업데이트
    WRITE_WORD(GET_FOOTER(ptr), PACK_SIZE_AND_ALLOC(*size, 0)); // footer 업데이트
}

static void coalesce_with_prev(void *ptr, size_t *size, void **new_ptr)
{
    if (ptr == NULL || size == NULL || new_ptr == NULL)
    {
        fprintf(stderr, "Error: Invalid pointer or size in coalesce_with_prev.\n"); // 오류 메시지 출력
        return;                                                                     // 함수 종료
    }

    void *prev_ptr = GET_PREV_BLOCK(ptr); // 이전 block 포인터
    if (prev_ptr == NULL)
    {
        fprintf(stderr, "Error: Previous block is null in coalesce_with_prev.\n"); // 오류 메시지 출력
        return;                                                                    // 함수 종료
    }

    delete_free_block(ptr);                                          // 현재 block 삭제
    delete_free_block(prev_ptr);                                     // 이전 block 삭제
    *size += READ_SIZE(GET_HEADER(prev_ptr));                        // 크기 업데이트
    *new_ptr = prev_ptr;                                             // 새로운 block 포인터 업데이트
    WRITE_WORD(GET_HEADER(*new_ptr), PACK_SIZE_AND_ALLOC(*size, 0)); // header 업데이트
    WRITE_WORD(GET_FOOTER(*new_ptr), PACK_SIZE_AND_ALLOC(*size, 0)); // footer 업데이트
}

/* block을 free 목록에 배치 */
static void *place(void *ptr, size_t asize)
{
    size_t size = READ_SIZE(GET_HEADER(ptr)); // 현재 block의 크기
    size_t free_size = size - asize;          // free block의 크기

    delete_free_block(ptr); // 현재 block 삭제

    if (free_size < 2 * DWORD_SIZE) // free block의 크기가 최소 크기보다 작은 경우
    {
        WRITE_WORD(GET_HEADER(ptr), PACK_SIZE_AND_ALLOC(size, 1)); // header 업데이트
        WRITE_WORD(GET_FOOTER(ptr), PACK_SIZE_AND_ALLOC(size, 1)); // footer 업데이트
    }
    else
    {
        WRITE_WORD(GET_HEADER(ptr), PACK_SIZE_AND_ALLOC(asize, 1)); // header 업데이트
        WRITE_WORD(GET_FOOTER(ptr), PACK_SIZE_AND_ALLOC(asize, 1)); // footer 업데이트

        void *next_block = GET_NEXT_BLOCK(ptr);                                // 다음 block 포인터
        WRITE_WORD(GET_HEADER(next_block), PACK_SIZE_AND_ALLOC(free_size, 0)); // 다음 block header 업데이트
        WRITE_WORD(GET_FOOTER(next_block), PACK_SIZE_AND_ALLOC(free_size, 0)); // 다음 block footer 업데이트
        insert_free_block(next_block, free_size);                              // free block 삽입
    }
    return ptr; // block 포인터 반환
}

/* free 목록에서 적절한 block 찾기 */
static void *find_fit(size_t asize)
{
    int index = find_list_index(asize); // 크기에 맞는 목록 인덱스 찾기
    void *best_fit = NULL;              // 최적의 block 초기화
    size_t smallest_diff = (size_t)-1;  // 가장 작은 차이 초기화

    for (int i = index; i < NUM_FREE_LISTS; i++) // 목록 인덱스부터 시작
    {
        void *p = segregated_free_lists[i]; // free 목록의 block
        while (p != NULL)
        {
            size_t p_size = READ_SIZE(GET_HEADER(p));                // block의 크기
            if (p_size >= asize && (p_size - asize) < smallest_diff) // 적절한 block 찾기
            {
                best_fit = p;                   // 최적의 block 업데이트
                smallest_diff = p_size - asize; // 가장 작은 차이 업데이트
                if (smallest_diff == 0)         // 차이가 0이면 최적의 block
                    return best_fit;            // 최적의 block 반환
            }
            p = GET_NEXT_FREE_REF(p); // 다음 block으로 이동
        }
        if (best_fit != NULL) // 최적의 block이 있으면
            return best_fit;  // 최적의 block 반환
    }
    return NULL; // 적절한 block이 없으면 NULL 반환
}

/* heap을 새로운 free block으로 확장 */
static void *extend_heap(size_t size)
{
    void *ptr = mem_sbrk(size); // 메모리 할당 요청
    if (ptr == (void *)-1)      // 할당 실패 시
    {
        fprintf(stderr, "Error: Could not extend heap.\n"); // 오류 메시지 출력
        return NULL;                                        // NULL 반환
    }

    WRITE_WORD(GET_HEADER(ptr), PACK_SIZE_AND_ALLOC(size, 0));              // header 업데이트
    WRITE_WORD(GET_FOOTER(ptr), PACK_SIZE_AND_ALLOC(size, 0));              // footer 업데이트
    WRITE_WORD(GET_HEADER(GET_NEXT_BLOCK(ptr)), PACK_SIZE_AND_ALLOC(0, 1)); // Epilogue header 업데이트

    insert_free_block(ptr, size); // free block 삽입
    return coalesce(ptr);         // 병합 후 반환
}

/* block 해제 */
void mm_free(void *ptr)
{
    if (ptr == NULL) // 포인터가 NULL이면
    {
        return; // 함수 종료
    }
    size_t size = READ_SIZE(GET_HEADER(ptr));                  // block의 크기
    WRITE_WORD(GET_HEADER(ptr), PACK_SIZE_AND_ALLOC(size, 0)); // header 업데이트
    WRITE_WORD(GET_FOOTER(ptr), PACK_SIZE_AND_ALLOC(size, 0)); // footer 업데이트
    insert_free_block(ptr, size);                              // free block 삽입
    coalesce(ptr);                                             // 병합
}

/* block 할당 */
void *mm_malloc(size_t size)
{
    if (size == 0)   // 크기가 0이면
        return NULL; // NULL 반환

    size_t asize;       // 조정된 크기
    size_t extend_size; // 확장 크기
    void *ptr;          // block 포인터

    if (size <= DWORD_SIZE)     // 크기가 DWORD_SIZE 이하이면
        asize = 2 * DWORD_SIZE; // 조정된 크기 설정
    else
        asize = DWORD_SIZE * ((size + (DWORD_SIZE) + (DWORD_SIZE - 1)) / DWORD_SIZE); // 조정된 크기 계산

    ptr = find_fit(asize); // 적절한 block 찾기

    if (ptr == NULL) // 적절한 block이 없으면
    {
        extend_size = MAX_VALUE(asize, INITIAL_CHUNK_SIZE); // 확장 크기 설정
        ptr = extend_heap(extend_size);                     // heap 확장
        if (ptr == (void *)-1)                              // heap 확장 실패 시
            return NULL;                                    // NULL 반환
    }
    ptr = place(ptr, asize); // block 배치
    return ptr;              // block 포인터 반환
}

/* block 재할당 */
void *mm_realloc(void *ptr, size_t size)
{
    if (size == 0) // 크기가 0이면
    {
        mm_free(ptr); // block 해제
        return NULL;  // NULL 반환
    }

    if (ptr == NULL) // 포인터가 NULL이면
    {
        return mm_malloc(size); // 새 block 할당
    }

    size_t asize; // 조정된 크기

    // 조정된 크기 계산
    if (size <= DWORD_SIZE)
        asize = 2 * DWORD_SIZE;
    else
        asize = DWORD_SIZE * ((size + (DWORD_SIZE) + (DWORD_SIZE - 1)) / DWORD_SIZE);

    // 현재 block이 충분히 큰 경우
    if (READ_SIZE(GET_HEADER(ptr)) >= asize)
        return ptr;

    void *next_block = GET_NEXT_BLOCK(ptr);           // 다음 block 포인터
    size_t current_size = READ_SIZE(GET_HEADER(ptr)); // 현재 block 크기

    // 다음 block이 free block인 경우
    if (!READ_ALLOC(GET_HEADER(next_block)) || !READ_SIZE(GET_HEADER(next_block)))
    {
        size_t next_size = READ_SIZE(GET_HEADER(next_block)); // 다음 block 크기
        size_t total_size = current_size + next_size;         // 총 크기

        if (total_size >= asize) // 총 크기가 조정된 크기보다 크거나 같은 경우
        {
            delete_free_block(next_block); // 다음 block 삭제

            // 새로운 크기로 header와 footer 업데이트
            WRITE_WORD(GET_HEADER(ptr), PACK_SIZE_AND_ALLOC(total_size, 1));
            WRITE_WORD(GET_FOOTER(ptr), PACK_SIZE_AND_ALLOC(total_size, 1));

            return ptr; // block 포인터 반환
        }
    }

    // 현재 block과 이웃 block이 충분하지 않은 경우 새 block 할당
    void *newptr = mm_malloc(asize); // 새 block 할당
    if (newptr == NULL)              // 할당 실패 시
    {
        fprintf(stderr, "Error: Could not allocate memory for realloc.\n"); // 오류 메시지 출력
        return NULL;                                                        // NULL 반환
    }

    // 데이터를 새 block으로 복사
    memcpy(newptr, ptr, MIN_VALUE(size, asize)); // 데이터 복사
    mm_free(ptr);                                // 기존 block 해제

    return newptr; // 새 block 포인터 반환
}
