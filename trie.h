#ifndef TRIE_H
#define TRIE_H

#include "bitband.h"
#include "rule.h"

#define MAX_CHILDREN	256		// BAND_SIZE * CUT_BANDS
#define LEAF_RULES		4		// upper limit on rules in lefa nodes


enum { LEAF, NONLEAF };

typedef struct trie_t	Trie;

struct trie_t {
	int		layer;
	int		id;
	int		type;
	Trie*	parent;
	Band	bands[CUT_BANDS];		// cut bands from parent

	int		nrules;
	Rule**	rules;

	Band	cut_bands[CUT_BANDS];	// cuts bands for generating children (no specific val field)
	int		nchildren;
	Trie**	children;
	uint8_t	band_map[MAX_CHILDREN];	// if nchildren < 256, 255 means empty child node
};


Trie* build_trie(Rule **rules, int nrules);


#endif
