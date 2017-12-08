# DPLL_SAT
A dpll-based satisfiability solver focused on traceability.

The solver consists of three parts:
1) Disambiguating parser that accepts arbitrary input
2) Tseitin transformer to convert formula into CNF
3) DPLL-based SAT solver

# Build Instructions
This project uses standard C++ with no external libraries.
To build on a standard linux machine, run `make`.

The build result will be an executable: bin/sat

# Usage Instructions
## Inputs
Input is given as a string via stdin. Only one formula at a time is allowed.

This SAT solver accepts arbitrary logical formula input. The supported logical operations are:

|operator|symbol|
| ---  | --- |
| and  |  &  |
| or   |  \| |
| not  |  !  |
| imply|  -> |
| equiv| <-> |

Logical not has the highest operator precedence. All others have the same level of precedence. As by convention, parens can be used to override operator precedence.

Ambiguous inputs are allowed. The parser will perform left association. The disambiguated version of the input formula can be displayed, as well as the complete parse tree (with option -t).

Note - variable renaming:
To minimize the size of intermediate output, variables will be renamed using 0-indexed integers. This is especially helpful when performing Tseitin transform, as we will be creating new variables in the process.
The final output, however, will display the satisfying assignment using the original variable names. Only the intermediate steps will use replaced variables. A table mapping original variable names to internal variable names can be displayed.

## Options
  -t  Display parse tree (hidden by default).
      Parse tree is drawn sideways (increasing depth to the right).

  -q  Quieter output. Displays only the final results of each step.
  
  -qq Quietest output. Displays only the final sat/non-sat result.

# Project Structure
```
./
  bin/   - output executable
  build/ - build temp output
  src/   - source files
  test/  - examples of test cases
  tools/ - misc tools
    verifier.py - auto verifier script
```
# Author
Billy Zhu (yuxiangz)
