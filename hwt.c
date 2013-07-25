#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include "hwt.h"
#include "trie.h"

typedef uint8_t byte;


int		max_fit = 0;


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


extern Range	**rule_covers;

#define MAX_EVEN	(BAND_SIZE + 1)

// lower triange meaningless, as bad eveness means lots of small rules and low duplicate
int		fit_table[(BAND_SIZE << 1) + 1][MAX_EVEN] = {
{  0,   0,   0,   0,   0,   0,   0,   0,   0, 255, 255, 255, 255, 255, 255, 255, 255}, // dup=0
{  1,   5,  13,  23,  29,  45,  55,  66,  84, 106, 107, 122, 124, 125, 126, 127, 255},    
{  2,   6,  14,  24,  30,  46,  56,  67,  85, 121, 122, 123, 150, 151, 255, 255, 255},
{  3,  11,  19,  33,  43,  51,  63,  91, 104, 146, 148, 152, 153, 255, 255, 255, 255},
{  4,  12,  20,  34,  44,  52,  64,  92, 105, 147, 149, 255, 255, 255, 255, 255, 255}, // dup=4
{  7,  17,  27,  35,  49,  59,  70, 108, 112, 154, 155, 255, 255, 255, 255, 255, 255},
{  8,  18,  28,  36,  50,  60,  71, 109, 113, 156, 255, 255, 255, 255, 255, 255, 255},
{  9,  21,  31,  41,  57,  68,  82, 110, 132, 157, 255, 255, 255, 255, 255, 255, 255},
{ 10,  22,  32,  42,  58,  69,  83, 111, 133, 255, 255, 255, 255, 255, 255, 255, 255}, // dup=8
{ 15,  25,  39,  53,  61,  72,  93, 114, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{ 16,  26,  40,  54,  62,  73,  94, 115, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{ 37,  47,  65,  74,  82,  89, 136, 144, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{ 38,  48,  66,  75,  83,  90, 137, 145, 255, 255, 255, 255, 255, 255, 255, 255, 255}, // dup=12
{ 76,  78,  80,  95, 119, 134, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{ 77,  79,  81,  96, 120, 135, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{ 86,  98, 100, 103, 116, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{ 87,  99, 101, 103, 117, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}, // dup=16
{118, 128, 142, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{119, 129, 143, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{130, 140, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{131, 141, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}, // dup=20
{138, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{139, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}, // dup=24
{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}, // dup=28
{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}  // dup=32
//even                
//0                   4                   8                   12                  16
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

	even = (even*(BAND_SIZE << 1) + (nrules >> 1) - 1)/nrules;
	return even;
}



// get the fitness of a cut
int bcut_fit(Trie *v, TBits *path_tb, int dim, int bid)
{
	Rule	*hwt_rules[64], *rule;
	Range	*cover;
	int		val, total = 0, dup, even, max = 0, i, j;
static int	counter = 0;

	// change path_tb for computing the fit of a band cut
	// and recover it afer done (no return is allowed in between)
	path_tb[dim].nbands++;
	path_tb[dim].bandmap[bid] = 1;

	for (val = 0; val < BAND_SIZE; val++) {
		hwt[val] = 0;
		set_tbits(&path_tb[dim], bid, val);
		for (i = 0; i < v->nrules; i++) {
			rule = v->rules[i];
			if (!rule_collide(rule, &path_tb[dim]))
				continue;
			if (hwt[val] >= 64) {
				hwt[val]++;
				continue;
			}

			// check rule redundancy only if < 64 previous rules (to reduce computation, so it is
			// not an exhaustive redundancy removal, but does not affect correctness by including
			// some redundant rules, and this tradefoff is expected to work pretty good in practice
			for (j = 0; j < hwt[val]; j++) {
				cover = rule_covers[hwt_rules[j]->id];
				if (redundant_rule(rule, path_tb, cover))
					break;
			}
			if (j == hwt[val]) {
				// not redundant, record this rule and its cover for checking by later rules,
				// and increase hwt[val] to count this rule in
				hwt_rules[hwt[val]] = rule;
				cover = rule_covers[rule->id];
				rule_cover(rule->field, path_tb, cover);
				hwt[val]++;
			}
		}
		total += hwt[val];
		if (hwt[val] > max)
			max = hwt[val];
	}

	// recover path_tb, which wil be used again in future
	path_tb[dim].nbands--;
	path_tb[dim].bandmap[bid] = 0;

	do_hwt(hwt, BAND_SIZE);
	
	dup = ((total << 1) + (v->nrules >> 1) - 1)/v->nrules;
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
int advance_dim(Trie *v, TBits *path_tb, int dim, int locked_bid)
{
	int		d, e, fit = 10000, fit1;
	int		bid, cut_bid, max;

	bid = free_band(&path_tb[dim], field_bands[dim] - 1);
	if (bid == locked_bid)
		bid = free_band(&path_tb[dim], bid - 1);
	if (bid < 0)
		return -1;
	
	cut_bid = bid;
	while (bid >= 0) {
		fit1 = bcut_fit(v, path_tb, dim, bid);
		if (fit1 < fit) {
			fit = fit1;
			cut_bid = bid;
		}
		bid = free_band(&path_tb[dim], bid - 1);
		if (bid == locked_bid)
			bid = free_band(&path_tb[dim], bid - 1);
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
if (fit > max_fit)
	max_fit = fit;
	return dim;
}



void choose_bands(Trie *v, TBits *path_tb)
{
	int		bid[NFIELDS], dim, i;

	for (i = 0; i < NFIELDS; i++)
		bid[i] = advance_dim(v, path_tb, i, -10);

	dim = choose_dim(bid);
	v->cut_bands[0].dim = dim;
	v->cut_bands[0].id = fitness[dim].band;

	bid[dim] = advance_dim(v, path_tb, dim, fitness[dim].band);
	dim = choose_dim(bid);
	v->cut_bands[1].dim = dim;
	v->cut_bands[1].id = fitness[dim].band;

	add_tbits_band(path_tb, &v->cut_bands[0]);
	add_tbits_band(path_tb, &v->cut_bands[1]);
}
