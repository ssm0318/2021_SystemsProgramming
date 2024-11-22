/*
 * mm-2014-17831.c - The fastest, least memory-efficient malloc package.
 * 
 * The overall structure of this approach is based on explicit free lists,
 * with boundary tags, coalescing, and next/prev free pointers.
 * 
 * The structure of the free and allocated blocks are as follows:
 *  - allocated blocks: |                   Padding for 8 byte alignment             |
 *     (MIN 16 bytes)   | Header (29bit size + 1bit LSB as allocation indicator bit) |
 *                      |                        Payload                             |
 *                      | Footer (29bit size + 1bit LSB as allocation indicator bit) |
 *                      |                   Padding for 8 byte alignment             |
 *    
 *
 *  - free blocks:      |                   Padding for 8 byte alignment             |
 *     (MIN 24 bytes)   | Header (29bit size + 1bit LSB as allocation indicator bit) |
 *                      |                    Prev Free block                         |
 *                      |                       Free space                           |
 *                      |                    Next Free block                         |
 *                      | Footer (29bit size + 1bit LSB as allocation indicator bit) |
 *                      |                   Padding for 8 byte alignment             |
 * 
 * The overall structure of the free list is adopted from Figure 9.42 of the CSAPP textbook.
 * This includes a prologue block header/footer, an epilogue block header, and some ad hoc paddings.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

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

// /* Write prev and next pointers for free blocks */
#define SET_PTR(p, ptr)   (*(unsigned int *)(p) = (unsigned int)(ptr))

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
    if ((heap_listp = mem_sbrk(8*WSIZE)) == (void *)-1) {
        return -1;
    }
    PUT(heap_listp, 0);                             /* Alignment padding */
    PUT(heap_listp + (1*WSIZE), PACK(FSIZE, 1));    /* Prologue header */
    SET_PTR(heap_listp + (2*WSIZE), NULL);          /* Next free block */
    SET_PTR(heap_listp + (3*WSIZE), NULL);          /* Prev free block */
    PUT(heap_listp + (4*WSIZE), PACK(FSIZE, 1));    /* Prologue footer */
    PUT(heap_listp + (5*WSIZE), 0);                 /* Alignment padding */
    PUT(heap_listp + (6*WSIZE), 0);                 /* Alignment padding */
    PUT(heap_listp + (7*WSIZE), PACK(0, 1));        /* Epilogue header */
    heap_listp += DSIZE;
    free_listp = heap_listp;

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }
    return 0;
}

/* 
 * extend_heap - Extend the size of the heap as given by the argument `words`.
 *               Any necessary coalescing of the newly allocated block is done here.
 */
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
 *             Always allocate a block whose size is a multiple of the alignment.
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

/* 
 * find_fit - perform a first-fit search of the implicit free list.
 */
static void *find_fit(size_t asize)
{
    /* First-fit search */
    void *bp;

    for (bp = free_listp; NEXT_FREEP(bp); bp = NEXT_FREE(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            return bp;
        }
    }
    return NULL;    /* No fit */
}

/* place - Place the requested block at the beginning of the free block, 
 * splitting only if the size of the remainder would 
 * equal or exceed the minimum block size. (i.e. 16 bytes) 
 */
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

/*
 * coalesce - coalesce free blocks according to the four case.
 *            make any necessary adjustments to the explicit free list as necessary.
 */
static void *coalesce(void *bp) 
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {             /* Case 1 */
    }

    else if (prev_alloc && !next_alloc) {       /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        /* remove the coalesced block (i.e. next free block) from the explicit free list */
        remove_block(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        /* Size of bp is already updated @HDRP at this point */
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) {       /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        /* remove the coalesced block (i.e. prev free block) from the explicit free list */
        remove_block(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else {                                      /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        /* remove the coalesced blocks (i.e. prev & next free blocks) from the explicit free list */
        remove_block(NEXT_BLKP(bp));
        remove_block(PREV_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    /* add the newly coalesced free block to the explicit free list */
    insert_block(bp);
    return bp;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free.
 *              After allocation, copy the contents in the range from the start of the region 
 *              up to the minimum of the old and new sizes.  
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
        /* compute the actual size that we have to copy */
        copySize = GET_SIZE(HDRP(ptr));
        newptr = mm_malloc(size);
        /* if malloc fails, return */
        if (!newptr) {
            return NULL;
        }
        /* copy memory contents as necessary */
        memcpy(newptr, oldptr, MIN(copySize, size));
        mm_free(oldptr);
        return newptr;
    }
}

/* 
 * insert_block - insert free block to the explicit free list in ascending address-order.
 */
static void insert_block(void *bp)
{
    void *ptr = free_listp;

    /* find the correct address space */
    while (!NEXT_FREE(ptr) && ((char *)bp < NEXT_FREEP(ptr))) {
        ptr = NEXT_FREE(ptr);
    }

    /* if the free block is the last block on the list */
    if (!NEXT_FREE(ptr)) {
        SET_PTR(PREV_FREEP(bp), ptr);
        SET_PTR(NEXT_FREEP(bp), NULL);
        SET_PTR(NEXT_FREEP(ptr), bp);
    } else {
        SET_PTR(PREV_FREEP(bp), ptr);
        SET_PTR(NEXT_FREEP(bp), NEXT_FREE(ptr));
        SET_PTR(PREV_FREEP(NEXT_FREE(bp)), bp);
        SET_PTR(NEXT_FREEP(ptr), bp);
    }
}

/*
 * remove_block - remove specified block from the explicit free list.
 */
static void remove_block(void *bp)
{
    /* if the specified block was the last block in the list */
    if (!NEXT_FREE(bp)) {
        SET_PTR(NEXT_FREEP(PREV_FREE(bp)), NULL);
        SET_PTR(PREV_FREE(bp), NULL);
    } else {
        SET_PTR(NEXT_FREEP(PREV_FREE(bp)), NEXT_FREE(bp));
        SET_PTR(PREV_FREEP(NEXT_FREE(bp)), PREV_FREE(bp));
    }
	return;
}












