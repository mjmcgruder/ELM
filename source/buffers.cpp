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
#include <utility>

#include "basic_types.cpp"
#include "transform.cpp"


// misc helper functions

VkFormat find_supported_format(const VkFormat* candidates, u32 ncandidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features);

VkFormat find_depth_format();

u32 find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties);


// image helper functions

void make_image(u32 width, u32 height, VkFormat format, VkImageTiling tiling,
                VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                VkImage& image, VkDeviceMemory& image_memory);

VkImageView make_image_view(VkImage image, VkFormat format,
                            VkImageAspectFlags aspect_flags);


// buffer helper functions

void make_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                 VkMemoryPropertyFlags properties, VkBuffer& buffer,
                 VkDeviceMemory& buffer_memory);

void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);


// device buffer object and memory transfer handling

template<typename T>
struct dbuffer
{
  VkBuffer           buffer;
  VkDeviceMemory     memory;
  u32                nelems;
  VkBufferUsageFlags usage_flags;

  // ---

  dbuffer();
  dbuffer(u32 _nelems,
          VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

  dbuffer(const dbuffer& oth)      = delete;
  dbuffer& operator=(dbuffer& oth) = delete;

  dbuffer(dbuffer&& oth)            noexcept;
  dbuffer& operator=(dbuffer&& oth) noexcept;

  ~dbuffer();

  // ---

  void clear();
};

template<typename T>
void dmalloc(dbuffer<T>& b);

template<typename T>
void memcpy_htod(dbuffer<T>& dst, T* src);

template<typename T>
void memcpy_dtoh(T* dst, dbuffer<T>& src);


/* IMPLEMENTATION ----------------------------------------------------------- */


VkFormat find_supported_format(const VkFormat* candidates, u32 ncandidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features)
{
  VkFormat format;
  for (u32 i = 0; i < ncandidates; ++i)
  {
    format = candidates[i];
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);
    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features)
    {
      return format;
    }
    else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
             (props.optimalTilingFeatures & features) == features)
    {
      return format;
    }
  }
  VKTERMINATE("failed to find a format for depth buffering!");
  return format;
}

VkFormat find_depth_format()
{
  const u32 ncandidates            = 3;
  VkFormat candidates[ncandidates] = {VK_FORMAT_D32_SFLOAT,
                                      VK_FORMAT_D32_SFLOAT_S8_UINT,
                                      VK_FORMAT_D24_UNORM_S8_UINT};
  VkFormat depth_format =
  find_supported_format(candidates, ncandidates, VK_IMAGE_TILING_OPTIMAL,
                        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
  return depth_format;
}

u32 find_memory_type(u32 type_filter, VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);
  for (u32 i = 0; i < mem_properties.memoryTypeCount; ++i)
  {
    if ((type_filter & (1 << i)) &&
        ((mem_properties.memoryTypes[i].propertyFlags & properties) ==
         properties))
    {
      return i;
    }
  }
  VKTERMINATE("failed to find a suitable memory type!");
  return 1;
}

void make_image(u32 width, u32 height, VkFormat format, VkImageTiling tiling,
                VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                VkImage& image, VkDeviceMemory& image_memory)
{
  VkImageCreateInfo cinf{};
  cinf.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  cinf.imageType     = VK_IMAGE_TYPE_2D;
  cinf.extent.width  = width;
  cinf.extent.height = height;
  cinf.extent.depth  = 1;
  cinf.mipLevels     = 1;
  cinf.arrayLayers   = 1;
  cinf.format        = format;
  cinf.tiling        = tiling;
  cinf.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  cinf.usage         = usage;
  cinf.samples       = VK_SAMPLE_COUNT_1_BIT;
  cinf.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
  VK_CHECK(vkCreateImage(device, &cinf, nullptr, &image),
           "image creation failed!");

  VkMemoryRequirements mem_req;
  vkGetImageMemoryRequirements(device, image, &mem_req);

  VkMemoryAllocateInfo ainf{};
  ainf.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  ainf.allocationSize  = mem_req.size;
  ainf.memoryTypeIndex = find_memory_type(mem_req.memoryTypeBits, properties);
  VK_CHECK(vkAllocateMemory(device, &ainf, nullptr, &image_memory),
           "failed to allocate image memory!");

  vkBindImageMemory(device, image, image_memory, 0);
}

VkImageView make_image_view(VkImage image, VkFormat format,
                            VkImageAspectFlags aspect_flags)
{
  VkImageView image_view;

  VkImageViewCreateInfo cinf{};
  cinf.sType                         = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  cinf.image                         = image;
  cinf.viewType                      = VK_IMAGE_VIEW_TYPE_2D;
  cinf.format                        = format;
  cinf.subresourceRange.aspectMask   = aspect_flags;
  cinf.subresourceRange.baseMipLevel = 0;
  cinf.subresourceRange.levelCount   = 1;
  cinf.subresourceRange.baseArrayLayer = 0;
  cinf.subresourceRange.layerCount     = 1;
  VK_CHECK(vkCreateImageView(device, &cinf, nullptr, &image_view),
           "failed to create image view!");

  return image_view;
}

