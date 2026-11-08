#define dll_export __attribute__((visibility("default")))

// TODO
#define SWIN_IMPL
#include "../../../swin/swin.h"

typedef struct {
    u32 width;
    u32 height;
    Swin swin;
    u32* pixels;
} State;

dll_export State* init() {
    State* state = malloc(sizeof(State));
    *state = (State){};

    state->width = 400;
    state->height = 300;
    state->swin = swinInit((SwinInitInfo){.width = state->width, .height = state->height});
    swinSetWindowFlags(state->swin, SwinWindow_Topmost);
    state->pixels = (u32*)malloc(state->width * state->height * sizeof(u32));

    return state;
}

dll_export bool update(State* state) {
    swinDraw(state->swin, state->pixels);

    for (u32 i = 0; i < state->width * state->height; i++) {
        u8 r = 255, g = 0, b = 0, a = 255;
        state->pixels[i] = (a << 24) | (b << 16) | (g << 8) | r;
    }

    return swinPoll(state->swin);
}
