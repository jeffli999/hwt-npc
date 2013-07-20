#ifndef HWT_H
#define HWT_H

#include "bitband.h"
#include "trie.h"


void choose_bands(Trie *v, TBits *path_tbits);
void rule_distrib(Trie *v, TBits *tb, int bid, int *hwt);


#endif
