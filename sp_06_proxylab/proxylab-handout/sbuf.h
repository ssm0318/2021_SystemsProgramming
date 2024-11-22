#ifndef __SBUF_H__
#define __SBUF_H__

#include "csapp.h"

/*
 * Reference: CSAPP 3e (`code/conc/sbuf.h`)
 *  Figure 12.24 `sbuf_t`: Bounded buffer used by the SBUF package
 */

typedef struct{
    int *buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
} sbuf_t;

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

#endif
