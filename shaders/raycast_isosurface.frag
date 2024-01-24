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


// void obj_dist(const in vec3 ro, const in vec3 rd,
//               const in vec3 p, const in mat3 j,
//               out float dist, out vec3 dist_ref)
// {
//   vec3  d         = (p - ro) - (dot(p - ro, rd) * rd);
//         dist      = length(d);
//   float idist     = 1. / dist;
//
//   mat3  d_p       = mat3(1.) - outerProduct(rd, rd);
//   vec3  dist_glob = (d_p * d) * idist;
//
//         dist_ref  = dist_glob * j;
// }
//
//
// void obj_xmom(const in float s[5],
//               const in float sx[5], const in float sy[5], const in float sz[5],
//               out float obj, out vec3 obj_ref)
// {
//   const float inorm = 1. / (0.1 * 0.1);
//
//   float err    = s[1] - 0.075;
//         obj    = (err * err) * inorm;
//
//   vec3 o_ref   = vec3(sx[1], sy[1], sz[1]);
//        obj_ref = 2. * inorm * err * o_ref;
// }
//
//
// void obj(const in vec3 ro, const in vec3 rd,
//          const in vec3 ref, const in int elem_num,
//          out float f, out vec3 f_ref, out vec3 glo)
// {
//   mat3 j;
//   mapinfo(ref, elem_num, params.q, glo, j);
//
//   float s[5]; float sx[5]; float sy[5]; float sz[5];
//   interp_state_grad(ref, elem_num, params.p, s, sx, sy, sz);
//
//   float dist; vec3 dist_ref;
//   obj_dist(ro, rd, glo, j, dist, dist_ref);
//
//   float oerr; vec3 oerr_ref;
//   obj_xmom(s, sx, sy, sz, oerr, oerr_ref);
//
//   f     = oerr + dist;
//   f_ref = dist_ref + oerr_ref;
// }
//
//
// vec4 test_out = vec4(0., 0., 0., 1.);
//
//
// float pinpoint(in float al, in float ah,
//                in float fl, in float fh,
//                in float f_pl, in float f_ph,
//                const in vec3 ro, const in vec3 rd, const in int elem_num,
//                const in vec3 ref0, in vec3 p,
//                const in float f0, const in float f_p0,
//                const in float mu1, const in float mu2)
// {
//   for (uint i = 0; i < 6; ++i)
//   {
//     // float ap = 0.5 * (al + ah);
//     float ap = (2. * al * (fh - fl) + f_pl * (al * al - ah * ah)) /
//                (2. * (fh - fl + f_pl * (al - ah)));
//
//     float fp; vec3 f_refp; vec3 _;
//     vec3 refp = ref0 + p * ap;
//     obj(ro, rd, refp, elem_num, fp, f_refp, _);
//     float f_pp = dot(p, f_refp);
//
//     if ((fp > f0 + mu1 * ap * f_p0) || (fp > fl))
//     {
//       ah   = ap;
//       fh   = fp;
//       f_ph = f_pp;
//
//       // test_out.x += 0.1;
//     }
//     else
//     {
//       if (abs(f_pp) <= mu2 * -f_p0)
//       {
//         return ap;
//       }
//       else if (f_pp * (ah - al) >= 0.)
//       {
//         ah   = al;
//         fh   = fl;
//         f_ph = f_pl;
//       }
//       al   = ap;
//       fl   = fp;
//       f_pl = f_pp;
//
//       // test_out.y += 0.1;
//     }
//   }
//
//   return -1.;
// }
//
//
// float line_search(const in vec3 ro, const in vec3 rd, const in int elem_num,
//                   const in vec3 ref0, in vec3 p,
//                   const in float f0, const in float f_p0,
//                   const in float mu1, const in float mu2)
// {
//         p    = normalize(p);
//   float a1   = 0.;
//   float a2   = 0.5;
//   float f1   = f0;
//   float f_p1 = f_p0;
//
//   for (uint i = 0; i < 2; ++i)
//   {
//     float f2; vec3 f_ref2; vec3 _;
//
//     vec3 ref2 = ref0 + p * a2;
//     obj(ro, rd, ref2, elem_num, f2, f_ref2, _);
//     float f_p2 = dot(p, f_ref2);
//
//     if ((f2 > f0 + mu1 * a2 * f_p0) || (i > 0 && f2 > f1))  // call pinpoint
//     {
//       return pinpoint(a1, a2, f1, f2, f_p1, f_p2,
//                       ro, rd, elem_num, ref0, p, f0, f_p0, mu1, mu2);
//     }
//
//     if (abs(f_p2) <= mu2 * -f_p0)
//     {
//       return a2;
//     }
//     else if (f_p2 >= 0.)
//     {
//       return pinpoint(a2, a1, f2, f1, f_p2, f_p1,
//                       ro, rd, elem_num, ref0, p, f0, f_p0, mu1, mu2);
//     }
//     else
//     {
//       a1   = a2;
//       f1   = f2;
//       f_p1 = f_p2;
//       a2  *= 2.;
//     }
//   }
//
//   return -2.;
// }
//
//
// bool intersect_elem(const in vec3 ro, const in vec3 rd, const in int elem_num,
//                     out vec3 ref, out float t)
// {
//        float step_size = -1.;
//   const uint max_steps = 4;
//   // const float tol       = 0.005;
//               ref       = vec3(0.55);
//
//   for(uint step = 0; step < max_steps; ++step)
//   {
//     float f; vec3 f_ref; vec3 glo;
//     obj(ro, rd, ref, elem_num, f, f_ref, glo);
//
//     // if (f < tol)
//     // {
//     //   t = (glo.x - ro.x) / rd.x;  // ya... something like that
//     //   return true;
//     // }
//
//     vec3  p   = normalize(-f_ref);
//     float f_p = dot(f_ref, p);
//
//     step_size = line_search(ro, rd, elem_num, ref, p, f, f_p, 0.1, 0.9);
//
//     // if (step_size >= 0.)
//     // {
//     //   test_out = vec4(1., 1., 1., 1.);
//     // }
//
//     ref += p * step_size;
//
//   }
//   test_out = vec4(ref, 1.f);
//
//   return false;
// }


