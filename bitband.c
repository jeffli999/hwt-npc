#include <stdio.h>
#include "common.h"
#include "bitband.h"

int field_bands[NFIELDS] = {8, 8, 4, 4, 2};


// ===========================================
// operations on bit-band in ternary bits
// ===========================================

// return the low-order bit position of a band
inline
int band_low(int band_id)
{
	return band_id * BAND_BITS;
}



// return the high-order bit position of bands[i] in tbits
inline
int band_high(int band_id)
{
	return (band_id + 1) * BAND_BITS - 1;
}



// add n to v starting from bid band, only * bands indicated by tbits are involved in adding
// note n < BAND_SIZE, otherwise unpredictable result arises
int tbits_add(TBits *tbits, int bid, int n, int *v)
{
	int			lo, hi;
	uint32_t	bits;

	for (; bid < field_bands[tbits->dim]; bid++) {
		if (tbits->bandmap[bid])
			continue;
		lo = band_low(bid); hi = band_high(bid);
		bits = extract_bits(*v, hi, lo) + n;
		n = 1;
		if (bits >= BAND_SIZE) {
			bits -= BAND_SIZE;
			set_bits(v, hi, lo, bits);	// overflow on this band, proceed to higher * bands
		} else {
			set_bits(v, hi, lo, bits); 	// no overflow, done
			break;
		}
	}
	if (bid < field_bands[tbits->dim])
		return 1;	
	else
		return 0;	// adding leads to overall overflow
}



// Given a point and a set of space banks specified by tbits: return the left border of the bank
// colliding with point or right to it, it might fail to find such a bank
int bank_left_border(uint32_t point, TBits *tbits, uint32_t *lborder)
{
	uint32_t	bits1, bits2;
	int			i, lo, hi, last_band, nbands = 0, found = 0;

	*lborder = point;
	for (i = field_bands[tbits->dim]-1; i >= 0; i--) {
		if (nbands >= tbits->nbands)
			break;
		if (!tbits->bandmap[i])
			continue;	// band of * bits
		nbands++;
		lo = band_low(i); hi = band_high(i);
		bits1 = extract_bits(point, hi, lo);
		bits2 = extract_bits(tbits->val, hi, lo);
		if (bits1 == bits2)
			continue;

		// following code for point not colliding with any bank

		if (bits1 > bits2) {
			if (!tbits_add(tbits, i+1, 1, lborder))
				return 0;
		}

		// handle lower bit bands
		i--;
		for (; i >= 0; i--) {
			lo = band_low(i); hi = band_high(i);
			if (tbits->bandmap[i]) {
				bits1 = extract_bits(tbits->val, hi, lo);
				set_bits(lborder, hi, lo, bits1);
			} else {
				clear_bits(lborder, hi, lo);
			}
		}
		return 1;
	}

	// point colliding with a bank
	hi = band_low(i) - 1;
	clear_bits(lborder, hi, 0);
	return 1;
}



// To decide whether a range overlaps with a space encoded by a ternary bit string
int range_overlap_tbits(Range *range, TBits *tbits)
{
	Range	bank;

	if (!bank_left_border(range->lo, tbits, &bank.lo))
		return 0;
	return (range->hi >= bank.lo) ? 1 : 0;
}



// find the bit band that are * starting from curr_band
inline
int free_band(TBits *tbits, int curr_band)
{
	int		i;

	for (i = curr_band; i >= 0; i--) {
		if (tbits->bandmap[i] == 0)
			return i;
	}
	return i;
}
