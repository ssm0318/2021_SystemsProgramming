## Implementation

`int mm_init(void);`:
- 프로그램 파일의 head comment에 작성한 바와 같이, 교재의 free list structure을 그대로 참고하여 구현하였습니다.
- (padding - prologue header) (next free block - prev free block) (prologue footer - padding) (padding - epilogue header)의 형식으로 32 byte의 기본 heap 구조를 initialize하였습니다. 

`void *mm_realloc(void *ptr, size_t size);`:
- mm_malloc과 mm_free를 이용하여 공간을 재사용하지 않고 비효율적인 방식으로 구현하였습니다.

`static void *find_fit(size_t asize);`:
- 단순히 first fit search로 구현을 했는데, next fit으로만 구현을 했어도 좀 더 좋았을 것 같다는 생각도 듭니다.
 `static void place(void *bp, size_t asize);`:
- coalescing을 하는 부분을 제외하면 교재의 구현 방식과 동일합니다.

## Results
### 실습 서버 실행 결과
```bash
stu15@ubuntu:~/sp_03_malloclab/src$ ./mdriver -V
Using default tracefiles in ./traces/
Measuring performance with gettimeofday().

Testing mm malloc
Reading tracefile: amptjp-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: cccp-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: cp-decl-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: expr-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: coalescing-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: random-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: random2-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: binary-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: binary2-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: realloc-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: realloc2-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.

Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   89%    5694  0.001418  4017
 1       yes   92%    5848  0.001110  5271
 2       yes   94%    6648  0.002005  3316
 3       yes   96%    5380  0.001653  3255
 4       yes   66%   14400  0.001828  7877
 5       yes   88%    4800  0.002619  1833
 6       yes   85%    4800  0.002805  1712
 7       yes   55%   12000  0.007264  1652
 8       yes   51%   24000  0.006634  3618
 9       yes   26%   14401  0.173821    83
10       yes   34%   14401  0.004855  2966
Total          71%  112372  0.206010   545

Perf index = 42 (util) + 36 (thru) = 79/100
```

### 개인 PC 실행 결과
```bash
jaewon@jaewon-900X5N:~/Documents/malloclab-handout/src$ ./mdriver -V
Using default tracefiles in ./traces/
Measuring performance with gettimeofday().

Testing mm malloc
Reading tracefile: amptjp-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: cccp-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: cp-decl-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: expr-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: coalescing-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: random-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: random2-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: binary-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: binary2-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: realloc-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.
Reading tracefile: realloc2-bal.rep
Checking mm_malloc for correctness, efficiency, and performance.

Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   89%    5694  0.000748  7615
 1       yes   92%    5848  0.000409 14281
 2       yes   94%    6648  0.000854  7781
 3       yes   96%    5380  0.000833  6459
 4       yes   66%   14400  0.000877 16423
 5       yes   88%    4800  0.001263  3801
 6       yes   85%    4800  0.001436  3343
 7       yes   55%   12000  0.005910  2030
 8       yes   51%   24000  0.005395  4449
 9       yes   26%   14401  0.132644   109
10       yes   34%   14401  0.003768  3822
Total          71%  112372  0.154137   729

Perf index = 42 (util) + 40 (thru) = 82/100
```
- realloc이 많이 사용된 9번 tracer에서 굉장히 좋지 않은 memory utilization과 throughput을 보여 전반적인 점수에도 영향을 상당히 미치게 된 것 같습니다. realloc을 단순히 malloc과 free만을 이용하여 구현하는 것이 아니라 별도의 optimization을 해야한다는 것을 짐작할 수 있었으나, segregated free list에서 다양한 버그로 인해 막혀 reallocation에 관한 optimization은 미처 하지 못했습니다. 이 부분이 상당히 아쉽고, 매번 다짐하는 것이지만 더 빨리 과제를 시작해야할 것 같습니다..
- 교과서에 안내된 implicit free list를 그대로 사용했을 때에는 40~50점 대의 점수가 나왔던 것을 고려했을 때, explicit free list가 implicit free list에 비해 확연하게 나은 성능을 보여준다는 것을 확인할 수 있었습니다. 90점 이상의 좋은 성능은 segregated free list와 reallocation에 대한 optimization, 그리고 allocation block에 대한 boundary tag optimization, 등의 방식으로 도달할 수 있을 것으로 짐작이 됩니다.

