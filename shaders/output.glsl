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


#ifndef OUTPUT_HEADER
#define OUTPUT_HEADER


float eval_output(const in int output_num, const in float state[5],
                  const in float gamma)
{
  float output_val = 0.;

  switch (output_num)
  {
    case 0:  // mach (TODO: change back later)
      {
        // float u    = state[1] / state[0];
        // float v    = state[2] / state[0];
        // float w    = state[3] / state[0];
        // float s    = sqrt(u * u + v * v + w * w);
        // float p    = (gamma - 1.) * (state[4] - 0.5 * s * s);
        // float c    = sqrt(gamma * p / state[0]);
        // output_val = s / c;

        output_val = state[1] / state[0];
      }
      break;
    case 1:   // density
      output_val = state[0];
      break;
    case 2:   // x velocity
      output_val = state[1] / state[0];
      break;
    case 3:   // y velocity
      output_val = state[2] / state[0];
      break;
    case 4:   // z velocity
      output_val = state[3] / state[0];
      break;
  }

  return output_val;
}


void eval_output_grad(const in int output_num, const in float s[5],
                      const in float gamma, out float o_s[5])
{
  switch (output_num)
  {
    case 0:  // mach (TODO: change back later)
      {
        o_s = float[5](-2. * s[1] / (s[0] * s[0]), 1. / s[0], 0., 0., 0.);
      }
      break;
    case 1:   // density
      o_s = float[5](1., 0., 0., 0., 0.);
      break;
    case 2:   // x velocity
      o_s = float[5](-2. * s[1] / (s[0] * s[0]), 1. / s[0], 0., 0., 0.);
      break;
    case 3:   // y velocity
      o_s = float[5](-2. * s[2] / (s[0] * s[0]), 0., 1. / s[0], 0., 0.);
      break;
    case 4:   // z velocity
      o_s = float[5](-2. * s[3] / (s[0] * s[0]), 0., 0., 1. / s[0], 0.);
      break;
  }
}


#endif
