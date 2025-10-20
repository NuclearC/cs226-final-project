

#include <stdio.h>
#include <stdlib.h>  // For setenv

#include "graphics.h"
#include "window.h"

static void Cleanup(void) {
  DestroyWindow();
  VulkanCleanup();
}

int main(void) {
  int window_width = 800;
  int window_height = 600;
  setenv("SDL_VIDEODRIVER", "wayland", 1);
  /* initialize Vulkan */
  if (0 != VulkanInitialize()) {
    fprintf(stderr, "Failed to initialize Vulkan\n");
    return -1;
  }
  /* create the main window */
  if (0 != CreateWindow(window_width, window_height,
                        "SDL3 Output Window [Vulkan]")) {
    fprintf(stderr, "Failed to create window\n");
    return -1;
  }

  if (0 != VulkanCreateSurface(GetWindowHandle())) {
    fprintf(stderr, "Failed to create Vulkan surface\n");
    Cleanup();
    return -1;
  }
  /* create swapchain */
  if (0 != VulkanCreateSwapchain(window_width, window_height)) {
    fprintf(stderr, "Failed to create Vulkan swapchain\n");
    Cleanup();
    return -1;
  }

  /* main loop */
  for (;;) {
    if (0 != PollEvents()) {
      break;
    }

    break;
  }

  /* cleanup */
  Cleanup();
  printf("cleanup done\n");

  return 0;
}