bool intersect_once(const in vec3 ro, const in vec3 rd,
                    const in float isoval, const in int elem_num,
                    inout vec3 ref, inout float t)
{
  const uint max_rounds = 10;
        uint max_steps  = 3;  // starting value, drops to "step_drop" later
  const uint step_drop  = 2;
  const float damp      = 1.;
  const float hit_tol   = 1e-3;

  aabb refbox;
  refbox.l = vec3(0.);
  refbox.h = vec3(1.);

  uint round = 0;
  bool hit   = false;
  for (uint round = 0; round < max_rounds; ++round)
  {
    mat3 j, ij;
    vec3 glo, glo_target = ro + t * rd;
    for (uint step = 0; step < max_steps; ++step)
    {
      mapinfo(ref, elem_num, params.q, glo, j);
      ij   = inverse(j);
      ref -= ij * (glo - glo_target);
    }

    float s[5]; float s_x[5]; float s_y[5]; float s_z[5];
    interp_state_grad(ref, elem_num, params.p, s, s_x, s_y, s_z);

    float o = eval_output(output_option, s, params.gamma);

    if (inside_aabb(ref, refbox) && (abs(o - isoval) < hit_tol))
    {
      hit = true;
      break;
    }

    float o_s[5];
    eval_output_grad(output_option, s, params.gamma, o_s);

    vec3 o_xi = vec3(
    o_s[0] * s_x[0] + o_s[1] * s_x[1] + o_s[2] * s_x[2] + o_s[3] * s_x[3] + o_s[4] * s_x[4],
    o_s[0] * s_y[0] + o_s[1] * s_y[1] + o_s[2] * s_y[2] + o_s[3] * s_y[3] + o_s[4] * s_y[4],
    o_s[0] * s_z[0] + o_s[1] * s_z[1] + o_s[2] * s_z[2] + o_s[3] * s_z[3] + o_s[4] * s_z[4]);

    float o_t = dot(o_xi * ij, rd);
    t        -= damp * ((o - isoval) / o_t);

    max_steps = step_drop;
  }

  return hit;
}


bool intersect_elem(const in vec3 ro, const in vec3 rd, const in int elem_num,
                    out vec3 ref, out float t)
{
  const uint  ntrial     = 3;
        float isocontour = 0.075;

  // output limit check
  if (isocontour < otp_bounds[elem_num].x || isocontour > otp_bounds[elem_num].y)
  {
    return false;
  }

  // bounding box check
  vec2 bbox_intersect = aabb_intersect(ro, rd, bboxes[elem_num]);
  if (bbox_intersect.x == -1. && bbox_intersect.y == -1.)
  {
    return false;
  }

  float tinterval = 1.;
  if (ntrial > 1)
  {
    tinterval = (bbox_intersect.y - bbox_intersect.x) / float(ntrial - 1);
  }

  bool hit = false;
  for (uint trial = 0; trial < ntrial; ++trial)  // font to back trials
  {
    vec3  try_ref = vec3(0.5);
    float try_t   = bbox_intersect.x + tinterval * float(trial);
    bool  try_hit = intersect_once(ro, rd, isocontour, elem_num, try_ref, try_t);

    if (try_hit)
    {
      hit = true;
      ref = try_ref;
      t   = try_t;
      break;
    }
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

  /* set color if intersection successful */

  if (hit_geom)
  {
    vec3 glo_hit = ro + thit * rd;

    float dom_height = domain_bbox.h.y - domain_bbox.l.y;

    float intensity = (0.5 * dom_height - abs(glo_hit.y)) / (0.5 * dom_height);
    out_color       = vec4(intensity, intensity, intensity, 1.);
  }
  else
  {
    out_color = clear_color;
  }
}
