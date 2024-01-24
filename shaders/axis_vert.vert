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

layout(set = 0, binding = 0) uniform scene_buffer {
  mat4 view;
  mat4 proj;
} scene_ubo;

layout(set = 1, binding = 0) uniform object_buffer {
  mat4 model;
  mat4 view;
} object_ubo;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

layout(location = 0) out vec3 frag_color;

void main() {
  vec4 position = scene_ubo.proj * object_ubo.view * object_ubo.model *
                  vec4(in_position, 1.);

  // perform perspective division
  position /= position.w;

  // translate in NDC
  position.x -= 0.75;
  position.y += 0.75;

  gl_Position = position;
  frag_color  = in_color;
}
