#include <stdio.h>
#include "trie.h"
#include "stats.h"


extern Trie **trie_nodes, *trie_root;
extern int	total_nodes;


Stats	stats;

void node_dist()
{
	int		i;
	Trie	*v;

	for (i = 0; i < total_nodes; i++) {
		v = trie_nodes[i];
		stats.num_layer[v->layer]++;
		if (v->type == NONLEAF) {
			stats.nchildren[v->nchildren]++;
			stats.num_nonleaf++;
		} else {
			stats.num_layer_leaf[v->layer]++;
		}
	}
}



void collect_stats()
{
	node_dist();
}



void dump_stats()
{
	int		i, n;
	Trie	*v;

	printf("\nNONLEAF nodes: %d\n", stats.num_nonleaf);
	for (i = 0; i < MAX_CHILDREN; i++) {
		n = stats.nchildren[i];
		if (n > 0)
			printf("CH[%d]: %d (%.2f%)\n", i, n, (float)(n*100)/stats.num_nonleaf);
	}

	printf("\nLayer distribution:\n");
	for (i = 0; i < MAX_LAYER; i++) {
		n = stats.num_layer[i];
		if (n > 0)
			printf("Layer[%d]: %d/%d\n", i, n, stats.num_layer_leaf[i]);
	}
}
