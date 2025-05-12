#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define BALL_RADIUS 20

// Global variables
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* framebuffer = NULL;

// PID controller parameters
typedef struct {
    float Kp;       // Proportional gain
    float Ki;       // Integral gain
    float Kd;       // Derivative gain
    float setpoint; // Target value
    float integral; // Integral term
    float prev_error; // Previous error for derivative term
} PIDController;

// Box parameters
typedef struct {
    float x, y;
    float width, height;
    float target_x, target_y;
    float target_width, target_height;
} Box;

PIDController pid_x = {0.1f, 0.01f, 0.05f, 0, 0, 0};
PIDController pid_y = {0.1f, 0.01f, 0.05f, 0, 0, 0};
PIDController pid_width = {0.1f, 0.01f, 0.05f, 100, 0, 0};
PIDController pid_height = {0.1f, 0.01f, 0.05f, 100, 0, 0};

Box box = {400, 300, 100, 100, 400, 300, 100, 100};

float pid_update(PIDController* pid, float current_value) {
    float error = pid->setpoint - current_value;
    
    // Proportional term
    float P = pid->Kp * error;
    
    // Integral term with anti-windup
    pid->integral += error;
    if (pid->integral > 1000) pid->integral = 1000;
    if (pid->integral < -1000) pid->integral = -1000;
    float I = pid->Ki * pid->integral;
    
    // Derivative term
    float D = pid->Kd * (error - pid->prev_error);
    pid->prev_error = error;
    
    return P + I + D;
}

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("Animation Demo (Texture Framebuffer)",
                             SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED,
                             SCREEN_WIDTH,
                             SCREEN_HEIGHT,
                             SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("SDL_CreateRenderer failed: %s", SDL_GetError());
        return false;
    }

    // Create framebuffer texture (ARGB8888 format)
    framebuffer = SDL_CreateTexture(renderer,
                                  SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_TARGET,
                                  SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!framebuffer) {
        SDL_Log("SDL_CreateTexture failed: %s", SDL_GetError());
        return false;
    }

    return true;
}

void kill() {
    if (framebuffer) SDL_DestroyTexture(framebuffer);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void render() {
    // Clear the framebuffer with white
    SDL_SetRenderTarget(renderer, framebuffer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    
    // Draw the box
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_FRect box_rect = {box.x - box.width/2, box.y - box.height/2, box.width, box.height};
    SDL_RenderFillRectF(renderer, &box_rect);
    
    // Draw the target position (as a red outline)
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_FRect target_rect = {box.target_x - box.target_width/2, box.target_y - box.target_height/2, 
                            box.target_width, box.target_height};
    SDL_RenderDrawRectF(renderer, &target_rect);
    
    // Reset render target to screen
    SDL_SetRenderTarget(renderer, NULL);
    
    // Copy the framebuffer texture to the screen
    SDL_RenderCopy(renderer, framebuffer, NULL, NULL);
    SDL_RenderPresent(renderer);
}

bool loop() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }
        
        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_LEFT:
                    box.target_x -= 100;
                    break;
                case SDLK_RIGHT:
                    box.target_x += 100;
                    break;
                case SDLK_UP:
                    box.target_y -= 100;
                    break;
                case SDLK_DOWN:
                    box.target_y += 100;
                    break;
                case SDLK_PLUS:
                case SDLK_EQUALS:
                    box.target_width += 10;
                    box.target_height += 10;
                    break;
                case SDLK_MINUS:
                    box.target_width -= 10;
                    box.target_height -= 10;
                    if (box.target_width < 20) box.target_width = 20;
                    if (box.target_height < 20) box.target_height = 20;
                    break;
                case SDLK_ESCAPE:
                    return false;
            }
        }
    }
    
    // Update PID controllers
    pid_x.setpoint = box.target_x;
    pid_y.setpoint = box.target_y;
    pid_width.setpoint = box.target_width;
    pid_height.setpoint = box.target_height;
    
    // Apply PID updates
    box.x += pid_update(&pid_x, box.x);
    box.y += pid_update(&pid_y, box.y);
    box.width += pid_update(&pid_width, box.width);
    box.height += pid_update(&pid_height, box.height);
    
    return true;
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    if (!init()) return 1;

    // Main loop
    while (loop()) {
        // Cap to 60 fps
        static Uint32 lastTime = 0;
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - lastTime < 16) {
            SDL_Delay(16 - (currentTime - lastTime));
        }
        lastTime = currentTime;

        render();
    }

    kill();
    return 0;
}
