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


TBits		tbits;

void test_band()
{
	Range		range;
	int			result;


	range.lo = 0x1200;
	range.hi = 0x1220;

/*
	tbits.dim = 2; tbits.nbands = 3; 
	tbits.bandmap[3] = tbits.bandmap[1] = tbits.bandmap[0] = 1;
	tbits.val = 0x1020;

	tbits.dim = 2; tbits.nbands = 3; 
	tbits.bandmap[3] = tbits.bandmap[1] = tbits.bandmap[0] = 1;
	tbits.val = 0x1030;

	tbits.dim = 2; tbits.nbands = 3; 
	tbits.bandmap[3] = tbits.bandmap[1] = tbits.bandmap[0] = 1;
	tbits.val = 0x1000;
*/

	tbits.dim = 2; tbits.nbands = 2; 
	tbits.bandmap[3] = tbits.bandmap[2] = 1;
	tbits.val = 0x1000;

	result = range_overlap_bank(&range, &tbits);

	if (result == 1)
		printf("Overlap\n");
	else
		printf("Not Overlap\n");
}



int main(int argc, char **argv)
{
	test_band();
}
