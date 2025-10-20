#include "graphics.h"

#include <stdio.h>
#include <vulkan/vulkan.h>

static VkInstance instance = NULL;
static VkPhysicalDevice* physical_device = NULL;

static int InitializeInstance(void) {
  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.apiVersion = VK_API_VERSION_1_4;
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pApplicationName = "Vulkan Application";
  app_info.pEngineName = "No Engine";

  VkInstanceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  if (vkCreateInstance(&create_info, NULL, &instance) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create Vulkan instance\n");
    return -1;
  }
}

static void DestroyInstance(void) {
  if (instance != NULL) {
    vkDestroyInstance(instance, NULL);
    instance = NULL;
  }
}

int VulkanInitialize(void) {
  if (0 != InitializeInstance()) {
    return -1;
  }

  return 0;
}

void VulkanCleanup(void) { DestroyInstance(); }
