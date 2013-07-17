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

	TBits**			cut_tbits[CUT_BANDS];
	Band*			cut_bands[CUT_BANDS];

	if (v->parent != v_parent) {
		v_parent = v->parent;
		form_path_tbits(v_parent, path_tbits);
	}

	choose_bands(v);

	for (i = 0; i < CUT_BANDS; i++) {
		cut_bands[i].dim = v->cut_bands[i].dim;
		cut_bands[i].seq = v->cut_bands[i].seq;
		cut_bands[i].val = 0;
		cut_tbits[i] = add_band(path_tbits, cut_bands[i]);
	}

	for (i = 0; i < CUT_BANDS; i++) {
		for (val = 0; val < (1 << BAND_SIZE); val++) {
			set_tbits_val(cut_tbits[i], val);
		}
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
