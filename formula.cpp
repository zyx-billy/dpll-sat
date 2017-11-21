#include <iostream>
#include <unordered_map>
#include <vector>

#define INPUT_BUF_SIZE 256

// variable to int mapper
int var_next_int = 0;
std::unordered_map<std::string, int> Vmap;
std::vector<std::string> Rmap;

// classes to represent formula
enum Connective {land, lor, limply, lequiv};

class Formula {
public:
  enum Type {invalid, variable, negated, binary};

  Type t;
  Formula(Type type): t(type) {}
  virtual void print_self(std::string prefix) {
    std::cout << prefix << "Generic formula" << std::endl;
  }
  virtual Formula *find_error() {
    return 0;
  }
};

class Invalid : public Formula {
public:
  char *error_char;
  char expects;

  Invalid(char *error_pos, char exp) :
    Formula(invalid), error_char(error_pos), expects(exp) {}

  virtual void print_self(std::string prefix) {
    std::cout << prefix << "Invalid formula" << std::endl;
  }
  virtual Formula *find_error() {
    return this;
  }
};

class Variable : public Formula {
public:
  int var;

  Variable(int n) :
    Formula(variable), var(n) {}

  virtual void print_self(std::string prefix) {
    std::cout << prefix << Rmap[var] << std::endl;
  }
  virtual Formula *find_error() {
    return 0;
  }
};

class Negated : public Formula {
public:
  Formula *f;

  Negated(Formula *fa) :
    Formula(negated), f(fa) {}
  virtual void print_self(std::string prefix) {
    std::cout << prefix << "NOT" << std::endl;
    f->print_self(prefix + "  ");
  }
  virtual Formula *find_error() {
    return f->find_error();
  }
};

class Binary : public Formula {
public:
  Formula *l;
  Formula *r;
  Connective op;

  Binary(Formula *left, Formula *right, Connective conn) :
    Formula(binary), l(left), r(right), op(conn) {}
  virtual void print_self(std::string prefix) {
    l->print_self(prefix + "  ");

    std::cout << prefix;
    switch(op) {
      case land:
        std::cout << "AND";
        break;
      case lor:
        std::cout << "OR";
        break;
      case limply:
        std::cout << "IMPLIES";
        break;
      case lequiv:
        std::cout << "EQUIVALENT";
        break;
    }
    std::cout << std::endl;

    r->print_self(prefix + "  ");
  }
  virtual Formula *find_error() {
    Formula *l_error = l->find_error();
    if (l_error) return l_error;
    return r->find_error();
  }
};

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
  char *left_end = 0;
  char *right_start = 0;

  int depth = 0;
  bool expect_expr = true;
  char *first_char = 0;
  char *first_paren_start = 0;
  char *first_paren_end = 0;
  Formula *left_formula = 0;
  Formula **next_formula_ptr = &left_formula;
  int var_int = -1;
  bool simple_left = true;
  Connective bin_op;

  while(f < end && *f && !right_start) {
    if (*f == ' ') {
      // skip whitespace between tokens
      f++;
      continue;
    }

    if (expect_expr) {
      // expecting an expression
      switch(*f) {
        case '!':
          if (!first_char) first_char = f;
          {
            Negated *new_one = new Negated(0);
            *next_formula_ptr = new_one;
            next_formula_ptr = &(new_one->f);
          }
          
          break;
        case '(':
          if (!first_paren_start) first_paren_start = f;
          depth++;
          break;
        default:
          // allow a-z A-z 0-9
          if (!is_var_char(f)) return new Invalid(f, 'e');
          if (!first_char) first_char = f;
          f = parse_var(f, &var_int);

          {
            Variable *new_one = new Variable(var_int);
            *next_formula_ptr = new_one;
          }

          expect_expr = false; // we now expect a connective
          break;
      }
    } else {
      // expecting a connective
      switch(*f) {
        case '&':
          bin_op = land;
          expect_expr = true; // we now expect an expression

          if (depth == 0) right_start = f+1;
          else simple_left = false;

          break;
        case '|':
          bin_op = lor;
          expect_expr = true; // we now expect an expression

          if (depth == 0) right_start = f+1;
          else simple_left = false;

          break;
        case ')':
          depth--;
          if (depth < 0) return new Invalid(f-1, '(');
          else if (depth == 0 && !first_paren_end) first_paren_end = f;
          break;
        case '<':
          // start of equiv
          if (f[1] != '-') return new Invalid(f+1, '-');
          if (f[2] != '>') return new Invalid(f+2, '>');
          bin_op = lequiv;
          expect_expr = true; // we now expect an expression

          if (depth == 0) right_start = f+3;
          else simple_left = false;

          break;
        case '-':
          // start of imply
          if (f[1] != '>') return new Invalid(f+1, '>');
          bin_op = limply;
          expect_expr = true;

          if (depth == 0) right_start = f+2;
          else simple_left = false;

          break;
        default:
          return new Invalid(f, 'c');
      }
    }

    f++;
  }
  left_end = f;

  if (depth != 0) return new Invalid(f, ')');

  if (!simple_left) {
    // parse left side recursively
    *next_formula_ptr = parse_formula(first_paren_start+1, first_paren_end);
  }

  if (right_start) {
    // is binary
    return new Binary(left_formula, parse_formula(right_start, end), bin_op);
  } else {
    // is unary
    return left_formula;
  }
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