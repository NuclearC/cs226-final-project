
#include "renderer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "mem.h"

extern VkDevice device;
extern uint32_t queue_family_index;
extern VkQueue graphics_queue;
extern uint32_t swapchain_image_count;
extern uint32_t swapchain_frame_count;
extern uint32_t swapchain_current_frame;
extern uint32_t swapchain_current_image;
extern VkExtent2D swapchain_size;
extern VkImage* swapchain_images;
extern VkImageView* swapchain_image_views;
extern VkSemaphore* image_available_semaphores;
extern VkSemaphore* render_finished_semaphores;
extern VkFence* in_flight_fences;

static VkCommandPool command_pool = VK_NULL_HANDLE;

VkCommandBuffer* command_buffers = NULL;

static VkBuffer vertex_buffer;
static VkDeviceMemory vertex_buffer_memory;

/* depth image */
static VkImage* depth_images;
static VkImageView* depth_image_views;
static VkDeviceMemory depth_images_memory;
static VkFormat depth_image_format;

static int CreateCommandBuffers(void) {
  VkCommandPoolCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  create_info.queueFamilyIndex = queue_family_index;

  if (VK_SUCCESS != vkCreateCommandPool(device, &create_info, VK_NULL_HANDLE,
                                        &command_pool)) {
    return -1;
  }

  VkCommandBufferAllocateInfo allocate_info = {};
  allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  allocate_info.commandPool = command_pool;
  allocate_info.commandBufferCount = swapchain_image_count;

  command_buffers =
      (VkCommandBuffer*)malloc(sizeof(VkCommandBuffer) * swapchain_image_count);
  if (VK_SUCCESS !=
      vkAllocateCommandBuffers(device, &allocate_info, command_buffers)) {
    return -1;
  }

  return 0;
}

static int SubmitRenderCommandBuffer() {
  VkCommandBuffer cmd = command_buffers[swapchain_current_frame];

  VkSubmitInfo submit_info = {};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores =
      &image_available_semaphores[swapchain_current_frame];
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores =
      &render_finished_semaphores[swapchain_current_image];

  VkPipelineStageFlags wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submit_info.pWaitDstStageMask = wait_stages;

  vkQueueSubmit(graphics_queue, 1, &submit_info,
                in_flight_fences[swapchain_current_frame]);
}

static int CreateDepthImages(void) {
  depth_image_format = VK_FORMAT_D32_SFLOAT;

  depth_images = (VkImage*)malloc(sizeof(VkImage) * swapchain_frame_count);
  depth_image_views =
      (VkImageView*)malloc(sizeof(VkImageView) * swapchain_frame_count);
  VkImageCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .mipLevels = 1,
      .imageType = VK_IMAGE_TYPE_2D,
      .extent =
          {
              .width = swapchain_size.width,
              .height = swapchain_size.height,
              .depth = 1,
          },
      .arrayLayers = 1,
      .format = depth_image_format};

  VkImageViewCreateInfo view_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .subresourceRange = {.levelCount = 1,
                           .layerCount = 1,
                           .baseArrayLayer = 0,
                           .baseMipLevel = 0,
                           .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT},
      .format = depth_image_format,
      .components = {
          VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
          VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY}};

  for (uint32_t i = 0; i < swapchain_frame_count; i++) {
    if (VK_SUCCESS !=
        vkCreateImage(device, &create_info, VK_NULL_HANDLE, &depth_images[i])) {
      return -1;
    }
  }

  if (0 != AllocateImagesMemory(depth_images, swapchain_frame_count,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                &depth_images_memory)) {
    return -1;
  }

  for (uint32_t i = 0; i < swapchain_frame_count; i++) {
    view_create_info.image = depth_images[i];
    if (VK_SUCCESS != vkCreateImageView(device, &view_create_info,
                                        VK_NULL_HANDLE,
                                        &depth_image_views[i])) {
      return -1;
    }
  }
  return 0;
}

