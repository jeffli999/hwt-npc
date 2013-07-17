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
	Range		range;
	TString		str;
	int			result;

/* 1 band, MSB
	range.lo = 0x0000;
	range.hi = 0x0000;
	str.nbands = 1; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x0;

	range.lo = 0x0300;
	range.hi = 0x03f0;
	str.nbands = 1; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x0;

	range.lo = 0x0000;
	range.hi = 0x0000;
	str.nbands = 1; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;

	range.lo = 0x0000;
	range.hi = 0x0a00;
	str.nbands = 1; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;

	range.lo = 0x0000;
	range.hi = 0x1000;
	str.nbands = 1; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;

	range.lo = 0x1000;
	range.hi = 0x1020;
	str.nbands = 1; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 1; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;
*/

/* 1 middle band
	range.lo = 0x0000;
	range.hi = 0x0000;
	str.nbands = 1; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 2; str.bands[0].val = 0x0;

	range.lo = 0x0300;
	range.hi = 0x03f0;
	str.nbands = 1; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 2; str.bands[0].val = 0x0;

	range.lo = 0x0000;
	range.hi = 0x0000;
	str.nbands = 1; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 2; str.bands[0].val = 0x1;

	range.lo = 0x0000;
	range.hi = 0x0a00;
	str.nbands = 1; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 2; str.bands[0].val = 0x1;

	range.lo = 0x0000;
	range.hi = 0x0100;
	str.nbands = 1; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 2; str.bands[0].val = 0x1;

	range.lo = 0x1000;
	range.hi = 0x1020;
	str.nbands = 1; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 2; str.bands[0].val = 0x1;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 1; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 2; str.bands[0].val = 0x1;
*/

/* 1 last band
	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 1; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 0; str.bands[0].val = 0x1;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 1; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 0; str.bands[0].val = 0x30;
*/

/* 2 consecutive bands
	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 2; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 1; str.bands[0].val = 0x0;
	str.bands[1].dim = 2; str.bands[1].seq = 0; str.bands[1].val = 0x3;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 2; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 1; str.bands[0].val = 0x3;
	str.bands[1].dim = 2; str.bands[1].seq = 0; str.bands[1].val = 0x0;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 2; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 2; str.bands[0].val = 0x1;
	str.bands[1].dim = 2; str.bands[1].seq = 1; str.bands[1].val = 0x0;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 2; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 2; str.bands[0].val = 0x2;
	str.bands[1].dim = 2; str.bands[1].seq = 1; str.bands[1].val = 0x0;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 2; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x2;
	str.bands[1].dim = 2; str.bands[1].seq = 2; str.bands[1].val = 0x0;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 2; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;
	str.bands[1].dim = 2; str.bands[1].seq = 2; str.bands[1].val = 0x0;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 2; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;
	str.bands[1].dim = 2; str.bands[1].seq = 2; str.bands[1].val = 0x2;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 2; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;
	str.bands[1].dim = 2; str.bands[1].seq = 2; str.bands[1].val = 0x3;
*/

/* 2 disjoint bands
	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 2; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;
	str.bands[1].dim = 2; str.bands[1].seq = 1; str.bands[1].val = 0x3;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 2; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;
	str.bands[1].dim = 2; str.bands[1].seq = 1; str.bands[1].val = 0x2;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 2; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;
	str.bands[1].dim = 2; str.bands[1].seq = 1; str.bands[1].val = 0x1;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 2; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;
	str.bands[1].dim = 2; str.bands[1].seq = 0; str.bands[1].val = 0x1;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 2; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;
	str.bands[1].dim = 2; str.bands[1].seq = 0; str.bands[1].val = 0xf;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 2; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 2; str.bands[0].val = 0x1;
	str.bands[1].dim = 2; str.bands[1].seq = 0; str.bands[1].val = 0xf;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 2; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 2; str.bands[0].val = 0x2;
	str.bands[1].dim = 2; str.bands[1].seq = 0; str.bands[1].val = 0xf;
*/

/* 3 bands
	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 3; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x2;
	str.bands[1].dim = 2; str.bands[1].seq = 2; str.bands[1].val = 0x2;
	str.bands[2].dim = 2; str.bands[2].seq = 1; str.bands[2].val = 0xf;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 3; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;
	str.bands[1].dim = 2; str.bands[1].seq = 2; str.bands[1].val = 0x2;
	str.bands[2].dim = 2; str.bands[2].seq = 1; str.bands[2].val = 0x1;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 3; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;
	str.bands[1].dim = 2; str.bands[1].seq = 2; str.bands[1].val = 0x2;
	str.bands[2].dim = 2; str.bands[2].seq = 0; str.bands[2].val = 0x1;

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 3; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;
	str.bands[1].dim = 2; str.bands[1].seq = 1; str.bands[1].val = 0x2;
	str.bands[2].dim = 2; str.bands[2].seq = 0; str.bands[2].val = 0x1;
*/

	range.lo = 0x1200;
	range.hi = 0x1220;
	str.nbands = 3; str.len = 16;
	str.bands[0].dim = 2; str.bands[0].seq = 3; str.bands[0].val = 0x1;
	str.bands[1].dim = 2; str.bands[1].seq = 1; str.bands[1].val = 0x2;
	str.bands[2].dim = 2; str.bands[2].seq = 0; str.bands[2].val = 0x0;

	result = range_overlap_bank(&range, &str);

	if (result == 1)
		printf("Overlap\n");
	else
		printf("Not Overlap\n");
}



int main(int argc, char **argv)
{
	test_band();
}
