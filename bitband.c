#include <stdio.h>
#include "common.h"
#include "bitband.h"



// ===========================================
// operations on bit-band in a tenary string
// ===========================================

// return the low-order bit position of bands[i] in str
inline
int band_low(TString *str, int idx)
{
	return str->bands[idx].seq * BAND_SIZE;
}



// return the high-order bit position of bands[i] in str
inline
int band_high(TString *str, int idx)
{
	return (str->bands[idx].seq + 1) * BAND_SIZE - 1;
}



inline
int band_size(TString *str)
{
	int		n = str->bands[str->nbands-1].seq * BAND_SIZE;
	return 1 << n;
}



// Corresponding to case 1/2 in find_bank(), i.e.
//     +--------+
//  p  | bank   |
//     +--------+
int bank_case1(TString *str, int idx, uint32_t *bank)
{
	int			i, lo;
	uint32_t	bits;

	// 1. set all low-order bits (if any) to 0 (to get the left border of the bank)
	lo = band_low(str, idx);
	if (lo > 0) 
		clear_bits(bank, lo-1, 0); 

	// 2. set bits of the rest bands (non-* bits)
	for (i = idx; i < str->nbands; i++) {
		lo = band_low(str, i);
		set_bits(bank, lo+BAND_SIZE-1, lo, str->bands[i].val);
	}

	return 1;
}



// Corresponding to case 3 (or 4) in find_bank(), i.e.
// +--------+        +--------+
// | bank'  |  p     | bank   |
// +--------+        +--------+
int bank_case2(TString *str, int idx, uint32_t *bank)
{
	int			i, lo, hi, len, found = 0;
	uint32_t	bits;

	// 1. add the value of bits between band[i-1] and band[i] by 1 to get the bank on right
	for (i = idx; i > 0; i--) {
		len = (str->bands[i-1].seq - str->bands[i].seq - 1) * BAND_SIZE; 
		if (len == 0)	// no * bits in between two band[i-1] & band[i]
			continue;
		lo = (str->bands[i].seq + 1) * BAND_SIZE;
		hi = lo + len - 1;
		if (all_one(*bank, hi, lo)) { // overflow, proceed to higher * bands
			clear_bits(bank, hi, lo);
		} else { // no overflow, add operation is done
			*bank += 1 << lo;
			break;
		}
	}

	// if not found yet, try the most significant * bands (if any)
	if (i == 0) {
		hi = band_high(str, 0);
		if (hi == str->len - 1)
			return 0;	// no banks on the right of point
		lo = band_low(str, 0);
		if (all_one(*bank, hi, lo)) {
			return 0;	// no banks on the right of point
		} else {
			*bank += 1 << lo;
		}
	}
	
	// 2. clear lower bits
	lo = band_low(str, idx);
	if (lo > 0)
		clear_bits(bank, lo-1, 0); 

	// 3. set bits of lower non-* bands
	for (i = idx; i < str->nbands; i++) {
		lo = band_low(str, i);
		set_bits(bank, lo+BAND_SIZE-1, lo, str->bands[i].val);
	}

	return 1;
}



// Among the space banks encoded by str, return the one containing or immediately right to point.
// Note: bit bands in str must be ordered, e.g.,
// with str:  0001-****-1010-**** encoding 16 4-bit space banks: 
// 1. for point: 0001-1100-1010-1000, the bank is 0001-1100-1010-****, which contains point
// 2. for point: 0001-1100-1000-1000, the bank is 0001-1100-1000-****, which is right to point
// 3. for point: 0001-1100-1011-1000, the bank is 0001-1101-1010-****, which is also right to point,
//    but the bank 0001-1100-1010-**** with *-bit bands setting to point's values is left to  point,
//    which is not the bank that we desire. In some cases, we may not find a right bank, e.g,
// 4. for point: 0001-1111-1011-1000, no bank is found no matter what values are set for * bits
int find_bank(uint32_t point, TString *str, uint32_t *bank)
{
	uint32_t	bits;
	int			i, lo, hi, found = 0;

	*bank = point;
	for (i = 0; i < str->nbands; i++) {
		lo = band_low(str, i);
		hi = band_high(str, i);
		bits = extract_bits(point, hi, lo);
		if (bits < str->bands[i].val) {
			// case 1: point on the left of current bank
			found = bank_case1(str, i, bank);
			break;
		} else if (bits > str->bands[i].val) {
			// case 2: point on the right of current bank
			found = bank_case2(str, i, bank);
			break;
		}
	}
	if (i == str->nbands) {
		// case 3: point inside current bank
		hi = band_low(str, i-1) - 1;
		clear_bits(bank, hi, 0);
		found = 1;
	}
	return found;
}



// To decide whether a range overlaps with a space bank
int range_overlap_bank(Range *range, TString *str)
{
	Range	bank;

	if (!find_bank(range->lo, str, &(bank.lo)))
		return 0;
printf("range: [%x, %x], bank.lo: %x\n", range->lo, range->hi, bank.lo);
printf("1\n");
	if (range->hi >= bank.lo)
		return 1;
	return 0;
}
