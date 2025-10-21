
#include "window.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include <stdio.h>

static SDL_Window* window = NULL;

static void PrintSDLError(const char* message) {
  fprintf(stderr, "%s: %s\n", message, SDL_GetError());
}

int CreateWindow(int width, int height, const char* title) {
  printf("SDL Verison: linked %d included %d\n ", SDL_GetVersion(),
         SDL_VERSION);

  if (SDL_Init(SDL_INIT_VIDEO) == 0) {
    PrintSDLError("CreateWindow: SDL_Init failed");
    return -1;
  }

  window = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN);
  if (window == NULL) {
    PrintSDLError("CreateWindow: SDL_CreateWindow failed");
    SDL_Quit();
    return -1;
  }

  return 0;
}

int PollEvents(void) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      return -1;
    }
  }
  return 0;
}

int DestroyWindow(void) {
  if (window != NULL) {
    SDL_DestroyWindow(window);
    window = NULL;
  }
  SDL_Quit();
  return 0;
}

int VulkanInitializeSurface(void) { return 0; }

void* GetWindowHandle(void) { return window; }
