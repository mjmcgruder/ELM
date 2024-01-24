/* ELM                                                                        */
/* Copyright (C) 2024  Miles McGruder                                         */
/*                                                                            */
/* This program is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by       */
/* the Free Software Foundation, either version 3 of the License, or          */
/* (at your option) any later version.                                        */
/*                                                                            */
/* This program is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of             */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              */
/* GNU General Public License for more details.                               */
/*                                                                            */
/* You should have received a copy of the GNU General Public License          */
/* along with this program.  If not, see <https://www.gnu.org/licenses/>.     */

#ifndef BASIS
#define BASIS

void split3(const in uint i, const in uint N, 
            out uint ix, out uint iy, out uint iz)
{
  iz = i / (N * N);           // truncation intentional
  iy = (i - N * N * iz) / N;  // truncation intentional
  ix = i - (N * N * iz) - (N * iy);
}

/* generic Lagrange functions */

float lagrange1d(const in uint i, const in uint p, const in float x)
{
  float xj, eval = 1.;
  const float xi = float(i) / float(p);
  for (uint j = 0; j < p + 1; ++j)
  {
    if (i != j)
    {
      xj = float(j) / float(p);
      eval *= (x - xj) / (xi - xj);
    }
  }
  return eval;
}

float lagrange3d(const in uint bi, const in uint p, const in vec3 pos)
{
  uint bix, biy, biz;
  split3(bi, p + 1, bix, biy, biz);

  return lagrange1d(bix, p, pos.x) * lagrange1d(biy, p, pos.y) *
         lagrange1d(biz, p, pos.z);
}

/* hard coded Lagrange functions */

const uint max_order = 3;
const uint mop1      = max_order + 1;

void lagrange_lin0(const in float x, out float phi[mop1])
{
  phi[0] = 1.;
}
void dlagrange_lin0(const in float x, out float dphi[mop1])
{
  dphi[0] = 0.;
}

void lagrange_lin1(const in float x, out float phi[mop1])
{
  phi[0] = 1. - x;
  phi[1] = x;
}
void dlagrange_lin1(const in float x, out float dphi[mop1])
{
  dphi[0] = -1.;
  dphi[1] = +1.;
}

void lagrange_lin2(const in float x, out float phi[mop1])
{
  phi[0] = +2. * x * x - 3. * x + 1.;
  phi[1] = -4. * x * x + 4. * x;
  phi[2] = +2. * x * x - 1. * x;
}
void dlagrange_lin2(const in float x, out float dphi[mop1])
{
  dphi[0] = +4. * x - 3.;
  dphi[1] = -8. * x + 4.;
  dphi[2] = +4. * x - 1.;
}

void lagrange_lin3(const in float x, out float phi[mop1])
{
  phi[0] = -4.5 * x * x * x + 9. * x * x - 5.5 * x + 1.;
  phi[1] = +4.5 * x * (x - 1.) * (3. * x - 2.);
  phi[2] = x * (-13.5 * x * x + 18. * x - 4.5);
  phi[3] = x * (4.5 * x * x - 4.5 * x + 1.);
}
void dlagrange_lin3(const in float x, out float dphi[mop1])
{
  dphi[0] = -13.5 * x * x + 18. * x - 5.5;
  dphi[1] = +40.5 * x * x - 45. * x + 9. ;
  dphi[2] = -40.5 * x * x + 36. * x - 4.5;
  dphi[3] = +13.5 * x * x -  9. * x + 1. ;
}

void lagrange_lin(const in uint p, const in float x, out float phi[mop1])
{
  switch (p)
  {
    case 0:
      lagrange_lin0(x, phi);
      break;
    case 1:
      lagrange_lin1(x, phi);
      break;
    case 2:
      lagrange_lin2(x, phi);
      break;
    case 3:
      lagrange_lin3(x, phi);
      break;
  }
}

void dlagrange_lin(const in uint p, const in float x, out float dphi[mop1])
{
  switch (p)
  {
    case 0:
      dlagrange_lin0(x, dphi);
      break;
    case 1:
      dlagrange_lin1(x, dphi);
      break;
    case 2:
      dlagrange_lin2(x, dphi);
      break;
    case 3:
      dlagrange_lin3(x, dphi);
      break;
  }
}

#endif
