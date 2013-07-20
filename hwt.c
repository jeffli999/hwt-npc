#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include "hwt.h"

typedef uint8_t byte;

typedef struct {
	uint8_t		dim;
	uint8_t		band;
	uint8_t		dom;		// dominance - weighted ratio over hwt average (hwt[0])
	uint16_t	dup;		// rule duplication ratio
} Fitness;


int	weight[BAND_SIZE] = {64, 48, 32, 32, 24, 24, 24, 24, 16, 16, 16, 16, 16, 16, 16, 16};
#define	GOOD_FIT	4


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
/*
	for (i = 0; i < BAND_SIZE; i++)
		printf("%d ", array[i]);
	printf("\n");
*/
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



void rule_distrib(Trie *v, TBits *tb, int bid, int *hwt)
{
	int		val, i, total = 0;

	tb->nbands++;
	tb->bandmap[bid] = 1;

	for (val = 0; val < BAND_SIZE; val++) {
		hwt[val] = 0;
		set_tbits(tb, bid, val);
		for (i = 0; i < v->nrules; i++) {
			if (rule_collide(v->rules[i], tb))
				hwt[val]++;
		}
	}

	tb->nbands--;
	tb->bandmap[bid] = 0;
/*
	for (i = 0; i < BAND_SIZE; i++)
		printf("%d ", hwt[i]);
	printf("\n");
*/
}



int check_fitness(Trie *v, TBits *path_tbits, Fitness *fit, Fitness *fit1, int bid)
{
	int		hwt[BAND_SIZE];
	int		i, dom = fit->dom, dom1, dup;

	rule_distrib(v, path_tbits, bid, hwt);
	do_hwt(hwt, BAND_SIZE);

	// special handing for hwt[0], care about its duplication
	dup = (hwt[0]*BAND_SIZE*100) / v->nrules;
	i = dup < 240 ? 0 : 1;

	for (; i < BAND_SIZE; i++) {
		dom1 = abs(hwt[i])*weight[i]/v->nrules;
		if (dom1 > dom)
			dom = dom1;
	}

	if ((dom != fit->dom) || (fit->dom == 0)) {
		*fit1 = *fit;
		fit->band = bid;
		fit->dom = dom;
		fit->dup = (hwt[0]*BAND_SIZE*100) / v->nrules;
	}
}



// TODO: now only consider fitness, duplication should be accounted latr
inline
int good_fitness(Fitness *fit)
{
	return (fit->dom < GOOD_FIT);
}



int best_dim_band(Fitness *fits)
{
	int i, dim = 0;

	for (i = 1; i < NFIELDS; i++) {
		if (fits[i].dup < fits[dim].dup)
			dim = i;
	}

	for (i = 1; i < NFIELDS; i++) {
		if (fits[i].dom < fits[dim].dom) {
			if (fits[i].dup <= fits[dim].dup * 1.2)
				dim = i;
		}
	}
	return dim;
}



void choose_bands(Trie *v, TBits *path_tbits)
{
	int			hwt[BAND_SIZE], bid[NFIELDS];
	int			good[NFIELDS], num_good = 0, dim, i;
	Fitness		fits[NFIELDS], fits1[NFIELDS];

	for (i = 0; i < NFIELDS; i++) {
		fits[i].dim = fits1[i].dim = i;
		fits[i].dom = fits1[i].dom = 0;
		bid[i] = free_band(&path_tbits[i], field_bands[i] - 1);
		if (bid[i] < 0) {
			good[i] = 1;
			num_good++;
			fits[i].dom = 0xff;
		} else {
			good[i] = 0;
		}
	}

	while (num_good < NFIELDS) {
		for (i = 0; i < NFIELDS; i++) {
			if (good[i])
				continue;
			check_fitness(v, &path_tbits[i], &fits[i], &fits1[i], bid[i]);
			bid[i] = free_band(&path_tbits[i], bid[i] - 1);
			if (good_fitness(&fits[i]) || (bid[i] < 0)) {
				good[i] = 1;
				num_good++;
			}
		}
	}

	dim = best_dim_band(fits);
	v->cut_bands[0].dim = dim;
	v->cut_bands[0].id = fits[dim].band;
printf("dup=%d\n", fits[dim].dup);

	if (fits1[dim].dom != 0) {
		fits[dim] = fits1[dim];
	} else {
		fits[dim].dom = 0;
		fits[dim].dup = BAND_SIZE*100;
	}

	bid[dim] = free_band(&path_tbits[dim], bid[dim] - 1);
	while (bid[dim] >= 0) {
		check_fitness(v, &path_tbits[dim], &fits[dim], &fits1[dim], bid[dim]);
		bid[dim] = free_band(&path_tbits[dim], bid[dim] - 1);
	}
	dim = best_dim_band(fits);
	v->cut_bands[1].dim = dim;
	v->cut_bands[1].id = fits[dim].band;
printf("dup=%d\n", fits[dim].dup);
printf("choose_bands[%d]: dim0=%d, band0=%d\n", v->id, v->cut_bands[0].dim, v->cut_bands[0].id);
printf("                  dim1=%d, band1=%d\n", v->cut_bands[1].dim, v->cut_bands[1].id);

}
