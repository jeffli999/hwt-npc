#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "bitband.h"
#include "rule.h"


void test_range()
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

	range.lo = 0x1f10;
	range.hi = 0x1f20;

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

	tbits.dim = 2; tbits.nbands = 1; 
	tbits.bandmap[1] = 1;
	tbits.val = 0x0000;

	result = range_overlap_tbits(&range, &tbits);

	if (result == 1)
		printf("Overlap\n");
	else
		printf("Not Overlap\n");
}


FILE		*fp = NULL;
Rule		*ruleset = NULL;
int			num_rules = 0;

int main(int argc, char **argv)
{

	fp = fopen(argv[1], "r");

	if (fp == NULL) {
		fprintf(stderr, "Failed to open file\n");
		exit(0);
	}

	num_rules = loadrules(fp, &ruleset);
	dump_ruleset(ruleset, num_rules);
	build_trie(ruleset, num_rules);

	fclose(fp);

	//test_band();
}
