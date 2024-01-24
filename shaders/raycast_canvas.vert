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
} ubo;

vec2 canvas[6] = vec2[](
  vec2(-1., -1.),
  vec2(-1., +1.),
  vec2(+1., -1.),
  vec2(+1., +1.),
  vec2(+1., -1.),
  vec2(-1., +1.)
);

layout(location = 0) out vec4 pos;

void main()
{
  pos         = vec4(canvas[gl_VertexIndex], 0., 1.);
  gl_Position = pos;
}
