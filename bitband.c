#include <stdio.h>
#include "common.h"
#include "bitband.h"

int field_bands[NFIELDS] = {8, 8, 4, 4, 2};


// ===========================================
// operations on bit-band in ternary bits
// ===========================================

// return the least significant bit position of a band
inline
int band_lsb(int band_id)
{
	return band_id * BAND_BITS;
}



// return the most significant bit position of bands[i] in tbits
inline
int band_msb(int band_id)
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
		lo = band_lsb(bid); hi = band_msb(bid);
		bits = extract_bits(*v, hi, lo) + n;
		n = 1;
		if (bits < BAND_SIZE) {
			set_bits(v, hi, lo, bits); 	// no overflow, done
			break;
		} else {
			bits -= BAND_SIZE;
			set_bits(v, hi, lo, bits);	// overflow on this band, proceed to higher * bands
		}
	}
	if (bid < field_bands[tbits->dim])
		return 1;	
	else
		return 0;	// adding leads to overall overflow
}



// subtract n to v starting from bid band, only * bands indicated by tbits are involved
// note n < BAND_SIZE, otherwise unpredictable result arises
int tbits_subtract(TBits *tbits, int bid, int n, int *v)
{
	int			lo, hi;
	uint32_t	bits;

	for (; bid < field_bands[tbits->dim]; bid++) {
		if (tbits->bandmap[bid])
			continue;
		lo = band_lsb(bid); hi = band_msb(bid);
		bits = extract_bits(*v, hi, lo);
		n = 1;
		if (bits >= n) {
			set_bits(v, hi, lo, bits-n); // no underflow, done
			break;
		} else {
			bits = BAND_SIZE - (n - bits);
			set_bits(v, hi, lo, bits); 	// no overflow, done
		}
	}
	if (bid < field_bands[tbits->dim])
		return 1;	
	else
		return 0;	// overall underflow
}



// if point is in one of the space banks specified by tbits, return 1, othewise return 0
// band is used to remember the lowest non-* band id (if point is in bank), or the first non-* band
// that differs between point and tbits
int point_in_bank(uint32_t point, TBits *tbits, int *band)
{
	uint32_t	bits1, bits2;
	int			nbands = 0, i, lo, hi;

	for (i = field_bands[tbits->dim]-1; i >= 0; i--) {
		if (nbands == tbits->nbands) {
			*band = i - 1;
			return 1;
		}
		if (!tbits->bandmap[i])
			continue;	// a * band
		else
			nbands++;	
		lo = band_lsb(i); hi = band_msb(i);
		bits1 = extract_bits(point, hi, lo);
		bits2 = extract_bits(tbits->val, hi, lo);
		if (bits1 != bits2) {
			*band = i;
			return 0;
		}
	}

	*band = 0;
	return 1;
}



// Given a point P and space banks specified by tbits: 
// - P contained in one bank: return the left border of that bank
// - Otherwise: return the left border of the right bank, it may fail to find one in this case
int right_bank_left_border(uint32_t point, TBits *tbits, uint32_t *border)
{
	uint32_t	bits1, bits2;
	int			i, lo, hi, stop_band, nbands = 0, found = 0;

	// point in bank, just clear lower bits will get the left border
	*border = point;
	if (point_in_bank(point, tbits, &stop_band)) {
		if (stop_band > 0)
			clear_bits(border, band_lsb(stop_band)-1, 0);
		return 1;
	}

	// point not in bank, first find the right bank (if applicable)
	lo = band_lsb(stop_band); hi = band_msb(stop_band);
	bits1 = extract_bits(point, hi, lo);
	bits2 = extract_bits(tbits->val, hi, lo);
	if (bits1 > bits2) {
		if (!tbits_add(tbits, stop_band+1, 1, border))
			return 0;
	}

	// after finding the right bank, handle lower bands starting from stop_band:
	// clear * bands, and set non-* bands given by tbits
	for (i = stop_band; i >= 0; i--) {
		lo = band_lsb(i); hi = band_msb(i);
		if (tbits->bandmap[i]) {
			bits1 = extract_bits(tbits->val, hi, lo);
			set_bits(border, hi, lo, bits1);
		} else {
			clear_bits(border, hi, lo);
		}
	}
	return 1;
}



// Given a point P and space banks specified by tbits: 
// - P contained in one bank: return the right border of that bank
// - Otherwise: return the right border of the left bank, it may fail to find one in this case
int left_bank_right_border(uint32_t point, TBits *tbits, uint32_t *border)
{
	uint32_t	bits1, bits2;
	int			i, lo, hi, stop_band, nbands = 0, found = 0;

	// point in bank, just fill lower bits will get the right border
	*border = point;
	if (point_in_bank(point, tbits, &stop_band)) {
		if (stop_band > 0)
			fill_bits(border, band_lsb(stop_band)-1, 0);
		return 1;
	}

	// point not in bank, first find the left bank (if applicable)
	lo = band_lsb(stop_band); hi = band_msb(stop_band);
	bits1 = extract_bits(point, hi, lo);
	bits2 = extract_bits(tbits->val, hi, lo);
	if (bits1 < bits2) {
		if (!tbits_subtract(tbits, stop_band+1, 1, border))
			return 0;
	}

	// after finding the left bank, handle lower bands starting from stop_band:
	// clear * bands, and set non-* bands given by tbits
	for (i = stop_band; i >= 0; i--) {
		lo = band_lsb(i); hi = band_msb(i);
		if (tbits->bandmap[i]) {
			bits1 = extract_bits(tbits->val, hi, lo);
			set_bits(border, hi, lo, bits1);
		} else {
			fill_bits(border, hi, lo);
		}
	}
	return 1;
}



// return the border of the space specified by tbits, side = 0 means left border
inline
uint32_t tbits_border(TBits *tbits, int side)
{
	int			i, hi, lo;
	uint32_t	border, bits, side_bits;

	side_bits = (side == 0) ? 0 : (BAND_SIZE-1);
	for (i = field_bands[tbits->dim]-1; i >= 0; i--) {
		hi = band_msb(i); lo = band_lsb(i);
		if (tbits->bandmap[i])
			bits = extract_bits(tbits->val, hi, lo);
		else
			bits = side_bits;
		set_bits(&border, hi, lo, bits);
	}
	return border;
}



// To decide whether a range r overlaps with a space encoded by a ternary bit string
int range_tbits_overlap(Range *r, TBits *tbits)
{
	Range	bank;

	if (!right_bank_left_border(r->lo, tbits, &bank.lo))
		return 0;
	return (r->hi >= bank.lo) ? 1 : 0;
}



// the widest range that covers r, but no more overlap with the territory of tbits than r
void range_tbits_cover(Range *r, TBits *tbits)
{
	int			band;
	uint32_t	border;

	if (!point_in_bank(r->lo, tbits, &band)) {
		if (!left_bank_right_border(r->lo, tbits, &border))
			border = tbits_border(tbits, 0);
		r->lo = border + 1;
	}

	if (!point_in_bank(r->hi, tbits, &band)) {
		if (!right_bank_left_border(r->hi, tbits, &border))
			border = tbits_border(tbits, 1);
		r->hi = border - 1;
	}
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

