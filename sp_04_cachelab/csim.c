/* 
 * 2014-17831 김재원
 * 
 * csim.c - Cache Simulator
 *
 * This program takes a valgrind memory trace as input,
 * simulates the hit/miss behavior of a cache memory on this trace, 
 * and outputs the total number of hits, misses, and evictions.
 * 
 * The simulator takes the following command-line arguments:
 * ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>
 * -h: Optional help flag that prints usage info
 * -v: Optional verbose flag that displays trace info
 * -s <s>: Number of set index bits (S = 2 s is the number of sets)
 * -E <E>: Associativity (number of lines per set)
 * -b <b>: Number of block bits (B = 2 b is the block size)
 * -t <tracefile>: Name of the valgrind trace to replay
 * (options -h and -v are idle in this simulator)
 * 
 * The simulator works for arbitrary values of s, E, and b.
 * LRU (least-recently used) replacement policy is used when choosing which cache line to evict.
 */ 

#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

typedef struct cache_line {
    char is_valid;
    unsigned tag;
    unsigned lru_count;
} cache_line;                               // type that stores a single line of cache

cache_line * cache;                         // like a 2D array of cache lines; cache[S][E]

static int E, s, S, b, B, t;                // command-line arguments
static int is_verbose = 0;                  // verbose mode indicator
static int op_counter = 0;                // counter for instructions to keep track of LRU data
static int hit = 0, miss = 0, evict = 0;    // count number of hits, misses, and evictions

#define GET_SET_INDEX(x) ((((x) >> b) << (b+t)) >> (b+t))   // get set index bits
#define GET_TAG(x) (((x) & (0xffffffff)) >> (b+s))          // get tag bits

void get_cache(unsigned address, int size);  
void print_cache();                          

/*
 * main(): parse command-line arguments, read valgrind traces, and call appropriate functions
 * 
 * (-h, -v options are not handled properly)
 */
int main(int argc, char **argv)
{
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
}

/*
 * get_cache(): simulates cache operations
 * load data, and store data in case of misses/evictions
 */
void get_cache(unsigned address, int size)
{
    int i, set_index;                           
    int lru_idx = -1, min_lru_count = INT_MAX;  // to handle evictions (according to LRU policy)
    int first_empty_block = -1;                 // to handle cold miss
    cache_line curr_block;                      // temp variable for iterations
    
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
}

/*
 * print_cache(): simple cache printer (for debugging purposes only)
 */
void print_cache()
{
    int i, j;
    for(i=0; i<S; i++) {
        for(j=0; j<E; j++) {
            cache_line curr_line = cache[i*E+j];
            printf("%c %u %u | ", curr_line.is_valid, curr_line.tag, curr_line.lru_count);
        }
        printf("\n");
    }
}