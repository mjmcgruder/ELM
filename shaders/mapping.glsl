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


#ifndef SHDR_MAPPING
#define SHDR_MAPPING


#include "basis.glsl"


vec3 ref2glo(const in vec3 r_pos, const in uint elem, const in uint q)
{
  float phix[mop1], phiy[mop1], phiz[mop1];

  lagrange_lin(q, r_pos.x, phix);
  lagrange_lin(q, r_pos.y, phiy);
  lagrange_lin(q, r_pos.z, phiz);

  uint qp1   = q + 1;
  uint qp1p3 = qp1 * qp1 * qp1;

  vec3 g_pos = vec3(0.);
  for (uint iz = 0; iz < qp1; ++iz)
  {
    for (uint iy = 0; iy < qp1; ++iy)
    {
      for (uint ix = 0; ix < qp1; ++ix)
      {
        uint i    = qp1 * qp1 * iz + qp1 * iy + ix;
        vec3 node = vec3(nodes[3 * qp1p3 * elem + 3 * i + 0],
                         nodes[3 * qp1p3 * elem + 3 * i + 1],
                         nodes[3 * qp1p3 * elem + 3 * i + 2]);

        g_pos += node * (phix[ix] * phiy[iy] * phiz[iz]);
      }
    }
  }

  return g_pos;
}


void mapinfo(const in vec3 r_pos, const in int elem, const in uint q,
             out vec3 g_pos, out mat3 j)
{
  float phix[mop1],   phiy[mop1],   phiz[mop1];
  float phix_x[mop1], phiy_y[mop1], phiz_z[mop1];

  lagrange_lin(q, r_pos.x, phix);
  lagrange_lin(q, r_pos.y, phiy);
  lagrange_lin(q, r_pos.z, phiz);

  dlagrange_lin(q, r_pos.x, phix_x);
  dlagrange_lin(q, r_pos.y, phiy_y);
  dlagrange_lin(q, r_pos.z, phiz_z);

  uint qp1   = q + 1;
  uint qp1p3 = qp1 * qp1 * qp1;

  g_pos = vec3(0.);
  j     = mat3(0.);
  for (uint iz = 0; iz < qp1; ++iz)
  {
    for (uint iy = 0; iy < qp1; ++iy)
    {
      for (uint ix = 0; ix < qp1; ++ix)
      {
        uint i    = qp1 * qp1 * iz + qp1 * iy + ix;
        vec3 node = vec3(nodes[3 * qp1p3 * elem + 3 * i + 0],
                         nodes[3 * qp1p3 * elem + 3 * i + 1],
                         nodes[3 * qp1p3 * elem + 3 * i + 2]);

        g_pos += node * (phix[ix] * phiy[iy] * phiz[iz]);
        j     += outerProduct(node,
                              vec3(phix_x[ix] * phiy[iy]   * phiz[iz],
                                   phix[ix]   * phiy_y[iy] * phiz[iz],
                                   phix[ix]   * phiy[iy]   * phiz_z[iz]));
      }
    }
  }
}


void interp_state(const in vec3 r_pos, const in int elem, const in uint p, 
                  out float state[5])
{
  const uint pp1   = p + 1;
  const uint pp1p3 = pp1 * pp1 * pp1;

  float phix[mop1], phiy[mop1], phiz[mop1];

  lagrange_lin(p, r_pos.x, phix);
  lagrange_lin(p, r_pos.y, phiy);
  lagrange_lin(p, r_pos.z, phiz);

  state = float[5](0., 0., 0., 0., 0.);
  for (uint iz = 0; iz < pp1; ++iz)
  {
    for (uint iy = 0; iy < pp1; ++iy)
    {
      for (uint ix = 0; ix < pp1; ++ix)
      {
        uint i   = pp1 * pp1 * iz + pp1 * iy + ix;
        float bf = phix[ix] * phiy[iy] * phiz[iz];

        state[0] += U[pp1p3 * (5 * elem + 0) + i] * bf;
        state[1] += U[pp1p3 * (5 * elem + 1) + i] * bf;
        state[2] += U[pp1p3 * (5 * elem + 2) + i] * bf;
        state[3] += U[pp1p3 * (5 * elem + 3) + i] * bf;
        state[4] += U[pp1p3 * (5 * elem + 4) + i] * bf;
      }
    }
  }
}


void interp_state_grad(const in vec3 r_pos, const in int elem, const in uint p, 
                       out float state[5],
                       out float state_x[5], 
                       out float state_y[5], 
                       out float state_z[5])
{
  const uint pp1   = p + 1;
  const uint pp1p3 = pp1 * pp1 * pp1;

  float phix[mop1],   phiy[mop1],   phiz[mop1];
  float phix_x[mop1], phiy_y[mop1], phiz_z[mop1];

  lagrange_lin(p, r_pos.x, phix);
  lagrange_lin(p, r_pos.y, phiy);
  lagrange_lin(p, r_pos.z, phiz);

  dlagrange_lin(p, r_pos.x, phix_x);
  dlagrange_lin(p, r_pos.y, phiy_y);
  dlagrange_lin(p, r_pos.z, phiz_z);

  state   = float[5](0., 0., 0., 0., 0.);
  state_x = float[5](0., 0., 0., 0., 0.);
  state_y = float[5](0., 0., 0., 0., 0.);
  state_z = float[5](0., 0., 0., 0., 0.);
  for (uint iz = 0; iz < pp1; ++iz)
  {
    for (uint iy = 0; iy < pp1; ++iy)
    {
      for (uint ix = 0; ix < pp1; ++ix)
      {
        uint i     = pp1 * pp1 * iz + pp1 * iy + ix;

        float bf   = phix[ix] * phiy[iy] * phiz[iz];

        float bf_x = phix_x[ix] * phiy[iy] * phiz[iz];
        float bf_y = phix[ix] * phiy_y[iy] * phiz[iz];
        float bf_z = phix[ix] * phiy[iy] * phiz_z[iz];

        for (uint ir = 0; ir < 5; ++ir)
        {
          float coeff = U[pp1p3 * (5 * elem + ir) + i];

          state[ir]   += coeff * bf;

          state_x[ir] += coeff * bf_x;
          state_y[ir] += coeff * bf_y;
          state_z[ir] += coeff * bf_z;
        }
      }
    }
  }
}


#endif
