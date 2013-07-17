#ifndef BIT_BAND_H
#define BIT_BAND_H

#include <stdint.h>

#define BAND_SIZE	4
#define SIP_BANDS	8
#define DIP_BANDS	8
#define SP_BANDS	4
#define	DP_BANDS	4
#define	PROT_BANDS	2
#define MAX_BANDS	8	// max #bands among sip, dip, sp, dp, prot
#define	TOTAL_BANDS	26	// sum of bit-bands of a rule (sip + dip + sp + ...)
#define CUT_BANDS	2	// #bands at each tree node to partition the space


// bit-band data structure, keep it 16-bit for operation efficiency
typedef struct {
	unsigned int	dim : 4;	// dimension of the band
	unsigned int	seq : 4;	// sequential order of the band in dim (7~0 for sip)
	unsigned int	val : 8;	// e.g., 0b0110
} Band;


// Tenary string in (BAND_SIZE) bit-bands, e.g., 0010****1101****
typedef struct {
	Band	bands[MAX_BANDS];
	int		nbands;		// #bands
	int		len;		// length of tenary bit string
} TString;


int find_bank(uint32_t point, TString *str, uint32_t *bank);

int range_overlap_bank(Range *range, TString *str);

#endif
