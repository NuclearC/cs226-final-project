

#include <stdio.h>
#include <stdlib.h>  // For setenv

#include "graphics.h"
#include "renderer.h"
#include "window.h"

#define CHECK_RESULT(x, msg)   \
  if (0 != x) {                \
    fprintf(stderr, msg "\n"); \
    Cleanup();                 \
    return -1;                 \
  }

static void Cleanup(void) {
  DestroyWindow();
  VulkanCleanup();
}

int main(void) {
  int window_width = 800;
  int window_height = 600;
  setenv("SDL_VIDEODRIVER", "wayland", 1);
  /* initialize Vulkan */

  CHECK_RESULT(VulkanInitialize(),
               "Failed to initialize Vulkan instance and device");
  /* create the main window */
  CHECK_RESULT(
      CreateWindow(window_width, window_height, "SDL3 Output Window [Vulkan]"),
      "Failed to create window");
  CHECK_RESULT(VulkanCreateSurface(GetWindowHandle()),
               "Failed to create Vulkan surface");
  CHECK_RESULT(VulkanCreateSwapchain(window_width, window_height),
               "Failed to create Vulkan swapchain");

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
