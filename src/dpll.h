#ifndef DPLL_H
#define DPLL_H

#include "cnf.h"

typedef std::vector<bool> assignment;
void print_assignment(assignment &asmt);

bool dpll_sat(CNF *cnf, int num_vars, assignment &result);

#endif /* DPLL_H */
