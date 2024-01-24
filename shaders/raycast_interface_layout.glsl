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


#ifndef RAYCAST_INTERFACE
#define RAYCAST_INTERFACE


#include "data_structures.glsl"


layout(set = 0, binding = 0) uniform uniform_buffer_object {
  mat4 view;
  mat4 proj;

  mat4 slice_model;
} ubo;

layout(std430, set = 2, binding = 0) buffer solver_params {
  uint p;
  uint q;
  uint nelem;
  uint etype;
  uint dim;
  uint nbfp;
  uint nbfq;
  float gamma;
} params;
layout(std430, set = 2, binding = 1) buffer node_data    { float nodes[];     };
layout(std430, set = 2, binding = 2) buffer state_data   { float U[];         };
layout(std430, set = 2, binding = 3) buffer bbox_data    { aabb bboxes[];     };
layout(std430, set = 2, binding = 4) buffer otbound_data { vec2 otp_bounds[]; };
layout(std430, set = 2, binding = 5) buffer dombbox_data { aabb domain_bbox;  };
layout(std430, set = 2, binding = 6) buffer domot_data   { vec2 domain_otlim; };
layout(std430, set = 2, binding = 7) buffer kdnode_data  { kdnode kdnodes[];  };
layout(std430, set = 2, binding = 8) buffer kdleaf_data  { int kdleafelems[]; };
layout(std430, set = 2, binding = 9) buffer cmap_data    { float cmap[];      };
layout(std430, set = 2, binding = 10) buffer output_data { int output_option; };

layout(location = 0) in vec4 ndc_pos;

layout(location = 0) out vec4 out_color;


const vec4 clear_color = vec4(0., 0., 0., 1.);


#endif
