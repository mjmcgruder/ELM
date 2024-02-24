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


// List of supported reduction operations.
// Each supported operation needs a CPU side binary operator and a GPU kernel
// for each data type. This is because, at least at the moment, reductions are
// carried out block-wise on the GPU and finished CPU side. A better solution
// would probably be to use GPU atomics to finalize the reduction but some
// Vulkan implementations do not support atomics.


enum struct redop
{
  sum,
  min,
  max,
};


// CPU side reduction binary operators.


template<typename T> T redop_sum(T a, T b) { return a + b;           }
template<typename T> T redop_min(T a, T b) { return (a < b) ? a : b; }
template<typename T> T redop_max(T a, T b) { return (a > b) ? a : b; }


// Template and specializations coordinating reduction operations between CPU
// and GPU. Keeping this information together should minimize the necessary code
// and prevent mistakes. The templating ensures these values match at compile
// time as long as the template specialization is written correctly. These are
// intended to provide information about reduction operations in the style of
// std::numeric_limits.


template<redop op, typename dtype>
class redop_info
{
  static dtype       reduce(dtype a, dtype b);
  static std::string shader_file();
  static dtype       nullval();
};


template<>
class redop_info<redop::min, float>
{
  static float       reduce(float a, float b) { return redop_min(a, b);     }
  static std::string shader_file()            { return "reduction_min.spv"; }
  static float       nullval()                { return FLT_MAX;             }
};


template<>
class redop_info<redop::max, float>
{
  static float       reduce(float a, float b) { return redop_max(a, b);     }
  static std::string shader_file()            { return "reduction_max.spv"; }
  static float       nullval()                { return FLT_MIN;             }
};


template<>
class redop_info<redop::sum, float>
{
  static float       reduce(float a, float b) { return redop_sum(a, b);     }
  static std::string shader_file()            { return "reduction_sum.spv"; }
  static float       nullval()                { return 0.;                  }
};


// Workspace for reduction operations.
// If repeated reduction operations are performed, it wouldn't make sense to
// keep reallocating supporting structures. These structures are the reduction
// kernel code itself, the reduced GPU side buffer, and the reduced CPU side
// buffer. An object of this type is passed to the final reduction routine to
// enable simple reuse of pre-allocated values.


template<redop op, typename dtype>
struct reduction_workspace
{
  compute_pipeline shdr;

  u32                nblock;
  std::vector<float> h_dst;
  dbuffer<float>     d_dst;

  reduction_workspace(u32 src_size, u32 blk_size);
};


template<redop op, typename dtype>
reduction_workspace<op, dtype>::reduction_workspace(u32 src_size,
                                                    u32 blk_size) :
shdr(SHADER_DIR + redop_info<op, dtype>::shader_file())
{
  nblock = (src_size + (blk_size - 1)) / blk_size;
  h_dst  = std::vector<float>(nblock);
  d_dst  = dbuffer<float>(nblock);
  dmalloc(d_dst);
}


// The reduction operator.
// Performs a reduction using the template specified operation on a GPU
// allocated array of the template specified data type. Takes a workspace
// structure to avoid repeated reallocations in certain scenarios. Reduction is
// performed block-wise on the GPU and transferred to the CPU for final
// processing.


template<redop op, typename dtype>
dtype reduce(dbuffer<dtype>& arr, dbuffer<u32>& size,
             reduction_workspace<op, dtype>& worksp)
{
  worksp.op.dset.update(size,         0);
  worksp.op.dset.update(arr,          1);
  worksp.op.dset.update(worksp.d_dst, 2);
  worksp.op.run(worksp.nblock, 1, 1);

  memcpy_dtoh(worksp.h_dst.data(), worksp.d_dst);

  float result = redop_info<op, dtype>::nullval();
  for (usize i = 0; i < worksp.nblock; ++i)
  {
    result = redop_info<op, dtype>::reduce(result, worksp.h_dst[i]);
  }

  return result;
}
