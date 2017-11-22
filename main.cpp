#include "formula.h"
#include "parser.h"
#include "cnf.h"
#include "tseitin.h"

#define INPUT_BUF_SIZE 256

int main() {
  char input[INPUT_BUF_SIZE];
  std::cin.getline(input, INPUT_BUF_SIZE);

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

  std::cout << "Parse result:" << std::endl << *(pr->f) << std::endl;

  std::cout << "Parse tree:" << std::endl;
  pr->f->print_tree("");

  tseitin_transform(pr);

  return 0;
}