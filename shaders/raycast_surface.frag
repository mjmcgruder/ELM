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


vec3 refbox_nearest_vec(vec3 p)
{
  p      = p - 0.5;
  vec3 b = vec3(0.5);

  return clamp(p, -b, b) - p;
}

bool intersect_elem(const in vec3 ro, const in vec3 rd, const in int elem_num,
                    out vec3 r_p, out float t)
{
  vec2 bbox_intersect = aabb_intersect(ro, rd, bboxes[elem_num]);

  if (bbox_intersect.x == -1. && bbox_intersect.y == -1.)
  {
    return false;
  }

  r_p        = vec3(0.5);  // starting reference guess
  t          = bbox_intersect.x;
  float tmax = bbox_intersect.y;

  const uint max_rounds = 50;
        uint max_steps  = 2;
  const float damp      = 0.5;
  const float hit_tol   = 5e-3;

  uint round    = 0;
  bool hit      = false;
  while (t < tmax && round < max_rounds)
  {
    mat3 j;
    vec3 g_p, g_target = ro + t * rd;
    for (uint step = 0; step < max_steps; ++step)
    {
      mapinfo(r_p, elem_num, params.q, g_p, j);

      r_p -= inverse(j) * (g_p - g_target);
    }

    hit = r_p.x > 0. - hit_tol && r_p.x < 1. + hit_tol &&
          r_p.y > 0. - hit_tol && r_p.y < 1. + hit_tol &&
          r_p.z > 0. - hit_tol && r_p.z < 1. + hit_tol;

    if (hit)
      break;
    else
      t += damp * length(j * refbox_nearest_vec(r_p));

    ++round;
    max_steps = 1;
  }

  return hit;
}

#include "kd_traversal.glsl"

void main()
{
  vec3 ro, rd;
  find_ray(ndc_pos, ubo.view, ubo.proj, ro, rd);

  int elem_num; bool hit_geom; vec3 hit_pos; float thit;
  kd_ray_traverse(ro, rd, elem_num, hit_geom, hit_pos, thit);

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
