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


#include <cstring>

#include "bindings.cpp"
#include "error_vulkan.cpp"


void vkinit(bool print_vkfeatures)
{
  /*
   * GLFW window ---------------------------------------------------------------
   */

  {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);  // no OpenGL context
    window = glfwCreateWindow(
    initial_win_width, initial_win_height, "laputa", nullptr, nullptr);
    if (!window)
    {
      VKTERMINATE("GLFW window creation failed!");
    }

    glfwSetFramebufferSizeCallback(window, frame_buffer_resized_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);
  }

  /*
   * vulkan instance -----------------------------------------------------------
   */

  {
    // check for GLFW required extensions
    u32 nreq_ext         = 0;
    const char** req_ext = glfwGetRequiredInstanceExtensions(&nreq_ext);
    // find vulkan extensions
    u32 nvk_ext = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &nvk_ext, nullptr);
    VkExtensionProperties* vk_ext = new VkExtensionProperties[nvk_ext]();
    vkEnumerateInstanceExtensionProperties(nullptr, &nvk_ext, vk_ext);
    // ensure vulkan supports required GLFW extensions
    for (u32 ri = 0; ri < nreq_ext; ++ri)
    {
      bool found = false;
      for (u32 ei = 0; ei < nvk_ext; ++ei)
      {
        if (strcmp(req_ext[ri], vk_ext[ei].extensionName) == 0)
        {
          found = true;
          break;
        }
      }
      if (!found)
      {
        VKTERMINATE("required extension \"%s\" not found!", req_ext[ri]);
      }
    }

    // check if portability enumeration is supported
    bool portability_enumeration_supported = false;
    for (u64 ei = 0; ei < nvk_ext; ++ei)
    {
      if (strcmp("VK_KHR_portability_enumeration", vk_ext[ei].extensionName) ==
          0)
      {
        portability_enumeration_supported = true;
        break;
      }
    }

    // create final extension list
    u32 num_instance_extensions = nreq_ext;
    if (portability_enumeration_supported)
      ++num_instance_extensions;

    const char** instance_extensions = new const char*[num_instance_extensions];
    for (u64 ei = 0; ei < nreq_ext; ++ei)
      instance_extensions[ei] = req_ext[ei];
    if (portability_enumeration_supported)
      instance_extensions[nreq_ext] = "VK_KHR_portability_enumeration";

    if (print_vkfeatures)
    {
      printf("GLFW required extensions:\n");
      for (u32 ei = 0; ei < nreq_ext; ++ei)
      {
        printf("  %s\n", req_ext[ei]);
      }
      printf("\n");

      printf("extensions supported:\n");
      for (u32 ei = 0; ei < nvk_ext; ++ei)
      {
        printf("  %s\n", vk_ext[ei].extensionName);
      }
      printf("\n");
    }

    // determine supported validation layers
    u32 nlayers = 0;
    vkEnumerateInstanceLayerProperties(&nlayers, nullptr);
    VkLayerProperties* layers = new VkLayerProperties[nlayers];
    vkEnumerateInstanceLayerProperties(&nlayers, layers);
    // check for requested validation layers in supported list
    for (u32 vi = 0; vi < num_validation_layers; ++vi)
    {
      bool found = false;
      for (u32 si = 0; si < nlayers; ++si)
      {
        if (strcmp(validation_layers[vi], layers[si].layerName) == 0)
        {
          found = true;
          break;
        }
      }
      if (!found)
      {
        VKTERMINATE("requested validation layer \"%s\" not available!",
                  validation_layers[vi]);
      }
    }

    // create a the vulkan instance
    VkApplicationInfo app_info{};
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName   = "rend";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.engineVersion      = VK_MAKE_VERSION(0, 0, 0);
    app_info.apiVersion         = VK_API_VERSION_1_3;

    VkInstanceCreateInfo create_info{};
    create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
