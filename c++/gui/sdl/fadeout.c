#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define SCALE 4  // Scaling factor for better visibility

// Global variables
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture = NULL;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Could not initialize SDL: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("OLED Fade Out Effect",
                             SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED,
                             OLED_WIDTH * SCALE,
                             OLED_HEIGHT * SCALE,
                             SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("Could not create window: %s", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Could not create renderer: %s", SDL_GetError());
        return false;
    }
    SDL_RenderSetScale(renderer, SCALE, SCALE);

    // Create texture for pixel manipulation
    texture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               OLED_WIDTH, OLED_HEIGHT);
    if (!texture) {
        SDL_Log("Could not create texture: %s", SDL_GetError());
        return false;
    }

    return true;
}

void kill() {
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

// ===================================================================
// User code 
// ===================================================================
#include "assert.h"

// Fill texture with test pattern
void fillTestPattern() {
    void* pixels;
    int pitch;
    if (SDL_LockTexture(texture, NULL, &pixels, &pitch)) {
        SDL_Log("Could not lock texture: %s", SDL_GetError());
        return;
    }
    assert(pitch == OLED_WIDTH * sizeof(uint32_t)); // 确保 pitch 没有额外的填充字节，否则不能直接转成二维数组
    uint32_t (*pixel_buffer)[OLED_WIDTH] = (uint32_t(*)[OLED_WIDTH])pixels;

    // Draw test pattern
    for (int y = 0; y < OLED_HEIGHT; y++) {
        for (int x = 0; x < OLED_WIDTH; x++) {
            // Draw grid lines
            pixel_buffer[y][x] = 0xFFFFFFFF; // ARGB white
        }
    }

    SDL_UnlockTexture(texture);
}

// Apply fade out effect to the texture
void applyFadeOutEffect(int fadeLevel) {
    void* pixels;
    int pitch;
    if (SDL_LockTexture(texture, NULL, &pixels, &pitch)) {
        SDL_Log("Could not lock texture: %s", SDL_GetError());
        return;
    }
    assert(pitch == OLED_WIDTH * sizeof(uint32_t)); // 确保 pitch 没有额外的填充字节，否则不能直接转成二维数组
    uint32_t (*pixel_buffer)[OLED_WIDTH] = (uint32_t(*)[OLED_WIDTH])pixels;

    assert(fadeLevel < 5 || fadeLevel >= 0 ); 

    
    // Define 2x2 grid fade patterns
    const uint8_t patterns[5][2][2] = {
        {{0, 0}, {0, 0}},  // Level 1: all on
        {{1, 0}, {0, 0}},   // Level 2: top-left off
        {{1, 0}, {0, 1}},   // Level 3: top-left and bottom-right off
        {{1, 0}, {1, 1}},   // Level 4: only top-right on
        {{1, 1}, {1, 1}}    // Level 5: all off
    };

    // Apply fade effect
    for (int y = 0; y < OLED_HEIGHT; y++) {
        for (int x = 0; x < OLED_WIDTH; x++) {
            // Calculate position in 2x2 grid
            int grid_x = x % 2;
            int grid_y = y % 2;
            
            // If pattern says to turn off this pixel
            if (patterns[fadeLevel][grid_y][grid_x]) {
                pixel_buffer[y][x] = 0xFF000000; // ARGB black
            }
        }
    }

    SDL_UnlockTexture(texture);
}

void render() {
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

bool loop() {
    static int currentFadeLevel = 0 ;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        } 


        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_UP:
                    currentFadeLevel = ( currentFadeLevel + 1 ) % 5;
                    break;
                case SDLK_DOWN:
                    currentFadeLevel = ( currentFadeLevel + 4 ) % 5; // 4 == -1 (mod5)
                    break;
                case SDLK_r:
                    currentFadeLevel = 0;
                    break;
                case SDLK_ESCAPE:
                    return false;
            }
        }
    }

    fillTestPattern();
    applyFadeOutEffect(currentFadeLevel);
    
    return true;
}

// ===================================================================
// Main
// ===================================================================

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    if (!init()) return 1;

    // Initialize with test pattern
    fillTestPattern();
    render();

    while (loop()) {
        render();
        SDL_Delay(16);  // Cap at ~60 FPS
    }

    kill();
    return 0;
}
