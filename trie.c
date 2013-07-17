#include "trie.h"
#include "stdlib.h"
#include "string.h"

#define		QTHRESH		0x1000000


Trie*	trie_root;
Trie**	queue;
int		qhead, qtail, qsize = 1024;



void init_queue()
{
	queue = calloc(qsize, sizeof(Trie *));
}



void grow_queue()
{
	if (qsize > QTHRESH)
		qsize += QTHRESH;
	else
		qsize <<=  1;
	queue = (Trie **) realloc(queue, qsize * sizeof(Trie *));
}



inline
int queue_empty()
{
	return (qhead == qtail);
}



inline
int queue_full()
{
	// for simplicity, everytime qhead reaches qsize, it is deemed full (only doubles mem usage)
	return (qhead == (qsize - 1));
}


inline
void enqueue(Trie* node)
{
	if (queue_full())
		grow_queue();
	queue[qhead++] = node;
}



inline
Trie* dequeue()
{
	return queue[qtail++];
}



Trie* init_trie(Rule **rules, int nrules)
{
	Trie* 	node = calloc(1, sizeof(Trie));

	node->type = NONLEAF;
	node->nrules = nrules;
	node->rules = malloc(nrules * sizeof(Rule *));
	memcpy(node->rules, rules, nrules * sizeof(Rule *));
	
	return node;
}



void form_path_tbits(Trie* v, TBits* path_tbits)
{

}



// cut the space into banks using the best bit bands
void band_cut(Trie* v)
{
	static TBits	path_tbits[NFIELDS];
	static Trie*	v_parent = NULL;

	Band			*band1, *band2;
	TBits			*tb1, *tb2;
	uint32_t		val1, val2;

	if (v->parent != v_parent) {
		v_parent = v->parent;
		form_path_tbits(v_parent, path_tbits);
	}

	choose_bands(v);

	// FIXME: not generalized to arbitrary CUT_BANDS (CUT_BANDS = 2 only)
	band1 = &(v->cut_bands[0]);
	tb1 = &(path_tbits[band1->dim]);
	tb1->bandmap[band1->id] = 1;
	tb1->nbands++;
	band2 = &(v->cut_bands[1]);
	tb2 = &(path_tbits[band2->dim]);
	tb2->bandmap[band2->id] = 1;
	tb2->nbands++;
	for (val1 = 0; val1 < (1 << BAND_SIZE); val1++) {
		set_bits(&(tb1->val), band_high(band1->id), band_low(band1->id), val1);
		for (val2 = 0; val2 < (1 << BAND_SIZE); val2++) {
			set_bits(&(tb2->val), band_high(band2->id), band_low(band2->id), val2);
		}
	}
	
	for (i = 0; i < MAX_CHILDREN; i++) {
		
	}

}



Trie* build_trie(Rule **rules, int nrules)
{
	Trie*	v;
	int		i;

	init_queue();
	trie_root = init_trie(rules, nrules);
	enqueue(trie_root);

	while (!queue_empty()) {
		v = dequeue();
		band_cut(v);
		for (i = 0; i < v->nchildren; i++) {
			if (v->children[i]->type == NONLEAF)
				enqueue(v->children[i]);
		}
	}
}