#ifdef __APPLE__
    create_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    if (num_validation_layers > 0)
    {
      create_info.enabledLayerCount   = num_validation_layers;
      create_info.ppEnabledLayerNames = validation_layers;
    }
    create_info.enabledExtensionCount   = num_instance_extensions;
    create_info.ppEnabledExtensionNames = instance_extensions;
    VK_CHECK(vkCreateInstance(&create_info, nullptr, &instance),
             "instance creation failed!");

    delete[] vk_ext;
    delete[] layers;

    delete[] instance_extensions;
  }

  /*
   * GLFW surface --------------------------------------------------------------
   */

  {
    VK_CHECK(glfwCreateWindowSurface(instance, window, nullptr, &surface),
             "surface creation failed!");
  }

  /*
   * physical device selection -------------------------------------------------
   */

  {
    // find list of physical devices
    u32 ndevices = 0;
    vkEnumeratePhysicalDevices(instance, &ndevices, nullptr);
    if (ndevices == 0)
    {
      VKTERMINATE("no devices supporting Vulkan found!");
    }
    VkPhysicalDevice* devices = new VkPhysicalDevice[ndevices];
    vkEnumeratePhysicalDevices(instance, &ndevices, devices);

    if (print_vkfeatures)
    {
      printf("devices found:\n");
      for (u32 di = 0; di < ndevices; ++di)
      {
        VkPhysicalDeviceProperties device_properties;
        vkGetPhysicalDeviceProperties(devices[di], &device_properties);
        printf("  %s\n", device_properties.deviceName);
      }
      printf("\n");
    }

    // picking the first device for now (already checked one exists)
    physical_device = devices[0];
    delete[] devices;
  }

  /*
   * physical device support checks (only a tad late) --------------------------
   */

  u32 num_device_extensions      = num_required_device_extensions;
  const char** device_extensions = nullptr;
  VkExtensionProperties* supported_device_extensions = nullptr;
  {
    // ensure the device supports swapchain
    u32 nextensions;
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &nextensions,
                                         nullptr);
    supported_device_extensions = new VkExtensionProperties[nextensions];
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &nextensions,
                                         supported_device_extensions);

    if (print_vkfeatures)
    {
      printf("device extensions found:\n");
      for (u32 i = 0; i < nextensions; ++i)
      {
        printf("  %s\n", supported_device_extensions[i].extensionName);
      }
      printf("\n");
    }

    for (u32 di = 0; di < num_required_device_extensions; ++di)
    {
      bool found = false;
      for (u32 ei = 0; ei < nextensions; ++ei)
      {
        if (strcmp(supported_device_extensions[ei].extensionName,
                   required_device_extensions[di]) == 0)
        {
          found = true;
          break;
        }
      }
      if (!found)
      {
        VKTERMINATE("required device extension \"%s\" not found!",
                    required_device_extensions[di]);
      }
    }

    // check if device supports portability subset
    bool portability_subset_supported = false;
    for (u64 ei = 0; ei < nextensions; ++ei)
    {
      if (strcmp("VK_KHR_portability_subset",
                 supported_device_extensions[ei].extensionName) == 0)
      {
        portability_subset_supported = true;
        break;
      }
    }

    if (portability_subset_supported)
      ++num_device_extensions;

    device_extensions = new const char*[num_device_extensions];
    for (u64 ri = 0; ri < num_required_device_extensions; ++ri)
      device_extensions[ri] = required_device_extensions[ri];
    if (portability_subset_supported)
      device_extensions[num_required_device_extensions] =
      "VK_KHR_portability_subset";
  }

  /*
   * finding queue families ----------------------------------------------------
   */

  {
    // find supported queue families
    u32 nqfams = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &nqfams, nullptr);
    VkQueueFamilyProperties* qfams = new VkQueueFamilyProperties[nqfams];
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &nqfams, qfams);

    if (print_vkfeatures)
    {
      printf("queue families:\n");
      for (u64 fi = 0; fi < nqfams; ++fi)
      {
        printf("  %d\n", (int)fi);
        printf("    queue count: %d\n", qfams[fi].queueCount);
        printf("    family types: ");

        if (qfams[fi].queueFlags & VK_QUEUE_GRAPHICS_BIT)
          printf("graphics ");
        if (qfams[fi].queueFlags & VK_QUEUE_COMPUTE_BIT)
          printf("compute ");
        if (qfams[fi].queueFlags & VK_QUEUE_TRANSFER_BIT)
          printf("transfer ");
        if (qfams[fi].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
          printf("sparse_binding ");
        if (qfams[fi].queueFlags & VK_QUEUE_PROTECTED_BIT)
          printf("protected ");

        printf("\n");
      }
      printf("\n");
    }

    // find the graphcis and presentation queue family indices
    bool graphics_found = false;
    bool present_found  = false;
    bool compute_found  = false;
    bool transfer_found = false;
    VkBool32 present_support;

    for (u32 qi = 0; qi < nqfams; ++qi)
    {
      if ((!graphics_found) && (qfams[qi].queueFlags & VK_QUEUE_GRAPHICS_BIT))
      {
        queue_family_indices.graphics = qi;
        graphics_found                = true;
      }

      if ((!compute_found) && (qfams[qi].queueFlags & VK_QUEUE_COMPUTE_BIT))
      {
        queue_family_indices.compute = qi;
        compute_found                = true;
      }

      if ((!transfer_found) && (qfams[qi].queueFlags & VK_QUEUE_TRANSFER_BIT))
      {
        queue_family_indices.transfer = qi;
        transfer_found = true;
      }

      vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, qi, surface,
                                           &present_support);
      if ((!present_found) && present_support)
      {
        queue_family_indices.presentation = qi;
        present_found                     = true;
      }

      if (present_found && graphics_found && compute_found && transfer_found)
        break;
    }

    if (!graphics_found)
      VKTERMINATE("no graphics qeueue found for the selected device!");
    if (!present_found)
      VKTERMINATE("no presentation queue family found!");
    if (!compute_found)
      VKTERMINATE("no compute queue family found!");
    if (!transfer_found)
      VKTERMINATE("no transfer queue family found!");

    delete[] qfams;
  }

  /*
   * logical device creation ---------------------------------------------------
   */

  {
    // find unique queue family indices
    u32 queue_fam_index_list[num_queue] = {queue_family_indices.graphics,
                                           queue_family_indices.presentation,
                                           queue_family_indices.compute,
                                           queue_family_indices.transfer};
    u32 num_unique_queue_fam_indices    = 0;
    s32 unique_queue_fam_indices[num_queue];
    for (u64 i = 0; i < num_queue; ++i)
      unique_queue_fam_indices[i] = -1;

    for (u32 i = 0; i < num_queue; ++i)
    {
      bool newval = true;
      for (u32 k = 0; k < num_queue; ++k)
      {
        if (queue_fam_index_list[i] == (u32)unique_queue_fam_indices[k])
        {
          newval = false;
          break;
        }
      }
      if (newval)
      {
        unique_queue_fam_indices[num_unique_queue_fam_indices] =
        queue_fam_index_list[i];
        ++num_unique_queue_fam_indices;
      }
    }

    // form logical device
    float queue_priority                                  = 1.f;
    VkDeviceQueueCreateInfo queue_create_infos[num_queue] = {};
    for (u32 qf = 0; qf < num_unique_queue_fam_indices; ++qf)
    {
      queue_create_infos[qf].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_create_infos[qf].queueFamilyIndex =
      (u32)unique_queue_fam_indices[qf];
      queue_create_infos[qf].queueCount       = 1;
      queue_create_infos[qf].pQueuePriorities = &queue_priority;
    }

    VkPhysicalDeviceFeatures device_features{};

    VkPhysicalDeviceSynchronization2Features sync2feat;
    sync2feat.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2feat.pNext            = NULL;
    sync2feat.synchronization2 = true;

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext                   = &sync2feat;
    device_create_info.pQueueCreateInfos       = queue_create_infos;
    device_create_info.queueCreateInfoCount    = num_unique_queue_fam_indices;
    device_create_info.pEnabledFeatures        = &device_features;
    device_create_info.enabledExtensionCount   = num_device_extensions;
    device_create_info.ppEnabledExtensionNames = device_extensions;

    if (num_validation_layers != 0)
    {
      device_create_info.enabledLayerCount   = num_validation_layers;
      device_create_info.ppEnabledLayerNames = validation_layers;
    }

    VK_CHECK(
    vkCreateDevice(physical_device, &device_create_info, nullptr, &device),
    "logical device creation failed!");

    // get queue handles created with the logical device
    vkGetDeviceQueue(device, queue_family_indices.graphics, 0, &graphics_queue);
    vkGetDeviceQueue(device, queue_family_indices.presentation, 0,
                     &present_queue);
    vkGetDeviceQueue(device, queue_family_indices.compute, 0, &compute_queue);
    vkGetDeviceQueue(device, queue_family_indices.transfer, 0, &transfer_queue);
  }
  delete[] supported_device_extensions;
  delete[] device_extensions;

  /*
   * command pool creation -----------------------------------------------------
   */

  {
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    pool_info.queueFamilyIndex = queue_family_indices.graphics;
    VK_CHECK(
    vkCreateCommandPool(device, &pool_info, nullptr, &graphics_command_pool),
    "command pool creation failed!");

    pool_info.queueFamilyIndex = queue_family_indices.compute;
    VK_CHECK(
    vkCreateCommandPool(device, &pool_info, nullptr, &compute_command_pool),
    "compute command pool creation failed!");

    pool_info.queueFamilyIndex = queue_family_indices.transfer;
    VK_CHECK(
    vkCreateCommandPool(device, &pool_info, nullptr, &transfer_command_pool),
    "transfer command pool creation failed!");
  }
}
