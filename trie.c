#include <stdlib.h>
#include <string.h>
#include "trie.h"

#define		QTHRESH		0x1000000


Trie	*trie_root;
int		total_nodes = 0, unique_nodes = 0;
Trie	**trie_nodes;
int		trie_nodes_size = 10240;
Trie	**queue;
int		qhead, qtail, qsize = 102400;

#define	TMP_NRULES		64
Rule	**tmp_rulesets[TMP_NRULES];


void dump_trie(Trie *root);
void dump_node(Trie *v, int simple);
void dump_node_rules(Trie *v);


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

	trie_nodes = (Trie **) malloc(trie_nodes_size*sizeof(Trie *));
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
		set_bits(&p->val, band_msb(i), band_lsb(i), v->bands[0].val);

		i = v->bands[1].id;
		p = &path_tbits[v->bands[1].dim];
		p->nbands++;
		p->bandmap[i] = 1;
		set_bits(&p->val, band_msb(i), band_lsb(i), v->bands[1].val);

		v = v->parent;
	}
}



inline
void set_tbits(TBits *tb, uint32_t band_id, uint32_t val)
{
	set_bits(&tb->val, band_msb(band_id), band_lsb(band_id), val);
}



inline
int rule_collide(Rule *rule, TBits *tb)
{
	return range_tbits_overlap(&rule->field[tb->dim], tb);
}



// try to find a ruleset in current children with limited effort 
// (not an exhaustive search, only the recent set with the same number of rules is checked)
int find_ruleset(Rule **ruleset, int nrules, int dim0, int dim1)
{
	int			found;

	if (tmp_rulesets[nrules-1] == NULL)
		found = 0;
	else if (dim0 < 2 && dim1 < 2)	// IP prefix has special inclusion property
		found = memcmp(ruleset, tmp_rulesets[nrules-1], nrules*sizeof(Rule *)) == 0 ? 1 : 0;
	else	// TODO: for range, any method to identify reduandant child?
		found = 0;
	
	if (!found)
		tmp_rulesets[nrules-1] = ruleset;

	return found;
}



// identify rules that overlap with tb1 and tb2, and create a child if rules found
void new_child(Trie *v, TBits *tb0, TBits *tb1, uint32_t val0, uint32_t val1)
{
	Trie		*u;
	Rule		*rule, **ruleset;
	Range		*f;
	int			nrules = 0, found, i;
	Band		*b0, *b1;
	//printf("new_child[%d]@%d\n", total_nodes, v->layer+1);
	b0 = &v->cut_bands[0];
	b1 = &v->cut_bands[1];
	set_tbits(tb0, b0->id, val0);
	set_tbits(tb1, b1->id, val1);
	ruleset = (Rule **) malloc(v->nrules * sizeof(Rule *));

	for (i = 0; i < v->nrules; i++) {
		if (rule_collide(v->rules[i], tb0) && rule_collide(v->rules[i], tb1))
			ruleset[nrules++] = v->rules[i];
		//else
		//	printf("nocollide:v[%d], rule[%d]\n", v->id, i);
	}
	if (nrules == 0) {
		free(ruleset);
		return;
	}
	if (nrules <= TMP_NRULES)
		found = find_ruleset(ruleset, nrules, v->cut_bands[0].dim, v->cut_bands[1].dim);
	else
		found = 0;
	if (found) {
		free(ruleset);
		return;
	}
	if (nrules < v->nrules)
		ruleset = realloc(ruleset, nrules * sizeof(Rule *));
	u = &v->children[v->nchildren];
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
		trie_nodes = realloc(trie_nodes, trie_nodes_size*sizeof(Trie *));
	}
	trie_nodes[total_nodes-1] = u;
