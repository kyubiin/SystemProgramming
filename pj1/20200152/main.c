#include "list.h"
#include "hash.h"
#include "bitmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct list_item
{
    struct list_elem elem;
    int data;
    // Other members you want
};

struct hash_item
{
    struct hash_elem elem;
    int data;
    // Other members you want
};

// 리스트 컨테이너 구조체 정의
typedef struct ListContainer_
{
    char name[20];     // 리스트의 이름
    struct list *link; // 실제 리스트 구조체에 대한 포인터
} ListContainer;
ListContainer listContainers[10]; // 리스트 컨테이너 배열 선언

// 해시 컨테이너 구조체 정의
typedef struct HashContainer_
{
    char name[20];     // 해시의 이름
    struct hash *link; // 실제 해시 테이블 구조체에 대한 포인터
} HashContainer;
HashContainer hashContainers[10]; // 해시 컨테이너 배열 선언

// 비트맵 컨테이너 구조체 정의
typedef struct BitmapContainer_
{
    char name[20];       // 비트맵의 이름
    struct bitmap *link; // 실제 비트맵 구조체에 대한 포인터
} BitmapContainer;
BitmapContainer bitmapContainers[10]; // 비트맵 컨테이너 배열 선언

// 주어진 이름으로 자료구조의 인덱스를 찾는 함수
int findStructureIndex(char c, char *inst)
{
    int i = 0;
    int cnt = 0;

    // 각 자료구조 유형('l' = 리스트, 'h' = 해시, 'b' = 비트맵)에 따라 현재 요소의 개수를 설정
    switch (c)
    {
    case 'l':                                                     // 리스트의 경우
        cnt = sizeof(listContainers) / sizeof(listContainers[0]); // 전체 배열 크기를 개별 요소 크기로 나누어 요소 수를 계산
        break;
    case 'h': // 해시의 경우
        cnt = sizeof(hashContainers) / sizeof(hashContainers[0]);
        break;
    case 'b': // 비트맵의 경우
        cnt = sizeof(bitmapContainers) / sizeof(bitmapContainers[0]);
        break;
    default:
        return -1; // 알 수 없는 자료구조 유형일 경우 -1을 반환
    }

    // 이름이 일치하는 자료구조의 인덱스를 찾기 위해 반복문을 실행
    for (i = 0; i < cnt; i++)
    {
        char *currentName = NULL; // 현재 확인 중인 자료구조의 이름을 저장할 포인터

        // 자료구조 유형에 따라 해당 이름을 currentName에 할당
        if (c == 'l')
            currentName = listContainers[i].name;
        else if (c == 'h')
            currentName = hashContainers[i].name;
        else if (c == 'b')
            currentName = bitmapContainers[i].name;

        // 현재 이름이 입력받은 이름(inst)과 일치하는지 확인
        if (currentName && !strcmp(inst, currentName))
        {
            return i; // 일치하는 첫 번째 인덱스를 반환
        }
    }

    return -1; // 주어진 이름과 일치하는 자료구조를 찾지 못한 경우 -1을 반환
}

// 두 리스트 항목을 비교하는 함수
bool compareListItems(const struct list_elem *a, const struct list_elem *b, void *sub)
{
    struct list_item *l_i1, *l_i2;

    l_i1 = list_entry(a, struct list_item, elem); // 첫 번째 리스트 항목 변환
    l_i2 = list_entry(b, struct list_item, elem); // 두 번째 리스트 항목 변환

    return l_i1->data < l_i2->data; // 첫 번째 데이터가 두 번째 데이터보다 작은지 비교
}

// 두 해시 항목을 비교하는 함수
bool compareHashItems(const struct hash_elem *a, const struct hash_elem *b, void *sub)
{
    struct hash_item *h_i1, *h_i2;

    h_i1 = hash_entry(a, struct hash_item, elem); // 첫 번째 해시 항목 변환
    h_i2 = hash_entry(b, struct hash_item, elem); // 두 번째 해시 항목 변환

    return h_i1->data < h_i2->data; // 첫 번째 데이터가 두 번째 데이터보다 작은지 비교
}

// 해시 아이템 데이터를 해싱하는 함수
unsigned hashItemData(const struct hash_elem *e, void *sub)
{
    struct hash_item *temp;

    temp = hash_entry(e, struct hash_item, elem); // 해시 항목 변환
    return hash_int(temp->data);                  // 해시 데이터 반환
};

// 해시 요소의 데이터 값을 제곱하는 함수
void square(struct hash_elem *e, void *sub)
{
    struct hash_item *temp;
    temp = hash_entry(e, struct hash_item, elem); // hash_elem으로부터 hash_item 구조체로 변환
    temp->data *= temp->data;                     // 데이터 값을 제곱
}

// 해시 요소의 데이터 값을 세제곱하는 함수
void triple(struct hash_elem *e, void *sub)
{
    struct hash_item *temp;
    temp = hash_entry(e, struct hash_item, elem);
    // hash_elem으로부터 hash_item 구조체로 변환
    temp->data *= (temp->data) * (temp->data); // 데이터 값을 세제곱
}

// 해시 요소를 소멸하는 함수
void destructor(struct hash_elem *e, void *sub)
{
    struct hash_item *temp;
    temp = hash_entry(e, struct hash_item, elem); // hash_elem으로부터 hash_item 구조체로 변환
    free(temp);                                   // 할당된 메모리를 해제
}

