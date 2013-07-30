#ifndef TRIE_H
#define TRIE_H

#include "bitband.h"
#include "rule.h"

#define MAX_CHILDREN	256		// BAND_SIZE power2 CUT_BANDS
#define LEAF_RULES		4		// upper limit on rules in lefa nodes
#define	SMALL_NODE		16		// node is small with rules less than this
#define MAX_LAYER		6


enum { LEAF, NONLEAF };

typedef struct trie_t	Trie;

struct trie_t {
	uint8_t		layer;
	uint8_t		type;
	uint16_t	nequals;			// number of equal nodes pointing to me
	int			id;
	int			nrules;
	Rule**		rules;

	Trie*		parent;
	Band		bands[CUT_BANDS];	// cut bands from parent

	Band		cut_bands[CUT_BANDS];	// cuts bands for generating children (val meaningless)
	int			nchildren;
	Trie*		children;
	uint8_t		bandmap[MAX_CHILDREN];	// if nchildren < 256, 255 means empty child node
};


Trie* build_trie(Rule *rules, int nrules);
int redundant_rule(Rule *rule, TBits *path_tbits, Range *cover);

void dum_trie();
void dump_trie(Trie *root);
void dump_node(Trie *v, int simple);
void dump_node_rules(Trie *v);
void check_small_rules(int size);
void dump_nodes(int max, int min);
void dump_rules(Rule **rules, int nrules);


#endif
