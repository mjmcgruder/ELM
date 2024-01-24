import math

import scipy
import numpy as np


max_pnt = 9


def lobatto_rule(n):

  x = np.empty(2 + n - 2)
  w = np.empty(x.size)

  Pnm1  = scipy.special.legendre(n - 1)
  dPnm1 = np.polyder(Pnm1)

  x[0]    = -1.
  x[-1]   = 1.
  x[1:-1] = np.sort(np.roots(dPnm1))

  for i in range(x.size):

    p    = np.polyval(Pnm1, x[i])
    w[i] = 2. / (n * (n - 1) * p * p)

  return x, w


print("\nLegendre polynomials evaluated at Gauss Lobatto quad points\n")


for npnt in range(3, max_pnt + 1, 2):

  max_deg     = (2 * npnt - 3) // 2
  print_width = int(math.log10(max_deg)) + 1

  print(f"{npnt} point rule")
  x, w = lobatto_rule(npnt)

  print(f"{'x':<{print_width}}", end=' ')
  for i in range(x.size):
    print(f"{x[i]:+.8e}", end=' ')
  print()
  print(f"{'w':<{print_width}}", end=' ')
  for i in range(x.size):
    print(f"{w[i]:+.8e}", end=' ')
  print()

  print("evaluated polynomials up to the degree to which we can fit a function")

  for deg in range(max_deg + 1):

    P = scipy.special.eval_legendre(deg, x)
  
    print(f"{deg:<{print_width}}", end=' ')
    for i in range(P.size):
      print(f"{P[i]:+.8e}", end=' ')
    print("")

  print()

print()
print("polynomial coefficients (lowest degree coefficient first)")

for deg in range(max_deg + 1):

  coefficients = scipy.special.legendre(deg).c
  print(f"{deg:<{print_width}}", end=' ')
  for c in reversed(coefficients):
    print(f"{c:+.8e}", end=' ')
  print()
