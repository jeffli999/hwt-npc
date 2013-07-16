#include <stdio.h>
#include "common.h"
#include "bitband.h"


void
test_range()
{
	Range	a, b;

	a.lo = 1; a.hi = 5;
	b.lo = 5; b.hi = 10;

	if (range_overlap(a, b))
		printf("overlapped\n");
	else
		printf("not overlapped\n");
}


void test_band()
{
	TString		str;
	uint32_t	point = 0x1ca8;
//	uint32_t	point = 0x1c88;
//	uint32_t	point = 0x12f4ff78;
//	uint32_t	point = 0x12f4fff8;
//	uint32_t	point = 0x1234eff8;
//	uint32_t	point = 0x1fb8;
	uint32_t	bank, found;

	str.bands[0].dim = 2; str.bands[0].seq = 3, str.bands[0].val = 0x1;
	str.bands[1].dim = 2; str.bands[1].seq = 1, str.bands[1].val = 0xa;

	str.nbands = 2; str.len = 16;

//	str.bands[0].dim = 2; str.bands[0].seq = 7, str.bands[0].val = 0x1;
//	str.bands[1].dim = 2; str.bands[1].seq = 4, str.bands[1].val = 0x4;
//	str.bands[2].dim = 2; str.bands[2].seq = 1, str.bands[2].val = 0x7;

	found = find_bank(point, &str, &bank);
	if (found)
		printf("Found: %x\n", bank);
	else
		printf("Not found: %x\n", bank);
}



int main(int argc, char **argv)
{
	test_band();
}
