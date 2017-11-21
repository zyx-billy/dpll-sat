#ifndef FORMULA_H
#define FORMULA_H

#include <string>

enum Connective {land, lor, limply, lequiv};

class Formula {
public:
  enum Type {invalid, variable, negated, binary};

  Type t;
  virtual void print(std::ostream& os) const = 0;
  virtual void print_tree(std::string prefix) const = 0;
  virtual Formula *find_error() = 0;
  friend std::ostream& operator<<(std::ostream& os, const Formula& f) {
    f.print(os);
    return os;
  }
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

  virtual void print(std::ostream& os) const {
    os << "INVALID";
  }
  virtual void print_tree(std::string prefix) const {
    std::cout << prefix << "Invalid formula" << std::endl;
  }
  virtual Formula *find_error() {
    return this;
  }
};

class Variable : public Formula {
public:
  int var;
  std::string name;

  Variable(int v, std::string n) :
    Formula(variable), var(v), name(n) {}

  virtual void print(std::ostream& os) const {
    os << name;
  }
  virtual void print_tree(std::string prefix) const {
    std::cout << prefix << name << std::endl;
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

  virtual void print(std::ostream& os) const {
    os << "!";
    f->print(os);
  }
  virtual void print_tree(std::string prefix) const {
    std::cout << prefix << "NOT" << std::endl;
    if (f) f->print_tree(prefix + "  ");
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

  virtual void print(std::ostream& os) const {
    os << "(";
    l->print(os);
    switch (op) {
      case land:
        os << "&";
        break;
      case lor:
        os << "|";
        break;
      case limply:
        os << "->";
        break;
      case lequiv:
        os << "<->";
        break;
    }
    r->print(os);
    os << ")";
  }
  virtual void print_tree(std::string prefix) const {
    if (l) l->print_tree(prefix + "  ");

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

    if (r) r->print_tree(prefix + "  ");
  }
  virtual Formula *find_error() {
    Formula *l_error = l->find_error();
    if (l_error) return l_error;
    return r->find_error();
  }
};

#endif /* FORMULA_H */
