#include <vector>

#include "cnf.h"

// the input args can no longer be used after this
CNF *merge_cnf(CNF *CNF_A, CNF *CNF_B) {
  std::vector<Clause *> *A = &(CNF_A->clauses);
  std::vector<Clause *> *B = &(CNF_B->clauses);

  A->reserve(A->size() + B->size());
  A->insert(A->end(), B->begin(), B->end());
  delete CNF_B;
  return CNF_A;
}