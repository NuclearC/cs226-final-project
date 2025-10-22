
#include "mem.h"

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

  return 0;
}
