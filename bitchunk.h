#ifndef BIT_CHUNK_H
#define BIT_CHUNK_H

#include <stdint.h>

#define CHUNK_SIZE	4
#define	MAX_CHUNKS	26	// max chunks of a rule (sip + dip + sp + ...)
#define CUT_CHUNKS	2
#define SIP_CHUNKS	8
#define DIP_CHUNKS	8
#define SP_CHUNKS	4
#define	DP_CHUNKS	4
#define	PROT_CHUNKS	2


// bit chunk data structure, keep it 16-bit for operation efficiency
typedef struct {
	unsigned int	dim : 4;	// dimension of the chunk
	unsigned int	seq : 4;	// sequential order of the chunk in dim (7~0 for sip)
	unsigned int	val : 8;	// e.g., 0b0110
} Chunk;


int find_frame(uint32_t *frame, uint32_t point, Chunk *chunks, int num_chunks, int width);


#endif
