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


#include <cstdlib>
#include <cstdint>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>


typedef size_t   usize;
typedef int8_t   s8;
typedef int32_t  s32;
typedef int64_t  s64;
typedef uint8_t  u8;
typedef uint32_t u32;
typedef uint64_t u64;


enum struct raycast_mode
{
  surface,
  slice,
  isosurface
};


enum struct output_type: int
{
  mach,
  rho,
  u,
  v,
  w,
  rhoE,
};


struct vertex
{
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 ref;

  static VkVertexInputBindingDescription binding_description()
  {
    VkVertexInputBindingDescription description{};
    description.binding   = 0;
    description.stride    = sizeof(vertex);
    description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return description;
  }

  static void attribute_descriptions(
  VkVertexInputAttributeDescription descriptions[3])
  {
    descriptions[0].binding  = 0;
    descriptions[0].location = 0;
    descriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[0].offset   = offsetof(vertex, pos);
    descriptions[1].binding  = 0;
    descriptions[1].location = 1;
    descriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[1].offset   = offsetof(vertex, color);
    descriptions[2].binding  = 0;
    descriptions[2].location = 2;
    descriptions[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
    descriptions[2].offset   = offsetof(vertex, ref);
  }
};


struct aabb
{
  alignas(16) glm::vec3 l = glm::vec3(+FLT_MAX);  // lower corner
  alignas(16) glm::vec3 h = glm::vec3(-FLT_MAX);  // upper corner
};


struct kdnode
{
  aabb  bbox;

  int   axis    = -1;
  float split   = 0.;
  int   parent  = -1;
  int   child_r = -1;

  int   offset  = -1;
  u32   count   = 0;
};


template<typename T>
T clamp(T v, T min, T max) {
  if (v < min) {
    return min;
  } else if (max < v) {
    return max;
  } else {
    return v;
  }
}


template<typename T>
T max(T a, T b)
{
  if (a > b)
    return a;
  else
    return b;
}


template<typename T>
T min(T a, T b)
{
  if (a < b)
    return a;
  else
    return b;
}
