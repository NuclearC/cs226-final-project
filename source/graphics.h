
#ifndef GRAPHICS_H_
#define GRAPHICS_H_

#include <stdint.h>
#include <vulkan/vulkan.h>
/* Initialize Vulkan instance, device, and so on */
int VulkanInitialize(void);

int VulkanCreateSurface(void* window);
int VulkanCreateSwapchain(uint32_t width, uint32_t height);

int VulkanSCAcquireImage(void);
int VulkanSCPresent(void);

void VulkanCleanup(void);

#endif  // GRAPHICS_H_
