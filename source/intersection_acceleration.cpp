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

#pragma once


#include <algorithm>

#include "buffers.cpp"


void aabb_grow(aabb& box, glm::vec3 pos)
{
  if (pos.x > box.h.x) box.h.x = pos.x;
  if (pos.x < box.l.x) box.l.x = pos.x;
  if (pos.y > box.h.y) box.h.y = pos.y;
  if (pos.y < box.l.y) box.l.y = pos.y;
  if (pos.z > box.h.z) box.h.z = pos.z;
  if (pos.z < box.l.z) box.l.z = pos.z;
}


bool aabb_overlap(aabb& a, aabb& b)
{
  if (a.l.x <= b.h.x && a.h.x >= b.l.x &&
      a.l.y <= b.h.y && a.h.y >= b.l.y &&
      a.l.z <= b.h.z && a.h.z >= b.l.z)
    return true;
  else
    return false;
}


float aabb_surface_area(aabb& bbox)
{
  float lx = bbox.h.x - bbox.l.x;
  float ly = bbox.h.y - bbox.l.y;
  float lz = bbox.h.z - bbox.l.z;

  return 2.f * (ly * lz + lx * lz + lx * ly);
}


bool aabb_point_intersect(glm::vec3 pnt, aabb& bbox)
{
  if (pnt.x >= bbox.l.x && pnt.x <= bbox.h.x &&
      pnt.y >= bbox.l.y && pnt.y <= bbox.h.y &&
      pnt.z >= bbox.l.z && pnt.z <= bbox.h.z)
    return true;
  else
    return false;
}


glm::vec2 aabb_ray_intersect(glm::vec3 ro, glm::vec3 rd, const aabb& bbox)
{
  glm::vec3 tn = (bbox.l - ro) / rd;
  glm::vec3 tf = (bbox.h - ro) / rd;

  if (tf.x < tn.x) std::swap(tn.x, tf.x);
  if (tf.y < tn.y) std::swap(tn.y, tf.y);
  if (tf.z < tn.z) std::swap(tn.z, tf.z);

  float n = max(max(tn.x, tn.y), tn.z);
  float f = min(min(tf.x, tf.y), tf.z);

  if (n < f) return glm::vec2(n, f);
  else       return glm::vec2(-1., -1.);
}


struct render_metadata
{
  aabb              domain_bbox;
  std::vector<aabb> elem_bboxes;
};


struct kdtree
{
  static constexpr int max_depth = 21;
  aabb                 bbox;
  std::vector<kdnode>  nodes;
  std::vector<int>     leaf_elements;
};


enum struct bboxedge_type: u32
{
  low  = 0,
  high = 1
};


struct bboxedge
{
  float         pos;
  int           elem;
  bboxedge_type type;
};


bool bboxedge_comp(const bboxedge& a, const bboxedge& b)
{
  const float tol = 1e-5;

  if (std::abs(b.pos - a.pos) < tol)  // close comparison, on type
  {
    return a.type > b.type;
  }
  else                                // far comparison, on position
  {
    return a.pos < b.pos;
  }
}


struct split_location
{
  float cost;
  usize edge;
};


split_location find_best_split(int axis, aabb& bbox, std::vector<int>& overlap,
                               render_metadata& metadata,
                               std::vector<bboxedge>& edges)
{
  aabb bbox_l, bbox_r;

  for (usize i = 0; i < overlap.size(); ++i)
  {
    int  elem        = overlap[i];
    aabb elem_bbox   = metadata.elem_bboxes[elem];
    edges[2 * i + 0] = {elem_bbox.l[axis], elem, bboxedge_type::low};
    edges[2 * i + 1] = {elem_bbox.h[axis], elem, bboxedge_type::high};
  }

  std::sort(edges.begin(), edges.end(), bboxedge_comp);

  split_location best_split;
  float sa_f      = aabb_surface_area(bbox);
  usize nl        = 0;
  usize nr        = overlap.size();
  best_split.cost = FLT_MAX;
  best_split.edge = 0;
  for (usize i = 0; i < edges.size(); ++i)
  {
    if (edges[i].type == bboxedge_type::high)
    {
      --nr;
    }

    bbox_l         = bbox_r         = bbox;
    bbox_l.h[axis] = bbox_r.l[axis] = edges[i].pos;

    float pl = aabb_surface_area(bbox_l) / sa_f;
    float pr = aabb_surface_area(bbox_r) / sa_f;

    float cost = pl * nl + pr * nr;

    if (cost < best_split.cost)
    {
      best_split.cost = cost;
      best_split.edge = i;
    }

    if (edges[i].type == bboxedge_type::low)
    {
      ++nl;
    }
  }

  return best_split;
}