## Segregated Free List
- 수업 시간에 안내되었듯 segregated free list의 성능이 가장 좋기에, 해당 방식으로 구현하는 것을 시도해보았으나, 시간 안에 디버깅을 하지 못해서 explicit free list로 구현했던 것을 제출하게 되었습니다. 아쉬운 마음에 미완성인 코드라도 첨부해봅니다.
시간 관계상 주석을 적지 못했는데, free_listp에 12개의 class를 두고, 각각의 크기를 two-power size로 정의하고자 하였습니다. free block의 최소 크기인 24 byte를 FSIZE와 12개의 class 각각을 오름차순으로 번호매긴 class_number를 선언한 후, `FSIZE << class_number` 이상, `FSIZE << (class_number+1)` 미만의 범위로 class를 정의하였습니다.

```C
/*
 * mm-2014-17831.c - incomplete segregated free list implementation. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

#define NUM_CLASSES 12

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE       4           /* Word and header/footer size (bytes) */
#define DSIZE       8           /* Double word size (bytes) */
#define FSIZE       24          /* Minimum free block size (bytes) */
#define CHUNKSIZE   (1 << 12)   /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)            (*(unsigned int *)(p))
#define PUT(p, val)       (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)       (GET(p) & ~0x7)
#define GET_ALLOC(p)      (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)          ((char *)(bp) - WSIZE)
#define FTRP(bp)          ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous block */
#define NEXT_BLKP(bp)     ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)     ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* Given block ptr bp, compute address of pointers to next and previous free blocks */
#define NEXT_FREEP(bp)    ((char *)(bp))
#define PREV_FREEP(bp)    ((char *)(bp + WSIZE))

/* Given block ptr bp, compute address of the actual next and previous free blocks */
#define NEXT_FREE(bp)     (*(char **)(bp))
#define PREV_FREE(bp)     (*(char **)(PREV_FREEP(bp)))

/* Write prev and next pointers for free blocks */
#define SET_PTR(p, val)   (*(unsigned int *)(p) = (unsigned int)(val))

/* Global variables */
static char *heap_listp;
static char *free_listp;

/* Interface */
int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);

/* Helper functions */
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

static void insert_block(void *bp);
static void remove_block(void *bp);

static inline size_t get_asize(size_t size)
{
    if (size <= DSIZE) {
        return 2*DSIZE;
    } else {
        return DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    }
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* Create the initial empty heap */
    if ((free_listp = mem_sbrk((NUM_CLASSES+4+8)*WSIZE)) == (void *)-1) {
        return -1;
    }

    free_listp += WSIZE;

    int i;
    for (i = 0; i < NUM_CLASSES; i++) {
        SET_PTR(free_listp + (i*WSIZE), NULL);
    }

    heap_listp = free_listp + NUM_CLASSES*WSIZE;
    PUT(heap_listp, 0);                             /* Alignment padding */
    PUT(heap_listp + (1*WSIZE), PACK(FSIZE, 1));    /* Prologue header */
    SET_PTR(heap_listp + (2*WSIZE), NULL);          /* Next free block */
    SET_PTR(heap_listp + (3*WSIZE), NULL);          /* Prev free block */
    PUT(heap_listp + (4*WSIZE), PACK(FSIZE, 1));    /* Prologue footer */
    PUT(heap_listp + (5*WSIZE), 0);                 /* Alignment padding */
    PUT(heap_listp + (6*WSIZE), 0);                 /* Alignment padding */
    PUT(heap_listp + (7*WSIZE), PACK(0, 1));        /* Epilogue header */
    heap_listp += DSIZE;
    // free_listp = heap_listp;

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }
    printImg(0);
    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) {
        /* ERROR: mem_sbrk failed. Ran out of memory... */
        return NULL;
    }

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));           /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));           /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));   /* New epilogue header */

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;       /* Adjusted block size */
    size_t extendsize;  /* Amount to extend heap if no fit */
    char *bp;    

    /* Ignore spurious requests */
    if (size == 0) {
        return NULL;
    }

    asize = get_asize(size);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) {
        return NULL;
    }
    /* Optionally split the block */
    place(bp, asize);

    return bp;
}

/* find_fit - perform a first-fit search of the implicit free list. */
static void *find_fit(size_t asize)
{
    /* First-fit search */
    void *bp;

    int class_num = NUM_CLASSES; 

    while ((class_num > 0) && (asize <= (FSIZE << --class_num))) {
        if (!(free_listp + class_num*WSIZE)) {
            bp = free_listp + (class_num*WSIZE);
            for (bp = free_listp; NEXT_FREEP(bp); bp = NEXT_FREE(bp)) {
                if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
                    return bp;
                }
            } 
        }
    }

    return NULL;    /* No fit */
}

/* Place the requested block at the beginning of the free block, 
    splitting only if the size of the remainder would 
    equal or exceed the minimum block size. (i.e. 16 bytes) */
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    if ((csize - asize) > FSIZE) {
        /* Newly allocated block */
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        remove_block(bp);
        /* Next block */
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        coalesce(bp);
    }
    /* Size of the remainder is smaller than the minimum block size */
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        remove_block(bp);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

static void *coalesce(void *bp) 
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || (PREV_BLKP(bp) == bp);
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {             /* Case 1 */
    }

    else if (prev_alloc && !next_alloc) {       /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        remove_block(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        /* Size of bp is already updated @HDRP at this point */
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) {       /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        remove_block(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else {                                      /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        remove_block(NEXT_BLKP(bp));
        remove_block(PREV_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    insert_block(bp);
    printImg(0);
    return bp;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    /* If size is equal to zero, and ptr is not NULL, 
        then the call is equivalent to free(ptr) */
    if (size <= 0) {
        mm_free(oldptr);
        return NULL;
    }

    /* If ptr is NULL, then the call is equivalent to malloc(size) */
    else if (ptr == NULL) {
        return mm_malloc(size);
    }
    
    else {
        copySize = GET_SIZE(HDRP(ptr));
        newptr = mm_malloc(size);
        if (!newptr) {
            return NULL;
        }
        memcpy(newptr, oldptr, MIN(copySize, size));
        mm_free(oldptr);
        return newptr;
    }
}

static void insert_block(void *bp)
{
    void *ptr, *prev_ptr;
    size_t size = GET_SIZE(HDRP(bp));
    int class_num = NUM_CLASSES;

    size_t asize = get_asize(size);

    while((class_num > 0) && (asize <= (FSIZE << --class_num)));

    prev_ptr = free_listp + class_num*WSIZE;
    ptr = NEXT_FREE(prev_ptr);

    while (ptr && (size <= GET_SIZE(HDRP(ptr)))) {
        prev_ptr = ptr;
        ptr = NEXT_FREE(ptr);
    }

    if (prev_ptr) {
        if (ptr) {
            SET_PTR(PREV_FREEP(bp), prev_ptr);
            SET_PTR(NEXT_FREEP(bp), ptr);
            SET_PTR(PREV_FREEP(NEXT_FREE(bp)), bp);
            SET_PTR(NEXT_FREEP(prev_ptr), bp);
        } else {
            SET_PTR(PREV_FREEP(bp), prev_ptr);
            SET_PTR(NEXT_FREEP(bp), NULL);
            SET_PTR(NEXT_FREEP(prev_ptr), bp);
        }
    } else {
        if (ptr) {
            SET_PTR(PREV_FREEP(bp), prev_ptr);
            SET_PTR(NEXT_FREEP(bp), ptr);
            SET_PTR(PREV_FREEP(NEXT_FREE(bp)), bp);
            SET_PTR(NEXT_FREEP(PREV_FREE(bp)), bp);
        } else {
            SET_PTR(PREV_FREEP(bp), prev_ptr);
            SET_PTR(NEXT_FREEP(bp), NULL);
            SET_PTR(NEXT_FREEP(prev_ptr), bp);
        }
    }
}

static void remove_block(void *bp)
{
    void *ptr, *prev_ptr;
    size_t size = GET_SIZE(HDRP(bp));
    int class_num = NUM_CLASSES;

    size_t asize = get_asize(size);

    while((class_num > 0) && (asize <= (FSIZE << --class_num)));

    // ptr = free_listp + class_num*WSIZE;
    // prev_ptr = free_listp + class_num*WSIZE;

    // while (!ptr && (size <= GET_SIZE(HDRP(ptr)))) {
    //     prev_ptr = ptr;
    //     ptr = NEXT_FREE(ptr);
    // }

    // if (PREV_FREE(bp)) {
    //     if (NEXT_FREE(bp)) {
    //         SET_PTR(NEXT_FREEP(PREV_FREE(bp)), NEXT_FREE(bp));
    //         SET_PTR(PREV_FREEP(NEXT_FREE(bp)), PREV_FREE(bp));
    //     } else {
    //         SET_PTR(NEXT_FREEP(PREV_FREE(bp)), NULL);
    //     }
    // } else {
    //     if (NEXT_FREE(bp)) {
    //         SET_PTR(NEXT_FREEP(PREV_FREE(bp)), NEXT_FREE(bp));
    //         SET_PTR(PREV_FREEP(NEXT_FREE(bp)), PREV_FREE(bp));
    //     } else {
    //         SET_PTR(prev_ptr, NULL);
    //     }
    // }
    if (NEXT_FREE(bp)) {
        SET_PTR(NEXT_FREEP(PREV_FREE(bp)), NEXT_FREE(bp));
        SET_PTR(PREV_FREEP(NEXT_FREE(bp)), PREV_FREE(bp));
    } else {
        SET_PTR(NEXT_FREEP(PREV_FREE(bp)), NULL);
    }

	return;
}
```