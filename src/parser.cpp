#include <iostream>
#include <stack>
#include <utility>
#include <cassert>

#include "formula.h"
#include "parser.h"

/* for recording states in the parent_stack */
struct record {
  Formula **fp;
  int depth;
  bool quick_backtrack; // default: false

  record(Formula **f, int d, bool q = false) :
    fp(f), depth(d) , quick_backtrack(q) {}
};

// is a valid character for a variable
bool is_var_char(char *f) {
  return (*f >= 'a' && *f <= 'z') ||
         (*f >= 'A' && *f <= 'Z') ||
         (*f >= '0' && *f <= '9');
}

// return last position of variable name
char *parse_var(char *f, vmap_t *Vmap, rmap_t *Rmap, Variable **ret_var) {
  int len = 0;
  while (is_var_char(f)) {
    f++;
    len++;
  }

  std::string var_name(f-len, len);
  auto var_int = Vmap->find(var_name);
  if (var_int == Vmap->end()) {
    // new var
    (*Vmap)[var_name] = Rmap->size();
    Rmap->push_back(var_name);
  }

  *ret_var = new Variable((*Vmap)[var_name], var_name);
  return f - 1;
}

parse_result *parse_formula(char *f, char *end) {
  vmap_t *Vmap = new vmap_t;
  rmap_t *Rmap = new rmap_t;

  int depth = 0;
  bool expect_expr = true;
  Connective bin_op;

  std::stack<record> parent_stack;

  // kick start
  Formula *root = 0;
  parent_stack.emplace(&root, 0);
  parent_stack.emplace(&root, 0);

  while(f < end && *f) {
    if (*f == ' ') {
      // skip whitespace between tokens
      f++;
      continue;
    }

    if (expect_expr) {
      // expecting an expression
      switch(*f) {
        case '!':
          {
            assert(parent_stack.size() >= 1);
            record curr_record = parent_stack.top();
            Negated *new_one = new Negated(0);
            *(curr_record.fp) = new_one;
            parent_stack.emplace(&(new_one->f), depth, true);
          }
          break;
        case '(':
          depth++;
          break;
        default:
          // allow a-z A-z 0-9
          if (!is_var_char(f)) return new parse_result(f, 'e');

          {
            Variable *new_one;
            f = parse_var(f, Vmap, Rmap, &new_one);

            assert(parent_stack.size() >= 1);
            record curr_record = parent_stack.top();
            *(curr_record.fp) = new_one;

            // clean up negations
            while (depth <= curr_record.depth && curr_record.quick_backtrack) {
              parent_stack.pop();
              curr_record = parent_stack.top();
            }
            if (depth <= curr_record.depth) parent_stack.pop();
          }

          expect_expr = false; // we now expect a connective
          break;
      }
    } else {
      // expecting a connective
      if (*f == ')') {
        depth--;
        if (depth < 0) return new parse_result(f-1, '(');

        // move back up
        record curr_record = parent_stack.top();
        while (depth <= curr_record.depth && curr_record.quick_backtrack) {
          parent_stack.pop();
          curr_record = parent_stack.top();
        }
        if (depth <= curr_record.depth) parent_stack.pop();
      } else {
        // check which connective it is
        switch(*f) {
          case '&':
            bin_op = land;
            break;
          case '|':
            bin_op = lor;
            break;
          case '<':
            // start of equiv
            if (f[1] != '-') return new parse_result(f+1, '-');
            if (f[2] != '>') return new parse_result(f+2, '>');
            bin_op = lequiv;
            f += 2; // bring f to last char of connective
            break;
          case '-':
            // start of imply
            if (f[1] != '>') return new parse_result(f+1, '>');
            bin_op = limply;
            f += 1; // bring f to last char of connective
            break;
          default:
            return new parse_result(f, 'c');
        }
        expect_expr = true; // we now expect an expression

        /* parent stack guaranteed not empty rn */
        assert(parent_stack.size() >= 1);
        record curr_record = parent_stack.top();
        Binary *new_one = new Binary(*(curr_record.fp), 0, bin_op);
        *(curr_record.fp) = new_one;
        parent_stack.emplace(&(new_one->r), depth);
      }
    }

    f++;
  }

  if (depth != 0) return new parse_result(f, ')');

  return new parse_result(root, Vmap, Rmap);
}

/* helpers */
void print_rmap(rmap_t *Rmap, std::ostream &output) {
  for (int i = 0; i < Rmap->size(); i++) {
    output << i << ": " << (*Rmap)[i] << std::endl;
  }
}