void kd_build(aabb bbox, int parent, std::vector<int>& overlap, int depth,
              render_metadata& metadata, kdtree& tree)
{
  int node_number = tree.nodes.size();
  tree.nodes.push_back(kdnode());

  tree.nodes[node_number].bbox   = bbox;
  tree.nodes[node_number].parent = parent;

  bool             is_leaf = false;
  aabb             bbox_l, bbox_r;
  std::vector<int> overlap_l, overlap_r;

  // determine split axis and location using surface area heuristic

  usize nedges = 2 * overlap.size();

  float                 best_cost = FLT_MAX;
  usize                 best_edge = 0;
  int                   best_axis = 0;
  std::vector<bboxedge> best_edges(nedges);

  std::vector<bboxedge> edges_workspace(nedges);
  for (int axis_candidate = 0; axis_candidate < 3; ++axis_candidate)
  {
    split_location best_split =
    find_best_split(axis_candidate, bbox, overlap, metadata, edges_workspace);

    if (best_split.cost < best_cost)
    {
      best_cost = best_split.cost;
      best_edge = best_split.edge;
      best_axis = axis_candidate;

      memcpy(best_edges.data(), edges_workspace.data(),
             sizeof(bboxedge) * best_edges.size());
    }
  }

  // determine overlaps if justified by best cost estimate

  if (best_cost < (float)overlap.size())
  {
    bbox_l              = bbox_r              = bbox;
    bbox_l.h[best_axis] = bbox_r.l[best_axis] = best_edges[best_edge].pos;

    tree.nodes[node_number].axis  = best_axis;
    tree.nodes[node_number].split = best_edges[best_edge].pos;

    for (usize i = 0; i < best_edge; ++i)
    {
      if (best_edges[i].type == bboxedge_type::low)
      {
        overlap_l.push_back(best_edges[i].elem);
      }
    }

    for (usize i = best_edge + 1; i < best_edges.size(); ++i)
    {
      if (best_edges[i].type == bboxedge_type::high)
      {
        overlap_r.push_back(best_edges[i].elem);
      }
    }
  }
  else
  {
    is_leaf = true;
  }

  // continue building or terminate at leaf

  if (depth < tree.max_depth && !is_leaf)
  {
    int new_depth = ++depth;

    kd_build(bbox_l, node_number, overlap_l, new_depth, metadata, tree);

    tree.nodes[node_number].child_r = tree.nodes.size();
    kd_build(bbox_r, node_number, overlap_r, new_depth, metadata, tree);
  }
  else
  {
    tree.nodes[node_number].offset = tree.leaf_elements.size();
    tree.nodes[node_number].count  = overlap.size();

    for (usize i = 0; i < overlap.size(); ++i)
    {
      tree.leaf_elements.push_back(overlap[i]);
    }
  }
}


struct kd_tree_stats
{
  usize leaf_count         = 0;
  usize overlap_count      = 0;

  float mean_leaf_overlaps = 0.f;
  float mean_depth         = 0.f;  // used as accumulator until division

  usize max_leaf_overlaps  = 0;
  usize max_depth          = 0;
};


void find_tree_stats(usize node_num, usize depth, kdtree& tree, kd_tree_stats& stats)
{
  kdnode& node = tree.nodes[node_num];

  if (node.offset != -1)
  {
    stats.leaf_count    += 1;
    stats.overlap_count += node.count;
    stats.mean_depth    += float(depth);  // dividing for mean later

    if (node.count > stats.max_leaf_overlaps)
    {
      stats.max_leaf_overlaps = node.count;
    }
    if (depth > stats.max_depth)
    {
      stats.max_depth = depth;
    }
  }
  else
  {
    find_tree_stats(node_num + 1, depth + 1, tree, stats);
    find_tree_stats(node.child_r, depth + 1, tree, stats);
  }

  if (node_num == 0)
  {
    stats.mean_leaf_overlaps = float(stats.overlap_count) / float(stats.leaf_count);
    stats.mean_depth         = stats.mean_depth / float(stats.leaf_count);
  }
}
