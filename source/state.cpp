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

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "colormaps.cpp"
#include "dg_solution.cpp"


/* -------------------- */
/* render configuration */
/* -------------------- */

#ifndef SHADER_DIR
#define SHADER_DIR "./"
#endif

/* --------------- */
/* game play state */
/* --------------- */

dg_solution  rendering_data;
render_field current_field;

double cam_dist            = 8.;
double cursor_xpos         = 0.;
double cursor_ypos         = 0.;
double last_cursor_xpos    = 0.;
double last_cursor_ypos    = 0.;
double mouse_dx            = 0.;
double mouse_dy            = 0.;
bool   mouse_left_pressed  = false;
bool   mouse_right_pressed = false;

raycast_mode RAYCAST_MODE  = raycast_mode::surface;
output_type  render_output = output_type::mach;
float*       colormap      = colormap_jet;

bool mesh_display_toggle_on = false;
bool modify_slice           = false;
bool view_axis_x_set        = false;
bool view_axis_y_set        = false;
bool view_axis_z_set        = false;
bool render_ui              = true;

/* --------------- */
/* rendering state */
/* --------------- */

/* constatns */

const u32 initial_win_width  = 800;
const u32 initial_win_height = 600;

/* fundamentals */

GLFWwindow*      window                = nullptr;
VkInstance       instance              = VK_NULL_HANDLE;
VkSurfaceKHR     surface               = VK_NULL_HANDLE;
VkPhysicalDevice physical_device       = VK_NULL_HANDLE;
VkDevice         device                = VK_NULL_HANDLE;
VkCommandPool    graphics_command_pool = VK_NULL_HANDLE;
VkCommandPool    compute_command_pool  = VK_NULL_HANDLE;
VkCommandPool    transfer_command_pool = VK_NULL_HANDLE;

/* swap chain and dependencies */

VkSwapchainKHR swap_chain             = VK_NULL_HANDLE;
u32            num_swap_chain_images  = 0;
VkImage*       swap_chain_images      = nullptr;
VkImageView*   swap_chain_image_views = nullptr;
VkFormat       swap_chain_image_format;
VkExtent2D     swap_chain_extent;

VkImage        depth_image        = VK_NULL_HANDLE;
VkDeviceMemory depth_image_memory = VK_NULL_HANDLE;
VkImageView    depth_image_view   = VK_NULL_HANDLE;

VkImage        render_image        = VK_NULL_HANDLE;
VkDeviceMemory render_image_memory = VK_NULL_HANDLE;
VkImageView    render_image_view   = VK_NULL_HANDLE;
usize          render_image_scale  = 1;

bool frame_buffer_resized = false;

/* queue information */

const u32 num_queue = 4;
struct QueueFamilyIndices
{
  u32 graphics;
  u32 presentation;
  u32 compute;
  u32 transfer;
} queue_family_indices;

VkQueue graphics_queue = VK_NULL_HANDLE;
VkQueue present_queue  = VK_NULL_HANDLE;
VkQueue compute_queue  = VK_NULL_HANDLE;
VkQueue transfer_queue = VK_NULL_HANDLE;

/* validation layer info */

const u32 num_validation_layers                      = 1;
const char* validation_layers[num_validation_layers] = {
"VK_LAYER_KHRONOS_validation",
};

/* requested device extension info */

const u32 num_required_device_extensions                               = 2;
const char* required_device_extensions[num_required_device_extensions] = {
VK_KHR_SWAPCHAIN_EXTENSION_NAME,
VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
};

/* ---------------------------- */
/* memory management and errors */
/* ---------------------------- */

void clean_swap_chain()
{
  vkDestroySwapchainKHR(device, swap_chain, nullptr);
  for (u32 i = 0; i < num_swap_chain_images; ++i)
  {
    vkDestroyImageView(device, swap_chain_image_views[i], nullptr);
  }
  vkDestroyImageView(device, depth_image_view, nullptr);
  vkDestroyImage(device, depth_image, nullptr);
  vkFreeMemory(device, depth_image_memory, nullptr);
  vkDestroyImageView(device, render_image_view, nullptr);
  vkDestroyImage(device, render_image, nullptr);
  vkFreeMemory(device, render_image_memory, nullptr);
  delete[] swap_chain_images;
  delete[] swap_chain_image_views;
}

void clean_global_resources()
{
  clean_swap_chain();
  vkDestroyCommandPool(device, graphics_command_pool, nullptr);
  vkDestroyCommandPool(device, compute_command_pool, nullptr);
  vkDestroyCommandPool(device, transfer_command_pool, nullptr);
  vkDestroyDevice(device, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);
  glfwDestroyWindow(window);
  glfwTerminate();
}

#define ERR_MSG_SIZE 4096
void clean_and_exit(const char* file, int line, ...)
{
  // print a nice messsage
  char message[ERR_MSG_SIZE];
  va_list args;
  va_start(args, line);
  vsnprintf(message, ERR_MSG_SIZE, va_arg(args, char*), args);
  va_end(args);
  printf("error! (line %d of %s):\n", line, file);
  printf("  %s\n\n", message);
  // kill the fancy objects
  clean_global_resources();
  // bail out
  exit(1);
}

#define VKTERMINATE(...) clean_and_exit(__FILE__, __LINE__, __VA_ARGS__)

#define VK_CHECK(func, ...)                     \
  {                                             \
    VkResult fail_result;                       \
    if ((fail_result = (func)) != VK_SUCCESS)   \
    {                                           \
      printf("result code: %d\n", fail_result); \
      VKTERMINATE(__VA_ARGS__);                 \
    }                                           \
  }

#define VK_CHECK_SUBOPTIMAL(func, ...)             \
  {                                                \
    VkResult fail_result = (func);                 \
    if (fail_result != VK_SUCCESS &&               \
        fail_result != VK_ERROR_OUT_OF_DATE_KHR && \
        fail_result != VK_SUBOPTIMAL_KHR)          \
    {                                              \
      printf("result code: %d\n", fail_result);    \
      VKTERMINATE(__VA_ARGS__);                    \
    }                                              \
  }
