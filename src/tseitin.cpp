#include <string>
#include <sstream>
#include <set>

#include "tseitin.h"
#include "formula.h"
#include "cnf.h"
#include "parser.h"

vmap_t *Vmap;
rmap_t *Rmap;

// var_int to pointer of literal (pos literal, then neg literal)
struct lpair {
  Literal *pos;
  Literal *neg;

  lpair(Literal *p, Literal *n) :
    pos(p), neg(n) {}
  void flip() {
    Literal *temp = pos;
    pos = neg;
    neg = temp;
  }
};

typedef std::vector<lpair> lmap_t;
lmap_t *Lmap;

struct tseitin_unit {
  lpair A;
  lpair B;
  lpair C;
  Connective op;
  bool is_unary;

  tseitin_unit(lpair a, lpair b, lpair c, Connective conn) :
    A(a), B(b), C(c), op(conn), is_unary(false) {}

  // Create a unary tseitin_unit, B is useless in this case
  tseitin_unit(lpair a, lpair c) :
    A(a), B(c), C(c), is_unary(true) {}

  std::string print() const {
    std::ostringstream oss;
    oss << *(A.pos) << ";"
        << *(B.pos) << ";"
        << *(C.pos) << ";"
        << static_cast<int>(op);
    return oss.str();
  }
};

bool tseitin_unit_compare(const tseitin_unit &P, const tseitin_unit &Q) {
  return P.print() < Q.print();
}

typedef std::set<tseitin_unit, bool(*)(const tseitin_unit &A, const tseitin_unit &B)> tu_set;


lpair find_or_assign_var(Formula *f) {
  if (f->type == Formula::variable) {
    Variable *v = static_cast<Variable *>(f);
    return (*Lmap)[v->var];
  }

  std::ostringstream name_stream;
  name_stream << *f;
  std::string var_name = name_stream.str();
  auto var_int_it = Vmap->find(var_name);
  if (var_int_it == Vmap->end()) {
    // new var
    int var_int = Rmap->size();
    (*Vmap)[var_name] = var_int;
    Rmap->push_back(var_name);
    lpair lp = lpair(new Literal(var_int, true),
                     new Literal(var_int, false));
    Lmap->push_back(lp);
    return lp;
  }

  return (*Lmap)[(*Vmap)[var_name]];
}

// transform from C <-> (A & B) to CNF
// heuristic:
//   C <-> (A & B) = (!A | !B | C) & (A | !C) & (B | !C)
CNF *tseitin_basic_land(lpair A, lpair B, lpair C) {
  CNF *result = new CNF();
  Clause *cl1 = new Clause();
  Clause *cl2 = new Clause();
  Clause *cl3 = new Clause();

  cl1->literals.push_back(A.neg);
  cl1->literals.push_back(B.neg);
  cl1->literals.push_back(C.pos);

  cl2->literals.push_back(A.pos);
  cl2->literals.push_back(C.neg);

  cl3->literals.push_back(B.pos);
  cl3->literals.push_back(C.neg);

  result->clauses.push_back(cl1);
  result->clauses.push_back(cl2);
  result->clauses.push_back(cl3);

  return result;
}

CNF *tseitin_basic_lor(lpair A, lpair B, lpair C) {
  A.flip();
  B.flip();
  C.flip();

  return tseitin_basic_land(A, B, C);
}

CNF *tseitin_basic_not(lpair A, lpair C) {
  CNF *result = new CNF();
  Clause *cl1 = new Clause();
  Clause *cl2 = new Clause();

  cl1->literals.push_back(A.neg);
  cl1->literals.push_back(C.neg);

  cl2->literals.push_back(A.pos);
  cl2->literals.push_back(C.pos);

  result->clauses.push_back(cl1);
  result->clauses.push_back(cl2);
  return result;
}

CNF *tseitin_basic_lequiv(lpair A, lpair B, lpair C) {
  CNF *result = new CNF();
  Clause *cl1 = new Clause();
  Clause *cl2 = new Clause();
  Clause *cl3 = new Clause();
  Clause *cl4 = new Clause();

  cl1->literals.push_back(A.neg);
  cl1->literals.push_back(B.pos);

  cl2->literals.push_back(A.pos);
  cl2->literals.push_back(B.neg);

  cl3->literals.push_back(B.neg);
  cl3->literals.push_back(C.pos);

  cl4->literals.push_back(B.pos);
  cl4->literals.push_back(C.neg);

  result->clauses.push_back(cl1);
  result->clauses.push_back(cl2);
  result->clauses.push_back(cl3);
  result->clauses.push_back(cl4);
  return result;
}

