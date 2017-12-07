#ifndef DPLL_H
#define DPLL_H

#include "cnf.h"
#include "parser.h"

typedef std::vector<bool> assignment;

void print_assignment(assignment &asmt, rmap_t* Rmap, int num_vars);

bool dpll_sat(CNF *cnf, int num_vars, assignment &result);

#endif /* DPLL_H */
