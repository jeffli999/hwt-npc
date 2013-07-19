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
	return band_id * BAND_SIZE;
}



// return the high-order bit position of bands[i] in tbits
inline
int band_high(int band_id)
{
	return (band_id + 1) * BAND_SIZE - 1;
}



// Corresponding to case 1/2 in find_bank(), i.e.
//     +--------+
//  p  | bank   |
//     +--------+
int bank_case1(TBits *tbits, int band_id, uint32_t *bank_lo)
{
	int			i, lo, hi;
	uint32_t	bits;
	
	// 1. set lower * bands to 0, and set non-* bands to tbits band
	for (i = band_id; i >= 0; i--) {
		lo = band_low(i);
		hi = band_high(i);
		if (tbits->bandmap[i]) {
			bits = extract_bits(tbits->val, hi, lo);
			set_bits(bank_lo, hi, lo, bits);
		} else {
			clear_bits(bank_lo, hi, lo);
		}
	}

	return 1;
}



// Corresponding to case 3 (or 4) in find_bank(), i.e.
// +--------+        +--------+
// | bank'  |  p     | bank   |
// +--------+        +--------+
int bank_case2(TBits *tbits, int band_id, uint32_t *bank_lo)
{
	int			i, lo, hi, len, found = 0;
	uint32_t	bits;

	// 1. add the higher * band by 1 to get the right bank
	for (i = band_id + 1; i < field_bands[tbits->dim]; i++) {
		if (tbits->bandmap[i])
			continue;	// band of non-* bits
		lo = band_low(i);
		hi = band_high(i);
		if (all_one(*bank_lo, hi, lo))
			clear_bits(bank_lo, hi, lo);	// overflow, proceed to higher * bands
		else {
			*bank_lo += 1 << lo;	// no overflow, done
			break;
		}
	}

	if (i == field_bands[tbits->dim])
		return 0;	// no existence of right bank
	
	// 2. set lower * bands to 0, and set non-* bands to tbits band
	for (i = band_id; i >= 0; i--) {
		lo = band_low(i);
		hi = band_high(i);
		if (tbits->bandmap[i]) {
			bits = extract_bits(tbits->val, hi, lo);
			set_bits(bank_lo, hi, lo, bits);
		} else {
			clear_bits(bank_lo, hi, lo);
		}
	}

	return 1;
}



// Among the space banks encoded by tbits, return the one containing or immediately right to point.
// Note: bit bands in tbits must be ordered, e.g.,
// with tbits:   0001-****-1010-**** encoding 16 4-bit space banks: 
// 1. for point: 0001-1100-1000-1000, the bank is 0001-1100-1000-****, which is right to point
// 2. for point: 0001-1100-1011-1000, the bank is 0001-1101-1010-****, which is also right to point,
//    but the bank 0001-1100-1010-**** with *-bit bands setting to point's values is left to  point,
//    which is not the bank that we desire. 
// 3. for point: 0001-1100-1010-1000, the bank is 0001-1100-1010-****, which contains point
// For case 2, sometimes we may not find a right bank, e.g,
// 4. for point: 0001-1111-1011-1000, no bank is found no matter what values are set for * bits
int find_bank(uint32_t point, TBits *tbits, uint32_t *bank_lo)
{
	uint32_t	bits1, bits2;
	int			i, lo, hi, last_band, nbands = 0, found = 0;

	*bank_lo = point;
	for (i = field_bands[tbits->dim] - 1; (i >= 0) && (nbands < tbits->nbands); i--) {
		if (!tbits->bandmap[i])
			continue;	// band of * bits

		lo = band_low(i);
		hi = band_high(i);
		bits1 = extract_bits(point, hi, lo);
		bits2 = extract_bits(tbits->val, hi, lo);
		if (bits1 < bits2) { 		// case 1: point on the left of current bank
			found = bank_case1(tbits, i, bank_lo);
			break;
		} else if (bits1 > bits2) { // case 2: point on the right of current bank
			found = bank_case2(tbits, i, bank_lo);
			break;
		}
		nbands++;
		last_band = i;
	}

	if (nbands == tbits->nbands) { 	// case 3: point inside current bank
		if (last_band > 0) {
			hi = band_low(last_band) - 1;
			clear_bits(bank_lo, hi, 0);
		}
		found = 1;
	}
	return found;
}



// To decide whether a range overlaps with a space encoded by a ternary bit string
int range_overlap_tbits(Range *range, TBits *tbits)
{
	Range	bank;

	if (!find_bank(range->lo, tbits, &(bank.lo)))
		return 0;
//printf("range: [%x, %x], bank.lo: %x\n", range->lo, range->hi, bank.lo);
//printf("1\n");
	if (range->hi >= bank.lo)
		return 1;
	return 0;
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
