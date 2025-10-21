#include "graphics.h"

#include <SDL3/SDL_vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>

static VkInstance instance = VK_NULL_HANDLE;
static VkPhysicalDevice physical_device = VK_NULL_HANDLE;
static VkDevice device = VK_NULL_HANDLE;
static VkSurfaceKHR surface = VK_NULL_HANDLE;
static VkSwapchainKHR swapchain = VK_NULL_HANDLE;
static VkQueue graphics_queue = VK_NULL_HANDLE;

static VkImage* swapchain_images = NULL;
static uint32_t swapchain_image_count = 0;
static VkImageView* swapchain_image_views = NULL;

static VkFormat swapchain_image_format = VK_FORMAT_R8G8B8A8_SRGB;

static VkSemaphore *image_available_semaphores = NULL,
                   *render_finished_semaphores = NULL;

static uint32_t queue_family_index = 0;

static uint32_t swapchain_current_frame = 0;
static uint32_t swapchain_current_image = 0;

static const char* const instance_extensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME};
static const char* const instance_layers[] = {"VK_LAYER_KHRONOS_validation"};
static const char* const device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static int InitializeInstance(void) {
  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.apiVersion = VK_API_VERSION_1_4;
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pApplicationName = "CS226 Final Project";
  app_info.pEngineName = "AUA";

  VkInstanceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledExtensionCount =
      sizeof(instance_extensions) / sizeof(instance_extensions[0]);
  create_info.ppEnabledExtensionNames = instance_extensions;
  create_info.enabledLayerCount =
      sizeof(instance_layers) / sizeof(instance_layers[0]);
  create_info.ppEnabledLayerNames = instance_layers;

  if (vkCreateInstance(&create_info, VK_NULL_HANDLE, &instance) != VK_SUCCESS) {
    fprintf(stderr, "Failed to create Vulkan instance\n");
    return -1;
  }

  return 0;
}

static int FindCompatiblePhysicalDevice(void) {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance, &device_count, VK_NULL_HANDLE);
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
        queue_family_index = j;

        physical_device = physical_devices[i];

        printf("Found Compatible device: %s\n", device_properties.deviceName);
        found_compatible_device = true;
        break;
      }
    }

    free(queue_families);
  }

  free(physical_devices);

  return (found_compatible_device == true) ? 0 : -1;
}

static int CreateDevice(void) {
  VkDeviceQueueCreateInfo queue_create_info = {};

  queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_info.queueFamilyIndex = queue_family_index;
  float queue_priority = 1.0f;
  queue_create_info.pQueuePriorities = &queue_priority;
  queue_create_info.queueCount = 1;

  VkDeviceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.enabledExtensionCount =
      sizeof(device_extensions) / sizeof(device_extensions[0]);
  create_info.ppEnabledExtensionNames = device_extensions;

  create_info.enabledLayerCount =
      sizeof(instance_layers) / sizeof(instance_layers[0]);
  create_info.ppEnabledLayerNames = instance_layers;

  create_info.queueCreateInfoCount = 1;
  create_info.pQueueCreateInfos = &queue_create_info;

  VkPhysicalDeviceFeatures device_features = {};
  create_info.pEnabledFeatures = &device_features;

  if (vkCreateDevice(physical_device, &create_info, VK_NULL_HANDLE, &device) !=
      VK_SUCCESS) {
    return -1;
  }

  vkGetDeviceQueue(device, queue_family_index, 0, &graphics_queue);

  return 0;
}

static int CreateSwapchainSemaphores(void) {
  image_available_semaphores =
      (VkSemaphore*)malloc(sizeof(VkSemaphore) * swapchain_image_count);
  render_finished_semaphores =
      (VkSemaphore*)malloc(sizeof(VkSemaphore) * swapchain_image_count);
  for (uint32_t i = 0; i < swapchain_image_count; i++) {
    VkSemaphoreCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(device, &create_info, VK_NULL_HANDLE,
                          &image_available_semaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &create_info, VK_NULL_HANDLE,
                          &render_finished_semaphores[i]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create image semaphores %d \n", i);
      return -1;
    }
  }

  return 0;
}

static int CreateSwapchainImageViews(void) {
  swapchain_image_views =
      (VkImageView*)malloc(sizeof(VkImageView) * swapchain_image_count);
  for (uint32_t i = 0; i < swapchain_image_count; i++) {
    VkImageViewCreateInfo view_create_info = {};
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.image = swapchain_images[i];
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format = swapchain_image_format;
    view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount = 1;
    if (vkCreateImageView(device, &view_create_info, NULL,
                          &swapchain_image_views[i]) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create image views for swapchain images\n");
      return -1;
    }
  }
  return 0;
}

