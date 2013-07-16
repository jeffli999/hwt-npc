#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>


#define BITS	32


typedef struct {
	uint32_t	lo;
	uint32_t	hi;
} Range;


int range_overlap(Range a, Range b);


// bit operations for 8/16/32-bit unsigned integers
inline
uint32_t extract_bits(uint32_t a, int hi, int lo);

inline
void clear_bits(uint32_t *a, int  hi, int lo);

inline
void keep_bits(uint32_t *a, int hi, int lo);

inline
void set_bits(uint32_t *a, int hi, int lo, int bits);


inline
uint32_t all_one_bits(uint32_t a, int hi, int lo);

#endif
