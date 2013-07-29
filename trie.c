#include <stdlib.h>
#include <string.h>
#include "trie.h"

#define		QTHRESH		0x1000000

extern int	max_fit;

Trie	*trie_root;
int		total_nodes = 0, unique_nodes = 0;
Trie	**trie_nodes;
int		trie_nodes_size = 10240;
Trie	**queue;
int		qhead, qtail, qsize = 102400;

Range	**rule_covers;



// ============================================================================
// section for queue related operations
// ============================================================================

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



// ============================================================================
// section for handling redundant trie nodes
// ============================================================================

#define REDT_NODES	20	// only check at most this number of nodes
#define	REDT_NRULES	64	// only check nodes with rules less than this

// three results for redundant node check:
// INVALID: same ruleset found, but not valid for redundancy due to port cut
enum {NOTFOUND, FOUND, INVALID};


typedef struct {
	int			head[REDT_NRULES], tail[REDT_NRULES];
	Trie		*nodeset[REDT_NRULES][REDT_NODES];
	Range		ports[2];		// source/dest port range in parent
} RedtNodes;

RedtNodes	redt_nodes;



void reset_redt_nodes(TBits *tb_set)
{
	int		i;

	for (i = 0; i < REDT_NRULES; i++)
		redt_nodes.head[i] = redt_nodes.tail[i] = 0;

	redt_nodes.ports[0] = tbits_range(&tb_set[1]);
	redt_nodes.ports[1] = tbits_range(&tb_set[3]);
}



// two situations to qulify a range for redundancy check
// 1. it is a prefix in its parent space
// 2. it  fully cover the space of this node
int port_redund(Rule **rules, int nrules, TBits *tbits, Trie *v)
{
	TBits	tb0 = *tbits, *tb1;
	Range	range, node_range;
	int		dim0, dim1, i;

	if (v->bands[0].dim == tbits->dim)
		set_tbits(&tb0, v->bands[0].id, v->bands[0].val);
	if (v->bands[1].dim == tbits->dim)
		set_tbits(&tb0, v->bands[1].id, v->bands[1].val);

	for (i = 0; i < nrules; i++) {
		range = rules[i]->field[tbits->dim];
		if (!equal_cuts(range, &tb0, tbits))
			return 0;	// meet neither of the valid conditions
	}
	return 1;
}



int find_node(Rule **ruleset, int nrules)
{
	Rule	**rs;
	int		idx = nrules-1, i;
	
	for (i = redt_nodes.tail[idx]; i != redt_nodes.head[idx]; i = (i+1) % REDT_NODES) {
		rs = redt_nodes.nodeset[idx][i]->rules;
		if (memcmp(ruleset, rs, nrules*sizeof(Rule *)) == 0)
			break;
	}
	return i;
}


int		d3all = 0, d3found = 0, d3valid;
// if node is not redundant, need to include it in the check set for checking following nodes
int check_node_redund(Rule **ruleset, int nrules, int dim0, int dim1, TBits *tb_set)
{
	int		idx = nrules-1, i;
if (dim0 == 3 || dim1 == 3)
d3all++;
	i = find_node(ruleset, nrules);
	if (i == redt_nodes.head[idx])
		return NOTFOUND;	// no identical set found
if (dim0 == 3 || dim1 == 3)
d3found++;
	if (nrules <= LEAF_RULES)
		return FOUND;
	// find an identical set, it's redundant for prefix cut; but needs more check on port cut
	if (dim0 == 2 || dim0 == 3) {
		if (!port_redund(ruleset, nrules, &tb_set[dim0], redt_nodes.nodeset[idx][i]))
			return INVALID;
	}
	if ((dim1 != dim0) && (dim1 == 2 || dim1 == 3)) {
		if (!port_redund(ruleset, nrules, &tb_set[dim1], redt_nodes.nodeset[idx][i]))
			return INVALID;
	}
if (dim0 == 3 || dim1 == 3)
d3valid++;
	return FOUND;
}


void update_node_redund(Trie *v)
{
	int		i, idx = v->nrules-1;
	
	i = redt_nodes.head[idx];
	redt_nodes.nodeset[idx][i] = v;
	redt_nodes.head[idx] = (redt_nodes.head[idx] + 1) % REDT_NODES;
	if (redt_nodes.head[idx] == redt_nodes.tail[idx])
		redt_nodes.tail[idx] = (redt_nodes.tail[idx] + 1) % REDT_NODES;
}



// ============================================================================
// section for rule redundancy check
// ============================================================================



inline
int rule_collide(Rule *rule, TBits *tb)
{
	return range_tbits_overlap(&rule->field[tb->dim], tb);
}



