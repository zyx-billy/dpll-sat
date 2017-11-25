#ifndef CNF_H
#define CNF_H

#include <ostream>
#include <vector>

typedef int var;

class Literal {
public:
  var v;
  bool is_true;

  Literal(var vv, bool is_pos) :
    v(vv), is_true(is_pos) {}
  friend std::ostream& operator<<(std::ostream& os, const Literal& L) {
    if (!L.is_true) os << "!";
    os << L.v;
    return os;
  }
};

// Or-Clause
class Clause {
public:
  std::vector<Literal *> literals;

  Clause() {}
  friend std::ostream& operator<<(std::ostream& os, const Clause& C) {
    os << "(";

    if (C.literals.size() > 0) {
      auto it = C.literals.begin();
      os << **it;
      it++;

      for (; it != C.literals.end(); it++) {
        os << "|" << **it;
      }
    }

    os << ")";
    return os;
  }
};

class CNF {
public:
  std::vector<Clause *> clauses;

  CNF() {}
  friend std::ostream& operator<<(std::ostream& os, const CNF& cnf) {
    if (cnf.clauses.size() > 0) {
      auto it = cnf.clauses.begin();
      os << **it;
      it++;

      for (; it != cnf.clauses.end(); it++) {
        os << "&" << **it;
      }
    }

    return os;
  }
};

#endif /* CNF_H */