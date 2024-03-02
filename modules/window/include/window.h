#pragma once

#if __cplusplus
extern "C" {
#endif

#include "event.h"
#include "cursors.h"

typedef struct WindowHandle_T *WindowHandle;
typedef struct VkSurfaceKHR_T *VkSurfaceKHR;
typedef struct VkInstance_T *VkInstance;

typedef struct WindowCreateInfo {
	int x, y;
	unsigned width, height;
} WindowCreateInfo;

typedef unsigned WindowCursor;

typedef enum WindowResult {
	WINDOW_SUCCESS,
	WINDOW_CONNECTION_FAILED,
	WINDOW_ALLOCATION_FAILED,
} WindowResult;

/* Creates new window */
WindowResult window_new(WindowCreateInfo *pInfo, WindowHandle *pOut);

/* Cleans up the window */
void window_destroy(WindowHandle window);

/* Hides the window, basically minimizes */
void window_hide(WindowHandle window);

/* Show the window */
void window_show(WindowHandle window);

VkSurfaceKHR window_vk_create_surface(VkInstance instance, WindowHandle window);

int window_is_closed(WindowHandle window);

void window_close(WindowHandle window);

int window_next_event(WindowHandle handle, WindowEvent *pOutEvent);

int window_pending_event(WindowHandle handle);

char *window_get_clipboard(const WindowHandle window);

void window_set_clipboard(WindowHandle window, const char *text);

WindowCursor window_create_cursor(WindowHandle window, int idx);

int window_is_minimized(WindowHandle window);

void window_get_extent(WindowHandle window, int *pOutWidth, int *pOutHeight);

#if __cplusplus
}
#endif
