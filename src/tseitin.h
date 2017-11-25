#ifndef TSEITIN_H
#define TSEITIN_H

#include "cnf.h"
#include "parser.h"

CNF *tseitin_transform(Formula *f, vmap_t *vmap, rmap_t *rmap);

#endif /* TSEITIN_H */