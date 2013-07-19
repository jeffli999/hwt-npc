#include <stdlib.h>
#include <string.h>
#include "trie.h"

#define		QTHRESH		0x1000000


Trie	*trie_root;
int		total_nodes = 0;

Trie	**trie_nodes;
int		trie_nodes_size = 10240;

Trie**	queue;
int		qhead, qtail, qsize = 1024;

void dump_trie(Trie *root);


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



Trie* init_trie(Rule *rules, int nrules)
{
	Trie* 	node = calloc(1, sizeof(Trie));
	int		i;

	node->type = NONLEAF;
	node->id = total_nodes++;
	node->nrules = nrules;
	node->rules = malloc(nrules * sizeof(Rule *));
	for (i = 0; i < nrules; i++)
			node->rules[i] = &rules[i];

	trie_nodes = (Trie **) malloc( 10240 * sizeof(Trie *));
	trie_nodes[0] = node;
	
	return node;
}



void init_path_tbits(TBits *path_tbits)
{
	int		i, j;

	// initialize
	for (i = 0; i < NFIELDS; i++) {
		path_tbits[i].dim = i;
		path_tbits[i].nbands = 0;
		for (j = 0; j < MAX_BANDS; j++)
			path_tbits[i].bandmap[j] = 0;
	}
}



// formulate the ternary bit strings on all fields from the root to v's parent
void form_path_tbits(Trie* v, TBits* path_tbits)
{
	int			i;
	TBits		*p;

	init_path_tbits(path_tbits);

	while (v->id != 0) {
		// FIXME: only applicable to CUT_BANDS = 2
		i = v->bands[0].id;
		p = &path_tbits[v->bands[0].dim];
		p->nbands++;
		p->bandmap[i] = 1;
		set_bits(&p->val, band_high(i), band_low(i), v->bands[0].val);

		i = v->bands[1].id;
		p = &path_tbits[v->bands[1].dim];
		p->nbands++;
		p->bandmap[i] = 1;
		set_bits(&p->val, band_high(i), band_low(i), v->bands[1].val);

		v = v->parent;
	}
}



// TODO: The ultimate heuristic is HWT, but now use a simple one to test the other part
void choose_bands(Trie *v, TBits *path_tbits)
{
	Band		layers[4][2];
	int			i;

	layers[0][0].dim = 0; layers[0][0].id = 7;
	layers[0][1].dim = 0; layers[0][1].id = 6;
		
	layers[1][0].dim = 1; layers[1][0].id = 7;
	layers[1][1].dim = 1; layers[1][1].id = 6;
		
	layers[2][0].dim = 0; layers[2][0].id = 5;
	layers[2][1].dim = 1; layers[2][1].id = 5;
		
	layers[3][0].dim = 3; layers[3][0].id = 3;
	layers[3][1].dim = 3; layers[3][1].id = 2;

/*
	layers[0][0].dim = 0; layers[0][0].id = 0;
	layers[0][1].dim = 0; layers[0][1].id = 2;
		
	layers[1][0].dim = 1; layers[1][0].id = 1;
	layers[1][1].dim = 0; layers[1][1].id = 1;
		
	layers[2][0].dim = 0; layers[2][0].id = 5;
	layers[2][1].dim = 1; layers[2][1].id = 0;
		
	layers[3][0].dim = 2; layers[3][0].id = 0;
	layers[3][1].dim = 2; layers[3][1].id = 1;
*/

	i = v->layer;
	v->cut_bands[0].dim = layers[i][0].dim;
	v->cut_bands[0].id = layers[i][0].id;
	v->cut_bands[1].dim = layers[i][1].dim;
	v->cut_bands[1].id = layers[i][1].id;
}



// identify rules that overlap with tb1 and tb2, and create a child if rules found
void new_child(Trie *v, TBits *tb0, TBits *tb1, uint32_t val0, uint32_t val1)
{
	Trie		*u;
	Rule		*rule, **ruleset;
	Range		*f;
	int			nrules = 0, i;
	Band		*b0, *b1;
//printf("new_child[%d]@%d\n", total_nodes, v->layer+1);
	b0 = &(v->cut_bands[0]);
	b1 = &(v->cut_bands[1]);

	set_bits(&(tb0->val), band_high(b0->id), band_low(b0->id), val0);
	set_bits(&(tb1->val), band_high(b1->id), band_low(b1->id), val1);

	u = &v->children[v->nchildren];
	ruleset = (Rule **) malloc(v->nrules * sizeof(Rule *));

	for (i = 0; i < v->nrules; i++) {
		rule = v->rules[i];
		f = &(rule->field[tb0->dim]);
		if(!range_overlap_tbits(f, tb0))
			continue;
		f = &(rule->field[tb1->dim]);
		if(!range_overlap_tbits(f, tb1))
			continue;

		ruleset[nrules++] = rule;
	}
	if (nrules == 0) {
		free(ruleset);
		return;
	}
	if (nrules < v->nrules)
		ruleset = realloc(ruleset, nrules * sizeof(Rule *));

	u->layer = v->layer + 1;
	u->id = total_nodes++;
	u->parent = v;
	u->bands[0].dim = b0->dim; u->bands[0].id = b0->id; u->bands[0].val = val0;
	u->bands[1].dim = b1->dim; u->bands[1].id = b1->id; u->bands[1].val = val1;
	u->rules = ruleset;
	u->nrules = nrules;
	u->type = nrules > LEAF_RULES ? NONLEAF : LEAF;

	v->nchildren++;

	if (total_nodes >= trie_nodes_size) {
		trie_nodes_size += 10240;
		trie_nodes = realloc(trie_nodes, (trie_nodes_size + 10240) * sizeof(Trie *));
	}
	trie_nodes[total_nodes-1] = u;
}