// generate tseitin units
void gen_tu(Formula *f, tu_set *tus) {
  switch (f->type) {
    case Formula::binary:
      {
        Binary *b = static_cast<Binary *>(f);
        gen_tu(b->l, tus);
        tus->emplace(find_or_assign_var(b->l),
                     find_or_assign_var(b->r),
                     find_or_assign_var(b),
                     b->op);
        gen_tu(b->r, tus);
      }
      break;
    case Formula::variable:
      break;
    case Formula::negated:
      {
        Negated *n = static_cast<Negated *>(f);
        tus->emplace(find_or_assign_var(n->f),
                     find_or_assign_var(n));
        gen_tu(n->f, tus);
      }
      break;
  }
}

CNF *tu_to_cnf(const tseitin_unit &tu) {
  // make copies
  lpair A = tu.A;
  lpair B = tu.B;
  lpair C = tu.C;

  if (tu.is_unary) {
    return tseitin_basic_not(A, C);
  }

  switch (tu.op) {
    case land:
      return tseitin_basic_land(A, B, C);
    case lor:
      return tseitin_basic_lor(A, B, C);
    case limply:
      A.flip();
      return tseitin_basic_lor(A, B, C);
    case lequiv:
      return tseitin_basic_lequiv(A, B, C);
  }
}

CNF *tu_set_to_cnf(tu_set *tus) {
  CNF *result = new CNF();

  for (auto it = tus->begin(); it != tus->end(); it++) {
    merge_cnf(result, tu_to_cnf(*it));
  }

  return result;
}

// direct parse into CNF
Literal *parse_into_literal(Formula *f, bool negate) {
  if (f->type == Formula::variable) {
      lpair Vp = find_or_assign_var(f);
      if (negate) return Vp.neg;
      return Vp.pos;
  } else if (f->type == Formula::negated) {
      Negated *nv = static_cast<Negated *>(f);
      return parse_into_literal(nv->f, !negate);
  }
  return nullptr;
}

Clause *parse_into_clause(Formula *f) {
  if (f->type == Formula::binary) {
    Binary *b = static_cast<Binary *>(f);
    if (b->op != lor) return nullptr;
    Clause *left_res = parse_into_clause(b->l);
    Clause *right_res = parse_into_clause(b->r);
    if (!left_res || !right_res) return nullptr;
    return merge_clause(left_res, right_res);
  } else {
    Literal *L = parse_into_literal(f, false);
    if (!L) return nullptr;
    Clause *C = new Clause();
    C->literals.push_back(L);
    return C;
  }
}

// check if an arbitrary formula is in cnf already
CNF *parse_into_cnf(Formula *f) {
  if (f->type == Formula::binary) {
    Binary *b = static_cast<Binary *>(f);
    if (b->op == land) {
      // could be cnf
      CNF *left_result = parse_into_cnf(b->l);
      CNF *right_result = parse_into_cnf(b->r);
      if (!left_result || !right_result) return nullptr;
      return merge_cnf(left_result, right_result);
    } else if (b->op == lor) {
      // do nothing here so it falls out of 'if' case
      // try parsing into clause
    } else {
      return nullptr;
    }
  }

  Clause *C = parse_into_clause(f);
  if (!C) return nullptr;
  CNF *result = new CNF();
  result->clauses.push_back(C);
  return result;
}

// must call this first before tseitin_transform or parse_into_cnf
void tseitin_init(vmap_t *vmap, rmap_t *rmap) {
  // initialize global data
  Vmap = vmap;
  Rmap = rmap;

  Lmap = new lmap_t();
  for (int i = 0; i < Rmap->size(); i++) {
    Lmap->emplace_back(new Literal(i, true), new Literal(i, false));
  }
}

CNF *tseitin_transform(Formula *f) {
  CNF *result;

  // do full tseitin
  tu_set tus(&tseitin_unit_compare);
  gen_tu(f, &tus);

  // print the basic tseitin units
  // for (auto it = tus.begin(); it != tus.end(); it++) {
  //   std::cout << (*it).print() << std::endl;
  // }

  result = tu_set_to_cnf(&tus);

  // add the var representing the entire formula to result
  lpair entire_formula = find_or_assign_var(f);
  Clause *C = new Clause();
  C->literals.push_back(entire_formula.pos);
  result->clauses.push_back(C);

  return result;
}