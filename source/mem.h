#ifndef MEM_H_
#define MEM_H_

#include <vulkan/vulkan.h>

int FindRequiredMemoryType(VkMemoryPropertyFlags property_flags,
                           uint32_t type_bits);
int AllocateBufferMemory(VkBuffer buffer, VkMemoryPropertyFlags property_flags,
                         VkDeviceMemory* device_memory);

int AllocateImagesMemory(VkImage* images, uint32_t count,
                         VkMemoryPropertyFlags property_flags,
                         VkDeviceMemory* device_memory);

#endif  // MEM_H_
