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

#version 450

#include "raycast_interface_layout.glsl"

#include "intersections.glsl"
#include "mapping.glsl"
#include "colormapping.glsl"


bool point_in_elem(const in vec3 p, const in int elem_num, out vec3 r_p)
{
  if (!inside_aabb(p, bboxes[elem_num]))
  {
    return false;
  }

  r_p = vec3(0.5);

  const float hit_tol = 1e-3;
  uint max_steps      = 3;
  for (uint step = 0; step < max_steps; ++step)
  {
    mat3 j;
    vec3 g_p;
    mapinfo(r_p, elem_num, params.q, g_p, j);

    r_p -= inverse(j) * (g_p - p);
  }

  if (r_p.x > 0. - hit_tol && r_p.x < 1. + hit_tol && 
      r_p.y > 0. - hit_tol && r_p.y < 1. + hit_tol && 
      r_p.z > 0. - hit_tol && r_p.z < 1. + hit_tol)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void main()
{
  vec3 ro, rd;
  find_ray(ndc_pos, ubo.view, ubo.proj, ro, rd);

  vec3 pp = vec3(0., 0., 0.) + ubo.slice_model[3].xyz;
  vec3 pn = (ubo.slice_model * vec4(0., 0., 1., 0.)).xyz;

  // determine ray plane intersection point

  float t           = plane_intersect(ro, rd, pp, pn);
  vec3 intersection = ro + rd * t;

  if (!inside_aabb(intersection, domain_bbox) || t < 0.)
  {
    out_color = clear_color;
    return;
  }

  // find the element in which this point occurs

  int elem_num  = 0;
  bool hit_geom = false;
  vec3 hit_pos  = vec3(0.);
  int node_num  = 0;

  uint failsafe = 0;
  while (failsafe < 500)
  {
    kdnode node = kdnodes[node_num];

    if (node.offset == -1)
    {
      if (intersection[node.axis] <= node.split)
      {
        node_num = node_num + 1;
      }
      else
      {
        node_num = node.child_r;
      }
    }
    else
    {
      for (uint i = 0; i < node.count; ++i)
      {
        int test_elem = kdleafelems[node.offset + i];
        vec3 r_p;
        if(point_in_elem(intersection, test_elem, r_p))
        {
          hit_geom = true;
          elem_num = test_elem;
          hit_pos  = r_p;
          break;
        }
      }

      break;
    }

    ++failsafe;
  }

  if (hit_geom)
  {
    float min = domain_otlim.x;
    float max = domain_otlim.y;

    float state[5];
    interp_state(hit_pos, elem_num, params.p, state);

    out_color = map_color(output_option, min, max, state, params.gamma);
  }
  else
  {
    out_color = clear_color;
  }
}
