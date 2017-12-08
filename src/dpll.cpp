#include <vector>
#include <set>
#include <iostream>

#include "dpll.h"
#include "parser.h"

std::ostream *output;

// variable interpretation
enum vinterp {vtrue, vfalse, vundef};

void print_vinterp(vinterp vi) {
  switch (vi) {
    case vtrue:
      std::cout << "vtrue";
      break;
    case vfalse:
      std::cout << "vfalse";
      break;
    case vundef:
      std::cout << "vundef";
      break;
  }
}

// A full interpretation (maps variable to a variable interp)
struct Interp {
  std::vector<vinterp> asmts;

  Interp(size_t size) {
    asmts.reserve(size);
    for (size_t i = 0; i < size; i++) {
      asmts.push_back(vundef);
    }
  }

  vinterp satisfies(Literal *L) {
    switch (asmts[L->v]) {
      case vtrue:
        return L->is_true ? vtrue : vfalse;
      case vfalse:
        return L->is_true ? vfalse : vtrue;
      case vundef:
        return vundef;
    }
  }

  void update(int var, vinterp vi) {
    asmts[var] = vi;
  }
  void update(int var, bool vi) {
    asmts[var] = vi ? vtrue : vfalse;
  }

  void quick_print() {
    std::cout << "[" << asmts.size() << "] ";
    for (int i = 0; i < asmts.size(); i++) {
      print_vinterp(asmts[i]);
      std::cout << " ";
    }
    std::cout << std::endl;
  }
};

// clause interpretation
enum cinterp {ctrue, cfalse, cunit, cundef};

// dpll-related
struct dpll_propagation {
  int prop_var;
  bool prop_asmt;

  dpll_propagation(int var, bool asmt) :
    prop_var(var), prop_asmt(asmt) {}

  // comparator for use by std::set
  bool operator<(const dpll_propagation &rhs) const {
    return prop_var < rhs.prop_var;
  }
};

struct dpll_decision {
  int decision_var;
  bool decision_asmt;
  bool has_been_flipped;
  std::set<dpll_propagation> props;

  dpll_decision(int var, bool asmt) :
    decision_var(var), decision_asmt(asmt), has_been_flipped(false) {}

  void add_propagation(int prop_var, bool prop_asmt) {
    props.emplace(prop_var, prop_asmt);
  }

  void flip_decision() {
    decision_asmt = !decision_asmt;
    has_been_flipped = true;
    props.clear();
  }
};

class dpll_logger {
  std::ostream &output;
  std::string prefix;

  void indent() {
    prefix += "  ";
  }

  void dedent() { 
    if (prefix.length() < 2) return;
    prefix.pop_back();
    prefix.pop_back();
  }

  void write_prefix() {
    output << prefix;
  }

  void write_assignment(int var, bool asmt) {
    output << var << " <- ";
    if (asmt) output << "true ";
    else output << "false ";
  }

  void write_decision(dpll_decision &decision) {
    write_assignment(decision.decision_var, decision.decision_asmt);
  }

  void write_clause(Clause *C) {
    output << *C << " ";
  }

public:
  dpll_logger(std::ostream &o) : prefix(""), output(o) {}

  void log_decision(dpll_decision &decision) {
    write_prefix();
    output << "Decide ";
    write_decision(decision);
    output << std::endl;
    indent();
  }

  void log_redecision(dpll_decision &decision) {
    write_prefix();
    output << "Re-decide ";
    write_decision(decision);
    output << std::endl;
    indent();
  }

  void log_propagation(int var, bool var_asmt, Clause *C) {
    write_prefix();
    output << "Propagate ";
    write_assignment(var, var_asmt);
    write_clause(C);
    output << std::endl;
  }

  void log_backtrack(Clause *C) {
    write_prefix();
    output << "Backtrack ";
    if (C) write_clause(C);
    output << std::endl;
    dedent();
  }
};

dpll_logger *Logger;

// determine the interpretation of a disjunctive clause
cinterp interpret_clause(Clause *C, Interp *I, int *undef_var, bool *undef_sat_interp) {
  bool seen_undef = false;
  for (auto it = C->literals.begin(); it != C->literals.end(); it++) {
    switch (I->satisfies(*it)) {
      case vtrue:
        return ctrue;
      case vundef:
        if (seen_undef) return cundef;

        seen_undef = true;
        if (undef_var) *undef_var = (*it)->v;
        if (undef_sat_interp) *undef_sat_interp = (*it)->is_true ? true : false;
    }
  }

  if (!seen_undef) return cfalse;
  return cunit;
}

