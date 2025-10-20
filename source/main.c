

#include <stdio.h>

#include "graphics.h"
#include "window.h"

int main(void) {
  /* initialize Vulkan */
  if (0 != VulkanInitialize()) {
    fprintf(stderr, "Failed to initialize Vulkan\n");
    return -1;
  }
  /* create the main window */
  if (0 != CreateWindow(800, 600, "SDL3 Output Window [Vulkan]")) {
    fprintf(stderr, "Failed to create window\n");
    return -1;
  }

  /* main loop */
  for (;;) {
    if (0 != PollEvents()) {
      break;
    }
  }

  /* cleanup */
  DestroyWindow();
  VulkanCleanup();

  return 0;
}
