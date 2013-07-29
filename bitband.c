#include <stdio.h>
#include <stdlib.h>
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
// note n < BAND_SIZE, otherwise unpredictable result arises, return 0 when overlfow
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



// return the range of the space specified by tbits
inline
Range tbits_range(TBits *tbits)
{
	int			i, hi, lo;
	uint32_t	bits;
	Range		range;

	range.lo = range.hi = 0;
	for (i = field_bands[tbits->dim]-1; i >= 0; i--) {
		hi = band_msb(i); lo = band_lsb(i);
		if (tbits->bandmap[i]) {
			bits = extract_bits(tbits->val, hi, lo);
			set_bits(&range.lo, hi, lo, bits);
			set_bits(&range.hi, hi, lo, bits);
		} else {
			set_bits(&range.lo, hi, lo, 0);
			set_bits(&range.hi, hi, lo, (BAND_SIZE-1));
		}
	}
	return range;
}



// To decide whether a range r overlaps with a space encoded by a ternary bit string
int range_tbits_overlap(Range *r, TBits *tbits)
{
	Range	bank;

	if (!right_bank_left_border(r->lo, tbits, &bank.lo))
		return 0;
	return (r->hi >= bank.lo) ? 1 : 0;
}



// the min range that covers the same space of tbits as r, return 0 if non-existant
int min_tbits_cover(Range *r, TBits *tbits)
{
	int			band;
	uint32_t	border;
	Range		range_tb = tbits_range(tbits);

	if (!point_in_bank(r->lo, tbits, &band)) {
		if (right_bank_left_border(r->lo, tbits, &border))
			r->lo = border;
		else
			return 0;
	}

	if (!point_in_bank(r->hi, tbits, &band)) {
		if (left_bank_right_border(r->hi, tbits, &border))
			r->hi = border;
		else
			return 0;
	}

	return (r->hi >= r->lo);
}



// the max range that covers the same space of tbits as r
void max_tbits_cover(Range *r, TBits *tbits)
{
	int			band;
	uint32_t	border;
	Range		range_tb = tbits_range(tbits);

	if (!point_in_bank(r->lo, tbits, &band)) {
		if (left_bank_right_border(r->lo, tbits, &border))
			r->lo = border + 1;
		else
			r->lo = range_tb.lo;
	}

	if (!point_in_bank(r->hi, tbits, &band)) {
		if (right_bank_left_border(r->hi, tbits, &border))
			r->hi = border - 1;
		else
			r->hi = range_tb.hi;
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



inline
void set_tbits(TBits *tb, uint32_t band_id, uint32_t val)
{
	set_bits(&tb->val, band_msb(band_id), band_lsb(band_id), val);
}



// add a band to the corresponding one in the TBit set
inline
void add_tbits_band(TBits *tb_set, Band *band)
{
	TBits	*tb = &tb_set[band->dim];

	if (tb->bandmap[band->id] != 0) {
		printf("Oops, the band to add already set in the tbits\n");
		exit(1);
	}
	tb->nbands++;
	tb->bandmap[band->id] = 1;
	if (band->id > tb->band_hi)
		tb->band_hi = band->id;
	else if (band->id < tb->band_lo)
		tb->band_lo = band->id;
	set_tbits(tb, band->id, band->val);
}



// set one band in tbits given by a band parameter
inline
void set_tbits_band(TBits *tb_set, Band *band)
{
	TBits	*tb = &tb_set[band->dim];

	if (tb->bandmap[band->id] == 0) {
		printf("Oops, the band wasn't set before, use set_tbits_band instead!\n");
		exit(1);
	}
	set_tbits(tb, band->id, band->val);
}



// Given a range and two tbits with the same set of cut bands (but different cut values), 
// whether the overlaps of r with the two tbits are equal for subsequent cuts along this dimension
int equal_cuts(Range r, TBits *tb1, TBits *tb2)
{
	Range		r1 = r, r2 = r;
	uint32_t	bits1, bits2;
	int			i;

	min_tbits_cover(&r1, tb1);
	min_tbits_cover(&r2, tb2);
	for (i = field_bands[tb1->dim]; i >= 0; i--) {
		if (tb1->bandmap[i])
			continue;
		bits1 = extract_bits(r1.lo, band_msb(i), band_lsb(i));
		bits2 = extract_bits(r2.lo, band_msb(i), band_lsb(i));
		if (bits1 != bits2)
			return 0;
		bits1 = extract_bits(r1.hi, band_msb(i), band_lsb(i));
		bits2 = extract_bits(r2.hi, band_msb(i), band_lsb(i));
		if (bits1 != bits2)
			return 0;
	}
}



void dump_tbits(TBits *tb)
{
	int		i, n;

	if (tb->dim < 2)
		n = 8;
	else if (tb->dim > 3)
		n = 2;
	else
		n = 4;

	for (i = n; i >= 0; i--) {
		if (tb->bandmap[i])
			printf("%x", extract_bits(tb->val, band_msb(i), band_lsb(i)));
		else
			printf("*");
	}
}

