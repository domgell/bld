// Minimal windowing implementation for example purposes
// TODO: implement Windows + MacOS version

#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct WindowState_t* WindowState;

WindowState window_init(int width, int height);
bool window_poll(WindowState window);
void window_draw(WindowState window, const uint32_t* pixels);

#if defined __linux

#include <X11/Xlib.h>
#include <X11/Xutil.h>

struct WindowState_t {
    Display* display;
    Window window;
    bool quit;
    GC gc;
    XImage* image;
};

WindowState window_init(int width, int height) {
    WindowState window = malloc(sizeof(struct WindowState_t));
    *window = (struct WindowState_t){};

    window->display = XOpenDisplay(NULL);
    window->window = XCreateSimpleWindow(
        window->display,
        DefaultRootWindow(window->display),
        0, 0, width, height, 1,
        BlackPixel(window->display, DefaultScreen(window->display)),
        WhitePixel(window->display, DefaultScreen(window->display))
    );
    XMapWindow(window->display, window->window);
    XFlush(window->display);

    window->gc = XCreateGC(window->display, window->window, 0, NULL);
    window->image = XCreateImage(
        window->display,
        DefaultVisual(window->display, DefaultScreen(window->display)),
        DefaultDepth(window->display, DefaultScreen(window->display)),
        ZPixmap, 0, NULL, width, height, 32, 0
    );
    window->image->data = malloc(width * height * sizeof(uint32_t));
    memset(window->image->data, 0, width * height * sizeof(uint32_t));

    return window;
}

bool window_poll(WindowState window) {
    if (!window) return false;

    if (window->quit) {
        if (window->image) XDestroyImage(window->image);
        if (window->gc) XFreeGC(window->display, window->gc);
        if (window->window) XDestroyWindow(window->display, window->window);
        if (window->display) XCloseDisplay(window->display);
        free(window);
        return false;
    }

    while (XPending(window->display)) {
        XEvent event;
        XNextEvent(window->display, &event);
        switch (event.type) {
        case ClientMessage:
            window->quit = true;
            break;
        default:
            break;
        }
    }

    return true;
}

void window_draw(WindowState window, const uint32_t* pixels) {
    memcpy(window->image->data, pixels, window->image->width * window->image->height * sizeof(uint32_t));
    XPutImage(window->display, window->window, window->gc, window->image, 0, 0, 0, 0, window->image->width,
        window->image->height);
}

#elif defined _WIN32

// TODO

#elif defined __APPLE__

// TODO

#endif