// cut the space into banks using the best bit bands
void create_children(Trie* v)
{
	static TBits	path_tbits[NFIELDS];
	static Trie*	parent = NULL;

	Band			*band0, *band1;
	TBits			*tb0, *tb1;
	uint32_t		val0, val1, id, dim;


	if (v->id == 0) {
		init_path_tbits(path_tbits);
	} else if (v->parent != parent) {
		parent = v->parent;
		form_path_tbits(v, path_tbits);
	} else {
		dim = v->bands[0].dim;
		id = v->bands[0].id;
		val0 = v->bands[0].val;
		set_bits(&(path_tbits[dim].val), band_high(id), band_low(id), val0);
		dim = v->bands[1].dim;
		id = v->bands[1].id;
		val0 = v->bands[1].val;
		set_bits(&(path_tbits[dim].val), band_high(id), band_low(id), val0);
	}

	choose_bands(v, path_tbits);

	// update ternary bits with v's info to generate children
	// FIXME: only applicable to CUT_BANDS=2
	band0 = &(v->cut_bands[0]);
	tb0 = &(path_tbits[band0->dim]);
	tb0->bandmap[band0->id] = 1;
	tb0->nbands++;
	band1 = &(v->cut_bands[1]);
	tb1 = &(path_tbits[band1->dim]);
	tb1->bandmap[band1->id] = 1;
	tb1->nbands++;

	v->children = (Trie *) calloc(MAX_CHILDREN, sizeof(Trie));

	// assign each potential child its cut band values, then try to generate the child
	for (val0 = 0; val0 < (1 << BAND_SIZE); val0++) {
		for (val1 = 0; val1 < (1 << BAND_SIZE); val1++) {
			new_child(v, tb0, tb1, val0, val1);
		}
	}

	v->children = (Trie *) realloc(v->children, v->nchildren * sizeof(Trie));

	// recover path_tbits to the state before v's band cut
	tb0->bandmap[band0->id] = 0;
	tb0->nbands--;
	tb1->bandmap[band1->id] = 0;
	tb1->nbands--;
}



Trie* build_trie(Rule *rules, int nrules)
{
	Trie*	v;
	int		i;

	init_queue();
	trie_root = init_trie(rules, nrules);
	enqueue(trie_root);

	while (!queue_empty()) {
		v = dequeue();
		create_children(v);
		for (i = 0; i < v->nchildren; i++) {
			//if (v->children[i]->type == NONLEAF)
			if ((v->children[i].type == NONLEAF) && (v->children[i].layer <= 3))
				enqueue(&(v->children[i]));
		}
	}
	printf("Trie nodes: %d\n", total_nodes);
	dump_trie(trie_root);
}



void dump_node(Trie *v, int simple)
{
	int			i;

	if (v->parent != NULL)
		printf("Node[%d<-%d]@%d ", v->id, v->parent->id, v->layer);
	else
		printf("Node[%d]@%d ", v->id, v->layer);

	if (v->id > 0) {
		printf("d%d-b%d-v%x | ", v->bands[0].dim, v->bands[0].id, v->bands[0].val);
		printf("d%d-b%d-v%x ", v->bands[1].dim, v->bands[1].id, v->bands[1].val);
	}
if (v->nrules > 4)
	printf("\n*Rules: ");
else
	printf("\nRules: ");
	if (simple) {
		printf("%d", v->nrules);
	} else {
		for (i = 0; i < v->nrules; i++)
			printf("%d, ", v->rules[i]->id);
	}

	printf("\nNodes: ");
	if (simple) {
		printf("%d", v->nchildren);
	} else {
		for (i = 0; i < v->nchildren; i++)
			printf("%d, ", v->children[i].id);
	}
	printf("\n\n");
}



void dump_trie(Trie *root)
{
	Trie	*v;
	int		i;

	qhead = qtail = 0;

	enqueue(root);
	while (!queue_empty()) {
		v = dequeue();
		if (v->type == LEAF)
			continue;

		dump_node(v, 1);
		for (i = 0; i < v->nchildren; i++)
			enqueue(&(v->children[i]));
	}
}



void dump_node_rules(Trie *v)
{
	int			i;

	for (i = 0; i < v->nrules; i++) {
		printf("[%3d] ", v->rules[i]->id);
		dump_rule(v->rules[i]);
	}
}
