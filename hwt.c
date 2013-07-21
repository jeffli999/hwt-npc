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

typedef struct {
	int		band;
	int		even;		// evenness
	int		dup;		// rule duplication ratio
} Fitness;

Fitness			fitness[NFIELDS];
int				hwt[BAND_SIZE];


// small value means more even, so the worst-case coeff represents the evenness
inline
int evenness(int nrules)
{
	int		i, even, e;

	even = abs(hwt[1])*weight[1]/nrules;
	for (i = 2; i < BAND_SIZE; i++) {
		e = abs(hwt[i])*weight[i]/nrules;
		if (e > even)
			even = e;
	}
	return even;
}



// divide rules into groups, and perform hwt for evenness and duplicaiton judgement
// return the rule duplication ratio 
int hwt_rules(Trie *v, TBits *tb, int bid, int *max)
{
	int		val, i, total = 0;

	tb->nbands++;
	tb->bandmap[bid] = 1;

	*max = 0;
	for (val = 0; val < BAND_SIZE; val++) {
		hwt[val] = 0;
		set_tbits(tb, bid, val);
		for (i = 0; i < v->nrules; i++) {
			if (rule_collide(v->rules[i], tb))
				hwt[val]++;
		}
		total += hwt[val];
		if (hwt[val] > *max)
			*max = hwt[val];
	}
	tb->nbands--;
	tb->bandmap[bid] = 0;

	do_hwt(hwt, BAND_SIZE);
	
	return (total*100)/v->nrules;
}



// to check whether the combination of even and dup gets improved over previous round
inline
int cut_improved(int even, int dup, int even1, int dup1)
{
	return ((even1+1)*dup1 < (even+1)*dup);
}


#define		DUP_THRESH	400

// advance to lower bands and remember the best band along the process
int advance_dim(Trie *v, TBits *tb, int dim, int locked_bid)
{
	int		e, even = 255;				// 255 is an unreachable bad evenness
	int		d, dup = BAND_SIZE * 100;	// dup ratio initialized to an unreachable bad one
	int		bid, cut_bid, max;			// the selected band to cut

	bid = free_band(tb, field_bands[dim] - 1);
	if (bid == locked_bid)
		bid = free_band(tb, bid - 1);
	if (bid < 0)
		return -1;
	
	cut_bid = bid;
	while (bid >= 0) {
		d = hwt_rules(v, tb, bid, &max);
		e = (max <= LEAF_RULES) ? 0 :evenness(v->nrules);
		// combine evenness and duplicatio for cut fitness judgement
		if (cut_improved(even, dup, e, d)) {
			even = e; dup = d;
			cut_bid = bid;
		}
		bid = free_band(tb, bid - 1);
		if (bid == locked_bid)
			bid = free_band(tb, bid - 1);
	}

	fitness[dim].band = cut_bid;
	fitness[dim].even = even;
	fitness[dim].dup = dup;

	return cut_bid;
}



// smaller is better, no weight for even or dup here
int get_fitness(Fitness *fit)
{
	// sometimes even = 0, adding 1 to prevent picking high dup cut
	return (fit->even+1)*fit->dup;
}



int choose_dim(int *bids)
{
	int		fit = 10000, fit1, i, dim;

	for (i = 0; i < NFIELDS; i++) {
		if (bids[i] < 0)
			continue;
		fit1 = get_fitness(&fitness[i]);
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