// Given a ternary bit string tb, return the max cover space of fields that cover the same area of tb,
// this function is used for rule redundancy check
inline
void rule_cover(Range *field, TBits *path_tb, Range *cover)
{
	int		i;

	for (i = 0; i < NFIELDS; i++) {
		cover[i] = field[i];
		max_tbits_cover(&cover[i], &path_tb[i]);
	}
}



// cover was computed from a rule r in the same ruleset with higher priority,
// if rule's cover is within ri's cover, rule is reduandant to r
inline
int redundant_rule(Rule *rule, TBits *path_tbits, Range *cover)
{
	Range	cover1[NFIELDS];
	int		i;

	rule_cover(rule->field, path_tbits, cover1);
	for (i = 0; i < NFIELDS; i++) {
		if (!range_cover(cover[i], cover1[i]))
			return 0;
	}
	return 1;
}



// ===========================================================================
// section for the main body of trie construction
// ===========================================================================


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



// formulate the ternary bit strings on all fields from the root to v's parent
void get_path_tbits(Trie *v, TBits *path_tb)
{
	int			i, j;

	for (i = 0; i < NFIELDS; i++) {
		path_tb[i].dim = i;
		path_tb[i].nbands = 0;
		path_tb[i].val = 0;
		for (j = 0; j < MAX_BANDS; j++)
			path_tb[i].bandmap[j] = 0;
	}
	while (v->id != 0) {
		add_tbits_band(path_tb, &v->bands[0]);
		add_tbits_band(path_tb, &v->bands[1]);
		v = v->parent;
	}
}



// Add a rule if it is overlaps with territories of tb0 & tb1, and it is not reduandant with 
// previous rules. return 1 if rule added, otherwise return 0
inline
int add_rule(Rule **ruleset, int nrules, Rule *rule, TBits *path_tbits, Trie *v)
{
	Range	*cover;
	TBits	*tb;
	int		i;

	// check if the rule overlaps with current cut portion
	tb = &path_tbits[v->cut_bands[0].dim];
	if (!rule_collide(rule, tb))
		return 0;
	tb = &path_tbits[v->cut_bands[1].dim];
	if (!rule_collide(rule, tb))
		return 0;

	// check rule redundancy
	if (nrules <= REDT_NRULES) {
		for (i = 0; i < nrules; i++) {
			cover = rule_covers[ruleset[i]->id];
			if (redundant_rule(rule, path_tbits, cover))
				break;
		}
		if (i == nrules) {	// not redundant to any rule in v
			ruleset[nrules] = rule;
			// set rule[i]'s cover to rule_covers for checking later rules
			cover = rule_covers[rule->id];
			rule_cover(rule->field, path_tbits, cover);
			return 1;
		} else
			return 0;
	} else {
		ruleset[nrules] = rule;
		return 1;
	}
}


// identify rules that overlap with path_tbits, and create a child if rules found
void new_child(Trie *v, TBits *path_tbits)
{
	Trie		*u;
	int			nrules = 0, found, i, j, dim0, dim1;

	dim0 = v->cut_bands[0].dim; dim1 = v->cut_bands[1].dim;
	u = &v->children[v->nchildren];
	u->rules = (Rule **) malloc(v->nrules * sizeof(Rule *));

	// add rules
	for (i = 0; i < v->nrules; i++) {
		if (add_rule(u->rules, nrules, v->rules[i], path_tbits, v))
			nrules++;
	}
	if (nrules == 0) {
		free(u->rules);
		return;
	}

	// check node redundancy
	if (nrules <= REDT_NRULES) {
		found = check_node_redund(u->rules, nrules, dim0, dim1, path_tbits);
if (found == INVALID)
	printf("invalid:%d\n", nrules);
	} else {
		found = INVALID;
	}

	if (found == FOUND) {
		free(u->rules);
		return;
	}

	if (nrules < v->nrules)
		u->rules = realloc(u->rules, nrules * sizeof(Rule *));
	u->layer = v->layer + 1;
	u->id = total_nodes++;
	u->parent = v;
	u->bands[0] = v->cut_bands[0];
	u->bands[1] = v->cut_bands[1];
	u->nrules = nrules;
	u->type = nrules > LEAF_RULES ? NONLEAF : LEAF;
	v->nchildren++;

	if (total_nodes >= trie_nodes_size) {
		trie_nodes_size += 10240;
		trie_nodes = realloc(trie_nodes, trie_nodes_size*sizeof(Trie *));
	}
	trie_nodes[total_nodes-1] = u;
	
	if (found == NOTFOUND) {
		if ((u->type == NONLEAF) && (dim0 == 2 || dim0 == 3)) {
			if (!port_redund(u->rules, u->nrules, &path_tbits[dim0], u))
				return;
		}
		if ((u->type == NONLEAF) && (dim1 == 2 || dim1 == 3) && (dim1 != dim0)) {
			if (!port_redund(u->rules, u->nrules, &path_tbits[dim1], u))
				return;
		}
		update_node_redund(u);
	}
	
//printf("\tnode[%d<-%d#%d]@%d: %d rules\n", u->id, v->id, v->nrules, u->layer, u->nrules);
}