printf("node[%d]@%d: %d rules\n", u->id, u->layer, u->nrules);
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
		set_bits(&(path_tbits[dim].val), band_msb(id), band_lsb(id), val0);
		dim = v->bands[1].dim;
		id = v->bands[1].id;
		val0 = v->bands[1].val;
		set_bits(&(path_tbits[dim].val), band_msb(id), band_lsb(id), val0);
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
	memset(tmp_rulesets, 0, TMP_NRULES*sizeof(Rule **));

	// assign each potential child its cut band values, then try to generate the child
	for (val0 = 0; val0 < BAND_SIZE; val0++) {
		for (val1 = 0; val1 < BAND_SIZE; val1++) {
			new_child(v, tb0, tb1, val0, val1);
		}
	}

	v->children = (Trie *) realloc(v->children, v->nchildren*sizeof(Trie));

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
			if ((v->children[i].type == NONLEAF) && (v->layer < 3))
				enqueue(&(v->children[i]));
		}
		if (total_nodes > 1000)
			break;
	}
	printf("Trie nodes: %d\n", total_nodes);
	dump_trie(trie_root);
}



///////////////////////////////////////////////////////////////////////////////
//
// statistics/debug functions
//
///////////////////////////////////////////////////////////////////////////////


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
	printf("\tRules{%d}: ", v->nrules);
	if (simple) {
		printf("%d", v->nrules);
	} else {
		for (i = 0; i < v->nrules; i++)
			printf("%d, ", v->rules[i]->id);
	}

	printf("\nNodes{%d}: ", v->nchildren);
	if (simple) {
		printf("%d", v->nchildren);
	} else {
		for (i = 0; i < v->nchildren; i++)
			printf("%d, ", v->children[i].id);
	}
	printf("\n\n");
}



void small_large(int size)
{
	Trie	*v;
	int		i, n;

	qhead = qtail = 0;
	enqueue(trie_root);
	while (!queue_empty()) {
		v = dequeue();
		if ((v->nrules <= size) && (v->nchildren >= 16)) {
			n = 0;
			for (i = 0; i < v->nchildren; i++) {
				if (v->children[i].type != LEAF)
					n++;
				if (v->children[i].nrules > 4) {
					printf("==============================================d%db%d | d%db%d\n", 
							v->cut_bands[0].dim, v->cut_bands[0].id,
							v->cut_bands[1].dim, v->cut_bands[1].id);
					dump_node_rules(&v->children[i]);
				}
			}
			printf("vs[%d], rules: %d, children: %d/%d\n", v->id, v->nrules, v->nchildren, n);
		}
		
		for (i = 0; i < v->nchildren; i++) {
			if (v->children[i].type != LEAF)
				enqueue(&(v->children[i]));
		}
	}
}



void check_small_rules(int size)
{
	Trie	*v;
	int		i, np = 0, nc = 0;

	qhead = qtail = 0;
	enqueue(trie_root);
	while (!queue_empty()) {
		v = dequeue();
		if (v->nrules <= size) {
			np++;
			nc += v->nchildren + 1;
		}
		for (i = 0; i < v->nchildren; i++) {
			if (v->children[i].type != LEAF)
				enqueue(&(v->children[i]));
		}
	}
	printf("size[%d]: np=%d, nc=%d\n", size, np, nc);
}



void dump_trie(Trie *root)
{
	Trie	*v;
	int		i, n8 = 0, n16 = 0, sn8 = 0;

	qhead = qtail = 0;
	enqueue(root);
	while (!queue_empty()) {
		v = dequeue();
		if (v->nrules <= 8) {
			n8++;
			if (v->parent->nrules <= 16)
				sn8++;
		} else if (v->nrules <= 16)
			n16++;
		if (v->type == LEAF)
			continue;

		dump_node(v, 0);
		for (i = 0; i < v->nchildren; i++)
			enqueue(&(v->children[i]));
	}
	printf("n8: %d/%d, n16: %d, total: %d\n", sn8, n8, n16, total_nodes);
	check_small_rules(8);
	check_small_rules(16);
	small_large(16);
}


inline
void dump_rules(Rule **rules, int nrules)
{
	int			i;

	for (i = 0; i < nrules; i++) {
		printf("[%3d] ", rules[i]->id);
		dump_rule(rules[i]);
	}
}



void dump_node_rules(Trie *v)
{
	dump_rules(v->rules, v->nrules);
}



void dump_path(Trie *v, int simple)
{
	while (v != trie_root) {
		dump_node(v, simple);
		v = v->parent;
	}
}
