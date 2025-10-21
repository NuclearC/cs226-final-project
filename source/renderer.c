
#include "renderer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

extern VkDevice device;
extern uint32_t queue_family_index;
extern VkQueue graphics_queue;
extern uint32_t swapchain_image_count;
extern uint32_t swapchain_current_frame;
extern uint32_t swapchain_current_image;
extern VkImageView* swapchain_image_views;
extern VkSemaphore* image_available_semaphores;
extern VkSemaphore* render_finished_semaphores;
extern VkFence* in_flight_fences;

static VkCommandPool command_pool = VK_NULL_HANDLE;

VkCommandBuffer* command_buffers = NULL;

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

int CreateRenderer(void) {
  if (0 != CreateCommandBuffers()) {
    fprintf(stderr, "Failed to create command buffers");
    return -1;
  }
}

void Render(void) {
  /* wait for the previous submission on this frame */
  vkWaitForFences(device, 1, &in_flight_fences[swapchain_current_frame],
                  VK_TRUE, UINT64_MAX);
  vkResetFences(device, 1, &in_flight_fences[swapchain_current_frame]);

  VkCommandBuffer cmd = command_buffers[swapchain_current_frame];

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  /* reset and begin rendering */
  vkResetCommandBuffer(cmd, 0);
  vkBeginCommandBuffer(cmd, &begin_info);

  /* dynamic rendering */
  VkRenderingInfo rendering_info = {};
  rendering_info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  VkRenderingAttachmentInfo attachment_info = {};
  attachment_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

  static float wtf = 0.f;
  wtf += 0.01f;

  VkClearColorValue clear_value = {
      .float32 = {sin(wtf) * sin(wtf), 1.0f - sin(wtf) * sin(wtf), 0.f, 1.f}};

  attachment_info.clearValue.color = clear_value;
  attachment_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachment_info.imageView = swapchain_image_views[swapchain_current_image];
  attachment_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachment_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  rendering_info.colorAttachmentCount = 1;
  rendering_info.pColorAttachments = &attachment_info;

  rendering_info.layerCount = 1;
  VkRect2D render_area = {.offset = {.x = 0, .y = 0},
                          .extent = {.width = 800, .height = 600}};
  rendering_info.renderArea = render_area;

  vkCmdBeginRendering(cmd, &rendering_info);

  /* TODO: rendering */

  vkCmdEndRendering(cmd);
  vkEndCommandBuffer(cmd);

  SubmitRenderCommandBuffer();
}

void DestroyRenderer(void) {
  vkDeviceWaitIdle(device);
  DestroyCommandBuffers();
}