static void DestroySwapchainSemaphores(void) {
  if (image_available_semaphores != NULL) {
    for (uint32_t i = 0; i < swapchain_image_count; i++) {
      if (image_available_semaphores[i] != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, image_available_semaphores[i],
                           VK_NULL_HANDLE);
        image_available_semaphores[i] = VK_NULL_HANDLE;
      }
    }
    free(image_available_semaphores);
    image_available_semaphores = NULL;
  }
  if (render_finished_semaphores != NULL) {
    for (uint32_t i = 0; i < swapchain_image_count; i++) {
      if (render_finished_semaphores[i] != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, render_finished_semaphores[i],
                           VK_NULL_HANDLE);
        render_finished_semaphores[i] = VK_NULL_HANDLE;
      }
    }
    free(render_finished_semaphores);
    render_finished_semaphores = NULL;
  }
}
static void DestroySwapchainImageViews(void) {
  if (swapchain_image_views != NULL) {
    for (uint32_t i = 0; i < swapchain_image_count; i++) {
      if (swapchain_image_views[i] != VK_NULL_HANDLE) {
        vkDestroyImageView(device, swapchain_image_views[i], VK_NULL_HANDLE);
        swapchain_image_views[i] = VK_NULL_HANDLE;
      }
    }
    free(swapchain_image_views);
    swapchain_image_views = NULL;
  }
}

static void DestroySwapchain(void) {
  free(swapchain_images);
  if (swapchain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device, swapchain, VK_NULL_HANDLE);
    swapchain = VK_NULL_HANDLE;
  }
}

static void DestroySurface(void) {
  if (surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(instance, surface, VK_NULL_HANDLE);
    surface = VK_NULL_HANDLE;
  }
}

static void DestroyDevice(void) {
  if (device != VK_NULL_HANDLE) {
    vkDestroyDevice(device, NULL);
    device = VK_NULL_HANDLE;
  }
}

static void DestroyInstance(void) {
  if (instance != VK_NULL_HANDLE) {
    vkDestroyInstance(instance, VK_NULL_HANDLE);
    instance = VK_NULL_HANDLE;
  }
}

void VulkanCleanup(void) {
  DestroySwapchainSemaphores();
  DestroySwapchainImageViews();
  DestroySwapchain();
  DestroySurface();
  DestroyDevice();
  DestroyInstance();
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
  if (0 != CreateDevice()) {
    fprintf(stderr, "Failed to create logical device\n");
    VulkanCleanup();
    return -1;
  }

  printf("Vulkan initialized successfully\n");

  return 0;
}

int VulkanCreateSurface(void* window) {
  if (false ==
      SDL_Vulkan_CreateSurface((SDL_Window*)window, instance, NULL, &surface)) {
    fprintf(stderr, "Failed to create Vulkan surface: %s \n", SDL_GetError());
    return -1;
  }
  return 0;
}

int VulkanCreateSwapchain(uint32_t width, uint32_t height) {
  VkSwapchainCreateInfoKHR create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.surface = surface;

  VkSurfaceCapabilitiesKHR surface_capabilites;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface,
                                            &surface_capabilites);

  uint32_t format_count = 0u;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count,
                                       NULL);
  VkSurfaceFormatKHR* surface_formats =
      (VkSurfaceFormatKHR*)malloc(sizeof(VkSurfaceFormatKHR) * format_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count,
                                       surface_formats);

  uint32_t required_image_count = 4;
  required_image_count =
      SDL_clamp(required_image_count, surface_capabilites.minImageCount,
                surface_capabilites.maxImageCount);

  bool found_surface_format = false;
  for (uint32_t i = 0; i < format_count; i++) {
    if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
        surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      swapchain_image_format = surface_formats[i].format;
      create_info.imageFormat = surface_formats[i].format;
      create_info.imageColorSpace = surface_formats[i].colorSpace;
      found_surface_format = true;
      break;
    }
  }

  free(surface_formats);
  if (!found_surface_format) {
    fprintf(stderr, "failed to find a suitable surface format\n");
    return -1;
  }

  create_info.minImageCount = required_image_count;

  create_info.imageExtent.width =
      SDL_clamp(width, surface_capabilites.minImageExtent.width,
                surface_capabilites.maxImageExtent.width);
  create_info.imageExtent.height =
      SDL_clamp(height, surface_capabilites.minImageExtent.height,
                surface_capabilites.maxImageExtent.height);

  create_info.imageArrayLayers = 1;
  create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  create_info.preTransform = surface_capabilites.currentTransform;
  create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  create_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  create_info.clipped = VK_TRUE;
  create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateSwapchainKHR(device, &create_info, NULL, &swapchain) !=
      VK_SUCCESS) {
    return -1;
  }

  vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, NULL);
  swapchain_images = (VkImage*)malloc(sizeof(VkImage) * swapchain_image_count);
  vkGetSwapchainImagesKHR(device, swapchain, &swapchain_image_count,
                          swapchain_images);

  CreateSwapchainImageViews();
  CreateSwapchainSemaphores();

  return 0;
}

int VulkanSCAcquireImage(void) {
  if (VK_SUCCESS !=
      vkAcquireNextImageKHR(device, swapchain, 100u,
                            image_available_semaphores[swapchain_current_frame],
                            VK_NULL_HANDLE, &swapchain_current_image)) {
    return -1;
  }

  return swapchain_current_image;
}

int VulkanSCPresent(void) {
  VkPresentInfoKHR present_info = {};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.pImageIndices = &swapchain_current_image;

  present_info.pWaitSemaphores =
      render_finished_semaphores[swapchain_current_frame];

  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swapchain;

  if (VK_SUCCESS != vkQueuePresentKHR(graphics_queue, &present_info)) {
    return -1;
  }

  swapchain_current_frame =
      (swapchain_current_frame + 1) % swapchain_image_count;

  return 0;
}
