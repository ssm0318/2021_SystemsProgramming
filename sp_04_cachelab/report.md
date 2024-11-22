2014-17831 김재원

## Cache Lab
- 이번 과제의 목표는 cache가 어떻게 동작하는지 알아보는 것에 있다고 볼 수 있다. 컴퓨터구조 수업을 이수하지 못한 상태에서 이 과목을 듣게 되었기 때문에 과제를 시작하기 전에 많은 공부가 필요했다. 처음에는 cache의 개념을 TLB 등의 개념들을 공부하면서 시스템프로그래밍 수업에서도 어느 정도 접한 적이 있기에 바로 구현을 시작하려고 했지만, 대강의 개념 이해만 있는 채로 엄밀한 구현을 하려니 개념들이 헷갈리고 잘 기억이 나지 않았다. 그래서 유튜브에 올라와있는 CMU의 ICS 강의를 들은 후 cache에 대해 몇 가지 검색을 하면서 공부를 한 후에서야 과제를 시작할 수 있었다.
- 이번 Cache Lab은 총 두 파트로 나뉘어 있는데, Part A의 경우 valgrind 프로그램으로 generate할 수 있는 trace file을 가지고 cache의 data hit/miss/eviction을 시뮬레이션 하는 것이었다. Part B는 세 가지 종류의 matrix transposition을 효율적으로 하기 위해 어떻게 해야 하는지 cache 동작 방식을 고려하여 구현하는 것이었다. 
- Part A의 경우 cache의 기본 동작만 이해하고 있으면 코드로 옮기는 것이 어렵지 않아 구현이 비교적 수월했지만, Part B의 경우 set index 등을 전부 trace해봐야 해서 상당히 어렵게 느껴졌다. 특히 64x64 matrix의 경우 eviction이 자주 일어나서 최적화를 하는 데에 많은 어려움을 겪었다.

### Part A
- Part A의 구현은 주석에 상당히 상세히 기술했다고 생각되어 코드를 토대로 구현 방식을 설명하고자 한다. 우선 구현한 코드는 다음과 같다: 

#### Headers
```C
#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
```
- 과제에서 안내된 필수 헤더와 함께 추가적으로 필요한 헤더들이다.
- 이전 과제들과 달리 스켈레톤 코드가 상세히 주어지지 않아 처음으로 새로운 헤더를 직접 추가해봤던 것 같은데, 덕분에 구현이 자유로워 편했다.

#### Cache
```
typedef struct cache_line {
    char is_valid;
    unsigned tag;
    unsigned lru_count;
} cache_line;                               // type that stores a single line of cache

cache_line * cache;                         // like a 2D array of cache lines; cache[S][E]
```
- 과제 안내문에 제시된 것처럼 valid bit, tag, 그리고 LRU counter 정보를 모두 담을 수 있는 struct를 하나의 type으로 먼저 정의하였다.
- 그리고 마찬가지로 안내되었듯이 cache가 결국 cache line들의 2D 배열에 불과하므로, cache_line에 대한 포인터를 cache로 선언하였다.
- 이후 malloc (calloc)을 하여 실제로 필요한 만큼의 메모리 공간을 이 cache 포인터에 할당해주었다.

#### Static Global Variables
```
static int E, s, S, b, B, t;                // command-line arguments
static int is_verbose = 0;                  // verbose mode indicator
static int op_counter = 0;                // counter for instructions to keep track of LRU data
static int hit = 0, miss = 0, evict = 0;    // count number of hits, misses, and evictions
```
- s, b, e 등의 정보를 main에서 받아와 실제 cache operation을 simulate하는 함수로 전해줘야 했다. 
  - 이 때 직접 function argument 형태로 전달을 해줄수도 있지만, 별다른 style guideline이 없었기에 static한 전역 변수로 선언하여 사용하였다.
- op_counter의 경우 현재 처리하고 있는 operation이 몇 번째 operation인지를 나타내는 변수이다.
  - 즉, op_counter를 cache_line struct의 lru_count로 저장해주면, 이 lru_count의 값이 클수록 recently updated된 data라고 판단을 할 수 있게 된다.
  - 이런 식의 구현의 단점은, op_counter이 int의 양수 범위를 넘어가게 될 경우 문제가 생긴다는 것인데, 이번 과제에서 사용된 trace file들은 그렇게까지 크지 않아서 별다른 문제가 없었다.
  - 만일 더 큰 프로그램들까지 handle할 수 있도록 구현하기 위해서는 op_counter이 일정 범위를 넘어설 경우 cache의 모든 block들을 돌면서 현재의 op_counter 값을 전부 빼(subtract)주는 방법이 있다.
