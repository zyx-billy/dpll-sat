#include "formula.h"
#include "parser.h"
#include "cnf.h"
#include "tseitin.h"

#define INPUT_BUF_SIZE 256

int main() {
  char input[INPUT_BUF_SIZE];
  std::cin.getline(input, INPUT_BUF_SIZE);

  /***********
   * PARSING *
   ***********/
  parse_result *pr = parse_formula(input, input+INPUT_BUF_SIZE);

  if (!pr->has_error()) {
    std::cout << "Parse complete. No errors." << std::endl;
  } else {
    std::cout << "Parse Error:" << std::endl
      << "  position: " << (pr->error_char_pos - input) << std::endl
      << "  found: " << *pr->error_char_pos << std::endl
      << "  expects: ";
    switch (pr->expects) {
      case 'e':
        std::cout << "an expression";
        break;
      case 'c':
        std::cout << "a binary operator";
        break;
      default:
        std::cout << "'" << pr->expects << "'";
    }
    std::cout << std::endl;
    return 0;
  }

  std::cout << std::endl <<"Parse result (disambiguated):" << std::endl << *(pr->f) << std::endl;

  std::cout << std::endl << "Parse tree:" << std::endl;
  pr->f->print_tree("");

  vmap_t *Vmap = pr->Vmap;
  rmap_t *Rmap = pr->Rmap;

  /*************************
   * TSEITIN CNF TRANSFORM *
   *************************/
  CNF *cnf = tseitin_transform(pr->f, Vmap, Rmap);

  std::cout << std::endl << "Internal variable mapping:" << std::endl;
  print_rmap(Rmap);

  std::cout << std::endl << "Final CNF:" << std::endl << *cnf << std::endl;

  return 0;
}