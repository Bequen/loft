#include "../include/event.h"
#include "../include/window.h"

#if PLATFORM_WINDOWS
#include <X11/X.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <X11/Xutil.h>
#include <X11/Xlib.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>

struct WindowHandle_T {
	Window window;

	/* Used for comparing when ConfigureNotify event occurs, to see if we just
	 * moved or scaled */
	int x, y;
	unsigned width, height;

	Display *pDisplay;
	XVisualInfo *pVisual;
	Colormap colormap;

	Atom wmDeleteWindow;
	int isOpen;

	int isMinimized;
};

static int
xlib_debug_callback(Display* display, XErrorEvent* event) {
	char* buffer = (char*)malloc(2048);
	XGetErrorText(display, event->error_code, buffer, 2048);

	printf("Xlib error => %s", buffer);
	free(buffer);

	return 0;
}

WindowResult window_new(WindowCreateInfo *pInfo, WindowHandle *pOut) {
	*pOut = (struct WindowHandle_T*)malloc(sizeof(struct WindowHandle_T));
	if(!pOut) {
		return WINDOW_ALLOCATION_FAILED;
	}
	// memset(pOut, 0, sizeof(struct WindowHandle_T));

	/* XInitThreads(); */
	XSetErrorHandler(xlib_debug_callback);

	(*pOut)->pDisplay = XOpenDisplay(NULL);
	if(!(*pOut)->pDisplay) {
		return WINDOW_CONNECTION_FAILED;
	}

	long visualMask = VisualScreenMask;
	XVisualInfo visualInfo = {};
	visualInfo.screen = DefaultScreen((*pOut)->pDisplay);

	int visualCount = 0;
	(*pOut)->pVisual = XGetVisualInfo((*pOut)->pDisplay, visualMask, &visualInfo, &visualCount);
	(*pOut)->colormap = XCreateColormap((*pOut)->pDisplay,
									   DefaultRootWindow((*pOut)->pDisplay), 
									   (*pOut)->pVisual->visual, AllocNone);

	{
		XSetWindowAttributes windowAttributes = {};
		windowAttributes.colormap = (*pOut)->colormap;
		windowAttributes.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | StructureNotifyMask;
		windowAttributes.override_redirect = False;

		(*pOut)->window = XCreateWindow((*pOut)->pDisplay, DefaultRootWindow((*pOut)->pDisplay),
										pInfo->x, pInfo->y, 
										pInfo->width, pInfo->height, 0,
										(*pOut)->pVisual->depth, InputOutput,
										(*pOut)->pVisual->visual, 
										CWColormap | CWEventMask | CWOverrideRedirect, 
										&windowAttributes);
	}

	(*pOut)->isOpen = 1;

	/* handles closing the window by the user */
	(*pOut)->wmDeleteWindow = XInternAtom((*pOut)->pDisplay, "WM_DELETE_WINDOW", False);
	XSetWMProtocols((*pOut)->pDisplay, (*pOut)->window, &(*pOut)->wmDeleteWindow, 1);

	XMapWindow((*pOut)->pDisplay, (*pOut)->window);
	XFlush((*pOut)->pDisplay);

	(*pOut)->width = pInfo->width;
	(*pOut)->height = pInfo->height;

	return WINDOW_SUCCESS;
}


VkSurfaceKHR window_vk_create_surface(VkInstance instance, WindowHandle window) {
	VkXlibSurfaceCreateInfoKHR surfaceInfo = {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        .dpy = window->pDisplay,
        .window = window->window,
    };

	VkSurfaceKHR result = VK_NULL_HANDLE;

    if(vkCreateXlibSurfaceKHR(instance, &surfaceInfo, NULL, &result) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }

	return result;
}

int window_solve_event(XEvent *pEvent, WindowEvent *pOutEvent) {
	assert(pEvent && pOutEvent);

	WindowEvent result = {};
	result.type = NONE_EVENT_TYPE;
	switch(pEvent->type) {
		case KeyPress:
			result.type = KEY_EVENT_TYPE;
			result.key.state = STATE_PRESSED;
			result.key.key = XLookupKeysym(&pEvent->xkey, 0);
			break;
		case KeyRelease:
			result.type = KEY_EVENT_TYPE;
			result.key.state = STATE_RELEASED;
			result.key.key = XLookupKeysym(&pEvent->xkey, 0);
			result.key.txt = XKeysymToString(result.key.key);
			break;
		case ButtonPress:
			result.type = BUTTON_EVENT_TYPE;
			result.button.state = STATE_PRESSED;
			result.button.button = pEvent->xbutton.button;
			result.button.x = pEvent->xbutton.x;
			result.button.y = pEvent->xbutton.y;
			break;
		case ButtonRelease:
			result.type = BUTTON_EVENT_TYPE;
			result.button.state = STATE_RELEASED;
			result.button.button = pEvent->xbutton.button;
			result.button.x = pEvent->xbutton.x;
			result.button.y = pEvent->xbutton.y;
			break;
		case MotionNotify:
			result.type = MOTION_EVENT_TYPE;
			result.motion.x = pEvent->xmotion.x;
			result.motion.y = pEvent->xmotion.y;
			break;
		case ConfigureNotify:
			result.type = TRANSFORM_EVENT_TYPE;
			result.transform.x = pEvent->xconfigure.x;
			result.transform.y = pEvent->xconfigure.y;
			result.transform.width = pEvent->xconfigure.width;
			result.transform.height = pEvent->xconfigure.height;
			break;
	}

	*pOutEvent = result;

	return result.type != NONE_EVENT_TYPE;
}

int window_is_closed(WindowHandle window) {
	return !window->isOpen;
}

void window_close(WindowHandle window) {
	window->isOpen = 0;
}

int window_next_event(WindowHandle window, WindowEvent *pOutEvent) {
	assert(window && pOutEvent);

	XEvent event;
	int status = XNextEvent(window->pDisplay, &event);

	WindowEvent solvedEvent = {};
	status = window_solve_event(&event, &solvedEvent);
	
	if(event.type == ClientMessage) {
		if(event.xclient.data.l[0] == window->wmDeleteWindow) {
			window_close(window);
		}
	} else if(event.type == MapNotify) {
		window->isMinimized = 1;
	} else if(event.type == UnmapNotify) {
		window->isMinimized = 0;
	} else if(event.type == ConfigureNotify) {
		window->width = event.xconfigure.width;
		window->height = event.xconfigure.height;
	}

	if(status) {
		*pOutEvent = solvedEvent;
		return 1;
	}

	return 0;
}

int window_pending_event(WindowHandle window) {
	return XPending(window->pDisplay);
}

void window_destroy(WindowHandle window) {
	XDestroyWindow(window->pDisplay, window->window);
	XCloseDisplay(window->pDisplay);
}

int window_is_minimized(WindowHandle window) {
	return window->isMinimized;
}

void window_get_extent(WindowHandle window, int *pOutWidth, int *pOutHeight) {
	*pOutWidth = window->width;
	*pOutHeight = window->height;
}

void window_set_clipboard(WindowHandle window, const char *text) {

}

char *window_get_clipboard(WindowHandle window) {
	return NULL;
}

WindowCursor window_create_cursor(WindowHandle window, int idx) {
	return 0;
}
#endif