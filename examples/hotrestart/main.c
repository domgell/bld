#include "../example_window.h"

int main() {
    int width = 400;
    int height = 300;
    ExampleWindow window = window_init(width, height);
    uint32_t* pixels = (uint32_t*)malloc(width * height * sizeof(uint32_t));

    while (window_poll(window)) {
        for (int i = 0; i < width * height; i++) {
            // Try changing the RGB values and save the file while the app is running
            uint8_t r = 255, g = 0, b = 0, a = 255;
            pixels[i] = (a << 24) | (r << 16) | (g << 8) | b;
        }
        window_draw(window, pixels);
    }
}
