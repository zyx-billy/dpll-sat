#ifndef PARSER_H
#define PARSER_H

#include "formula.h"

// mapping from variable's original name to variable's int assignment
typedef std::unordered_map<std::string, int> vmap_t;
// reversed Vmap
typedef std::vector<std::string> rmap_t;

struct parse_result {
  Formula *f;
  vmap_t Vmap;
  rmap_t Rmap;
};

Formula *parse_formula(char *f, char *end);

#endif /* PARSER_H */