- hit, miss, evict의 경우 원래는 get_cache 함수의 return 값을 각각 1, 0, -1로 두고 리턴값에 따라 hit/miss/evict의 횟수를 처리해주려 했지만, 전역 변수로 선언하는 것이 더 깔끔하고 쉽다고 느껴서 이렇게 구현하였다.

#### Macros
```C
#define GET_SET_INDEX(x) ((((x) >> b) << (b+t)) >> (b+t))   // get set index bits
#define GET_TAG(x) (((x) & (0xffffffff)) >> (b+s))          // get tag bits
```
- malloc lab에서 macro 사용을 많이 해본 것이 도움이 되었던 부분으로, set index와 tag bit를 구하기 위해 간단한 매크로를 정의해서 사용하였다.

#### Helper Functions
```C
void get_cache(unsigned address, int size);  
void print_cache();                          
```
- `get_cache()`는 cache operation을 실제 시뮬레이션 하는 부분이고, `print_cache()`는 디버깅 목적으로만 정의하였다.

#### `main()`
```C
int opt;
char *tracefile;

while(-1 != (opt = getopt(argc, argv, "vs:E:b:t:"))) {
    switch(opt) {
        case 'v':   // verbose flag
            is_verbose = 1;
            break;
        case 's':   // number of set index bits
            s = atoi(optarg);
            break;
        case 'E':   // associativity
            E = atoi(optarg);
            break;
        case 'b':   // number of block bits
            b = atoi(optarg);
            break;
        case 't':   // name of trace file
            tracefile = malloc(strlen(optarg));
            strcpy(tracefile, optarg);
            break;
        case '?':
            printf("wrong argument\n");
            break;             
        default:
            break;
    }
}

FILE *pFile;        // pointer to FILE object

pFile = fopen(tracefile, "r");  // open file for reading

S = 1 << s;         // number of sets
B = 1 << b;         // block size
t = 64 - s - b;     // number of tag bits

cache = calloc(S*E, sizeof(cache_line));    // same operation as malloc+initialization

char  operation;    // operation type (I/M/L/S)
unsigned address;   // operation address (64-bit hexadecimal address)
int size;           // number of bytes accessed by operation

// read trace file
while(fscanf(pFile, " %c %x,%d", &operation, &address, &size) > 0) {
    // print operation type (for verbose mode only)
    if(is_verbose && operation != 'I') {    
        printf("%c", operation);
    }

    if(operation == 'M') {                              // data modify
        get_cache(address, size);                       // load data
        get_cache(address, size);                       // store data
    } else if(operation == 'L' || operation == 'S') {   // data load/store
        get_cache(address, size);                       // load/store data
    } else {
        continue;                                       // ignore instructions
    }
    ++op_counter;                                     
}
fclose(pFile);                      // close trace file

printSummary(hit, miss, evict);     // print results

free(cache);                        // free memory allocated for cache simulator
return 0;
```
- command line argument를 parsing하고 trace file을 읽는 부분은 안내문에 상세히 기술되어 있어 해당 부분을 참고해서 어렵지 않게 구현할 수 있었다.
- 처음에는 매뉴얼을 읽어봐도 `getopt()`의 사용 방식이 잘 와닿지 않아서 어렵게 느껴졌지만, 검색을 통해 예시를 보다 보니 `:`의 사용법이나 `?`의 사용법을 익힐 수 있었다.
- cache에 해당하는 메모리를 할당하기 위해 malloc 대신 calloc을 사용하였는데, 안내문에 제시된 내용이 `malloc`이라는 특정 명령어를 사용하라는 것이 아니라, 들어오는 argument에 따라 cache의 크기가 달라질 수 있으니 프로그램 동작 시 직접 할당해야된다는 안내 차원의 내용이라고 판단하여 `calloc`을 사용해도 된다고 판단하였다. `malloc` 후 0으로 initialization을 해주면 `calloc`과 동일한 동작을 하게 되는데, 0로 initialization하는 과정을 생략하는 것이 과제 스펙/채점에 어떠한 영향도 주지 않는다고 판단하였다.

