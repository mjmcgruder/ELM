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


#ifndef KD_TRAVERSAL
#define KD_TRAVERSAL


#include "data_structures.glsl"
#include "intersections.glsl"


void kd_ray_traverse(const in vec3 ro, const in vec3 rd,
                     out int elem_num, out bool hit_geom, out vec3 hit_pos,
                     out float min_thit)
{
  elem_num = 0;
  hit_geom = false;
  hit_pos  = vec3(0.);

  vec2 domain_bbox_intersect = aabb_intersect(ro, rd, domain_bbox);
  if (domain_bbox_intersect.x == -1. && domain_bbox_intersect.y == -1.)
  {
    return;
  }

  const int missed_cache_size         = 8;
  int missed_cache[missed_cache_size] = int[](-1, -1, -1, -1, -1, -1, -1, -1);
  int missed_cache_head               = 0;

  float domain_tmin = domain_bbox_intersect.x;
  float domain_tmax = domain_bbox_intersect.y;
  float tmin        = domain_tmin;
  float tmax        = domain_tmax;
  int node_num      = 0;

  uint failsafe = 0;
  while (failsafe < 10000)
  {
    kdnode node = kdnodes[node_num];

    if (node.offset == -1)
    {
      float thit = (node.split - ro[node.axis]) / rd[node.axis];

      int near_node, far_node;

      if (rd[node.axis] > 0.)
      {
        near_node = node_num + 1;
        far_node  = node.child_r;
      }
      else
      {
        near_node = node.child_r;
        far_node  = node_num + 1;
      }

      if (thit < tmin)  // "far" child only
      {
        node_num = far_node;
      }
      else if (thit > tmax || thit < 0.)  // "near" child only
      {
        node_num = near_node;
      }
      else  // "both" children (far node handled by restarting)
      {
        node_num = near_node;
        tmax     = thit;
      }
    }
    else
    {
      min_thit = FLT_MAX;
      for (uint i = 0; i < node.count; ++i)
      {
        int test_elem = kdleafelems[node.offset + i];

        // check if this element has been recently intersected

        bool already_missed = false;
        for (uint ci = 0; ci < missed_cache_size; ++ci)
        {
          if (missed_cache[ci] == test_elem) { already_missed = true; break; }
        }
        if (already_missed) { continue; }

        // if not recently intersected, check for hit

        vec3 r_p; float thit = FLT_MAX;
        bool hit = intersect_elem(ro, rd, test_elem, r_p, thit);

        // update closest hit if necessary

        if (hit && thit < min_thit)
        {
          min_thit = thit;
          hit_geom = true;
          hit_pos  = r_p;
          elem_num = test_elem;
        }
        else if (!hit)
        {
          missed_cache[missed_cache_head] = test_elem;
          missed_cache_head = ((missed_cache_head + 1) % missed_cache_size);
        }
      }

      if (hit_geom)
      {
        break;
      }
      else
      {
        tmin = tmax + 1e-4;
        tmax = domain_tmax;

        if (tmin > domain_tmax)
        {
          break;
        }

        // node_num = 0;  // restarting from root

        bool hit_bbox;
        do
        {
          node_num  = kdnodes[node_num].parent;
          vec2 test = aabb_intersect(ro, rd, kdnodes[node_num].bbox);
          hit_bbox  = !(test.x == -1. && test.y == -1.) &&
                      tmin >= test.x && tmin <= test.y;
          tmax      = test.y;
        }
        while (node_num > 0 && !hit_bbox);
      }
    }
    ++failsafe;
  }
}

#endif