int main()
{

    // 변수 선언
    char input[50], input_tmp[50];
    char cmd[50];
    char tmp1[10], tmp2[10], tmp3[10], tmp4[10], tmp5[10];

    int l_cnt = 0, l_idx = -1;
    int h_cnt = 0, h_idx = -1;
    size_t b_cnt = 0, b_idx = -1;

    while (1) // 무한 반복
    {
        fgets(input, sizeof(input), stdin);
        input[strlen(input) - 1] = '\0';
        strcpy(input_tmp, input);
        strcpy(cmd, strtok(input_tmp, " "));

        //---------------create delete dumpdata quit------------------//

        // "create"
        if (!strcmp(cmd, "create"))
        {
            strcpy(tmp1, strtok(NULL, " ")); // 자료구조 타입 추출(list, hash, bitmap 중 하나)
            if (!strncmp(tmp1, "list", 4))   // 리스트 생성
            {
                listContainers[l_cnt].link = (struct list *)malloc(sizeof(struct list)); // 리스트 메모리 할당
                list_init(listContainers[l_cnt].link);                                   // 리스트 초기화
                strcpy(tmp2, strtok(NULL, " "));                                         // 리스트 이름
                strcpy(listContainers[l_cnt].name, tmp2);                                // 리스트 컨테이너에 이름 저장
                l_cnt++;                                                                 // 리스트 개수 증가
            }
            else if (!strncmp(tmp1, "bitmap", 6)) // 비트맵 생성
            {
                strcpy(tmp2, strtok(NULL, " "));                       // 비트맵 이름
                strcpy(tmp3, strtok(NULL, " "));                       // 비트맵 크기
                size_t bit_cnt = atoi(tmp3);                           // 문자열을 정수로 변환
                bitmapContainers[b_cnt].link = bitmap_create(bit_cnt); // 비트맵 생성 및 메모리 할당
                strcpy(bitmapContainers[b_cnt].name, tmp2);            // 비트맵 컨테이너에 이름 저장
                b_cnt++;                                               // 비트맵 개수 증가
            }
            else if (!strncmp(tmp1, "hash", 4)) // 해시 테이블 생성
            {
                void *sub;
                hashContainers[h_cnt].link = (struct hash *)malloc(sizeof(struct hash)); // 해시 메모리 할당
                struct hash_elem *e;
                hash_init(hashContainers[h_cnt].link, hashItemData, compareHashItems, sub); // 해시 초기화
                strcpy(tmp2, strtok(NULL, " "));                                            // 해시 이름
                strcpy(hashContainers[h_cnt].name, tmp2);                                   // 해시 컨테이너에 이름 저장
                h_cnt++;                                                                    // 해시 개수 증가
            }
        }

        // "delete"
        else if (!strcmp(cmd, "delete"))
        {

            strcpy(tmp1, strtok(NULL, " ")); // 삭제할 자료구조의 이름
            for (int i = 0; i < 10; i++)
            {
                if (!strcmp(tmp1, listContainers[i].name))
                {
                    for (int j = i; j < 9; j++)
                    {
                        listContainers[j] = listContainers[j + 1];
                    }
                    break;
                }
                else if (!strcmp(tmp1, bitmapContainers[i].name))
                {
                    bitmap_destroy(bitmapContainers[i].link);
                    break;
                }
                else if (!strcmp(tmp1, hashContainers[i].name))
                {
                    hash_destroy(hashContainers[i].link, destructor);
                    break;
                }
            }
        }

        // "dumpdata"
        else if (!strcmp(cmd, "dumpdata"))
        {

            strcpy(tmp1, strtok(NULL, " "));
            if (!strncmp(tmp1, "list", 4))
            {
                // 리스트 데이터 출력
                l_idx = findStructureIndex('l', tmp1);
                if (l_idx < 0)
                {
                    printf("List not found.\n");
                    continue;
                }
                struct list_elem *e;
                for (e = list_begin(listContainers[l_idx].link); e != list_end(listContainers[l_idx].link); e = list_next(e))
                {
                    struct list_item *temp = list_entry(e, struct list_item, elem);
                    printf("%d ", temp->data);
                }
                printf("\n");
            }
            else if (!strncmp(tmp1, "bm", 2))
            {
                // 비트맵 데이터 출력
                b_idx = findStructureIndex('b', tmp1);
                if (b_idx < 0)
                {
                    printf("Bitmap not found.\n");
                    continue;
                }
                for (int i = 0; i < bitmap_size(bitmapContainers[b_idx].link); i++)
                {
                    printf("%d", bitmap_test(bitmapContainers[b_idx].link, i) ? 1 : 0);
                }
                printf("\n");
            }
            else if (!strncmp(tmp1, "hash", 4))
            {
                // 해시 테이블 데이터 출력
                h_idx = findStructureIndex('h', tmp1);
                if (h_idx < 0)
                {
                    printf("Hash not found.\n");
                    continue;
                }
                // 해시 테이블의 각 버킷에 대해 반복
                struct hash_iterator i;
                hash_first(&i, hashContainers[h_idx].link);
                while (hash_next(&i))
                {
                    struct hash_item *temp = hash_entry(hash_cur(&i), struct hash_item, elem);
                    printf("%d ", temp->data);
                }
                printf("\n");
            }
        }

        // "quit"
        else if (!strcmp(cmd, "quit"))
        {
            break; // 프로그램 종료
        }

        //---------------------------list-----------------------------//

        // "list_insert" 사용자로부터 입력 받은 위치에 데이터를 삽입
        else if (!strcmp(cmd, "list_insert"))
        {
            strcpy(tmp1, strtok(NULL, " "));  // 리스트 이름
            strcpy(tmp2, strtok(NULL, " "));  // 삽입할 위치
            int insert_position = atoi(tmp2); // 삽입 위치를 정수형으로 변환
            strcpy(tmp3, strtok(NULL, " "));  // 삽입할 데이터
            int data_value = atoi(tmp3);      // 데이터를 정수형으로 변환

            int list_index = findStructureIndex('l', tmp1); // 리스트 이름으로 리스트 인덱스를 찾음
            if (list_index < 0)                             // 리스트를 찾지 못한 경우
            {
                printf("List '%s' not found.\n", tmp1);
                continue; // 다음 명령어로 넘어감
            }

            struct list_elem *insert_point = list_begin(listContainers[list_index].link); // 삽입 지점을 리스트의 시작으로 설정
            for (int i = 0; i < insert_position && insert_point != list_end(listContainers[list_index].link); i++)
            {
                insert_point = list_next(insert_point); // 지정된 위치까지 이동
            }

            struct list_item *new_item = (struct list_item *)malloc(sizeof(struct list_item)); // 새로운 항목을 위한 메모리 할당
            if (!new_item)
            {
                printf("Memory allocation failed.\n");
                continue; // 메모리 할당 실패 시 다음 명령어로 넘어감
            }
            new_item->data = data_value;                // 새 항목에 데이터 설정
            list_insert(insert_point, &new_item->elem); // 지정된 위치에 새 항목 삽입
        }

        // "list_insert_ordered" 지정된 데이터 값을 오름차순으로 정렬된 위치에 삽입
        else if (!strcmp(cmd, "list_insert_ordered"))
        {
            char *list_name = strtok(NULL, " ");                 // 명령어에서 리스트 이름을 가져옴
            int data_value = atoi(strtok(NULL, " "));            // 삽입할 데이터 값을 파싱
            int list_index = findStructureIndex('l', list_name); // 리스트 이름으로 인덱스를 찾음

            if (list_index < 0) // 해당 이름의 리스트가 존재하지 않으면
            {
                printf("List '%s' not found.\n", list_name); // 에러 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }
            struct list_item *item = (struct list_item *)malloc(sizeof(struct list_item)); // 새로운 리스트 아이템을 할당
            if (!item)                                                                     // 메모리 할당 실패 시
            {
                printf("Memory allocation failed.\n"); // 할당 실패 메시지를 출력
                continue;                              // 다음 명령어로 넘어감
            }
            item->data = data_value;                                                                   // 할당된 아이템에 데이터 값을 설정
            list_insert_ordered(listContainers[list_index].link, &item->elem, compareListItems, NULL); // 정렬된 위치에 삽입
        }

        // "list_push_front" 리스트의 맨 앞에 데이터를 삽입
        else if (!strcmp(cmd, "list_push_front"))
        {
            char *list_name = strtok(NULL, " ");      // 리스트 이름 추출
            int data_value = atoi(strtok(NULL, " ")); // 삽입할 데이터 값을 정수로 변환

            int list_index = findStructureIndex('l', list_name); // 리스트 이름으로 리스트 인덱스를 찾음
            if (list_index < 0)                                  // 리스트를 찾지 못한 경우
            {
                printf("List '%s' not found.\n", list_name); // 리스트를 찾지 못했다는 메시지 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            struct list_item *new_item = (struct list_item *)malloc(sizeof(struct list_item)); // 새 항목을 위한 메모리 할당
            if (!new_item)                                                                     // 메모리 할당 실패 시
            {
                printf("Memory allocation failed.\n"); // 메모리 할당 실패 메시지 출력
                continue;                              // 다음 명령어로 넘어감
            }
            new_item->data = data_value;                                       // 새 항목에 데이터 설정
            list_push_front(listContainers[list_index].link, &new_item->elem); // 리스트의 맨 앞에 새 항목 삽입
        }

        // "list_push_back" 리스트의 맨 뒤에 데이터를 삽입
        else if (!strcmp(cmd, "list_push_back"))
        {
            char *list_name = strtok(NULL, " ");      // 리스트 이름 추출
            int data_value = atoi(strtok(NULL, " ")); // 삽입할 데이터 값을 정수로 변환

            int list_index = findStructureIndex('l', list_name); // 리스트 이름으로 리스트 인덱스를 찾음
            if (list_index < 0)                                  // 리스트를 찾지 못한 경우
            {
                printf("List '%s' not found.\n", list_name); // 리스트를 찾지 못했다는 메시지 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            struct list_item *new_item = (struct list_item *)malloc(sizeof(struct list_item)); // 새 항목을 위한 메모리 할당
            if (!new_item)                                                                     // 메모리 할당 실패 시
            {
                printf("Memory allocation failed.\n"); // 메모리 할당 실패 메시지 출력
                continue;                              // 다음 명령어로 넘어감
            }
            new_item->data = data_value;                                      // 새 항목에 데이터 설정
            list_push_back(listContainers[list_index].link, &new_item->elem); // 리스트의 맨 뒤에 새 항목 삽입
        }

        // "list_pop_front" 명령어 처리: 지정된 리스트의 첫 번째 요소를 제거
        else if (!strcmp(cmd, "list_pop_front"))
        {
            char *list_name = strtok(NULL, " ");                 // 명령어에서 리스트 이름을 추출
            int list_index = findStructureIndex('l', list_name); // 리스트 이름으로 리스트 인덱스를 찾음

            if (list_index < 0) // 리스트를 찾지 못한 경우
            {
                printf("List '%s' not found.\n", list_name); // 해당 리스트가 없음을 알리는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }
            if (!list_empty(listContainers[list_index].link)) // 리스트가 비어있지 않은 경우
            {
                list_pop_front(listContainers[list_index].link); // 리스트의 첫 번째 요소를 제거
            }
            else // 리스트가 이미 비어있는 경우
            {
                printf("List is already empty.\n"); // 리스트가 이미 비어있다는 메시지를 출력
            }
        }

        // "list_pop_back" 명령어 처리: 지정된 리스트의 마지막 요소를 제거
        else if (!strcmp(cmd, "list_pop_back"))
        {
            char *list_name = strtok(NULL, " ");                 // 명령어에서 리스트 이름을 추출
            int list_index = findStructureIndex('l', list_name); // 리스트 이름으로 리스트 인덱스를 찾음

            if (list_index < 0) // 리스트를 찾지 못한 경우
            {
                printf("List '%s' not found.\n", list_name); // 해당 리스트가 없음을 알리는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }
            if (!list_empty(listContainers[list_index].link)) // 리스트가 비어있지 않은 경우
            {
                list_pop_back(listContainers[list_index].link); // 리스트의 마지막 요소를 제거
            }
            else // 리스트가 이미 비어있는 경우
            {
                printf("List is already empty.\n");
            }
        }

        // "list_splice" 한 리스트의 일부분을 다른 리스트의 지정된 위치에 삽입
        else if (!strcmp(cmd, "list_splice"))
        {
            strcpy(tmp1, strtok(NULL, " ")); // 대상 리스트의 이름
            strcpy(tmp2, strtok(NULL, " ")); // 삽입할 위치의 인덱스
            strcpy(tmp3, strtok(NULL, " ")); // 원본 리스트의 이름
            strcpy(tmp4, strtok(NULL, " ")); // 복사할 범위의 시작 인덱스
            strcpy(tmp5, strtok(NULL, " ")); // 복사할 범위의 끝 인덱스 (이 인덱스 바로 전까지 복사)

            int before_idx = atoi(tmp2); // 삽입 위치 인덱스로 변환
            int first_idx = atoi(tmp4);  // 복사 시작 인덱스로 변환
            int last_idx = atoi(tmp5);   // 복사 끝 인덱스로 변환

            int l_idx1 = findStructureIndex('l', tmp1); // 대상 리스트 인덱스를 찾음
            int l_idx2 = findStructureIndex('l', tmp3); // 원본 리스트 인덱스를 찾음

            if (l_idx1 < 0 || l_idx2 < 0) // 둘 중 하나라도 리스트를 찾지 못한 경우
            {
                printf("List not found.\n"); // 리스트를 찾지 못했다는 메시지 출력
                continue;                    // 다음 명령어로 넘어감
            }

            // 삽입할 위치, 복사 시작 위치, 복사 끝 위치를 찾아 이동
            struct list_elem *before = list_begin(listContainers[l_idx1].link);
            for (int i = 0; i < before_idx && before != list_end(listContainers[l_idx1].link); i++)
            {
                before = list_next(before);
            }

            struct list_elem *first = list_begin(listContainers[l_idx2].link);
            for (int i = 0; i < first_idx && first != list_end(listContainers[l_idx2].link); i++)
            {
                first = list_next(first);
            }

            struct list_elem *last = list_begin(listContainers[l_idx2].link);
            for (int i = 0; i < last_idx && last != list_end(listContainers[l_idx2].link); i++)
            {
                last = list_next(last);
            }

            // 지정된 범위가 유효하지 않은 경우 메시지를 출력하고 다음 명령어로 넘어감
            if (before == list_end(listContainers[l_idx1].link) || first == list_end(listContainers[l_idx2].link) || last == list_end(listContainers[l_idx2].link))
            {
                printf("Invalid index for splice operation.\n");
                continue;
            }

            // 복사할 요소 범위를 대상 리스트의 지정된 위치에 삽입
            list_splice(before, first, last);
        }

        // "list_front" 지정된 리스트의 첫 번째 요소의 데이터 값을 출력
        else if (!strcmp(cmd, "list_front"))
        {
            char *list_name = strtok(NULL, " ");                 // 명령어에서 리스트 이름을 가져옴
            int list_index = findStructureIndex('l', list_name); // 리스트 이름으로 리스트 인덱스를 찾음

            if (list_index < 0) // 리스트를 찾지 못한 경우
            {
                printf("List '%s' not found.\n", list_name); // 리스트가 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            struct list_elem *e = list_front(listContainers[list_index].link); // 리스트의 첫 번째 요소를 가져옴
            if (e)                                                             // 첫 번째 요소가 존재하는 경우
            {
                struct list_item *temp = list_entry(e, struct list_item, elem); // list_elem을 list_item 구조체로 변환
                printf("%d\n", temp->data);                                     // 첫 번째 요소의 데이터 값을 출력
            }
            else // 리스트가 비어 있는 경우
            {
                printf("List is empty.\n"); // 리스트가 비었다는 메시지를 출력
            }
        }

        // "list_back" 명령어를 처리하여 지정된 리스트의 마지막 요소의 데이터 값을 출력
        else if (!strcmp(cmd, "list_back"))
        {
            char *list_name = strtok(NULL, " ");                 // 명령어 라인에서 다음 토큰(리스트의 이름)을 읽어옴
            int list_index = findStructureIndex('l', list_name); // 해당 리스트 이름에 대응하는 인덱스를 찾음

            if (list_index < 0) // 해당 이름의 리스트가 존재하지 않는 경우
            {
                printf("List '%s' not found.\n", list_name); // 에러 메시지를 출력하고
                continue;                                    // 다음 반복으로 넘어감
            }

            struct list_elem *e = list_back(listContainers[list_index].link); // 해당 인덱스의 리스트에서 마지막 요소를 가져
            if (e)                                                            // 마지막 요소가 존재하면
            {
                struct list_item *temp = list_entry(e, struct list_item, elem); // 이 요소를 실제 데이터를 포함하는 list_item 구조체로 변환
                printf("%d\n", temp->data);                                     // 변환된 구조체의 데이터 필드 값을 출력
            }
            else // 리스트가 비어있는 경우
            {
                printf("List is empty.\n");
            }
        }

        // "list_empty" 지정된 리스트가 비어있는지 확인
        else if (!strcmp(cmd, "list_empty"))
        {
            char *list_name = strtok(NULL, " ");                 // 명령어에서 리스트 이름을 가져옴
            int list_index = findStructureIndex('l', list_name); // 리스트 이름으로 인덱스를 찾음

            if (list_index < 0) // 해당 이름의 리스트가 존재하지 않으면
            {
                printf("List '%s' not found.\n", list_name); // 에러 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }
            printf("%s\n", list_empty(listContainers[list_index].link) ? "true" : "false"); // 리스트가 비어있으면 "true", 그렇지 않으면 "false"를 출력
        }

        // "list_size" 지정된 리스트의 요소 개수를 출력
        else if (!strcmp(cmd, "list_size"))
        {
            char *list_name = strtok(NULL, " ");                 // 명령어에서 리스트 이름을 가져옴
            int list_index = findStructureIndex('l', list_name); // 리스트 이름으로 인덱스를 찾음

            if (list_index < 0) // 해당 이름의 리스트가 존재하지 않으면
            {
                printf("List '%s' not found.\n", list_name); // 에러 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }
            printf("%zu\n", list_size(listContainers[list_index].link)); // 리스트의 요소 개수를 출력
        }

        // "list_min" 지정된 리스트에서 최소 데이터 값을 가진 요소의 데이터 값을 출력
        else if (!strcmp(cmd, "list_min"))
        {
            char *list_name = strtok(NULL, " ");                 // 명령어에서 리스트 이름을 가져옴
            int list_index = findStructureIndex('l', list_name); // 리스트 이름으로 리스트 인덱스를 찾음

            if (list_index < 0) // 리스트를 찾지 못한 경우
            {
                printf("List '%s' not found.\n", list_name); // 리스트가 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }
            struct list_elem *e = list_min(listContainers[list_index].link, compareListItems, NULL); // 최소 데이터 값을 가진 요소를 찾음
            if (e)                                                                                   // 요소가 존재하는 경우
            {
                struct list_item *temp = list_entry(e, struct list_item, elem); // list_elem을 list_item 구조체로 변환
                printf("%d\n", temp->data);                                     // 요소의 데이터 값을 출력
            }
            else // 리스트가 비어 있는 경우
            {
                printf("List is empty.\n"); // 리스트가 비었다는 메시지를 출력
            }
        }

        // "list_max" 지정된 리스트에서 최대 데이터 값을 가진 요소의 데이터 값을 출력
        else if (!strcmp(cmd, "list_max"))
        {
            char *list_name = strtok(NULL, " ");                 // 명령어에서 리스트 이름을 가져옴
            int list_index = findStructureIndex('l', list_name); // 리스트 이름으로 리스트 인덱스를 찾음

            if (list_index < 0) // 리스트를 찾지 못한 경우
            {
                printf("List '%s' not found.\n", list_name); // 리스트가 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }
            struct list_elem *e = list_max(listContainers[list_index].link, compareListItems, NULL); // 최대 데이터 값을 가진 요소를 찾음
            if (e)                                                                                   // 요소가 존재하는 경우
            {
                struct list_item *temp = list_entry(e, struct list_item, elem); // list_elem을 list_item 구조체로 변환
                printf("%d\n", temp->data);                                     // 요소의 데이터 값을 출력
            }
            else // 리스트가 비어 있는 경우
            {
                printf("List is empty.\n"); // 리스트가 비었다는 메시지를 출력
            }
        }

        // "list_remove" 지정된 리스트에서 특정 인덱스의 요소를 제거
        else if (!strcmp(cmd, "list_remove"))
        {
            char *list_name = strtok(NULL, " ");                 // 명령어에서 리스트 이름을 가져옴
            int remove_idx = atoi(strtok(NULL, " "));            // 제거할 요소의 인덱스를 가져옴
            int list_index = findStructureIndex('l', list_name); // 리스트 이름으로 리스트 인덱스를 찾음

            if (list_index < 0) // 리스트를 찾지 못한 경우
            {
                printf("List '%s' not found.\n", list_name); // 리스트가 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            struct list_elem *e = list_begin(listContainers[list_index].link);                     // 리스트의 시작부터 탐색 시작
            for (int i = 0; i < remove_idx && e != list_end(listContainers[list_index].link); i++) // 제거할 인덱스에 도달할 때까지 반복
            {
                e = list_next(e); // 다음 요소로 이동
            }
            if (e != list_end(listContainers[list_index].link)) // 유효한 인덱스인 경우
            {
                list_remove(e); // 해당 요소를 리스트에서 제거
            }
            else // 유효하지 않은 인덱스인 경우
            {
                printf("Invalid index.\n"); // 유효하지 않은 인덱스라는 메시지를 출력
            }
        }

        // "list_unique" 명령어: 지정된 리스트에서 중복된 요소를 제거
        else if (!strcmp(cmd, "list_unique"))
        {
            char *list_name = strtok(NULL, " ");                 // 명령어에서 리스트 이름을 가져옴
            char *duplicate_list_name = strtok(NULL, " ");       // 중복 제거 대상이 될 다른 리스트의 이름
            int list_index = findStructureIndex('l', list_name); // 리스트 이름으로 리스트 인덱스를 찾음

            if (list_index < 0) // 리스트를 찾지 못한 경우
            {
                printf("List '%s' not found.\n", list_name); // 리스트가 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            struct list *duplicate_list = NULL; // 중복 리스트 초기화
            if (duplicate_list_name != NULL)    // 다른 리스트 이름이 지정된 경우
            {
                int duplicate_list_index = findStructureIndex('l', duplicate_list_name); // 다른 리스트 인덱스 찾기
                if (duplicate_list_index >= 0)                                           // 유효한 리스트 인덱스인 경우
                {
                    duplicate_list = listContainers[duplicate_list_index].link; // 중복 리스트 설정
                }
            }

            list_unique(listContainers[list_index].link, duplicate_list, compareListItems, NULL); // 중복 요소 제거 실행
        }

        // "list_swap" 지정된 리스트에서 두 요소의 위치를 교환
        else if (!strcmp(cmd, "list_swap"))
        {
            char *list_name = strtok(NULL, " ");                 // 명령어에서 리스트 이름을 가져옴
            int first_idx = atoi(strtok(NULL, " "));             // 첫 번째 교환할 요소의 인덱스
            int second_idx = atoi(strtok(NULL, " "));            // 두 번째 교환할 요소의 인덱스
            int list_index = findStructureIndex('l', list_name); // 리스트 이름으로 리스트 인덱스를 찾음

            if (list_index < 0) // 리스트를 찾지 못한 경우
            {
                printf("List '%s' not found.\n", list_name); // 리스트가 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            struct list_elem *first = list_begin(listContainers[list_index].link);  // 첫 번째 요소에서 시작
            struct list_elem *second = list_begin(listContainers[list_index].link); // 두 번째 요소에서 시작

            for (int i = 0; i < first_idx && first != list_end(listContainers[list_index].link); i++) // 첫 번째 교환할 요소로 이동
            {
                first = list_next(first);
            }
            for (int i = 0; i < second_idx && second != list_end(listContainers[list_index].link); i++) // 두 번째 교환할 요소로 이동
            {
                second = list_next(second);
            }

            if (first != list_end(listContainers[list_index].link) && second != list_end(listContainers[list_index].link)) // 두 요소 모두 유효한 경우
            {
                list_swap(first, second); // 두 요소의 위치를 교환
            }
            else // 하나라도 유효하지 않은 경우
            {
                printf("Invalid indices.\n"); // 유효하지 않은 인덱스라는 메시지를 출력
            }
        }

        // "list_reverse" 리스트의 요소 순서를 뒤집음
        else if (!strcmp(cmd, "list_reverse"))
        {
            char *list_name = strtok(NULL, " ");                 // 명령어에서 리스트 이름을 가져옴
            int list_index = findStructureIndex('l', list_name); // 리스트 이름으로 리스트 인덱스를 찾음

            if (list_index < 0) // 리스트를 찾지 못한 경우
            {
                printf("List '%s' not found.\n", list_name); // 리스트가 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            list_reverse(listContainers[list_index].link); // 리스트 순서 뒤집기 실행
        }

        // "list_sort" 리스트를 오름차순으로 정렬
        else if (!strcmp(cmd, "list_sort"))
        {
            char *list_name = strtok(NULL, " ");                 // 명령어에서 리스트 이름을 가져옴
            int list_index = findStructureIndex('l', list_name); // 리스트 이름으로 리스트 인덱스를 찾음

            if (list_index < 0) // 리스트를 찾지 못한 경우
            {
                printf("List '%s' not found.\n", list_name); // 리스트가 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            list_sort(listContainers[list_index].link, compareListItems, NULL); // 리스트 정렬 실행
        }

        // "list_shuffle" 리스트의 요소 순서를 무작위로 섞음
        else if (!strcmp(cmd, "list_shuffle"))
        {
            char *list_name = strtok(NULL, " ");                 // 명령어에서 리스트 이름을 가져옴
            int list_index = findStructureIndex('l', list_name); // 리스트 이름으로 리스트 인덱스를 찾음

            if (list_index < 0) // 리스트를 찾지 못한 경우
            {
                printf("List '%s' not found.\n", list_name); // 리스트가 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            list_shuffle(listContainers[list_index].link); // 리스트 순서 섞기 실행
        }

        //---------------------------hash-----------------------------//

        // "hash_insert" 지정된 해시 테이블에 새 요소를 삽입
        else if (!strcmp(cmd, "hash_insert"))
        {
            char *hash_name = strtok(NULL, " ");                 // 명령어에서 해시 테이블 이름을 가져옴
            int data = atoi(strtok(NULL, " "));                  // 삽입할 데이터 값을 가져옴
            int hash_index = findStructureIndex('h', hash_name); // 해시 테이블 인덱스를 찾음

            if (hash_index < 0) // 해시 테이블을 찾지 못한 경우
            {
                printf("Hash '%s' not found.\n", hash_name); // 해시 테이블이 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            struct hash_item *item = malloc(sizeof(struct hash_item)); // 새 해시 아이템 메모리 할당
            if (!item)                                                 // 메모리 할당 실패
            {
                printf("Memory allocation failed.\n"); // 메모리 할당 실패 메시지 출력
                continue;                              // 다음 명령어로 넘어감
            }
            item->data = data;                                         // 아이템 데이터 설정
            hash_insert(hashContainers[hash_index].link, &item->elem); // 해시 테이블에 아이템 삽입
        }

        // "hash_find" 지정된 해시 테이블에서 특정 데이터 값을 가진 요소를 찾음
        else if (!strcmp(cmd, "hash_find"))
        {
            char *hash_name = strtok(NULL, " ");                 // 명령어에서 해시 테이블 이름을 가져옴
            int data_value = atoi(strtok(NULL, " "));            // 찾고자 하는 데이터 값을 가져옴
            int hash_index = findStructureIndex('h', hash_name); // 해시 테이블 인덱스를 찾음

            if (hash_index < 0) // 해시 테이블을 찾지 못한 경우
            {
                printf("Hash '%s' not found.\n", hash_name); // 해시 테이블이 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            struct hash_item temp_item;                                                                 // 임시 해시 아이템 생성
            temp_item.data = data_value;                                                                // 임시 아이템에 데이터 값 설정
            struct hash_elem *found_elem = hash_find(hashContainers[hash_index].link, &temp_item.elem); // 해시 테이블에서 아이템 찾기

            if (found_elem) // 찾는 요소가 있을 경우
            {
                struct hash_item *found_item = hash_entry(found_elem, struct hash_item, elem); // hash_elem을 hash_item으로 변환
                printf("%d\n", found_item->data);                                              // 찾은 요소의 데이터 값을 출력
            }
            // 찾는 값이 없을 경우 아무 것도 출력하지 않음
        }

        // "hash_replace" 지정된 해시 테이블에 존재하는 요소를 교체
        else if (!strcmp(cmd, "hash_replace"))
        {
            char *hash_name = strtok(NULL, " ");                 // 명령어에서 해시 테이블 이름을 가져옴
            int data = atoi(strtok(NULL, " "));                  // 교체할 데이터 값을 가져옴
            int hash_index = findStructureIndex('h', hash_name); // 해시 테이블 인덱스를 찾음

            if (hash_index < 0) // 해시 테이블을 찾지 못한 경우
            {
                printf("Hash '%s' not found.\n", hash_name); // 해시 테이블이 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            struct hash_item *item = malloc(sizeof(struct hash_item)); // 새 해시 아이템 메모리 할당
            if (!item)                                                 // 메모리 할당 실패
            {
                printf("Memory allocation failed.\n"); // 메모리 할당 실패 메시지 출력
                continue;                              // 다음 명령어로 넘어감
            }
            item->data = data;                                          // 아이템 데이터 설정
            hash_replace(hashContainers[hash_index].link, &item->elem); // 해시 테이블에서 아이템 교체
        }

        // "hash_delete" 지정된 해시 테이블에서 특정 데이터 값을 가진 요소를 삭제
        else if (!strcmp(cmd, "hash_delete"))
        {
            char *hash_name = strtok(NULL, " ");                 // 명령어에서 해시 테이블 이름을 가져옴
            int data = atoi(strtok(NULL, " "));                  // 삭제할 데이터 값을 가져옴
            int hash_index = findStructureIndex('h', hash_name); // 해시 테이블 인덱스를 찾음

            if (hash_index < 0) // 해시 테이블을 찾지 못한 경우
            {
                printf("Hash '%s' not found.\n", hash_name); // 해시 테이블이 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            struct hash_item temp_item;                                                          // 임시 해시 아이템 생성
            temp_item.data = data;                                                               // 임시 아이템에 데이터 값 설정
            struct hash_elem *e = hash_delete(hashContainers[hash_index].link, &temp_item.elem); // 해시 테이블에서 아이템 삭제

            if (e) // 삭제된 요소가 있을 경우
            {
                struct hash_item *item = hash_entry(e, struct hash_item, elem); // hash_elem을 hash_item으로 변환
                free(item);                                                     // 메모리 해제
            }
        }

        // "hash_size" 지정된 해시 테이블에 저장된 요소의 총 개수를 출력
        else if (!strcmp(cmd, "hash_size"))
        {
            char *hash_name = strtok(NULL, " ");                 // 명령어에서 해시 테이블 이름을 가져옴
            int hash_index = findStructureIndex('h', hash_name); // 해시 테이블 인덱스를 찾음

            if (hash_index < 0) // 해시 테이블을 찾지 못한 경우
            {
                printf("Hash '%s' not found.\n", hash_name); // 해시 테이블이 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            printf("%zu\n", hash_size(hashContainers[hash_index].link)); // 해시 테이블에 저장된 요소의 총 개수를 출력
        }

        // "hash_empty" 지정된 해시 테이블이 비어 있는지 여부를 확인
        else if (!strcmp(cmd, "hash_empty"))
        {
            char *hash_name = strtok(NULL, " ");                 // 명령어에서 해시 테이블 이름을 가져옴
            int hash_index = findStructureIndex('h', hash_name); // 해시 테이블 인덱스를 찾음

            if (hash_index < 0) // 해시 테이블을 찾지 못한 경우
            {
                printf("Hash '%s' not found.\n", hash_name); // 해시 테이블이 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            bool result = hash_empty(hashContainers[hash_index].link); // 해시 테이블이 비어 있는지 확인
            printf("%s\n", result ? "true" : "false");                 // 비어 있으면 "true", 그렇지 않으면 "false"를 출력
        }

        // "hash_apply" 지정된 해시 테이블의 모든 요소에 square, triple 적용
        else if (!strcmp(cmd, "hash_apply"))
        {
            char *hash_name = strtok(NULL, " ");                 // 명령어에서 해시 테이블 이름을 가져옴
            char *function_name = strtok(NULL, " ");             // 적용할 함수 이름을 가져옴
            int hash_index = findStructureIndex('h', hash_name); // 해시 테이블 인덱스를 찾음

            if (hash_index < 0) // 해시 테이블을 찾지 못한 경우
            {
                printf("Hash '%s' not found.\n", hash_name); // 해시 테이블이 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            if (!strcmp(function_name, "square")) // 함수 이름이 "square"일 경우
            {
                hash_apply(hashContainers[hash_index].link, square); // 모든 요소에 square 함수 적용
            }
            else if (!strcmp(function_name, "triple")) // 함수 이름이 "triple"일 경우
            {
                hash_apply(hashContainers[hash_index].link, triple); // 모든 요소에 triple 함수 적용
            }
            else // 알 수 없는 함수 이름일 경우
            {
                printf("Unknown function: %s\n", function_name); // 알 수 없는 함수라는 메시지를 출력
            }
        }

        // "hash_clear" 지정된 해시 테이블의 모든 요소를 제거
        else if (!strcmp(cmd, "hash_clear"))
        {
            char *hash_name = strtok(NULL, " ");                 // 명령어에서 해시 테이블 이름을 가져옴
            int hash_index = findStructureIndex('h', hash_name); // 해시 테이블 인덱스를 찾음

            if (hash_index < 0) // 해시 테이블을 찾지 못한 경우
            {
                printf("Hash '%s' not found.\n", hash_name); // 해시 테이블이 없다는 메시지를 출력
                continue;                                    // 다음 명령어로 넘어감
            }

            hash_clear(hashContainers[hash_index].link, destructor); // 해시 테이블의 모든 요소를 제거
        }

        //----------------------------bitmap--------------------------//

        // "bitmap_size" 지정된 비트맵의 크기를 출력
        else if (!strcmp(cmd, "bitmap_size"))
        {
            char *bitmap_name = strtok(NULL, " ");                   // 명령어에서 비트맵 이름을 가져옴
            int bitmap_index = findStructureIndex('b', bitmap_name); // 비트맵 인덱스를 찾음

            if (bitmap_index < 0) // 비트맵을 찾지 못한 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 비트맵이 없다는 메시지를 출력
                continue;                                        // 다음 명령어로 넘어감
            }

            printf("%zu\n", bitmap_size(bitmapContainers[bitmap_index].link)); // 비트맵의 크기를 출력
        }

        // "bitmap_set" 지정된 비트맵의 특정 위치의 비트 값을 설정
        else if (!strcmp(cmd, "bitmap_set"))
        {
            char *bitmap_name = strtok(NULL, " ");             // 명령어에서 비트맵 이름을 가져옴
            size_t bit_idx = atoi(strtok(NULL, " "));          // 설정할 비트의 인덱스를 가져옴
            bool val = strcmp(strtok(NULL, " "), "true") == 0; // 비트 값을 가져옴 "true"면 true, 그렇지 않으면 false

            int bitmap_index = findStructureIndex('b', bitmap_name); // 비트맵 인덱스를 찾음
            if (bitmap_index < 0)                                    // 비트맵을 찾지 못한 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 비트맵이 없다는 메시지를 출력
                continue;                                        // 다음 명령어로 넘어감
            }

            bitmap_set(bitmapContainers[bitmap_index].link, bit_idx, val); // 지정된 위치의 비트 값을 설정
        }

        // "bitmap_set_all" 지정된 비트맵의 모든 비트를 주어진 값으로 설정
        else if (!strcmp(cmd, "bitmap_set_all"))
        {
            char *bitmap_name = strtok(NULL, " "); // 비트맵 이름을 입력 받음
            // 주어진 값이 "true"이면 true로, 그렇지 않으면 false로 설정
            bool value = strcmp(strtok(NULL, " "), "true") == 0;

            // 비트맵 이름으로부터 해당 비트맵의 인덱스를 찾음
            int bitmap_index = findStructureIndex('b', bitmap_name);
            if (bitmap_index < 0) // 해당 비트맵을 찾지 못한 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 비트맵이 없다는 메시지를 출력하고 다음 명령어로 넘어감
                continue;
            }

            // 비트맵의 모든 비트를 주어진 값으로 설정 이때 비트맵 전체를 true 또는 false로 일괄 설정
            bitmap_set_all(bitmapContainers[bitmap_index].link, value);
        }

        // "bitmap_set_multiple" 지정된 비트맵의 특정 범위에 있는 여러 비트를 주어진 값으로 설정
        else if (!strcmp(cmd, "bitmap_set_multiple"))
        {
            char *bitmap_name = strtok(NULL, " ");      // 비트맵 이름을 입력 받음
            size_t start_idx = atoi(strtok(NULL, " ")); // 시작 인덱스를 입력 받음
            size_t count = atoi(strtok(NULL, " "));     // 설정할 비트의 개수를 입력 받음
            // 주어진 값이 "true"이면 true로, 그렇지 않으면 false로 설정
            bool value = strcmp(strtok(NULL, " "), "true") == 0;

            // 비트맵 이름으로부터 해당 비트맵의 인덱스를 찾음
            int bitmap_index = findStructureIndex('b', bitmap_name);
            if (bitmap_index < 0) // 해당 비트맵을 찾지 못한 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 비트맵이 없다는 메시지를 출력하고 다음 명령어로 넘어감
                continue;
            }

            // 지정된 시작 인덱스부터 count 개수만큼의 비트를 value 값으로 설정
            bitmap_set_multiple(bitmapContainers[bitmap_index].link, start_idx, count, value);
        }

        // "bitmap_reset" 지정된 비트맵의 특정 위치의 비트를 0으로 재설정
        else if (!strcmp(cmd, "bitmap_reset"))
        {
            char *bitmap_name = strtok(NULL, " ");    // 명령어에서 비트맵 이름을 가져옴
            size_t bit_idx = atoi(strtok(NULL, " ")); // 재설정할 비트의 인덱스를 가져옴

            int bitmap_index = findStructureIndex('b', bitmap_name); // 비트맵 인덱스를 찾음
            if (bitmap_index < 0)                                    // 비트맵을 찾지 못한 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 비트맵이 없다는 메시지를 출력
                continue;                                        // 다음 명령어로 넘어감
            }

            bitmap_reset(bitmapContainers[bitmap_index].link, bit_idx); // 지정된 위치의 비트를 0으로 재설정
        }

        // "bitmap_mark" 지정된 비트맵의 특정 위치의 비트를 1로 설정
        else if (!strcmp(cmd, "bitmap_mark"))
        {
            char *bitmap_name = strtok(NULL, " ");    // 명령어에서 비트맵 이름을 가져옴
            size_t bit_idx = atoi(strtok(NULL, " ")); // 마크할 비트의 인덱스를 가져옴

            int bitmap_index = findStructureIndex('b', bitmap_name); // 비트맵 인덱스를 찾음
            if (bitmap_index < 0)                                    // 비트맵을 찾지 못한 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 비트맵이 없다는 메시지를 출력
                continue;                                        // 다음 명령어로 넘어감
            }

            bitmap_mark(bitmapContainers[bitmap_index].link, bit_idx); // 지정된 위치의 비트를 1로 설정
        }

        // "bitmap_test" 지정된 비트맵의 특정 위치의 비트 값이 설정되어 있는지 검사
        else if (!strcmp(cmd, "bitmap_test"))
        {
            char *bitmap_name = strtok(NULL, " ");    // 명령어에서 비트맵 이름을 가져옴
            size_t bit_idx = atoi(strtok(NULL, " ")); // 검사할 비트의 인덱스를 가져옴

            int bitmap_index = findStructureIndex('b', bitmap_name); // 비트맵 인덱스를 찾음
            if (bitmap_index < 0)                                    // 비트맵을 찾지 못한 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 비트맵이 없다는 메시지를 출력
                continue;                                        // 다음 명령어로 넘어감
            }

            bool result = bitmap_test(bitmapContainers[bitmap_index].link, bit_idx); // 지정된 위치의 비트 값이 설정되어 있는지 검사
            printf("%s\n", result ? "true" : "false");                               // 검사 결과를 출력 설정되어 있으면 "true", 그렇지 않으면 "false"
        }

        // "bitmap_flip" 지정된 비트맵의 특정 위치의 비트 값을 반전
        else if (!strcmp(cmd, "bitmap_flip"))
        {
            char *bitmap_name = strtok(NULL, " ");    // 명령어에서 비트맵 이름을 가져옴
            size_t bit_idx = atoi(strtok(NULL, " ")); // 반전시킬 비트의 인덱스를 가져옴

            int bitmap_index = findStructureIndex('b', bitmap_name); // 비트맵 인덱스를 찾음
            if (bitmap_index < 0)                                    // 비트맵을 찾지 못한 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 비트맵이 없다는 메시지를 출력
                continue;                                        // 다음 명령어로 넘어감
            }

            bitmap_flip(bitmapContainers[bitmap_index].link, bit_idx); // 지정된 위치의 비트 값을 반전
        }

        // "bitmap_test" 지정된 비트맵의 특정 위치의 비트 값이 1인지 확인
        else if (!strcmp(cmd, "bitmap_test"))
        {
            char *bitmap_name = strtok(NULL, " ");    // 비트맵의 이름을 입력받음
            size_t bit_idx = atoi(strtok(NULL, " ")); // 테스트할 비트의 인덱스를 입력받음

            int bitmap_index = findStructureIndex('b', bitmap_name); // 비트맵 인덱스를 찾음
            if (bitmap_index < 0)                                    // 해당 비트맵이 존재하지 않는 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 에러 메시지 출력
                continue;                                        // 다음 명령어 처리로 넘어감
            }

            bool result = bitmap_test(bitmapContainers[bitmap_index].link, bit_idx); // 지정된 인덱스의 비트 값을 테스트
            printf("%s\n", result ? "true" : "false");                               // 결과를 true 또는 false로 출력
        }

        // "bitmap_flip" 지정된 비트맵의 특정 위치의 비트 값을 반전
        else if (!strcmp(cmd, "bitmap_flip"))
        {
            char *bitmap_name = strtok(NULL, " ");    // 비트맵의 이름을 입력받음
            size_t bit_idx = atoi(strtok(NULL, " ")); // 반전시킬 비트의 인덱스를 입력받음

            int bitmap_index = findStructureIndex('b', bitmap_name); // 비트맵 인덱스를 찾음
            if (bitmap_index < 0)                                    // 해당 비트맵이 존재하지 않는 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 에러 메시지 출력
                continue;                                        // 다음 명령어 처리로 넘어감
            }

            bitmap_flip(bitmapContainers[bitmap_index].link, bit_idx); // 지정된 인덱스의 비트 값을 반전
        }

        // "bitmap_contains" 지정된 비트맵 범위에 특정 값(value)이 존재하는지 확인
        else if (!strcmp(cmd, "bitmap_contains"))
        {
            char *bitmap_name = strtok(NULL, " ");               // 비트맵 이름 입력 받음
            size_t start_idx = atoi(strtok(NULL, " "));          // 시작 인덱스
            size_t count = atoi(strtok(NULL, " "));              // 검사할 비트 수
            bool value = strcmp(strtok(NULL, " "), "true") == 0; // 찾고자 하는 비트 값

            int bitmap_index = findStructureIndex('b', bitmap_name); // 비트맵 인덱스 조회
            if (bitmap_index < 0)                                    // 비트맵이 존재하지 않는 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 에러 메시지 출력
                continue;
            }

            // 지정된 범위에서 value와 일치하는 비트가 있는지 확인
            bool result = bitmap_contains(bitmapContainers[bitmap_index].link, start_idx, count, value);
            printf("%s\n", result ? "true" : "false"); // 결과 출력
        }

        // "bitmap_count" 지정된 범위 내에서 설정된 비트의 개수를 계산
        else if (!strcmp(cmd, "bitmap_count"))
        {
            char *bitmap_name = strtok(NULL, " ");
            size_t start_idx = atoi(strtok(NULL, " "));
            size_t count = atoi(strtok(NULL, " "));
            bool value = strcmp(strtok(NULL, " "), "true") == 0;

            int bitmap_index = findStructureIndex('b', bitmap_name);
            if (bitmap_index < 0)
            {
                printf("Bitmap '%s' not found.\n", bitmap_name);
                continue;
            }

            size_t result = bitmap_count(bitmapContainers[bitmap_index].link, start_idx, count, value);
            printf("%zu\n", result);
        }

        // "bitmap_any" 지정된 비트맵 범위 내에서 하나라도 1인 비트가 있는지 확인
        else if (!strcmp(cmd, "bitmap_any"))
        {
            char *bitmap_name = strtok(NULL, " ");  // 비트맵 이름 입력 받음
            size_t start = atoi(strtok(NULL, " ")); // 시작 인덱스
            size_t count = atoi(strtok(NULL, " ")); // 검사할 비트 수

            int bitmap_index = findStructureIndex('b', bitmap_name); // 비트맵 인덱스 조회
            if (bitmap_index < 0)                                    // 비트맵이 존재하지 않는 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 에러 메시지 출력
                continue;
            }

            // 지정된 범위 내에서 1인 비트가 하나라도 있는지 확인
            printf("%s\n", bitmap_any(bitmapContainers[bitmap_index].link, start, count) ? "true" : "false"); // 결과 출력
        }

        // "bitmap_none" 지정된 비트맵 범위 내 모든 비트가 0인지 확인
        else if (!strcmp(cmd, "bitmap_none"))
        {
            char *bitmap_name = strtok(NULL, " ");  // 비트맵 이름 입력 받음
            size_t start = atoi(strtok(NULL, " ")); // 시작 인덱스
            size_t count = atoi(strtok(NULL, " ")); // 검사할 비트 수

            int bitmap_index = findStructureIndex('b', bitmap_name); // 비트맵 인덱스 조회
            if (bitmap_index < 0)                                    // 비트맵이 존재하지 않는 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 에러 메시지 출력
                continue;
            }

            // 지정된 범위 내 모든 비트가 0인지 확인
            printf("%s\n", bitmap_none(bitmapContainers[bitmap_index].link, start, count) ? "true" : "false"); // 결과 출력
        }

        // "bitmap_all" 지정된 비트맵에서 주어진 범위 내의 모든 비트가 설정되어 있는지 확인
        else if (!strcmp(cmd, "bitmap_all"))
        {
            char *bitmap_name = strtok(NULL, " ");  // 비트맵의 이름을 입력 받음
            size_t start = atoi(strtok(NULL, " ")); // 검사를 시작할 인덱스를 입력 받음
            size_t count = atoi(strtok(NULL, " ")); // 검사할 비트의 개수를 입력 받음
            // 비트맵 이름으로부터 해당 비트맵의 인덱스를 찾음
            int bitmap_index = findStructureIndex('b', bitmap_name);

            if (bitmap_index < 0) // 해당 비트맵을 찾지 못한 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 비트맵이 없다는 메시지를 출력하고 다음 명령어로 넘어감
                continue;
            }

            // 지정된 범위 내의 모든 비트가 설정되어 있는지 확인
            // 모든 비트가 설정되어 있으면 "true", 하나라도 해제되어 있으면 "false"를 출력
            printf("%s\n", bitmap_all(bitmapContainers[bitmap_index].link, start, count) ? "true" : "false");
        }

        // "bitmap_scan" 주어진 범위 내에서 지정된 값을 가진 첫 번째 비트의 인덱스를 찾음
        else if (!strcmp(cmd, "bitmap_scan"))
        {
            char *bitmap_name = strtok(NULL, " ");  // 비트맵의 이름을 입력 받음
            size_t start = atoi(strtok(NULL, " ")); // 검색을 시작할 인덱스를 입력 받음
            size_t count = atoi(strtok(NULL, " ")); // 검색할 비트의 개수를 입력 받음
            // "true" 또는 "false" 값을 입력 받아 bool 타입으로 변환
            bool value = strcmp(strtok(NULL, " "), "true") == 0;
            // 비트맵 이름으로부터 해당 비트맵의 인덱스를 찾음
            int bitmap_index = findStructureIndex('b', bitmap_name);

            if (bitmap_index < 0) // 해당 비트맵을 찾지 못한 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 비트맵이 없다는 메시지를 출력하고 다음 명령어로 넘어감
                continue;
            }

            // 주어진 값과 일치하는 첫 번째 비트의 인덱스를 찾아 출력
            printf("%zu\n", bitmap_scan(bitmapContainers[bitmap_index].link, start, count, value));
        }

        // "bitmap_scan_and_flip" 주어진 범위 내에서 지정된 값을 가진 첫 번째 비트의 인덱스를 찾고, 해당 범위 내의 모든 비트를 반전
        else if (!strcmp(cmd, "bitmap_scan_and_flip"))
        {
            char *bitmap_name = strtok(NULL, " ");  // 비트맵의 이름을 입력 받음
            size_t start = atoi(strtok(NULL, " ")); // 검색을 시작할 인덱스를 입력 받음
            size_t count = atoi(strtok(NULL, " ")); // 검색할 비트의 개수를 입력 받음
            // "true" 또는 "false" 값을 입력 받아 bool 타입으로 변환
            bool value = strcmp(strtok(NULL, " "), "true") == 0;
            // 비트맵 이름으로부터 해당 비트맵의 인덱스를 찾음
            int bitmap_index = findStructureIndex('b', bitmap_name);

            if (bitmap_index < 0) // 해당 비트맵을 찾지 못한 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 비트맵이 없다는 메시지를 출력하고 다음 명령어로 넘어감
                continue;
            }

            // 주어진 값과 일치하는 첫 번째 비트의 인덱스를 찾아 출력하고, 해당 범위 내의 모든 비트를 반전시킴
            printf("%zu\n", bitmap_scan_and_flip(bitmapContainers[bitmap_index].link, start, count, value));
        }

        // "bitmap_dump" 지정된 비트맵의 모든 비트 값을 출력
        else if (!strcmp(cmd, "bitmap_dump"))
        {
            char *bitmap_name = strtok(NULL, " "); // 비트맵의 이름을 입력 받음
            // 비트맵 이름으로부터 해당 비트맵의 인덱스를 찾음
            int bitmap_index = findStructureIndex('b', bitmap_name);

            if (bitmap_index < 0) // 해당 비트맵을 찾지 못한 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 비트맵이 없다는 메시지를 출력하고 다음 명령어로 넘어감
                continue;
            }

            // 해당 비트맵의 모든 비트 값을 출력
            bitmap_dump(bitmapContainers[bitmap_index].link);
        }

        // "bitmap_expand" 지정된 비트맵의 크기를 새로운 비트 개수만큼 확장
        else if (!strcmp(cmd, "bitmap_expand"))
        {
            char *bitmap_name = strtok(NULL, " ");                   // 비트맵 이름을 입력 받음
            int new_bits = atoi(strtok(NULL, " "));                  // 확장할 새로운 비트 개수
            int bitmap_index = findStructureIndex('b', bitmap_name); // 비트맵 이름으로부터 해당 비트맵의 인덱스를 찾음

            if (bitmap_index < 0) // 해당 비트맵을 찾지 못한 경우
            {
                printf("Bitmap '%s' not found.\n", bitmap_name); // 비트맵이 없다는 메시지를 출력하고 다음 명령어로 넘어감
                continue;
            }

            // 비트맵을 새로운 비트 개수만큼 확장 확장된 비트맵은 기존 비트맵의 링크에 저장
            bitmapContainers[bitmap_index].link = bitmap_expand(bitmapContainers[bitmap_index].link, new_bits);
        }
    }

    return 0;
}