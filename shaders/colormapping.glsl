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

#ifndef COLORMAPPING
#define COLORMAPPING

#include "constants.glsl"
#include "output.glsl"

const float cmap_size     = 255.;
const float inv_cmap_size = 1. / cmap_size;

vec4 map_color(const in int output_num, const in float min, const in float max, 
               const in float state[5], const in float gamma)
{
  // calculate output value

  float val = eval_output(output_num, state, gamma);

  // linearly interpolate color output

  float intensity = clamp((val - min) / (max - min), 0., 1. - 2. * FLT_EPSILON);

  uint indx_l = uint(floor(intensity * cmap_size));
  uint indx_h = indx_l + 1;

  float intensity_l  = float(indx_l) * inv_cmap_size;
  float intensity_h  = intensity_l + inv_cmap_size;

  float w_l = (intensity_h - intensity) * cmap_size;
  float w_h = (intensity - intensity_l) * cmap_size;

  vec3 color = vec3(
    w_l * cmap[3 * indx_l + 0] + w_h * cmap[3 * indx_h + 0],
    w_l * cmap[3 * indx_l + 1] + w_h * cmap[3 * indx_h + 1],
    w_l * cmap[3 * indx_l + 2] + w_h * cmap[3 * indx_h + 2]
  );

  return vec4(color, 1.);
}

#endif
