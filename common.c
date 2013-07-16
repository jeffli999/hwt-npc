#include "common.h"


int range_overlap(Range a, Range b)
{
	if (a.lo <= b.lo)
		return (a.hi >= b.lo);
	else
		return (b.hi >= a.lo);
}



inline
uint32_t extract_bits(uint32_t a, uint32_t hi, uint32_t lo)
{
	a <<= BITS - hi - 1;
	a >>= BITS - (hi - lo) - 1;
	return a;
}



inline
void clear_bits(uint32_t *a, uint32_t hi, uint32_t lo)
{
	uint32_t	len = hi - lo + 1, mask;

	mask = ((1 << len) - 1) << lo;
	*a = *a & (~mask);
}



inline
void keep_bits(uint32_t *a, uint32_t hi, uint32_t lo)
{
	uint32_t	len = hi - lo + 1, mask;

	mask = ((1 << len) - 1) << lo;
	*a = *a & mask;
}



inline
void set_bits(uint32_t *a, uint32_t hi, uint32_t lo, uint32_t bits)
{
	uint32_t	mask;

	clear_bits(a, hi, lo);
	bits = bits << lo;
	*a = *a | bits;
}



inline
uint32_t all_one_bits(uint32_t a, uint32_t hi, uint32_t lo)
{
	uint32_t	b;

	a = extract_bits(a, hi, lo);
	b = (1 << (hi - lo + 1)) - 1;
	return a == b ? 1 : 0;
}
