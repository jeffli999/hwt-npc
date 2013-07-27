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

#define CHECK_NODES		4	// only check at most this number of nodes
#define	CHECK_NRULES	64	// only check nodes with rules less than this


typedef struct {
	int		head[REDT_NRULES], tail[REDT_NRULES];
	Trie	*nodeset[REDT_NUM][REDT_NRULES];


} RedtNodes;

int		check_heads[CHECK_NRULES], check_tails[CHECK_NRULES];
Rule	**check_rulesets[CHECK_NODES][CHECK_NRULES];
uint8_t range_flags[2][CHECK_NODES][CHECK_NRULES];	// 0: all rules fully cover parent port borders

void reset_check_rules()
{
	int		i, j;

	for (i = 0; i < CHECK_NODES; i++) {
		memset(check_rulesets[i], 0, CHECK_NRULES*sizeof(Rule **));
		memset(range_flags[0][i], 0, CHECK_NRULES*sizeof(uint8_t));
		memset(range_flags[1][i], 0, CHECK_NRULES*sizeof(uint8_t));
	}
	// set to indicate the empty of nodes of various lengths
	for (i = 0; i < CHECK_NRULES; i++) 
		check_heads[i] = check_tails[i] = 0;
}



// if all rules cover the whole range of its parent, set the flag to 1 to indicate redundancy
void set_range_cover(Rule **rules, int nrules, int dim, Range parent_border, int pos)
{
	int		i;

	range_flags[dim-2][pos][nrules-1] = 1;
	for (i = 0; i < nrules; i++) {
		if (!range_cover(rules[i]->field[dim], parent_border)) {
			range_flags[dim-2][pos][nrules-1] = 0;
			break;
		}
	}
}



// redundant trie node removal: try to find a ruleset in current children with limited effort
// (not an exhaustive search, only the recent set with the same number of rules is checked)
// reduandancy on leaf nodes or IP/prot prefix can be simply checked by idential ruleset
// port fileds need first check whether each rule in a child node fully covers parent range
int find_ruleset(Rule **ruleset, int nrules, int dim0, int dim1, Range *parent_borders)
{
	int			idx = nrules-1, i;
	
	for (i = check_tails[idx]; i != check_heads[idx]; i = (i+1) % CHECK_NODES) {
		if (memcmp(ruleset, check_rulesets[i][idx], nrules*sizeof(Rule *)) == 0)
			break;
	}
	return i;
}
}



// if node is not redundant, need to include it in the check set for checking following nodes
void check_node_redundancy(Rule **ruleset, int nrules, int dim0, int dim1, Range *parent_borders)
{
	int		idx = nrules-1, pos;

	pos = find_ruleset(ruleset, nrules, dim0, dim1, parent_borders);

	if (dim0 == 2 || dim0 == 3) {
		if (range_flags[dim0-2][i][idx] == 0)
			return 0;
	}
	if (dim1 == 2 || dim1 == 3) {
		if (range_flags[dim1-2][i][idx] == 0) 
			return 0;
	}
	
	return 1;

	check_rulesets[check_heads[idx]][idx] = ruleset;
	// set range cover flags for judge of port-cut redundancy
	if (dim0 == 2 || dim0 == 3)
		set_range_cover(ruleset, nrules, dim0, parent_borders[0], check_heads[idx]);
	if (dim1 == 2 || dim1 == 3)
		set_range_cover(ruleset, nrules, dim1, parent_borders[1], check_heads[idx]);

	check_heads[idx] = (check_heads[idx] + 1) % CHECK_NODES;
	if (check_heads[idx] == check_tails[idx])
		check_tails[idx] = (check_tails[idx] + 1) % CHECK_NODES;
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
		range_tbits_cover(&cover[i], &path_tb[i]);
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
	if (nrules <= CHECK_NRULES) {
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
void new_child(Trie *v, TBits *path_tbits, Range *parent_borders)
{
	Trie		*u;
	int			nrules = 0, found, i, j, dim0, dim1;
	Range		tb_borders[2];

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
	if (nrules <= CHECK_NRULES) {
		found = find_ruleset(u->rules, nrules, dim0, dim1, parent_borders);
	} else {
		found = 0;
	}
	if (found) {
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

	update_check_rulesets(u->rules, nrules, dim0, dim1, parent_borders);
	
//printf("\tnode[%d<-%d#%d]@%d: %d rules\n", u->id, v->id, v->nrules, u->layer, u->nrules);
}



// cut the space into banks using the best bit bands
void create_children(Trie* v)
{
	TBits		path_tbits[NFIELDS];
	uint32_t	val0, val1, bid, dim;
	Range		tb_borders[2];

//printf("create children[%d]\n", v->id);
	get_path_tbits(v, path_tbits);
	tb_borders[0].lo = tbits_border(&path_tbits[2], 0);
	tb_borders[0].hi = tbits_border(&path_tbits[2], 1);
	tb_borders[1].lo = tbits_border(&path_tbits[3], 0);
	tb_borders[1].hi = tbits_border(&path_tbits[3], 1);

	choose_bands(v, path_tbits);

	v->children = (Trie *) calloc(MAX_CHILDREN, sizeof(Trie));
	reset_check_rules();
//printf("create_children %d\n", v->id);
	// assign each potential child its cut band values, then try to generate the child
	for (val0 = 0; val0 < BAND_SIZE; val0++) {
		v->cut_bands[0].val = val0;
		set_tbits_band(path_tbits, &v->cut_bands[0]);
		for (val1 = 0; val1 < BAND_SIZE; val1++) {
			v->cut_bands[1].val = val1;
			set_tbits_band(path_tbits, &v->cut_bands[1]);
			new_child(v, path_tbits, tb_borders);
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


void dump_nodes(int max, int min)
{
	int		i, n;
	Trie	*v;

	for (i = 1; i < total_nodes; i++) {
		v = trie_nodes[i];
		n = v->nrules;
	//	if ((n <= max) && (n >= min))
			printf("N[%d<-%d#%d]@%d: #%d  ", 
					v->id, v->parent->id, v->parent->nrules, v->layer, v->nrules);

			if ((v->layer >= 3 && v->nrules >= 16) ||
				(v->layer >= 4 && v->nrules >= 8)  ||
				(v->layer >= 6 && v->nrules >= 6))
				printf("d%d-b%d-v%x | d%d-b%d-v%x", 
						v->bands[0].dim, v->bands[0].id, v->bands[0].val,
						v->bands[1].dim, v->bands[1].id, v->bands[1].val);

			printf("\n");

	}
}
