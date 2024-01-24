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

#pragma once


#include <string>
#include <vector>
#include <unordered_map>

#include "basic_types.cpp"


// element type and helper functions

enum struct elem_type: u32
{
  hex = 0,
};

u32 elem_dim(elem_type type);
u32 elem_nbf(elem_type type, u32 p);


// state type and helper functions

enum struct state_type
{
  scalar,
  conservative,
};

u32 state_rank(state_type type);


// render field

struct render_field
{
  state_type         type;
  std::vector<float> state;

  render_field();
  render_field(state_type type_, u32 nelem, u32 nbf);
};


// dg solution

struct dg_solution
{
  u32 p;
  u32 q;
  u32 nelem;

  elem_type etype;
  u32       dim;   // should remain consistent with etype
  u32       nbfp;  // *
  u32       nbfq;  // *

  float gamma;

  std::vector<float>                            nodes;
  std::unordered_map<std::string, render_field> fields;

  dg_solution();
  dg_solution(elem_type type_, u32 p_, u32 q_, u32 nelem, u32 noutput,
              float gamma);

  render_field& add_field(const std::string& key, state_type stype);
  float* node(u32 e, u32 n);
};


// i/o

dg_solution read_dg_solution(const char* fname);


/* IMPLEMENTATION ----------------------------------------------------------- */


u32 elem_dim(elem_type type)
{
  u32 dim;

  switch (type)
  {
    case elem_type::hex:
    {
      dim = 3;
    }
    break;
  }

  return dim;
}

u32 elem_nbf(elem_type type, u32 p)
{
  u32 nbf;

  switch (type)
  {
    case elem_type::hex:
    {
      u32 pp1 = p + 1;
      nbf     = pp1 * pp1 * pp1;
    }
    break;
  }

  return nbf;
}


u32 state_rank(state_type type)
{
  u32 rank;

  switch (type)
  {
    case state_type::scalar:
    {
      rank = 1;
    }
    break;

    case state_type::conservative:
    {
      rank = 5;
    }
    break;
  }

  return rank;
}


render_field::render_field() : type(state_type::scalar), state()
{}

render_field::render_field(state_type type_, u32 nelem, u32 nbf) :
type(type_), state(nelem * nbf * state_rank(type))
{}


dg_solution::dg_solution() :
p(0),
q(1),
nelem(0),
etype(elem_type::hex),
dim(elem_dim(etype)),
nbfp(elem_nbf(etype, p)),
nbfq(elem_nbf(etype, q)),
gamma(1.4),
nodes(),
fields()
{}

dg_solution::dg_solution(elem_type etype_, u32 p_, u32 q_, u32 nelem_,
                         u32 noutput, float gamma_) :
p(p_),
q(q_),
nelem(nelem_),
etype(etype_),
dim(elem_dim(etype)),
nbfp(elem_nbf(etype, p)),
nbfq(elem_nbf(etype, q)),
gamma(gamma_),
nodes(nelem * nbfq * dim),
fields()
{}

render_field& dg_solution::add_field(const std::string& key, state_type stype)
{
  auto insert_result =
  fields.emplace(key, render_field(stype, nelem, nbfp));

  return (insert_result.first)->second;
}

float* dg_solution::node(u32 e, u32 n)
{
  return &nodes[(nbfq * e + n) * dim];
}


#define iochk(f, num)                  \
  {                                    \
    if ((f) != (num))                  \
    {                                  \
      /*errout("i/o operation failed!");*/ \
    }                                  \
  }


dg_solution read_dg_solution(const char* fname)
{
  u64 nx, ny, nz, p, q, time_step;
  s64 bounds[6];
  float gamma = 1.4;

  FILE* fstr = fopen(fname, "rb");
  // if (!fstr)
  //   errout("failed to open file \"%s\" for state read!", fname);

  /* metadata */

  // time step
  iochk(fread(&time_step, sizeof(time_step), 1, fstr), 1);

  // dimensions
  iochk(fread(&nx, sizeof(nx), 1, fstr), 1);
  iochk(fread(&ny, sizeof(ny), 1, fstr), 1);
  iochk(fread(&nz, sizeof(nz), 1, fstr), 1);

  // orders
  iochk(fread(&p, sizeof(p), 1, fstr), 1);
  iochk(fread(&q, sizeof(q), 1, fstr), 1);

  // boundary types
  for (usize i = 0; i < 6; ++i)
  {
    iochk(fread(&bounds[i], sizeof(bounds[0]), 1, fstr), 1);
  }

  // output count
  u64 noutput;
  iochk(fread(&noutput, sizeof(noutput), 1, fstr), 1);

  /* construct solution geometry and state (only need metadata for sizing) */

  dg_solution solution(elem_type::hex, p, q, nx * ny * nz, noutput, gamma);

  /* read geometry nodes */

  for (usize i = 0; i < solution.nodes.size(); ++i)
  {
    double data;
    iochk(fread(&data, sizeof(double), 1, fstr), 1);
    solution.nodes[i] = (float)data;
  }

  /* read each output */

  for (usize oi = 0; oi < noutput; ++oi)
  {
    u64 key_len;
    iochk(fread(&key_len, sizeof(key_len), 1, fstr), 1);
    char key_arr[1024];
    iochk(fread(key_arr, sizeof(char), key_len, fstr), key_len);
    key_arr[key_len] = '\0';

    render_field& field = solution.add_field(key_arr, state_type::conservative);

    for (usize i = 0; i < field.state.size(); ++i)
    {
      double data;
      iochk(fread(&data, sizeof(double), 1, fstr), 1);
      field.state[i] = data;
    }
  }

  fclose(fstr);

  return solution;
}
