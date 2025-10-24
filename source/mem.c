
#include "mem.h"

#include <stdlib.h>

extern VkDevice device;
extern VkPhysicalDevice physical_device;

static bool memory_properties_retrieved = false;
static VkPhysicalDeviceMemoryProperties memory_properties;

int FindRequiredMemoryType(VkMemoryPropertyFlags property_flags,
                           uint32_t type_bits) {
  /* caching xd */
  if (!memory_properties_retrieved) {
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
    memory_properties_retrieved = true;
  }
  uint32_t memory_type_index = 0;
  for (; memory_type_index < memory_properties.memoryTypeCount;
       memory_type_index++) {
    if (memory_properties.memoryTypes[memory_type_index].propertyFlags &
            property_flags == property_flags &&
        (type_bits & (1 << memory_type_index)) != 0) {
      /* we found the required memory type */
      uint32_t heap_index =
          memory_properties.memoryTypes[memory_type_index].heapIndex;
      return memory_type_index;
    }
  }

  return memory_properties.memoryTypeCount;
}

int AllocateBufferMemory(VkBuffer buffer, VkMemoryPropertyFlags property_flags,
                         VkDeviceMemory* device_memory) {
  VkMemoryRequirements req = {};
  vkGetBufferMemoryRequirements(device, buffer, &req);

  uint32_t memory_type_index =
      FindRequiredMemoryType(property_flags, req.memoryTypeBits);

  if (memory_type_index == memory_properties.memoryTypeCount) {
    return -1;
  }

  VkMemoryAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = req.size;
  alloc_info.memoryTypeIndex = memory_type_index;

  if (VK_SUCCESS !=
      vkAllocateMemory(device, &alloc_info, VK_NULL_HANDLE, device_memory)) {
    return -1;
  }

  if (VK_SUCCESS != vkBindBufferMemory(device, buffer, *device_memory, 0)) {
    return -1;
  }

  return 0;
}

int AllocateImagesMemory(VkImage* images, uint32_t count,
                         VkMemoryPropertyFlags property_flags,
                         VkDeviceMemory* device_memory) {
  VkMemoryRequirements req = {};

  VkDeviceSize* offsets = (VkDeviceSize*)malloc(sizeof(VkDeviceSize) * count);
  VkDeviceSize required_size = 0;
  uint32_t required_type_bits = 0;

  offsets[0] = 0;

  for (uint32_t i = 0; i < count; i++) {
    vkGetImageMemoryRequirements(device, images[i], &req);
    required_type_bits |= req.memoryTypeBits;

    required_size += req.size;

    if (required_size % req.alignment > 0 && i + 1 < count) {
      required_size += req.alignment - required_size % req.alignment;
    }

    if (i + 1 < count) {
      offsets[i + 1] = required_size;
    }
  }

  uint32_t memory_type_index =
      FindRequiredMemoryType(property_flags, required_type_bits);

  if (memory_type_index == memory_properties.memoryTypeCount) {
    free(offsets);
    return -1;
  }

  VkMemoryAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = required_size;
  alloc_info.memoryTypeIndex = memory_type_index;

  if (VK_SUCCESS !=
      vkAllocateMemory(device, &alloc_info, VK_NULL_HANDLE, device_memory)) {
    free(offsets);
    return -1;
  }

  for (uint32_t i = 0; i < count; i++) {
    if (VK_SUCCESS !=
        vkBindImageMemory(device, images[i], *device_memory, offsets[i])) {
      free(offsets);
      return -1;
    }
  }

  free(offsets);

  return 0;
}
