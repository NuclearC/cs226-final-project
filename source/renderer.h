#ifndef RENDERER_H_
#define RENDERER_H_

#include <vulkan/vulkan.h>

/* create the rendering resources */
int CreateRenderer(void);

VkBuffer CreateBuffer(uint32_t size, VkBufferUsageFlags usage_flags);

void Render(void);

void DestroyRenderer(void);

#endif  // RENDERER_H_