void make_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                 VkMemoryPropertyFlags properties, VkBuffer& buffer,
                 VkDeviceMemory& buffer_memory)
{

  VkBufferCreateInfo buffer_create_info{};
  buffer_create_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_create_info.size        = size;
  buffer_create_info.usage       = usage;
  buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VK_CHECK(vkCreateBuffer(device, &buffer_create_info, nullptr, &buffer),
           "buffer creation failed!");

  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex =
  find_memory_type(mem_requirements.memoryTypeBits, properties);
  VK_CHECK(vkAllocateMemory(device, &alloc_info, nullptr, &buffer_memory),
           "failed to allocate buffer memory!");

  vkBindBufferMemory(device, buffer, buffer_memory, 0);
}

void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
  // allocating the command buffer
  VkCommandBuffer command_buffer;
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandPool = transfer_command_pool;
  alloc_info.commandBufferCount = 1;
  vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

  // record command buffer

  VkCommandBufferBeginInfo begin_info{};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(command_buffer, &begin_info);

  VkBufferCopy copy_region{};
  copy_region.srcOffset = 0;
  copy_region.dstOffset = 0;
  copy_region.size      = size;
  vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

  vkEndCommandBuffer(command_buffer);

  // execute command buffer to transfer data

  VkSubmitInfo submit_info{};
  submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers    = &command_buffer;
  vkQueueSubmit(transfer_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(transfer_queue);

  vkFreeCommandBuffers(device, transfer_command_pool, 1, &command_buffer);
}

/* ------- */
/* dbuffer ------------------------------------------------------------------ */
/* ------- */

template<typename T>
dbuffer<T>::dbuffer()
{
  buffer      = VK_NULL_HANDLE;
  memory      = VK_NULL_HANDLE;
  nelems      = 0;
  usage_flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
}

template<typename T>
dbuffer<T>::dbuffer(u32 _nelems, VkBufferUsageFlags usage)
{
  buffer = VK_NULL_HANDLE;
  memory = VK_NULL_HANDLE;
  nelems = _nelems;

  usage_flags = usage;
}

template<typename T>
void dbuffer<T>::clear()
{
  vkDestroyBuffer(device, buffer, nullptr);
  vkFreeMemory(device, memory, nullptr);
}

template<typename T>
dbuffer<T>::~dbuffer()
{
  clear();
}

template<typename T>
dbuffer<T>::dbuffer(dbuffer&& oth) noexcept :
buffer(      std::move(oth.buffer)),
memory(      std::move(oth.memory)),
nelems(      std::move(oth.nelems)),
usage_flags( std::move(oth.usage_flags))
{
  oth.buffer        = VK_NULL_HANDLE;
  oth.memory        = VK_NULL_HANDLE;
  oth.nelems        = 0;
  oth.usage_flags   = 0;
}

template<typename T>
dbuffer<T>& dbuffer<T>::operator=(dbuffer&& oth) noexcept
{
  clear();

  buffer      = std::move(oth.buffer);
  memory      = std::move(oth.memory);
  nelems      = std::move(oth.nelems);
  usage_flags = std::move(oth.usage_flags);

  oth.buffer      = VK_NULL_HANDLE;
  oth.memory      = VK_NULL_HANDLE;
  oth.nelems      = 0;
  oth.usage_flags = 0;

  return *this;
}

template<typename T>
void dmalloc(dbuffer<T>& b)
{
  b.clear();

  VkDeviceSize buffer_size = b.nelems * sizeof(T);
  make_buffer(buffer_size,
              VK_BUFFER_USAGE_TRANSFER_DST_BIT |
              VK_BUFFER_USAGE_TRANSFER_SRC_BIT | b.usage_flags,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, b.buffer, b.memory);
}

template<typename T>
void memcpy_htod(dbuffer<T>& dst, T* src)
{
  VkDeviceSize buffer_size = dst.nelems * sizeof(*src);

  VkBuffer staging_buffer              = VK_NULL_HANDLE;
  VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

  make_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              staging_buffer, staging_buffer_memory);

  void* data;
  vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
  memcpy(data, src, (size_t)buffer_size);
  vkUnmapMemory(device, staging_buffer_memory);

  copy_buffer(staging_buffer, dst.buffer, buffer_size);

  vkDestroyBuffer(device, staging_buffer, nullptr);
  vkFreeMemory(device, staging_buffer_memory, nullptr);
}

template<typename T>
void memcpy_dtoh(T* dst, dbuffer<T>& src)
{
  u64 buffer_size = src.nelems * sizeof(*dst);

  VkBuffer staging_buffer              = VK_NULL_HANDLE;
  VkDeviceMemory staging_buffer_memory = VK_NULL_HANDLE;

  make_buffer(
  buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
  staging_buffer, staging_buffer_memory);

  copy_buffer(src.buffer, staging_buffer, buffer_size);

  void* data;
  vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
  memcpy(dst, data, (size_t)buffer_size);
  vkUnmapMemory(device, staging_buffer_memory);

  vkDestroyBuffer(device, staging_buffer, nullptr);
  vkFreeMemory(device, staging_buffer_memory, nullptr);
}
