#ifndef STATS_H
#define STATS_H

#include "trie.h"


typedef struct {
	int		num_nonleaf;
	int		num_layer[MAX_LAYER];
	int		num_layer_leaf[MAX_LAYER];
	int		nchildren[MAX_CHILDREN];
} Stats;

void collect_stats();
void dump_stats();

#endif
