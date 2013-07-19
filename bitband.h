#ifndef BIT_BAND_H
#define BIT_BAND_H

#include <stdint.h>
#include "common.h"
#include "rule.h"

#define BAND_SIZE	4

#define MAX_BANDS	8	// max #bands among sip, dip, sp, dp, prot
#define	TOTAL_BANDS	26	// sum of bit-bands of a rule (sip + dip + sp + ...)
#define CUT_BANDS	2	// #bands at each tree node to partition the space

extern int field_bands[NFIELDS];


// bit-band data structure, keep it 16-bit for operation efficiency
typedef struct {
	unsigned int	dim : 4;	// dimension of the band
	unsigned int	id  : 4;	// band id in dim (7~0 for sip, 3~0 for sp)
	unsigned int	val : 8;	// e.g., 0b0110
} Band;


// Ternary bits in (BAND_SIZE) bit-bands, e.g., 0010****1101****
typedef struct {
	uint8_t		dim;		// dimension of the ternary bit string
	uint8_t		nbands;		// reduandant to band_map, for efficiency purpose
	uint8_t		bandmap[MAX_BANDS];	// 0 for * bands; 1 for others; band_map[0] for LSB
	uint32_t	val;		// bit values
} TBits;


int find_bank(uint32_t point, TBits *tbits, uint32_t *bank);

int range_overlap_tbits(Range *range, TBits *tbits);

inline
int free_band(TBits *tbits, int curr_band);

#endif
