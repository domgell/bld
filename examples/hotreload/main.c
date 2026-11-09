#define dll_export __attribute__((visibility("default")))

#include "window.h"

typedef struct {
    int width;
    int height;
    WindowState window;
    uint32_t* pixels;
} State;

// Called the first time the program is compiled to initialize persistent state
dll_export State* init() {
    State* state = malloc(sizeof(State));
    *state = (State){};

    state->width = 400;
    state->height = 300;
    state->window = window_init(state->width, state->height);
    state->pixels = (uint32_t*)malloc(state->width * state->height * sizeof(uint32_t));

    return state;
}

// Called each frame as long as the window is open
dll_export bool update(State* state) {
    for (int i = 0; i < state->width * state->height; i++) {
        // Try changing the RGB values and save the file while the hotreload app is running
        uint8_t r = 255, g = 128, b = 128, a = 255;
        state->pixels[i] = (a << 24) | (r << 16) | (g << 8) | b;
    }
    window_draw(state->window, state->pixels);

    return window_poll(state->window);
}