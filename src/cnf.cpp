#include <vector>

#include "cnf.h"

template <class T>
void merge_vectors(std::vector<T> &A, std::vector<T> &B) {
  A.reserve(A.size() + B.size());
  A.insert(A.end(), B.begin(), B.end());
}

// the input args can no longer be used after this
CNF *merge_cnf(CNF *CNF_A, CNF *CNF_B) {
  merge_vectors(CNF_A->clauses, CNF_B->clauses);
  delete CNF_B;
  return CNF_A;
}

Clause *merge_clause(Clause *C_A, Clause *C_B) {
  merge_vectors(C_A->literals, C_B->literals);
  delete C_B;
  return C_A;
}