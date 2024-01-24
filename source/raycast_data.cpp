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

# pragma once


#include "pipeline.cpp"


struct raycast_data
{
  // raw geometry and state data
  dbuffer<dg_solution> d_geom;
  dbuffer<float>       d_nodes;
  dbuffer<float>       d_state;

  // gpu-side pre-computes
  dbuffer<aabb>        d_bboxes;
  dbuffer<glm::vec2>   d_output_bounds;

  // cpu augments to gpu pre-computes (to avoid atomics for portability)
  dbuffer<aabb>        d_domain_bbox;
  dbuffer<glm::vec2>   d_domain_output_bounds;

  // cpu-side pre-computes
  dbuffer<kdnode>      d_kdnodes;
  dbuffer<int>         d_kd_leaf_elements;

  // rendering options
  dbuffer<float>       d_colormap;
  dbuffer<output_type> d_output;


  descriptor_set_layout raycast_layout;
  descriptor_set        raycast_descset;

  raycast_data();

  void update_descset();
};

raycast_data::raycast_data() :
d_geom(),
d_nodes(),
d_state(),
d_bboxes(),
d_output_bounds(),
d_domain_bbox(),
d_domain_output_bounds(),
d_kdnodes(),
d_kd_leaf_elements(),
d_colormap(),
d_output(),
raycast_layout(11, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
raycast_descset(&raycast_layout)
{}

void raycast_data::update_descset()
{
  raycast_descset.update(d_geom,                 0);
  raycast_descset.update(d_nodes,                1);
  raycast_descset.update(d_state,                2);
  raycast_descset.update(d_bboxes,               3);
  raycast_descset.update(d_output_bounds,        4);
  raycast_descset.update(d_domain_bbox,          5);
  raycast_descset.update(d_domain_output_bounds, 6);
  raycast_descset.update(d_kdnodes,              7);
  raycast_descset.update(d_kd_leaf_elements,     8);
  raycast_descset.update(d_colormap,             9);
  raycast_descset.update(d_output,               10);
}
