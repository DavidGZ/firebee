/*
 * File:        nbuf.h
 * Purpose:     Definitions for network buffer management
 *
 * Notes:       These routines implement a network buffer scheme
 */

#ifndef _NBUF_H_
#define _NBUF_H_

#include "bas_types.h"

/*
 * Include the Queue structure definitions
 */
#include "queue.h"

/*
 * Number of network buffers to use
 */
#define NBUF_MAX    30

/*
 * Size of each buffer in bytes
 */
#ifndef NBUF_SZ
#define NBUF_SZ     2048
#endif 

/*
 * Defines to identify all the buffer queues
 *  - FREE must always be defined as 0
 */
#define NBUF_FREE       0   /* available buffers */
#define NBUF_TX_RING    1   /* buffers in the Tx BD ring */
#define NBUF_RX_RING    2   /* buffers in the Rx BD ring */
#define NBUF_SCRATCH    3   /* misc */
#define NBUF_MAXQ       4   /* total number of queueus */

/* 
 * Buffer Descriptor Format 
 * 
 * Fields:
 * next     Pointer to next node in the queue
 * data     Pointer to the data buffer
 * offset   Index into buffer
 * length   Remaining bytes in buffer from (data + offset)
 */
typedef struct
{
	QNODE node;
	uint8_t *data;   
	uint16_t offset;
	uint16_t length;
} NBUF;

/*
 * Functions to manipulate the network buffers.
 */
extern int nbuf_init(void);
extern void nbuf_flush(void);
extern NBUF *nbuf_alloc (void);
extern void nbuf_free(NBUF *);
extern NBUF *nbuf_remove(int);
extern void nbuf_add(int, NBUF *);
extern void nbuf_reset(void);
extern void nbuf_debug_dump(void);


#endif  /* _NBUF_H_ */
