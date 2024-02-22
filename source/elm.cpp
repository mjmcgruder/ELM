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


#include "init.cpp"
#include "optparse.cpp"
#include "render_loop.cpp"
#include "state.cpp"


int main(int argc, char** argv)
{
  /* input parsing */

  std::string ifile         = "output";
  bool print_vkfeatures     = false;
  std::string output_string = "mach";
  std::string cmap_string   = "jet";
  bool init_only            = false;

  const usize optc     = 5;
  option optlist[optc] = {
  mkopt("ifile", "input file prefix", &ifile),
  mkopt("features", "prints vulkan implementation features", &print_vkfeatures),
  mkopt("cmap", "colormap selection", &cmap_string),
  mkopt("output", "rendering output", &output_string),
  mkopt("initonly", "do not render, only initialize", &init_only),
  };

  bool help = false;
  if (optparse(argc, argv, optc, optlist, help))
    return 1;
  if (help) return 0;

  colormap      = cmap_map.at(cmap_string);
  render_output = output_map.at(output_string);

  /* read input file */

  ifile += ".dg";
  rendering_data = read_dg_solution(ifile.c_str());
  current_field  = rendering_data.fields["state"];

  /* center geometry on origin */

  printf("--- centering domain ---\n");
  auto c0 = std::chrono::steady_clock::now();

  glm::vec3 center(0.f, 0.f, 0.f);

  for (usize e = 0; e < rendering_data.nelem; ++e)
  {
    for (usize b = 0; b < rendering_data.nbfq; ++b)
    {
      float* node = rendering_data.node(e, b);
      center.x   += node[0];
      center.y   += node[1];
      center.z   += node[2];
    }
  }

  center /= float(rendering_data.nelem * rendering_data.nbfq);

  for (usize e = 0; e < rendering_data.nelem; ++e)
  {
    for (usize b = 0; b < rendering_data.nbfq; ++b)
    {
      float* node = rendering_data.node(e, b);
      node[0]    -= center.x;
      node[1]    -= center.y;
      node[2]    -= center.z;
    }
  }

  auto c1 = std::chrono::steady_clock::now();

  std::chrono::duration<double, std::milli> centering_duration = c1 - c0;
  printf("  done, finished in %.1f ms\n\n", centering_duration.count());

  /* render */

  printf("--- initializing Vulkan ---\n");
  auto i0 = std::chrono::steady_clock::now();

  vkinit(print_vkfeatures);

  auto i1 = std::chrono::steady_clock::now();

  std::chrono::duration<double, std::milli> initialization_duration = i1 - i0;
  printf("  done, finished in %.1f ms\n\n", initialization_duration.count());

  {

    compute_pipeline comp_metadata(SHADER_DIR "metadata.spv", 7);
    compute_pipeline flatten_bbox(SHADER_DIR "reduction_flatten_bbox.spv", 5);
    compute_pipeline flatten_lims(SHADER_DIR "reduction_flatten_outlims.spv", 3);

    raycast_data rcdata;

    /* transfer geometry and state data */

    rcdata.d_geom  = dbuffer<dg_solution>(1);
    rcdata.d_nodes = dbuffer<float>(rendering_data.nodes.size());
    rcdata.d_state = dbuffer<float>(current_field.state.size());

    dmalloc(rcdata.d_geom);
    dmalloc(rcdata.d_nodes);
    dmalloc(rcdata.d_state);

    memcpy_htod(rcdata.d_geom, &rendering_data);
    memcpy_htod(rcdata.d_nodes, rendering_data.nodes.data());
    memcpy_htod(rcdata.d_state, current_field.state.data());

    /* transfer rendering options */

    rcdata.d_colormap = dbuffer<float>(256 * 3);
    rcdata.d_output   = dbuffer<output_type>(1);

    dmalloc(rcdata.d_colormap);
    dmalloc(rcdata.d_output);

    memcpy_htod(rcdata.d_colormap, colormap);
    memcpy_htod(rcdata.d_output, &render_output);

    /* pre-compute rendering metadata */

    printf("--- computing metadata ---\n");
    auto t0 = std::chrono::steady_clock::now();

    rcdata.d_bboxes               = dbuffer<aabb>(rendering_data.nelem);
    rcdata.d_output_bounds        = dbuffer<glm::vec2>(rendering_data.nelem);
    rcdata.d_domain_bbox          = dbuffer<aabb>(1);
    rcdata.d_domain_output_bounds = dbuffer<glm::vec2>(1);

    dmalloc(rcdata.d_bboxes);
    dmalloc(rcdata.d_output_bounds);
    dmalloc(rcdata.d_domain_bbox);
    dmalloc(rcdata.d_domain_output_bounds);

    comp_metadata.dset.update(rcdata.d_geom,          0);
    comp_metadata.dset.update(rcdata.d_nodes,         1);
    comp_metadata.dset.update(rcdata.d_state,         2);
    comp_metadata.dset.update(rcdata.d_output,        3);
    comp_metadata.dset.update(rcdata.d_bboxes,        4);
    comp_metadata.dset.update(rcdata.d_output_bounds, 5);
    comp_metadata.run((rendering_data.nelem + (128 - 1)) / 128, 1, 1);

    render_metadata rcmetadata;
    rcmetadata.elem_bboxes = std::vector<aabb>(rendering_data.nelem);

    memcpy_dtoh(rcmetadata.elem_bboxes.data(), rcdata.d_bboxes);

    // compute domain bounding box (avoiding atomics for portability)

    {
      dbuffer<float> d_xpnts(2 * rendering_data.nelem);
      dbuffer<float> d_ypnts(2 * rendering_data.nelem);
      dbuffer<float> d_zpnts(2 * rendering_data.nelem);

      dmalloc(d_xpnts);
      dmalloc(d_ypnts);
      dmalloc(d_zpnts);

      flatten_bbox.dset.update(rcdata.d_geom,   0);
      flatten_bbox.dset.update(rcdata.d_bboxes, 1);
      flatten_bbox.dset.update(d_xpnts,         2);
      flatten_bbox.dset.update(d_ypnts,         3);
      flatten_bbox.dset.update(d_zpnts,         4);
      flatten_bbox.run((rendering_data.nelem + (128 - 1)) / 128, 1, 1);

      std::vector<float> h_xpnts(2 * rendering_data.nelem);
      std::vector<float> h_ypnts(2 * rendering_data.nelem);
      std::vector<float> h_zpnts(2 * rendering_data.nelem);

      memcpy_dtoh(h_xpnts.data(), d_xpnts);
      memcpy_dtoh(h_ypnts.data(), d_ypnts);
      memcpy_dtoh(h_zpnts.data(), d_zpnts);

      rcmetadata.domain_bbox = aabb();

      for (usize i = 0; i < 2 * rendering_data.nelem; ++i)
      {
        if (h_xpnts[i] < rcmetadata.domain_bbox.l.x)
        {
          rcmetadata.domain_bbox.l.x = h_xpnts[i];
        }
        if (h_xpnts[i] > rcmetadata.domain_bbox.h.x)
        {
          rcmetadata.domain_bbox.h.x = h_xpnts[i];
        }

        if (h_ypnts[i] < rcmetadata.domain_bbox.l.y)
        {
          rcmetadata.domain_bbox.l.y = h_ypnts[i];
        }
        if (h_ypnts[i] > rcmetadata.domain_bbox.h.y)
        {
          rcmetadata.domain_bbox.h.y = h_ypnts[i];
        }

        if (h_zpnts[i] < rcmetadata.domain_bbox.l.z)
        {
          rcmetadata.domain_bbox.l.z = h_zpnts[i];
        }
        if (h_zpnts[i] > rcmetadata.domain_bbox.h.z)
        {
          rcmetadata.domain_bbox.h.z = h_zpnts[i];
        }
      }

      // for (usize ei = 0; ei < rendering_data.nelem; ++ei)
      // {
      //   aabb bbox = rcmetadata.elem_bboxes[ei];
      //   aabb_grow(rcmetadata.domain_bbox, bbox.l);
      //   aabb_grow(rcmetadata.domain_bbox, bbox.h);
      // }

      memcpy_htod(rcdata.d_domain_bbox, &rcmetadata.domain_bbox);
    }  // ensure temp buffer deallocation

    // compute output bounds (avoiding atomics for portability)

    glm::vec2 domain_output_bounds(+FLT_MAX, -FLT_MAX);
    {
      dbuffer<float> d_bvals(2 * rendering_data.nelem);
      dmalloc(d_bvals);

      flatten_lims.dset.update(rcdata.d_geom,          0);
      flatten_lims.dset.update(rcdata.d_output_bounds, 1);
      flatten_lims.dset.update(d_bvals,                2);
      flatten_lims.run((rendering_data.nelem + (128 - 1)) / 128, 1, 1);

      std::vector<float> h_bvals(2 * rendering_data.nelem);

      memcpy_dtoh(h_bvals.data(), d_bvals);

      for (usize i = 0; i < 2 * rendering_data.nelem; ++i)
      {
        if (h_bvals[i] < domain_output_bounds.x)
        {
          domain_output_bounds.x = h_bvals[i];
        }
        if (h_bvals[i] > domain_output_bounds.y)
        {
          domain_output_bounds.y = h_bvals[i];
        }
      }

      memcpy_htod(rcdata.d_domain_output_bounds, &domain_output_bounds);
    }  // ensure temp buffer deallocation

    // glm::vec2* output_bounds = new glm::vec2[rendering_data.nelem];
    //
    // memcpy_dtoh(output_bounds, rcdata.d_output_bounds);
    //
    // glm::vec2 domain_output_bounds(+FLT_MAX, -FLT_MAX);
    // for (usize ei = 0; ei < rendering_data.nelem; ++ei)
    // {
    //   if (output_bounds[ei].x < domain_output_bounds.x)
    //     domain_output_bounds.x = output_bounds[ei].x;
    //   if (output_bounds[ei].y > domain_output_bounds.y)
    //     domain_output_bounds.y = output_bounds[ei].y;
    // }
    //
    // memcpy_htod(rcdata.d_domain_output_bounds, &domain_output_bounds);
    //
    // delete[] output_bounds;

    // metadata logging

    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> metadata_duration = t1 - t0;
    printf("  done, finished in %.1f ms\n", metadata_duration.count());

    printf("\n");
    printf("  metadata results:\n");
    printf("    output range:  %+.3f to %+.3f\n",
           domain_output_bounds.x, domain_output_bounds.y);
    printf("    domain bounds: %+.3f %+.3f %+.3f | %+.3f %+.3f %+.3f\n",
           rcmetadata.domain_bbox.l.x, rcmetadata.domain_bbox.l.y,
           rcmetadata.domain_bbox.l.z, rcmetadata.domain_bbox.h.x,
           rcmetadata.domain_bbox.h.y, rcmetadata.domain_bbox.h.z);
    printf("\n");

    /* compute and transfer kd tree */

    printf("--- building k-d tree ---\n");

    auto t2 = std::chrono::steady_clock::now();

    std::vector<int> overlap_list(rendering_data.nelem);
    for (usize i = 0; i < rendering_data.nelem; ++i)
    {
      overlap_list[i] = i;
    }

    kdtree tree;
    tree.bbox = rcmetadata.domain_bbox;
    kd_build(rcmetadata.domain_bbox, -1, overlap_list, 0, rcmetadata, tree);

    rcdata.d_kdnodes          = dbuffer<kdnode>(tree.nodes.size());
    rcdata.d_kd_leaf_elements = dbuffer<int>(tree.leaf_elements.size());

    dmalloc(rcdata.d_kdnodes);
    dmalloc(rcdata.d_kd_leaf_elements);

    memcpy_htod(rcdata.d_kdnodes, tree.nodes.data());
    memcpy_htod(rcdata.d_kd_leaf_elements, tree.leaf_elements.data());

    // k-d tree logging

    auto t3 = std::chrono::steady_clock::now();

    std::chrono::duration<double, std::milli> kdtree_duration = t3 - t2;
    printf("  done, finished in %.1f ms\n", kdtree_duration.count());

    kd_tree_stats tree_stats;
    find_tree_stats(0, 1, tree, tree_stats);

    printf("\n");
    printf("  k-d tree stats:\n");
    printf("    nodes              | %zu\n",  tree.nodes.size());
    printf("    leaf nodes         | %zu\n",  tree_stats.leaf_count);
    printf("    leaf overlaps      | %zu\n",  tree_stats.overlap_count);
    printf("    mean leaf overlaps | %.3f\n", tree_stats.mean_leaf_overlaps);
    printf("    mean depth         | %.3f\n", tree_stats.mean_depth);
    printf("    max leaf overlaps  | %zu\n",  tree_stats.max_leaf_overlaps);
    printf("    max depth          | %zu\n",  tree_stats.max_depth);

    //

    rcdata.update_descset();

    if (!init_only)
    {
      render_loop(rcdata, rcmetadata);
    }

  }  // ensures dbuffers clear before vulkan deinit

  /* exit */

  clean_global_resources();
  return 0;
}
