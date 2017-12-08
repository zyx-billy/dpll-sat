#include <unistd.h>
#include <ostream>

#include "formula.h"
#include "parser.h"
#include "cnf.h"
#include "tseitin.h"
#include "dpll.h"

#define INPUT_BUF_SIZE 256

class Logger {
  // a null buffer for the null ostream
  class null_buffer : public std::streambuf {
  public:
    int overflow(int c) {return c;}
  };

  null_buffer nb;
  std::ostream null_stream;
  int quietness;

public:
  Logger() : nb(), null_stream(&nb), quietness(0) {}

  void set_quietness(int q) {
    if (q > 2) q = 2;
    quietness = q;
  }

  std::ostream &log(int level) {
    if (level <= quietness) {
      return std::cout;
    }
    return null_stream;
  }
};


int main(int argc, char **argv) {

  int quietness = 2;
  bool show_parse_tree = false;

  int c;
  while ((c = getopt(argc, argv, "qt")) != -1) {
    switch (c) {
      case 't':
        show_parse_tree = true;
        break;
      case 'q':
        if (quietness > 0) quietness--;
        break;
    }
  }

  /* logging levels:
   * 0: always display
   * 1: step result
   * 2: misc info
   */
  Logger logger;
  logger.set_quietness(quietness);

  char input[INPUT_BUF_SIZE];
  std::cin.getline(input, INPUT_BUF_SIZE);

  /***********
   * PARSING *
   ***********/
  parse_result *pr = parse_formula(input, input+INPUT_BUF_SIZE);

  if (!pr->has_error()) {
    logger.log(2) << "Parse complete. No errors." << std::endl;
  } else {
    logger.log(0) << "Parse Error:" << std::endl
      << "  position: " << (pr->error_char_pos - input) << std::endl
      << "  found: " << *pr->error_char_pos << std::endl
      << "  expects: ";
    switch (pr->expects) {
      case 'e':
        logger.log(0) << "an expression";
        break;
      case 'c':
        logger.log(0) << "a binary operator";
        break;
      default:
        logger.log(0) << "'" << pr->expects << "'";
    }
    logger.log(0) << std::endl;
    return 0;
  }

  logger.log(1) << std::endl <<"Parse result (disambiguated):" << std::endl << *(pr->f) << std::endl;

  if (show_parse_tree) {
    logger.log(0) << std::endl << "Parse tree:" << std::endl;
    pr->f->print_tree("");
  }

  vmap_t *Vmap = pr->Vmap;
  rmap_t *Rmap = pr->Rmap;
  int num_primitive_vars = Rmap->size();

  /*************************
   * TSEITIN CNF TRANSFORM *
   *************************/
  tseitin_init(Vmap, Rmap);

  CNF *cnf;
  // attempt direct transform
  cnf = parse_into_cnf(pr->f);
  if (cnf) {
    logger.log(1) << std::endl << "Recognized input as CNF. No tseitin tranform needed" << std::endl;
  } else {
    logger.log(1) << std::endl << "Did not recognize input as CNF. Performing tseitin transform" << std::endl;
    // if cannot direct transform, perform full tseitin
    cnf = tseitin_transform(pr->f);
  }

  logger.log(2) << std::endl << "Internal variable mapping:" << std::endl;
  print_rmap(Rmap, logger.log(2));

  logger.log(1) << std::endl << "Final CNF:" << std::endl << *cnf << std::endl;

  /************
   * DPLL SAT *
   ************/
  assignment result;

  logger.log(2) << std::endl << "Running DPLL with " << Rmap->size()
            << " variables and "<< cnf->clauses.size()
            << " clauses..." << std::endl;
  bool is_sat = dpll_sat(cnf, Rmap->size(), result, logger.log(2));

  if (is_sat) {
    logger.log(0) << std::endl << "SAT" << std::endl;
    print_assignment(result, Rmap, num_primitive_vars);
  } else {
    logger.log(0) << std::endl << "NON-SAT" << std::endl;
  }

  return 0;
}