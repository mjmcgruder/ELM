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


#include "pipeline.cpp"


void make_depth_resources()
{
  VkFormat depth_format = find_depth_format();

  make_image(
  swap_chain_extent.width / render_image_scale,
  swap_chain_extent.height / render_image_scale, depth_format,
  VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_image, depth_image_memory);

  depth_image_view =
  make_image_view(depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);
}


void make_render_image()
{
  make_image(
  swap_chain_extent.width / render_image_scale,
  swap_chain_extent.height / render_image_scale, swap_chain_image_format,
  VK_IMAGE_TILING_OPTIMAL,
  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, render_image, render_image_memory);

  render_image_view = make_image_view(render_image, swap_chain_image_format,
                                      VK_IMAGE_ASPECT_COLOR_BIT);
}


void make_swap_chain()
{
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR surf_format;
  VkPresentModeKHR present_mode;
  VkExtent2D extent;
  u32 image_count;

  /*
   * determine swap chain settings ---------------------------------------------
   */

  {
    u32 nformats;
    VkSurfaceFormatKHR* formats;
    u32 npresent_modes;
    VkPresentModeKHR* present_modes;

    // find surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface,
                                              &capabilities);

    // find surface formats
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &nformats,
                                         nullptr);
    formats = new VkSurfaceFormatKHR[nformats];
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &nformats,
                                         formats);

    // find present modes
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface,
                                              &npresent_modes, nullptr);
    present_modes = new VkPresentModeKHR[npresent_modes];
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface,
                                              &npresent_modes, present_modes);

    // ensure format and present mode support
    if (nformats == 0)
    {
      VKTERMINATE("no swap chain format support!");
    }
    if (npresent_modes == 0)
    {
      VKTERMINATE("no swap chain present mode support!");
    }

    // choose surface format
    u32 sfi;
    for (sfi = 0; sfi < nformats; ++sfi)
    {
      if (formats[sfi].format == VK_FORMAT_B8G8R8A8_SRGB &&
          formats[sfi].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
        surf_format = formats[sfi];
        break;
      }
    }
    if (sfi == nformats)
    {
      surf_format = formats[0];
    }

    // choose present mode
    u32 pmi;
    for (pmi = 0; pmi < npresent_modes; ++pmi)
    {
      if (present_modes[pmi] == VK_PRESENT_MODE_FIFO_KHR)
      {
        present_mode = present_modes[pmi];
        break;
      }
    }
    if (pmi == npresent_modes)
    {
      present_mode = present_modes[0];
    }

    // choose extent (the complexity here handles resolution scaling)
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
      extent = capabilities.currentExtent;
    }
    else
    {
      int win_width, win_height;
      glfwGetFramebufferSize(window, &win_width, &win_height);
      VkExtent2D actual_extent = {(u32)win_width, (u32)win_height};

      actual_extent.width =
      clamp(actual_extent.width, capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);
      actual_extent.height =
      clamp(actual_extent.height, capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);

      extent = actual_extent;
    }

    // these'll be useful later
    swap_chain_image_format = surf_format.format;
    swap_chain_extent       = extent;

    // choose the number of swap chain images
    image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 &&
        image_count > capabilities.maxImageCount)
    {
      image_count = capabilities.maxImageCount;
    }

    delete[] formats;
    delete[] present_modes;
  }

  /*
   * create the swap chain -----------------------------------------------------
   */

  {
    u32 queue_fam_index_list[num_queue] = {queue_family_indices.graphics,
                                           queue_family_indices.presentation};
    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface          = surface;
    create_info.minImageCount    = image_count;
    create_info.imageFormat      = surf_format.format;
    create_info.imageColorSpace  = surf_format.colorSpace;
    create_info.imageExtent      = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if (queue_family_indices.graphics != queue_family_indices.presentation)
    {
      create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
      create_info.queueFamilyIndexCount = 2;
      create_info.pQueueFamilyIndices   = queue_fam_index_list;
    }
    else
    {
      create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    create_info.preTransform   = capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode    = present_mode;
    create_info.clipped        = VK_TRUE;
    create_info.oldSwapchain   = VK_NULL_HANDLE;
    VK_CHECK(vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain),
             "swap chain creation failed!");
  }

  /*
   * find swap chain images ----------------------------------------------------
   */

  {
    vkGetSwapchainImagesKHR(device, swap_chain, &num_swap_chain_images,
                            nullptr);
    swap_chain_images = new VkImage[num_swap_chain_images];
    vkGetSwapchainImagesKHR(device, swap_chain, &num_swap_chain_images,
                            swap_chain_images);
  }

  /*
   * make swap chain image views -----------------------------------------------
   */

  {
    swap_chain_image_views = new VkImageView[num_swap_chain_images];
    for (u64 i = 0; i < num_swap_chain_images; ++i)
    {
      VkImageViewCreateInfo create_info{};
      create_info.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      create_info.image        = swap_chain_images[i];
      create_info.viewType     = VK_IMAGE_VIEW_TYPE_2D;
      create_info.format       = swap_chain_image_format;
      create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      create_info.subresourceRange.baseMipLevel   = 0;
      create_info.subresourceRange.levelCount     = 1;
      create_info.subresourceRange.baseArrayLayer = 0;
      create_info.subresourceRange.layerCount     = 1;

      VK_CHECK(vkCreateImageView(device, &create_info, nullptr,
                                 &swap_chain_image_views[i]),
               "image view #%d creation failed!", (int)i);
    }
  }
}


void make_swap_chain_dependencies(
std::unordered_map<std::string, graphics_pipeline>& pipelines)
{
  make_swap_chain();
  make_depth_resources();
  make_render_image();
  for (auto& keyval : pipelines)
  {
    keyval.second.update_swap_chain_dependencies();
  }
}


void remake_swap_chain(
std::unordered_map<std::string, graphics_pipeline>& pipelines)
{
  // don't do anything if minimized
  int width = 0, height = 0;
  glfwGetFramebufferSize(window, &width, &height);
  while (width == 0 || height == 0)
  {
    glfwGetFramebufferSize(window, &width, &height);
    glfwWaitEvents();
  }
  vkDeviceWaitIdle(device);

  clean_swap_chain();

  make_swap_chain_dependencies(pipelines);
}
