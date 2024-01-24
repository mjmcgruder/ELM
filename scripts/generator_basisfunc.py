import sympy as sy

def lagrange1d(i, p, x):
  xi   = i / p
  eval = 1.
  for j in range(p + 1):
    if i != j:
      xj    = j / p
      eval *= (x - xj) / (xi - xj)
  return eval

x = sy.Symbol("x")

for p in range(1,4):
  print(f"p: {p}")
  for bfunc in range(p + 1):
    print(f"  func: {bfunc}")
    eval = lagrange1d(bfunc, p, x)
    print("   ", sy.simplify(eval))
    print("   ", sy.simplify(sy.diff(eval, x)))
  print("")