vinterp interpret_cnf(CNF *cnf, Interp *I, Clause **offending_clause) {
  bool seen_undef = false;

  for (auto C = cnf->clauses.begin(); C != cnf->clauses.end(); C++) {
    switch (interpret_clause(*C, I, nullptr, nullptr)) {
      case cfalse:
        if (offending_clause) *offending_clause = *C;
        return vfalse;
      case cunit: // fall thru
      case cundef:
        seen_undef = true;
    }
  }

  if (seen_undef) return vundef;
  return vtrue;
}

Clause *find_unit_clause(CNF *cnf, Interp *I, int *unit_var, bool *unit_interp) {
  for (auto C = cnf->clauses.begin(); C != cnf->clauses.end(); C++) {
    if (interpret_clause(*C, I, unit_var, unit_interp) == cunit) {
      return *C;
    }
  }
  return nullptr;
}

// attempts to unit propagate until it cannot
// returns true if completed without problems (result could be sat or undef)
// returns false if resulting function is non-sat and it backtracked
bool unit_propagate_all(CNF *cnf, Interp *I, dpll_decision &dec) {
  int unit_var;
  bool unit_interp;
  bool has_conflict = false;
  Clause *target_clause;
  
  target_clause = find_unit_clause(cnf, I, &unit_var, &unit_interp);
  if (!target_clause) {
    // failed to find unit, implies no conflicts
    return true;
  }

  Logger->log_propagation(unit_var, unit_interp, target_clause);
  I->update(unit_var, unit_interp);
  if (interpret_cnf(cnf, I, &target_clause) == vfalse) {
    Logger->log_backtrack(target_clause);
    has_conflict = true;
  } else if (!unit_propagate_all(cnf, I, dec)) {
    has_conflict = true;
  }

  if (has_conflict) {
    // found conflict, revert assignment
    I->update(unit_var, vundef);
    return false;
  } else {
    // no conflict, add self to decision's prop list
    dec.add_propagation(unit_var, unit_interp);
  }

  return true;
}

// returns true if decided a variable
// returns false if no undef clause exists
bool decide(CNF *cnf, Interp *I, int *undef_var, bool *undef_sat_interp) {
  for (auto C = cnf->clauses.begin(); C != cnf->clauses.end(); C++) {
    if (interpret_clause(*C, I, undef_var, undef_sat_interp) == cundef) {
      return true;
    }
  }

  return false;
}

// convert from a satisfying interpretation to assignment
void sat_interp_to_assignment(Interp *I, assignment &asmt) {
  int num_vars = I->asmts.size();
  asmt.reserve(num_vars);

  for (int i = 0; i < num_vars; i++) {
    switch (I->asmts[i]) {
      case vtrue:
        asmt.push_back(true);
        break;
      case vfalse:
        asmt.push_back(false);
        break;
      case vundef:
        asmt.push_back(true);
        break;
    }
  }
}

bool dpll_main(CNF *cnf, Interp *I) {
  int undef_var;
  bool undef_sat_interp;
  bool can_decide, can_propagate;
  bool is_sat = true;

  std::vector<dpll_decision> decisions;
  // kick start
  decisions.emplace_back(-1, true);

  while (true) {
    // I->quick_print();
    can_propagate = unit_propagate_all(cnf, I, decisions.back());

    if (!can_propagate) {
      // has conflict
      bool backtrack_success = false;
      // find the last decision that has not yet been flipped and flip it
      while (decisions.size() > 1) {
        dpll_decision &last_decision = decisions.back();
        if (!last_decision.has_been_flipped) {
          last_decision.flip_decision();
          backtrack_success = true;
          I->update(last_decision.decision_var, last_decision.decision_asmt);
          Logger->log_redecision(decisions.back());
          break;
        } else {
          Logger->log_backtrack(nullptr);
          I->update(last_decision.decision_var, vundef);
          decisions.pop_back();
        }
      }

      if (backtrack_success) {
        continue;
      } else {
        // backtrack fail, no sat assignment
        return false;
      }
    }
    
    can_decide = decide(cnf, I, &undef_var, &undef_sat_interp);
    if (!can_decide) break;

    // make the decision
    // undef_sat_interp = true; // uncomment this line to always decide true first
    decisions.emplace_back(undef_var, undef_sat_interp);
    I->update(undef_var, undef_sat_interp);
    Logger->log_decision(decisions.back());
  }

  return true;
}

bool dpll_sat(CNF *cnf, int num_vars, assignment &result, std::ostream &output_pref) {
  output = &output_pref;
  Logger = new dpll_logger(*output);

  Interp *I = new Interp(num_vars);

  bool is_sat = dpll_main(cnf, I);

  if (is_sat) {
    sat_interp_to_assignment(I, result);
  }
  
  delete I;
  delete Logger;
  return is_sat;
}

void print_assignment(assignment &asmt, rmap_t* Rmap, int num_vars) {
  for (int i = 0; i < num_vars; i++) {
    std::cout << (*Rmap)[i] << ": " << asmt[i] << std::endl;
  }
}