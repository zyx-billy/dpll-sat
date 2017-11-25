#ifndef PARSER_H
#define PARSER_H

#include <unordered_map>
#include <vector>
#include "formula.h"

// mapping from variable's original name to variable's int assignment
typedef std::unordered_map<std::string, int> vmap_t;
// reversed Vmap
typedef std::vector<std::string> rmap_t;

struct parse_result {
  Formula *f;
  vmap_t *Vmap;
  rmap_t *Rmap;

  char *error_char_pos;
  char expects;

  parse_result(Formula *ff, vmap_t *v, rmap_t *r) :
    f(ff), Vmap(v), Rmap(r), error_char_pos(0) {}
  parse_result(char *error_pos, char exp) :
    error_char_pos(error_pos), expects(exp) {}

  bool has_error() {
    return error_char_pos != 0;
  }
};

void print_rmap(rmap_t *Rmap);

parse_result *parse_formula(char *f, char *end);

#endif /* PARSER_H */