#### `get_cache()`
```C
int i, set_index;                           
int lru_idx = -1, min_lru_count = INT_MAX;  // to handle evictions (according to LRU policy)
int first_empty_block = -1;                 // to handle cold miss
cache_line curr_block;                      // temp variable for iterations
```
- `i`는 단순히 for loop을 위해 선언한 것이고, 들어온 operation address가 `set_index`는 cache의 몇 번째 set에 해당되는지 계산한 후 저장하기 위한 변수이다.
- `lru_idx`에는 cache에서 `set_index`에 해당되는 set를 특정한 후, 해당 set 내에서 어떤 cache line이 LRU에 해당되는지 해당 line의 번호를 저장하기 위한 변수이다.
  - 물론 eviction을 할 cache line을 찾기 위해 사용된다.
  - 앞서 언급한 `op_counter`을 이용해 LRU block을 찾기 때문에 저장된 op_counter/lru_count 값이 가장 작은 값이 LRU block이 된다.
  - 이에 min_lru_count는 INT_MAX로 initialize 한다. 
  - 현재의 구현 기준으로 op_counter이 INT_MAX보다 커질 수 없다.
- `first_empty_block`은 miss가 일어났는데 해당 cache set에 empty block이 있는지 판단하기 위해 사용된다.
  - empty block이 없을 시 초기값인 -1이 저장되어 있게 된다.

#### Cache Set Iteration
```C
set_index = GET_SET_INDEX(address);         // get set index for current address

for(i=0; i<E; i++) {                        // search through all cache lines in the set
    curr_block = cache[(set_index*E)+i];    

    // if tags match and the current block is a valid cache
    if((curr_block.tag == GET_TAG(address)) && (curr_block.is_valid)) {
        // curr_block.lru_count = op_counter;     // wrong code!!
        cache[(set_index*E)+i].lru_count = op_counter;    // update lru_count of the accessed block
        if(is_verbose) {
            printf(" %x,%d hit \n", address, size);
        }
        hit++;      // increment number of hits
        return;
    }

    // find an empty block (to handle cold miss)
    if(!curr_block.is_valid) {
        if (first_empty_block == -1) {
            // first_empty_block stores the line number of the first empty block within the current set
            first_empty_block = i;  
        }
    // find the LRU block (to handle evictions)
    } else {
        if(curr_block.lru_count < min_lru_count) {
            min_lru_count = curr_block.lru_count;   
            // lru_idx stores the line number of the LRU block within the current set
            lru_idx = i;            
        }
    }
}
```
- `GET_SET_INDEX` 매크로를 이용하여 set를 특정한 뒤, 해당 set 내의 모든 cache line을 순환하며 hit이 발생하는지, empty block이 존재하는지, LRU block이 어디에 있는지를 확인한다.
- hit이 발생하면 즉시 return을 한다.

#### Store Data to Cache
```C
// create a new block for the current data
cache_line new_block = { 1, GET_TAG(address), op_counter };

// if there is no empty block in the set, evict data
if(first_empty_block == -1) {
    if(is_verbose) {
        printf(" %x,%d miss eviction \n", address, size);
        printf("evict %d %d, insert %x\n", set_index, lru_idx, address);
        print_cache();
    }
    cache[(set_index*E)+lru_idx] = new_block;   // replace LRU block with new block
    miss++;     // increment number of misses (eviction is also a miss)
    evict++;    // increment number of evictions
} else {
    if(is_verbose) {
        printf(" %x,%d miss \n", address, size);
        print_cache();
    }
    cache[(set_index*E)+first_empty_block] = new_block; // replace an empty block with new block
    miss++;     // increment number of misses
}
```
- hit이 발생하지 않았을 시 cache에 새로운 데이터를 저장하기 위한 부분이다.
- 우선 주어진 정보를 가지고 새로운 cache line을 만들어준 뒤, first_empty_block이 존재하면 (즉, -1이 아니면) 해당 line에 저장을 하고, 그렇지 않으면 lru_index 위치에 있는 block을 evict한 뒤 해당 위치에 새로운 cache line을 저장한다.

