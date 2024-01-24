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


#ifndef INTERSECTION
#define INTERSECTION


#include "data_structures.glsl"


// Based on a pixel's screen position in normalized device coordinates, computes
// the appropriate ray based on the current model, view, projection transform.
// The final ray is in model coordinates so pre-computed geometry partitioning
// structures remain valid.
void find_ray(in vec4 ndc_pos, in mat4 view, in mat4 proj,
              out vec3 ro, out vec3 rd)
{
  mat4 iview = inverse(view);
  mat4 iproj = inverse(proj);

  vec4 ndc_near = ndc_pos;
  vec4 ndc_far  = ndc_pos;
  ndc_near.z    = -1.;  // near plane at z = -1 in normalized device coordinates
  ndc_far.z     = +1.;  // far  *            +1 *

  vec4 v_near = iproj * ndc_near;
  vec4 v_far  = iproj * ndc_far;
  v_near     /= v_near.w;
  v_far      /= v_far.w;

  vec4 m_near = iview * v_near;
  vec4 m_far  = iview * v_far;

  vec4 m_ray = m_far - m_near;

  ro = m_near.xyz;
  rd = m_ray.xyz;
  rd = normalize(rd);
}


// axis aligned box centered at the origin intersection from Inigo Quilez
vec2 boxIntersection(in vec3 ro, in vec3 rd, vec3 boxSize)
{
  vec3 m   = 1. / rd;
  vec3 n   = m * ro;
  vec3 k   = abs(m) * boxSize;
  vec3 t1  = -n - k;
  vec3 t2  = -n + k;
  float tN = max(max(t1.x, t1.y), t1.z);
  float tF = min(min(t2.x, t2.y), t2.z);
  if(tN > tF || tF < 0.) return vec2(-1.); // no intersection
  return vec2(tN, tF);
}


// requires normalized rd
vec2 aabb_intersect(const in vec3 ro, const in vec3 rd, const in aabb box)
{
  const vec3 size = 0.5 * (box.h - box.l);
  const vec3 cntr = 0.5 * (box.h + box.l);

  return boxIntersection(ro - cntr, rd, size);
}


// axis aligned box centered at the origin intersection from Inigo Quilez
vec2 boxIntersectionNormal(in vec3 ro, in vec3 rd, vec3 boxSize,
                           out vec3 outNormal)
{
  vec3 m   = 1. / rd;
  vec3 n   = m * ro;
  vec3 k   = abs(m) * boxSize;
  vec3 t1  = -n - k;
  vec3 t2  = -n + k;
  float tN = max(max(t1.x, t1.y), t1.z);
  float tF = min(min(t2.x, t2.y), t2.z);
  if(tN > tF || tF < 0.) return vec2(-1.); // no intersection

  outNormal  = step(t2,vec3(tF));  // returns outward facing normal at exit
  outNormal *= sign(rd);

  return vec2(tN, tF);
}


// requires normalized rd
vec2 aabb_intersect_normal(const in vec3 ro, const in vec3 rd, const in aabb box,
                           out vec3 normal)
{
  const vec3 size = 0.5 * (box.h - box.l);
  const vec3 cntr = 0.5 * (box.h + box.l);

  return boxIntersectionNormal(ro - cntr, rd, size, normal);
}


float plane_intersect(const in vec3 ro, const in vec3 rd,
                      const in vec3 pp, const in vec3 pn)
{
  return -(dot(pn, ro) - dot(pn, pp)) / dot(pn, rd);
}


bool inside_aabb(const in vec3 p, const in aabb box)
{
  return p.x >= box.l.x && p.y >= box.l.y && p.z >= box.l.z &&
         p.x <= box.h.x && p.y <= box.h.y && p.z <= box.h.z;
}


#endif
