#include <string>
#include <sstream>
#include <set>

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

CNF *merge_cnf(CNF *CNF_A, CNF *CNF_B) {
  std::vector<Clause *> *A = &(CNF_A->clauses);
  std::vector<Clause *> *B = &(CNF_B->clauses);

  A->reserve(A->size() + B->size());
  A->insert(A->end(), B->begin(), B->end());
  delete CNF_B;
  return CNF_A;
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

void print_rmap() {
  for (int i = 0; i < Rmap->size(); i++) {
    std::cout << i << ": " << (*Rmap)[i] << std::endl;
  }
}

CNF *tseitin_transform(parse_result *pr) {
  // make sure there's no errors in f
  if (pr->has_error()) return 0;

  // initialize global data
  int primitive_vars = pr->Rmap->size();
  Vmap = pr->Vmap;
  Rmap = pr->Rmap;

  Lmap = new lmap_t();
  for (int i = 0; i < Rmap->size(); i++) {
    Lmap->emplace_back(new Literal(i, true), new Literal(i, false));
  }

  // main
  tu_set tus(&tseitin_unit_compare);
  gen_tu(pr->f, &tus);

  for (auto it = tus.begin(); it != tus.end(); it++) {
    std::cout << (*it).print() << std::endl;
  }

  CNF *result = tu_set_to_cnf(&tus);

  print_rmap();

  std::cout << *result << std::endl;
  return result;
}