### Part B
- 우선 s=5, E=1, b=5로 고정되어 있다는 점에서 같은 set index에 다른 tag를 가진 데이터를 access하게 되면 즉시 eviction이 발생한다는 점을 알 수 있다. (E=1이기 때문에)
- 하나의 cache line은 5bit로 표현되기에 32byte 크기라는 것을 알 수 있고, 이에 따라 `int` type을 8개까지 담고 있을 수 있다는 점을 알 수 있다.
- 또한 s=5이기에 set는 총 32개라는 것을 알 수 있다.
- 이런 점들을 고려해서 가능했을 때, 최대한 한 번에 인접한 8개의 데이터를 한꺼번에 읽어와서 access를 하는 것이 miss를 줄이는 방법이 된다.
  - 그런데 32x32 matrix의 경우 하나의 row가 하나의 cache line과 크기가 동일해서 각각의 row를 읽어와서 연산을 해도 별다른 문제가 발생하지 않지만, 다른 크기의 matrix의 경우 동일한 방식으로 구현을 하면 겹치는 set index가 많아 eviction이 자주 발생하게 된다. 이에, 각각의 case에 대해 다른 방식으로 구현을 할 수밖에 없었다.
  - 처음에는 block size를 조정하여 모든 케이스에 대해 다 적용될 수 있는 코드를 짜고 싶었는데, 그렇게 했을 때 64x64 matrix의 miss 횟수가 4000회 이상 넘어가게 되었다.

#### Case 1: 32x32
```C
for(j = 0; j < N; j+=8) {
    for(i = 0; i < M; i++) {
        // store a block (8 ints) of values from a vertical axis of matrix A
        b0 = A[i][j];
        b1 = A[i][j+1];
        b2 = A[i][j+2];
        b3 = A[i][j+3];
        b4 = A[i][j+4];
        b5 = A[i][j+5];
        b6 = A[i][j+6];
        b7 = A[i][j+7];

        // assign the stored values to matrix B
        // all values should belong to the same block, hence no further evictions occur
        B[j][i] = b0;
        B[j+1][i] = b1;
        B[j+2][i] = b2;
        B[j+3][i] = b3;
        B[j+4][i] = b4;
        B[j+5][i] = b5;
        B[j+6][i] = b6;
        B[j+7][i] = b7;
    }
}
```
- 과제 슬라이드에 적혀있는대로, matrix B로 transpose를 할 때 세로로 access를 하게 되면 매번 새로운 cache line을 access하게 되는 셈이라 miss가 많이 발생한다.
- 따라서 이런 문제를 최소화하기 위해서는 B에서의 하나의 row를 구성하는 8개 element를 A로 부터 미리 저장한 후에 B로 옮겨주는 방법을 사용하였다.

#### Case 2: 64x64
```C
for(j = 0; j < N; j+=4) {
    for(i = 0; i < M; i+=8) {
        for(ii = 0; ii < 4; ii++) {
            b0 = A[i][j+ii];
            b1 = A[i+1][j+ii];
            b2 = A[i+2][j+ii];
            b3 = A[i+3][j+ii];
            B[j+ii][i] = b0;
            B[j+ii][i+1] = b1;
            B[j+ii][i+2] = b2;
            B[j+ii][i+3] = b3;
        }
        for(ii = 0; ii < 4; ii++) {
            b4 = A[i+4][j+ii];
            b5 = A[i+5][j+ii];
            b6 = A[i+6][j+ii];
            b7 = A[i+7][j+ii];
            B[j+ii][i+4] = b4;
            B[j+ii][i+5] = b5;
            B[j+ii][i+6] = b6;
            B[j+ii][i+7] = b7;
        }
    }
}
```
- 64x64 matrix의 경우 width가 32x32 matrix의 두 배이기 때문에 32x32 matrix에서 8줄 단위로 access를 해도 되었던 것과 달리 네 번째 줄을 넘어가게 되면 아래 네 줄이 위의 네 줄과 set index가 동일해 eviction이 일어났다.
- 이 문제를 최소화하기 위해 8줄 단위로 transpose를 하는게 아니라 4줄 단위로 access를 해서 eviction이 최소화되도록 하였다.

#### Case 3: 61x67
```C
for (i = 0; i < N; i += 18) {
    for (j = 0; j < M; j += 18) {
        // check (ii < N) and (jj < M) for corner cases, as 61 and 67 are not divisible by 18
        for (ii = i; ii < (i + 18) && ii < N; ii++) {
            for (jj = j; jj < (j + 18) && jj < M; jj++) {
                B[jj][ii] = A[ii][jj];
            }
        }
    }
}
```
- 이 케이스는 61이 소수기 때문에 conflict가 발생할 일이 적어서 적당한 크기의 block으로 iteration을 해주면 될 것이라고 생각했다.
- 여러가지 숫자로 실험을 해봤는데, 숫자가 너무 커도 (e.g. 34), 너무 작아도 (e.g. 9) miss의 수가 많아졌다. 16~19 정도의 숫자는 전부 2000회 이하로 miss가 발생했다.