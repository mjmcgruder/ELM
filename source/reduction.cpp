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


#include "pipeline.cpp"


enum struct redop
{
  sum,
  min,
  max,
};


template<typename T>
T redop_sum(T a, T b)
{
  return a + b;
}


template<typename T>
T redop_min(T a, T b)
{
  return (a < b) ? a : b;
}


template<typename T>
T redop_max(T a, T b)
{
  return (a > b) ? a : b;
}


template<typename T>
T reduce(redop op, T a, T b)
{
  switch (op)
  {
    case redop::sum:
      return redop_sum(a, b);
      break;
    case redop::min:
      return redop_min(a, b);
      break;
    case redop::max:
      return redop_max(a, b);
      break;
  }
}


float reduce_nullval(redop op)
{
  switch (op)
  {
    case redop::sum:
      return 0.f;
      break;
    case redop::min:
      return FLT_MAX;
      break;
    case redop::max:
      return FLT_MIN;
      break;
  }
}


struct reduction_workspace
{
  redop              optype;
  compute_pipeline&  op;

  u32                nblock;
  std::vector<float> h_dst;
  dbuffer<float>     d_dst;

  reduction_workspace(compute_pipeline& op, u32 src_size, u32 blk_size);
};


reduction_workspace::reduction_workspace(compute_pipeline& op_, u32 src_size,
                                         u32 blk_size) :
op(op_)
{
  nblock = (src_size + (blk_size - 1)) / blk_size;
  h_dst  = std::vector<float>(nblock);
  d_dst  = dbuffer<float>(nblock);
  dmalloc(d_dst);
}


float float_reduce(dbuffer<float>& arr, dbuffer<u32>& size,
                   reduction_workspace& worksp)
{
  worksp.op.dset.update(size,         0);
  worksp.op.dset.update(arr,          1);
  worksp.op.dset.update(worksp.d_dst, 2);
  worksp.op.run(worksp.nblock, 1, 1);

  memcpy_dtoh(worksp.h_dst.data(), worksp.d_dst);

  float result = reduce_nullval(worksp.optype);
  for (usize i = 0; i < worksp.nblock; ++i)
  {
    result = reduce(worksp.optype, result, worksp.h_dst[i]);
  }

  return result;
}
