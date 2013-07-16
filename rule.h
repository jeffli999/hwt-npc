#ifndef RULE_H
#define RULE_H

#include "common.h"

#define NFIELDS	5	// number of fields

typedef struct {
	uint32_t	id;
	Range		field[NFIELDS];
} Rule;


#endif
