#include <SDL2/SDL.h>
#include <stdbool.h>

// Define screen resolution
#define FB_WIDTH  800
#define FB_HEIGHT 600

// Global variables
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture = NULL;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Could not initialize SDL: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("SDL2 Live Render",
                             SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED,
                             FB_WIDTH, FB_HEIGHT,
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

    texture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               FB_WIDTH, FB_HEIGHT);
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
#include <assert.h>
#include <math.h>
#include <stdint.h>

// Helper macro for square
#define _sq(x) ((x)*(x))

#define SECONDS_PER_ROUND 2.0
#define FPS 60.0
#define ANGULAR_VELOCITY (2.0 * M_PI / (SECONDS_PER_ROUND * FPS))

uint16_t time = 0;

unsigned char RD(int x, int y, uint16_t time) {
    double angle = atan2(y - FB_HEIGHT/2, x - FB_WIDTH/2); // NOLINT(bugprone-integer-division)
    double half_angle = angle / 2;
    double t = time * ANGULAR_VELOCITY;
    double phase_adjusted = half_angle + t;
    double cosine = cos(phase_adjusted);
    double squared = _sq(cosine);
    double scaled = squared * 255;
    return (unsigned char)scaled;
}

unsigned char GR(int x, int y, uint16_t time) {
    double angle = atan2(y - FB_HEIGHT/2, x - FB_WIDTH/2); // NOLINT(bugprone-integer-division)
    double half_angle = angle / 2;
    double t = time * ANGULAR_VELOCITY;
    double phase_adjusted = half_angle - 2 * M_PI / 3 + t;
    double cosine = cos(phase_adjusted);
    double squared = _sq(cosine);
    double scaled = squared * 255;
    return (unsigned char)scaled;
}

unsigned char BL(int x, int y, uint16_t time) {
    double angle = atan2(y - FB_HEIGHT/2, x - FB_WIDTH/2); // NOLINT(bugprone-integer-division)
    double half_angle = angle / 2;
    double t = time * ANGULAR_VELOCITY;
    double phase_adjusted = half_angle + 2 * M_PI / 3 + t;
    double cosine = cos(phase_adjusted);
    double squared = _sq(cosine);
    double scaled = squared * 255;
    return (unsigned char)scaled;
}

uint32_t draw(int x, int y, uint16_t time) {
    return (RD(x, y, time) << 16 | GR(x, y, time) << 8 | BL(x, y, time));
}

bool loop() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }
    }

    // Lock texture for direct pixel access
    void* pixels;
    int pitch;
    if (SDL_LockTexture(texture, NULL, &pixels, &pitch)) {
        SDL_Log("Could not lock texture: %s", SDL_GetError());
        return false;
    }

    // 确保 pitch 没有额外的填充字节，否则不能直接转成二维数组！
    assert(pitch == FB_WIDTH * sizeof(uint32_t));
    // Method1: 安全地转换为二维数组（前提：pitch 无填充）
    uint32_t (*pixel_buffer)[FB_WIDTH] = (uint32_t(*)[FB_WIDTH])pixels;
    // Method2: Cast the pixels pointer to uint32_t* for easier access
    // uint32_t* pixel_buffer = (uint32_t*)pixels;


    // Update pixels directly in the texture buffer
    for (int y = 0; y < FB_HEIGHT; ++y) {
        for (int x = 0; x < FB_WIDTH; ++x) {
            pixel_buffer[y][x] = draw(x, y, time); // Method1: 前提：pitch 无填充
            // Method2: pixel_buffer[y * (pitch / sizeof(uint32_t)) + x] = draw(x, y, time);
        }
    }

    // Unlock the texture
    SDL_UnlockTexture(texture);

    // Increment time for animation
    time++;

    
    return true;
}

// ===================================================================
// Main
// ===================================================================

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    if (!init()) return 1;

    while (loop()) {
        // Main loop is handled by loop()
        // Render the texture
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        // Cap at ~60 FPS
        SDL_Delay(16);
    }

    kill();
    return 0;
}
