#include <SDL2/SDL.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <fcntl.h>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int POINT_HISTORY = 2000;
const Uint32 POINT_LIFETIME = 2000; // milliseconds

struct TouchPoint {
    int x, y;
    Uint32 timestamp;
    bool active;
};

std::vector<TouchPoint> points;

bool parseLine(const std::string& line, int& x, int& y) {
    // Expected format: DEVTODO:x:XXX:y:XXX
    if (line.substr(0, 8) != "DEVTODO:") return false;
    
    size_t xPos = line.find(":x:");
    size_t yPos = line.find(":y:");
    
    if (xPos == std::string::npos || yPos == std::string::npos) return false;
    
    try {
        x = std::stoi(line.substr(xPos + 3, yPos - (xPos + 3)));
        y = std::stoi(line.substr(yPos + 3));
        return true;
    } catch (...) {
        return false;
    }
}

void addPoint(int x, int y, Uint32 timestamp) {
    // Convert coordinates to screen space (assuming input is 0-1000 range)
    int screenX = std::clamp(x * WINDOW_WIDTH / 1000, 0, WINDOW_WIDTH - 1);
    int screenY = std::clamp(y * WINDOW_HEIGHT / 1000, 0, WINDOW_HEIGHT - 1);
    
    points.push_back({screenX, screenY, timestamp, true});
    
    // Remove old points
    while (points.size() > POINT_HISTORY) {
        points.erase(points.begin());
    }
}

void updatePoints(Uint32 current_time) {
    for (auto& point : points) {
        if (current_time - point.timestamp > POINT_LIFETIME) {
            point.active = false;
        }
    }
}

void render(SDL_Renderer* renderer) {
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    // Draw grid
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    for (int i = 1; i < 10; ++i) {
        SDL_RenderDrawLine(renderer, i * WINDOW_WIDTH / 10, 0, 
                          i * WINDOW_WIDTH / 10, WINDOW_HEIGHT);
        SDL_RenderDrawLine(renderer, 0, i * WINDOW_HEIGHT / 10, 
                          WINDOW_WIDTH, i * WINDOW_HEIGHT / 10);
    }
    
    // Draw points
    Uint32 current_time = SDL_GetTicks();
    for (const auto& point : points) {
        if (!point.active) continue;

        // Calculate alpha based on age
        Uint8 alpha = 255;
        Uint32 age = current_time - point.timestamp;
        if (age > POINT_LIFETIME * 0.75) {
            alpha = static_cast<Uint8>(255 * (1.0 - (age - POINT_LIFETIME * 0.75) / (POINT_LIFETIME * 0.25)));
        }

        // Draw single pixel with fading effect
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, alpha);
        SDL_RenderDrawPoint(renderer, point.x, point.y);
    }
    
    SDL_RenderPresent(renderer);
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Touchpad Debug Tool",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Enable non-blocking stdin read
    fcntl(0, F_SETFL, O_NONBLOCK);

    bool quit = false;
    SDL_Event e;
    Uint32 last_update = SDL_GetTicks();

    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        // Read input from stdin
        std::string line;
        while (std::getline(std::cin, line)) {
            if (!line.empty()) {
                int x, y;
                if (parseLine(line, x, y)) {
                    addPoint(x, y, SDL_GetTicks());
                }
            }
        }

        // Update and render at ~60 FPS
        Uint32 current_time = SDL_GetTicks();
        if (current_time - last_update > 16) {
            updatePoints(current_time);
            render(renderer);
            last_update = current_time;
        } else {
            SDL_Delay(1);
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