// cut the space into banks using the best bit bands
void create_children(Trie* v)
{
	TBits		path_tbits[NFIELDS];
	uint32_t	val0, val1, bid, dim;

//printf("create children[%d]\n", v->id);
	get_path_tbits(v, path_tbits);
	reset_redt_nodes(path_tbits);

	choose_bands(v, path_tbits);

	v->children = (Trie *) calloc(MAX_CHILDREN, sizeof(Trie));
//printf("create_children %d\n", v->id);
	// assign each potential child its cut band values, then try to generate the child
	for (val0 = 0; val0 < BAND_SIZE; val0++) {
		v->cut_bands[0].val = val0;
		set_tbits_band(path_tbits, &v->cut_bands[0]);
		for (val1 = 0; val1 < BAND_SIZE; val1++) {
			v->cut_bands[1].val = val1;
			set_tbits_band(path_tbits, &v->cut_bands[1]);
			new_child(v, path_tbits);
		}
	}

	v->children = (Trie *) realloc(v->children, v->nchildren*sizeof(Trie));
}



Trie* build_trie(Rule *rules, int nrules)
{
	Trie*	v;
	int		i;

	rule_covers = malloc(nrules * sizeof(Range *));
	for (i = 0; i < nrules; i++)
		rule_covers[i] = malloc(NFIELDS * sizeof(Range));

	init_queue();
	trie_root = init_trie(rules, nrules);
	enqueue(trie_root);

	while (!queue_empty()) {
		v = dequeue();
		if (v->layer >= 6) {
			printf("Stop working: trie depth >= %d \n", v->layer);
			break;
		}
		create_children(v);
		for (i = 0; i < v->nchildren; i++) {
			//if ((v->children[i].type == NONLEAF) && (v->layer <= 8))
			if (v->children[i].type == NONLEAF)
				enqueue(&(v->children[i]));
		}
		//if (total_nodes > 1000)
		//	break;
		if (total_nodes > 300000) {
			printf("Stop working: too many trie nodes (%d)!\n", total_nodes);
			break;
		}
	}
dump_nodes(16, 8);
//check_small_rules(8);
printf("d3all: %d; d3found: %d; d3valid: %d\n", d3all, d3found, d3valid);
	printf("Trie nodes: %d\n", total_nodes);
//printf("max_fit: %d\n", max_fit);
//dump_trie(trie_root);
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

		dump_node(v, 1);
		for (i = 0; i < v->nchildren; i++)
			enqueue(&(v->children[i]));
	}
	printf("n8: %d/%d, n16: %d, total: %d\n", sn8, n8, n16, total_nodes);
	//check_small_rules(8);
	//check_small_rules(16);
	//small_large(16);
}


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


void dump_nodes(int max, int min)
{
	int		i, n;
	Trie	*v, *p = NULL;

	for (i = 1; i < total_nodes; i++) {
		v = trie_nodes[i];
		n = v->nrules;
/*
	//	if ((n <= max) && (n >= min))
			printf("N[%d<-%d#%d]@%d: #%d  ", 
					v->id, v->parent->id, v->parent->nrules, v->layer, v->nrules);

			if ((v->layer >= 3 && v->nrules >= 16) ||
				(v->layer >= 4 && v->nrules >= 8)  ||
				(v->layer >= 6 && v->nrules >= 6))
				printf("d%d-b%d-v%x | d%d-b%d-v%x", 
						v->bands[0].dim, v->bands[0].id, v->bands[0].val,
						v->bands[1].dim, v->bands[1].id, v->bands[1].val);
if (v->bands[0].dim == 3 || v->bands[1].dim == 3)
	printf(" D3");
			printf("\n");
*/
if ((v->bands[0].dim == 3 || v->bands[1].dim == 3) && (v->nrules <= 5)) {
	if (p != v->parent) {
		printf("\n===============================================================================\n");
		p = v->parent;
	}
	printf("\n[%d<-%d#%d]@%d: d%d-b%d-v%x | d%d-b%d-v%x\n", 
			v->id, v->parent->id, v->parent->nrules, v->layer,
			v->bands[0].dim, v->bands[0].id, v->bands[0].val,
			v->bands[1].dim, v->bands[1].id, v->bands[1].val);
	dump_node_rules(v);
}

	}
}
