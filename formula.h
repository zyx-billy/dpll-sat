#ifndef FORMULA_H
#define FORMULA_H

#include <string>

enum Connective {land, lor, limply, lequiv};

class Formula {
public:
  enum Type {invalid, variable, negated, binary};

  Type t;
  virtual void print_self(std::string prefix) = 0;
  virtual Formula *find_error() = 0;
protected:
  Formula(Type type) : t(type) {}
  virtual ~Formula() {}
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
    std::cout << prefix << var << std::endl;
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

#endif /* FORMULA_H */
