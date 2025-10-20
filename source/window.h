
#ifndef WINDOW_H_
#define WINDOW_H_

/* Create the main window */
int CreateWindow(int width, int height, const char* title);

/* Poll for window events */
int PollEvents(void);

/* Destroy the main window */
int DestroyWindow(void);

/* initialize Vulkan Surface */
int VulkanInitializeSurface(void);

#endif  // WINDOW_H_
