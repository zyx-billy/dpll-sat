# naive verifier that generates random cnf formula
# if the result is SAT, it can check if it's correct.
# if the result is NON-SAT, cannot verify.

import sys
import random
import subprocess

def verify(formula, answer):
  for clause in formula:
    is_good = False
    for literal in clause:
      truthness = True
      var = literal
      if literal[0] == '!':
        truthness = False
        var = literal[1:]
      if var not in answer:
        print "Unrecognized variable: ", var
        return
      if answer[var] == truthness:
        is_good = True
        break
    if not is_good:
      print "Wrong!"
      print clause
      return
  print "Correct!"

def random_formula():
  # can configure the following params
  max_vars = 20
  max_clauses = 5
  max_literals = 5

  # num variables
  num_vars = random.randint(2, max_vars-1)

  # num clauses
  num_clauses = random.randint(1, max_clauses)

  formula = []
  for c in range(num_clauses):
    # num literals
    num_literals = random.randint(1, max_literals)
    clause = []
    for l in range(num_literals):
      lit = ''
      # is negated
      if random.randint(0,1) == 0:
        lit += '!'
      lit += "%d" % random.randint(0, num_vars-1)
      clause.append(lit)
    formula.append(clause)

  return formula

def formula_to_string(formula):
  result = ''
  for clause in formula:
    result += '('
    result += clause[0]

    if len(clause) > 1:
      for literal in clause:
        result += "|%s" % literal

    result += ')&'

  return result[:-1]

if __name__ == '__main__':
  
  formula = random_formula()
  print formula
  print formula_to_string(formula)

  p = subprocess.Popen(["./bin/sat","-qq"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
  output = p.communicate(input=formula_to_string(formula))[0]

  print output

  answer = {}
  is_sat = True
  seen_result = False
  for line in output.split('\n'):
    line = line.strip()
    if line == 'SAT':
      seen_result = True
      continue
    if line == 'NON-SAT':
      is_sat = False
      break
    if not seen_result:
      continue
    
    if line == '':
      break;

    stuff = line.split(': ')
    if (len(stuff) != 2):
      print "read dict error"
      exit(0)
    if (stuff[1] == '1'):
      answer[stuff[0]] = True
    else:
      answer[stuff[0]] = False

  print answer

  if is_sat:
    verify(formula, answer)