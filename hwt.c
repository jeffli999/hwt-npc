#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include "hwt.h"

#define SIZE 64

typedef double val;
typedef uint8_t byte;

double	s[SIZE];



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



void do_hwt(val *array, int size)
{
	val avg[SIZE], diff[SIZE];
	int i, count;

	for (count=size; count>1; count/=2)
	{
		for (i=0; i<count/2; i++)
		{
			avg[i] = (array[2*i] + array[2*i + 1]) / 2;
			diff[i] = array[2*i] - avg[i];
		}

		for (i=0; i<count/2; i++)
		{
			array[i] = avg[i];
			array[i + count/2] = diff[i];
		}
	}
}



void undo_hwt(val *array, int size)
{
	val tmp[SIZE];
	int i, count;

	for (count=2; count<=size; count*=2)
	{
		for (i=0; i<count/2; i++)
		{
			tmp[2*i]     = array[i] + array[i + count/2];
			tmp[2*i + 1] = array[i] - array[i + count/2];
		}

		for (i=0; i<count; i++) array[i] = tmp[i];
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



void rule_distrib(Trie *v, TBits *tb, int bid)
{
	int		hwt_s[1 << BAND_SIZE];
	int		val, i, total = 0;


	tb->nbands++;
	tb->bandmap[bid] = 1;

	for (val = 0; val < (1 << BAND_SIZE); val++) {
		hwt_s[val] = 0;
		set_tbits(tb, bid, val);
		for (i = 0; i < v->nrules; i++) {
			if (rule_collide(v->rules[i], tb))
				hwt_s[val]++;
		}
	}

	tb->nbands--;
	tb->bandmap[bid] = 0;

	for (val = 0; val < (1 << BAND_SIZE); val++) {
		total += hwt_s[val];
		printf("%d ", hwt_s[val]);
	}
	printf("Ratio=%d\n", (total * 100 / v->nrules));
}



void choose_bands(Trie *v, TBits *path_tbits)
{
	int			bid[NFIELDS], i;

	for (i = 0; i < NFIELDS; i++)
		bid[i] = free_band(&path_tbits[i], field_bands[i] - 1);
}
