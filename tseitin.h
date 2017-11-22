#ifndef TSEITIN_H
#define TSEITIN_H

#include "cnf.h"
#include "parser.h"

CNF *tseitin_transform(parse_result *pr);

#endif /* TSEITIN_H */