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


#include <vulkan/vulkan_core.h>
#include <string>
#include <unordered_map>

#include "buffers.cpp"
#include "pipeline.cpp"
#include "entity.cpp"


void transition_image_layout(VkCommandBuffer* command_buffer, VkImage* image,
                             VkPipelineStageFlags2 src_stages,
                             VkPipelineStageFlags2 dst_stages,
                             VkImageLayout old_layout, VkImageLayout new_layout)
{
  VkImageSubresourceRange imgrange{};
  imgrange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  imgrange.baseMipLevel   = 0;
  imgrange.levelCount     = 1;
  imgrange.baseArrayLayer = 0;
  imgrange.layerCount     = 1;

  VkImageMemoryBarrier2 imgbar{};
  imgbar.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  imgbar.pNext               = nullptr;
  imgbar.srcStageMask        = src_stages;
  imgbar.srcAccessMask       = 0;
  imgbar.dstStageMask        = dst_stages;
  imgbar.dstAccessMask       = 0;
  imgbar.oldLayout           = old_layout;
  imgbar.newLayout           = new_layout;
  imgbar.srcQueueFamilyIndex = 0;
  imgbar.dstQueueFamilyIndex = 0;
  imgbar.image               = *image;
  imgbar.subresourceRange    = imgrange;

  VkDependencyInfo depinfo{};
  depinfo.sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  depinfo.pNext                    = nullptr;
  depinfo.dependencyFlags          = 0;
  depinfo.memoryBarrierCount       = 0;
  depinfo.pMemoryBarriers          = nullptr;
  depinfo.bufferMemoryBarrierCount = 0;
  depinfo.pBufferMemoryBarriers    = nullptr;
  depinfo.imageMemoryBarrierCount  = 1;
  depinfo.pImageMemoryBarriers     = &imgbar;

  vkCmdPipelineBarrier2(*command_buffer, &depinfo);
}


void record_raycast_command_buffer(
u32 swap_chain_image, graphics_pipeline& raycast_pipeline,
graphics_pipeline& ui_pipeline,
std::unordered_map<std::string, entity>& ui_list,
uniform<scene_transform>& scene_ubo, descriptor_set& solution_dset,
VkCommandBuffer* command_buffer)
{
  const u32 nclear_values                  = 2;
  VkClearValue clear_values[nclear_values] = {};
  clear_values[0].color                    = {{0.0f, 0.0f, 0.0f, 1.0f}};
  clear_values[1].depthStencil             = {1.0f, 0};

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.pInheritanceInfo = nullptr;

  VkExtent2D render_extent;
  render_extent.width  = swap_chain_extent.width / render_image_scale;
  render_extent.height = swap_chain_extent.height / render_image_scale;

  VK_CHECK(vkBeginCommandBuffer(*command_buffer, &begin_info),
           "failed to start a command buffer!");

  /* scene pass */

  VkRenderPassBeginInfo scene_pass_info{};
  scene_pass_info.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  scene_pass_info.renderPass  = raycast_pipeline.render_pass;
  scene_pass_info.framebuffer = raycast_pipeline.framebuffers[0];
  scene_pass_info.renderArea.offset = {0, 0};
  scene_pass_info.renderArea.extent = render_extent;
  scene_pass_info.clearValueCount   = nclear_values;
  scene_pass_info.pClearValues      = clear_values;

  vkCmdBeginRenderPass(*command_buffer, &scene_pass_info,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(*command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    raycast_pipeline.pipeline);

  vkCmdBindDescriptorSets(*command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          raycast_pipeline.layout, 0, 1, &scene_ubo.dset.dset,
                          0, nullptr);

  vkCmdBindDescriptorSets(*command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          raycast_pipeline.layout, 2, 1, &solution_dset.dset, 0,
                          nullptr);

  vkCmdDraw(*command_buffer, 6, 1, 0, 0);

  vkCmdEndRenderPass(*command_buffer);

  /* ui pass */

  if (render_ui)
  {

    VkDeviceSize offsets[] = {0};

    VkRenderPassBeginInfo ui_pass_info{};
    ui_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    ui_pass_info.renderPass        = ui_pipeline.render_pass;
    ui_pass_info.framebuffer       = ui_pipeline.framebuffers[0];
    ui_pass_info.renderArea.offset = {0, 0};
    ui_pass_info.renderArea.extent = render_extent;
    ui_pass_info.clearValueCount   = nclear_values;
    ui_pass_info.pClearValues      = clear_values;

    vkCmdBeginRenderPass(*command_buffer, &ui_pass_info,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(*command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      ui_pipeline.pipeline);

    vkCmdBindDescriptorSets(*command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            ui_pipeline.layout, 0, 1, &scene_ubo.dset.dset, 0,
                            nullptr);

    for (const auto& pair : ui_list)
    {
      const entity& obj = pair.second;

      vkCmdBindVertexBuffers(*command_buffer, 0, 1, &obj.vertices.buffer,
                             offsets);

      vkCmdBindIndexBuffer(*command_buffer, obj.indices.buffer, 0,
                           VK_INDEX_TYPE_UINT32);

      vkCmdBindDescriptorSets(*command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              ui_pipeline.layout, 1, 1, &obj.ubo.dset.dset, 0,
                              nullptr);

      vkCmdDrawIndexed(*command_buffer, (u32)obj.indices.nelems, 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(*command_buffer);

  }

  // blit to full image

  transition_image_layout(
  command_buffer, &swap_chain_images[swap_chain_image],
  VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkImageSubresourceLayers resource_layers;
  resource_layers.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  resource_layers.mipLevel       = 0;
  resource_layers.baseArrayLayer = 0;
  resource_layers.layerCount     = 1;

  VkImageBlit blit;
  blit.srcSubresource = resource_layers;
  blit.srcOffsets[0]  = {0, 0, 0};
  blit.srcOffsets[1]  = {int(render_extent.width), int(render_extent.height), 1};
  blit.dstSubresource = resource_layers;
  blit.dstOffsets[0]  = {0, 0, 0};
  blit.dstOffsets[1]  = {int(swap_chain_extent.width), int(swap_chain_extent.height), 1};

  vkCmdBlitImage(*command_buffer,
                 render_image, VK_IMAGE_LAYOUT_GENERAL,
                 swap_chain_images[swap_chain_image], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                 1, &blit, VK_FILTER_LINEAR);

  transition_image_layout(
  command_buffer, &swap_chain_images[swap_chain_image],
  VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  // --

  VK_CHECK(vkEndCommandBuffer(*command_buffer),
           "failed to end command buffer!");
}
