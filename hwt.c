#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include "hwt.h"

typedef uint8_t byte;


void *xmalloc(size_t amount)
{
	void *ptr = malloc(amount);
	if (!ptr)
	{
		printf("Ran out of memory in xmalloc()\n");
		exit(EXIT_FAILURE);
	}
	return ptr;
}



void do_hwt(int *array, int size)
{
	int 	avg[BAND_SIZE], diff[BAND_SIZE];
	int 	i, count;

	for (count = size; count > 1; count >>= 1) {
		for (i = 0; i < (count >> 1); i++) {
			avg[i] = (array[2*i] + array[2*i + 1]) >> 1;
			diff[i] = array[2*i] - avg[i];
		}
		for (i = 0; i < (count >> 1); i++) {
			array[i] = avg[i];
			array[i + (count >> 1)] = diff[i];
		}
	}
}



void undo_hwt(int *array, int size)
{
	int 	tmp[BAND_SIZE];
	int 	i, count;

	for (count = 2; count <= size; count <<= 1) {
		for (i = 0; i < (count >> 1); i++) {
			tmp[2*i]     = array[i] + array[i + count/2];
			tmp[2*i + 1] = array[i] - array[i + count/2];
		}

		for (i = 0; i < count; i++) array[i] = tmp[i];
	}
}



/*
int
main(int argc, char **argv)
{
	int		i, size, num = 0, logi = 0;

	if (argc < 4)
		exit(1);

	size = atoi(argv[1]);

	for (i = 2; i < argc; i += 2) {
		s[atoi(argv[i])] = atoi(argv[i+1]);
		num += atoi(argv[i+1]);
	}

	undo_hwt(s, size);

	for (i = 0; i < size; i++)
		printf("%.0f, ", s[i]);
	printf("\n");
}
*/


int	weight[BAND_SIZE] = {64, 48, 32, 32, 24, 24, 24, 24, 16, 16, 16, 16, 16, 16, 16, 16};
#define	EVEN	4

#define MAX_EVEN	((BAND_SIZE >> 1) + 1)

// lower triange meaningless, as bad eveness means lots of small rules and low duplicate
int		fit_table[BAND_SIZE+1][MAX_EVEN] = {
	{  0,   0,   0,   0,   0,   0,   0,   0,   0},
	{  1,   2,   6,  11,  20,  49,  81, 102, 103},		// dup = 1
	{  3,   4,   8,  12,  21,  50,  82, 104, 114},
	{  5,   7,   9,  15,  23,  52,  83, 105, 128},
	{ 10,  13,  14,  22,  25,  53,  84, 106, 128},
	{ 16,  17,  18,  24,  30,  55,  89, 109, 128},
	{ 26,  27,  28,  29,  39,  56,  90, 110, 128},
	{ 31,  32,  33,  34,  44,  57,  91, 128, 128},
	{ 35,  36,  37,  38,  45,  59,  92, 128, 128},		// dup = 8
	{ 40,  41,  42,  43,  54,  60,  95, 128, 128},
	{ 46,  47,  48,  51,  58,  70,  97, 128, 128},
	{ 61,  62,  63,  68,  69,  73,  98, 128, 128},
	{ 65,  66,  67,  71,  72,  74,  99, 128, 128},
	{ 75,  76,  77,  78,  79,  80, 100, 128, 128},
	{ 85,  86,  87,  93,  94,  96, 101, 128, 128},
	{107, 108, 109, 110, 111, 112, 114, 128, 128},
	{128, 128, 128, 128, 128, 128, 128, 128, 128}
};

typedef struct {
	int		band;
	int		fit;
} Fitness;

Fitness			fitness[NFIELDS];
int				hwt[BAND_SIZE];


// small value means more even, so the worst-case coeff represents the evenness
inline
int evenness(int nrules)
{
	int		i, even, e;

	even = abs(hwt[1]);
	for (i = 2; i < BAND_SIZE; i++) {
		e = abs(hwt[i]);
		if (e > even)
			even = e;
	}

	even = even*BAND_SIZE/nrules;
	return even;
}



// divide rules into groups, and perform hwt for evenness and duplicaiton judgement
// return the rule duplication ratio 
int hwt_rules(Trie *v, TBits *tb, int bid)
{
	int		val, i, total = 0, dup, even, max = 0;

	tb->nbands++;
	tb->bandmap[bid] = 1;

	for (val = 0; val < BAND_SIZE; val++) {
		hwt[val] = 0;
		set_tbits(tb, bid, val);
		for (i = 0; i < v->nrules; i++) {
			if (rule_collide(v->rules[i], tb))
				hwt[val]++;
		}
		total += hwt[val];
		if (hwt[val] > max)
			max = hwt[val];
	}
	tb->nbands--;
	tb->bandmap[bid] = 0;

	do_hwt(hwt, BAND_SIZE);
	
	dup = (total + (v->nrules >> 1) - 1)/v->nrules;
	even = (max <= LEAF_RULES) ? 0 : evenness(total); 

	return  fit_table[dup][even];
}



// to check whether the combination of even and dup gets improved over previous round
inline
int better_fit(int even, int dup, int even1, int dup1)
{
	return fit_table[dup1][even1] < fit_table[dup][even];
}



// advance to lower bands and remember the best band along the process
int advance_dim(Trie *v, TBits *tb, int dim, int locked_bid)
{
	int		d, e, fit = 10000, fit1;
	int		bid, cut_bid, max;

	bid = free_band(tb, field_bands[dim] - 1);
	if (bid == locked_bid)
		bid = free_band(tb, bid - 1);
	if (bid < 0)
		return -1;
	
	cut_bid = bid;
	while (bid >= 0) {
		fit1 = hwt_rules(v, tb, bid);
		if (fit1 < fit) {
			fit = fit1;
			cut_bid = bid;
		}
		bid = free_band(tb, bid - 1);
		if (bid == locked_bid)
			bid = free_band(tb, bid - 1);
	}

	fitness[dim].band = cut_bid;
	fitness[dim].fit = fit;

	return cut_bid;
}



int choose_dim(int *bids)
{
	int		fit = 10000, fit1, i, dim;

	for (i = 0; i < NFIELDS; i++) {
		if (bids[i] < 0)
			continue;
		fit1 = fitness[i].fit;
		if (fit1 < fit) {
			fit = fit1;
			dim = i;
		}
	}
	return dim;
}



void choose_bands(Trie *v, TBits *path_tb)
{
	int		bid[NFIELDS], dim, i;

	for (i = 0; i < NFIELDS; i++)
		bid[i] = advance_dim(v, &path_tb[i], i, -10);

	dim = choose_dim(bid);
	v->cut_bands[0].dim = dim;
	v->cut_bands[0].id = fitness[dim].band;

	bid[dim] = advance_dim(v, &path_tb[dim], dim, fitness[dim].band);
	dim = choose_dim(bid);
	v->cut_bands[1].dim = dim;
	v->cut_bands[1].id = fitness[dim].band;
}
