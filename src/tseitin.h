#ifndef TSEITIN_H
#define TSEITIN_H

#include "cnf.h"
#include "parser.h"

void tseitin_init(vmap_t *vmap, rmap_t *rmap);
CNF *tseitin_transform(Formula *f);
CNF *parse_into_cnf(Formula *f);

#endif /* TSEITIN_H */