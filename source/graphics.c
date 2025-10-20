#include "graphics.h"

#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

static VkInstance instance = NULL;
static VkPhysicalDevice physical_device = NULL;

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

  return 0;
}

static int FindCompatiblePhysicalDevice(void) {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance, &device_count, NULL);
  if (device_count == 0) {
    fprintf(stderr, "Failed to find GPUs with Vulkan support\n");
    return -1;
  }

  VkPhysicalDevice* physical_devices =
      (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * device_count);

  vkEnumeratePhysicalDevices(instance, &device_count, physical_devices);

  bool found_compatible_device = false;

  for (uint32_t i = 0; i < device_count && !found_compatible_device; i++) {
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(physical_devices[i], &device_properties);

    uint32_t queue_family_count = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i],
                                             &queue_family_count, NULL);
    VkQueueFamilyProperties* queue_families = (VkQueueFamilyProperties*)malloc(
        sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_devices[i], &queue_family_count, queue_families);

    VkQueueFlags required_flags =
        (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);

    for (uint32_t j = 0; j < queue_family_count; j++) {
      if ((queue_families[j].queueFlags & required_flags == required_flags) &&
          queue_families[j].queueCount > 0) {
        physical_device = physical_devices[i];
        printf("Found Compatible device: %s\n", device_properties.deviceName);
        found_compatible_device = true;
        break;
      }
    }

    free(queue_families);
  }

  free(physical_device);

  return (found_compatible_device == true) ? 0 : -1;
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
  if (0 != FindCompatiblePhysicalDevice()) {
    fprintf(stderr, "Failed to find compatible physical device\n");
    VulkanCleanup();
    return -1;
  }

  return 0;
}

void VulkanCleanup(void) { DestroyInstance(); }
