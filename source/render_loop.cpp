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


#include <chrono>

#include "recording.cpp"
#include "swapchain.cpp"
#include "raycast_data.cpp"
#include "intersection_acceleration.cpp"


void render_loop(raycast_data& rcdata, render_metadata& rcmetadata)
{
  descriptor_set_layout scene_layout(1,  VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
  descriptor_set_layout object_layout(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

  std::unordered_map<std::string, graphics_pipeline> pipelines;

  pipelines.emplace(
  "ui",
  graphics_pipeline(SHADER_DIR "axis_vert.spv",
                    SHADER_DIR "axis_frag.spv",
                    VK_ATTACHMENT_LOAD_OP_LOAD, true, scene_layout,
                    object_layout, *rcdata.raycast_descset.layout));
  pipelines.emplace(
  "raycast_surface",
  graphics_pipeline(SHADER_DIR "raycast_canvas.spv",
                    SHADER_DIR "raycast_surface.spv",
                    VK_ATTACHMENT_LOAD_OP_CLEAR, false, scene_layout,
                    object_layout, *rcdata.raycast_descset.layout));
  pipelines.emplace(
  "raycast_slice",
  graphics_pipeline(SHADER_DIR "raycast_canvas.spv",
                    SHADER_DIR "raycast_slice.spv",
                    VK_ATTACHMENT_LOAD_OP_CLEAR, false, scene_layout,
                    object_layout, *rcdata.raycast_descset.layout));
  pipelines.emplace(
  "raycast_isosurface",
  graphics_pipeline(SHADER_DIR "raycast_canvas.spv",
                    SHADER_DIR "raycast_isosurface.spv",
                    VK_ATTACHMENT_LOAD_OP_CLEAR, false, scene_layout,
                    object_layout, *rcdata.raycast_descset.layout));

  /*
   * add axis to ui ------------------------------------------------------------
   */

  std::unordered_map<std::string, entity> ui_list;

  vertex axis_vertices[] = {
  {{0.f, 0.f, 0.f}, {0.f, 0.f, 0.f}, {0.5f, 0.5f, 0.5f}},
  {{1.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {0.5f, 0.5f, 0.5f}},
  {{0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.5f, 0.5f, 0.5f}},
  {{0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}, {0.5f, 0.5f, 0.5f}},
  };
  u32 axis_indices[] = {0, 2, 1, 1, 2, 3, 0, 3, 2, 0, 1, 3};

  ui_list.emplace("axis",
                  entity(&object_layout, axis_vertices, 4, axis_indices, 12));

  /*
   * render synchronization objects --------------------------------------------
   */

  VkCommandBuffer command_buffer        = VK_NULL_HANDLE;
  VkFence render_in_progress            = VK_NULL_HANDLE;
  VkSemaphore image_available_semaphore = VK_NULL_HANDLE;
  VkSemaphore render_finished_semaphore = VK_NULL_HANDLE;

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  VK_CHECK(vkCreateFence(device, &fence_info, nullptr, &render_in_progress),
           "render fence creation failed!");

  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VK_CHECK(vkCreateSemaphore(device, &semaphore_info, nullptr,
                             &image_available_semaphore),
           "image available semaphore creation failed!");
  VK_CHECK(vkCreateSemaphore(device, &semaphore_info, nullptr,
                             &render_finished_semaphore),
           "render finished semaphone creatino failed!");

  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = graphics_command_pool;
  alloc_info.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = (uint32_t)1;
  VK_CHECK(vkAllocateCommandBuffers(device, &alloc_info, &command_buffer),
           "render command buffer creation failed!");

  /*
   * render loop -------------------------------------------------------------
   */

  make_swap_chain_dependencies(pipelines);

  uniform<scene_transform> scene_ubo(&scene_layout);

  VkPipelineStageFlags wait_stages[] = {
  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  while (!glfwWindowShouldClose(window))
  {
    auto t0 = std::chrono::steady_clock::now();

    u32 swap_chain_image_indx;

    // process input

    glfwPollEvents();

    // check for window resize

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface,
                                                       &surface_capabilities),
             "failed to acquire surfce capabilies to check for resizing!");

    if (surface_capabilities.currentExtent.width  != swap_chain_extent.width  ||
        surface_capabilities.currentExtent.height != swap_chain_extent.height ||
        frame_buffer_resized)
    {
      remake_swap_chain(pipelines);
      frame_buffer_resized = false;
      continue;
    }

    // update ui elements

    update_scene_transform(scene_ubo.host_data, rcmetadata.domain_bbox);

    entity& axis = ui_list["axis"];
    update_axis_transform(scene_ubo.host_data, axis.ubo.host_data);

    axis.ubo.map();
    scene_ubo.map();

    // reset ui elements

    reset_ui();

    // reset / determine frame assets

    vkResetFences(device, 1, &render_in_progress);
    vkResetCommandBuffer(command_buffer, 0);

    VK_CHECK_SUBOPTIMAL(
    vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX,
                          image_available_semaphore, VK_NULL_HANDLE,
                          &swap_chain_image_indx),
    "failed to determine the swap chain image for this frame!");

    // record render command buffer

    switch (RAYCAST_MODE)
    {
      case raycast_mode::surface:
        record_raycast_command_buffer(
        swap_chain_image_indx, pipelines["raycast_surface"], pipelines["ui"],
        ui_list, scene_ubo, rcdata.raycast_descset, &command_buffer);
        break;
      case raycast_mode::slice:
        record_raycast_command_buffer(
        swap_chain_image_indx, pipelines["raycast_slice"], pipelines["ui"],
        ui_list, scene_ubo, rcdata.raycast_descset, &command_buffer);
        break;
      case raycast_mode::isosurface:
        record_raycast_command_buffer(
        swap_chain_image_indx, pipelines["raycast_isosurface"], pipelines["ui"],
        ui_list, scene_ubo, rcdata.raycast_descset, &command_buffer);
        break;
    }

    // submit command buffer

    VkSubmitInfo si{};
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount   = 1;
    si.pWaitSemaphores      = &image_available_semaphore;
    si.pWaitDstStageMask    = wait_stages;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &command_buffer;
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores    = &render_finished_semaphore;
    VK_CHECK(vkQueueSubmit(graphics_queue, 1, &si, render_in_progress),
             "submission to draw command buffer failed!");

    // present

    VkPresentInfoKHR pi{};
    pi.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores    = &render_finished_semaphore;
    pi.swapchainCount     = 1;
    pi.pSwapchains        = &swap_chain;
    pi.pImageIndices      = &swap_chain_image_indx;
    pi.pResults           = nullptr;
    VK_CHECK_SUBOPTIMAL(vkQueuePresentKHR(present_queue, &pi),
                        "presentation failed!");

    // ensure rendering is finished before continuing to the next frame
    //   (this is essential in this program because you might be re-generating
    //   geometry every frame you can't afford to use several times the memory
    //   for large entities to avoid data races with the cpu and gpu are out
    //   of sync)
    vkWaitForFences(device, 1, &render_in_progress, VK_TRUE, UINT64_MAX);


    auto t1 = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> frame_time = t1 - t0;

    char title[256];
    snprintf(title, 256, "cpu frame time: %.1f ms", frame_time.count());
    glfwSetWindowTitle(window, title);
  }

  vkDeviceWaitIdle(device);

  vkDestroyFence(device, render_in_progress, nullptr);
  vkDestroySemaphore(device, render_finished_semaphore, nullptr);
  vkDestroySemaphore(device, image_available_semaphore, nullptr);
  vkFreeCommandBuffers(device, graphics_command_pool, (uint32_t)1,
                       &command_buffer);
}
