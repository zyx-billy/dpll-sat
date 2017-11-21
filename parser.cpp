#include <iostream>
#include <unordered_map>
#include <vector>
#include <stack>
#include <utility>
#include <cassert>

#include "parser.h"

#define INPUT_BUF_SIZE 256

struct record {
  Formula **fp;
  int depth;
  bool quick_backtrack; // default: false

  record(Formula **f, int d, bool q = false) :
    fp(f), depth(d) , quick_backtrack(q) {}
};

// variable to int mapper
int var_next_int = 0;
std::unordered_map<std::string, int> Vmap;
std::vector<std::string> Rmap;

// is a valid character for a variable
bool is_var_char(char *f) {
  return (*f >= 'a' && *f <= 'z') ||
         (*f >= 'A' && *f <= 'Z') ||
         (*f >= '0' && *f <= '9');
}

// return last position of variable name
char *parse_var(char *f, int *ret_var_int) {
  int len = 0;
  while (is_var_char(f)) {
    f++;
    len++;
  }

  std::string var_name(f-len, len);
  auto var_int = Vmap.find(var_name);
  if (var_int == Vmap.end()) {
    // new var
    Vmap[var_name] = var_next_int++;
    Rmap.push_back(var_name);
  }

  *ret_var_int = Vmap[var_name];
  return f - 1;
}

Formula *parse_formula(char *f, char *end) {
  int depth = 0;
  bool expect_expr = true;
  int var_int = -1;
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
          if (!is_var_char(f)) return new Invalid(f, 'e');

          f = parse_var(f, &var_int);
          {
            assert(parent_stack.size() >= 1);
            record curr_record = parent_stack.top();
            Variable *new_one = new Variable(var_int);
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
        if (depth < 0) return new Invalid(f-1, '(');

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
            if (f[1] != '-') return new Invalid(f+1, '-');
            if (f[2] != '>') return new Invalid(f+2, '>');
            bin_op = lequiv;
            f += 2; // bring f to last char of connective
            break;
          case '-':
            // start of imply
            if (f[1] != '>') return new Invalid(f+1, '>');
            bin_op = limply;
            f += 1; // bring f to last char of connective
            break;
          default:
            return new Invalid(f, 'c');
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

  if (depth != 0) return new Invalid(f, ')');

  return root;
}

void print_all_variables() {
  std::cout << "All variables:" << std::endl;
  for (int i = 0; i < var_next_int; i++) {
    std::cout << i << ": " << Rmap[i] << std::endl;
  }
}

int main() {
  char input[INPUT_BUF_SIZE];
  std::cin.getline(input, INPUT_BUF_SIZE);

  Formula *f = parse_formula(input, input+INPUT_BUF_SIZE);

  Formula *f_error = f->find_error();
  if (!f_error) {
    std::cout << "Parse complete. No errors." << std::endl;
  } else {
    Invalid *error = static_cast<Invalid *>(f_error);
    std::cout << "Parse Error:" << std::endl
      << "  position: " << (error->error_char - input) << std::endl
      << "  found: " << *error->error_char << std::endl
      << "  expects: ";
    switch (error->expects) {
      case 'e':
        std::cout << "an expression";
        break;
      case 'c':
        std::cout << "a binary operator";
        break;
      default:
        std::cout << "'" << error->expects << "'";
    }
    std::cout << std::endl;
    return 0;
  }

  std::cout << "Parse tree:" << std::endl;
  f->print_self("");
}