VkBuffer CreateBuffer(uint32_t size, VkBufferUsageFlags usage_flags) {
  VkBufferCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info.size = size;
  create_info.usage = usage_flags;

  VkBuffer res = VK_NULL_HANDLE;
  if (VK_SUCCESS !=
      vkCreateBuffer(device, &create_info, VK_NULL_HANDLE, &res)) {
    fprintf(stderr, "failed to create buffer %d %d \n", size, usage_flags);
  }

  return res;
}

int CreateRenderer(void) {
  if (0 != CreateCommandBuffers()) {
    fprintf(stderr, "Failed to create command buffers");
    return -1;
  }

  vertex_buffer = CreateBuffer(100u, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  if (0 != AllocateBufferMemory(vertex_buffer,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                &vertex_buffer_memory)) {
    return -1;
  }

  if (0 != CreateDepthImages()) {
    return -1;
  }
}

static void PreRender(void) {
  VkCommandBuffer cmd = command_buffers[swapchain_current_frame];
  /* transition the current image to Present layout */
  VkImageMemoryBarrier image_memory_barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .image = swapchain_images[swapchain_current_image],
      .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
      }};

  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,
                       nullptr, 0, nullptr, 1, &image_memory_barrier);
}

static void PostRender(void) {
  VkCommandBuffer cmd = command_buffers[swapchain_current_frame];
  /* transition the current image to Present layout */
  VkImageMemoryBarrier image_memory_barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .image = swapchain_images[swapchain_current_image],
      .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
      }};

  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &image_memory_barrier);
}

void Render(void) {
  VkCommandBuffer cmd = command_buffers[swapchain_current_frame];

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  /* reset and begin rendering */
  vkResetCommandBuffer(cmd, 0);
  vkBeginCommandBuffer(cmd, &begin_info);

  PreRender();

  /* dynamic rendering */
  VkRenderingInfo rendering_info = {};
  rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  VkRenderingAttachmentInfo attachment_info = {};
  attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

  static float wtf = 0.f;
  wtf += 0.01f;

  VkClearColorValue clear_value = {
      .float32 = {sin(wtf) * sin(wtf), 1.0f - sin(wtf) * sin(wtf), 0.f, 1.f}};
  VkClearDepthStencilValue depth_clear_value = {.depth = 1.f, .stencil = 0};

  attachment_info.clearValue.color = clear_value;
  attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachment_info.imageView = swapchain_image_views[swapchain_current_image];
  attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  rendering_info.colorAttachmentCount = 1;
  rendering_info.pColorAttachments = &attachment_info;

  VkRenderingAttachmentInfo depth_attachment = {};
  depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  depth_attachment.clearValue.depthStencil = depth_clear_value;
  depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
  depth_attachment.imageView = depth_image_views[swapchain_current_frame];
  depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  rendering_info.pDepthAttachment = &depth_attachment;

  rendering_info.layerCount = 1;
  VkRect2D render_area = {.offset = {.x = 0, .y = 0},
                          .extent = {.width = 800, .height = 600}};
  rendering_info.renderArea = render_area;

  vkCmdBeginRendering(cmd, &rendering_info);

  /* TODO: rendering */

  vkCmdEndRendering(cmd);

  PostRender();

  vkEndCommandBuffer(cmd);

  SubmitRenderCommandBuffer();
}

static void DestroyDepthImages(void) {
  for (uint32_t i = 0; i < swapchain_frame_count; i++) {
    if (depth_images[i] != VK_NULL_HANDLE) {
      vkDestroyImage(device, depth_images[i], VK_NULL_HANDLE);
    }
    if (depth_image_views[i] != VK_NULL_HANDLE) {
      vkDestroyImageView(device, depth_image_views[i], VK_NULL_HANDLE);
    }
    if (depth_images_memory != VK_NULL_HANDLE) {
      vkFreeMemory(device, depth_images_memory, VK_NULL_HANDLE);
    }
  }
}

static void DestroyCommandBuffers(void) {
  if (command_buffers != NULL) {
    vkFreeCommandBuffers(device, command_pool, swapchain_image_count,
                         command_buffers);
    free(command_buffers);
  }
  if (command_pool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device, command_pool, VK_NULL_HANDLE);
  }
}
void DestroyRenderer(void) {
  vkDeviceWaitIdle(device);
  DestroyDepthImages();
  DestroyCommandBuffers();
}
