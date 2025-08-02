// q-gcc: -lSDL2 -lSDL2_ttf --
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <fcntl.h>

// In device
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 1680;

// For display
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

const int POINT_HISTORY = 5000;
const Uint32 POINT_LIFETIME = 2000; // milliseconds

// Expected format: DEVTODO:x:XXX:y:XXX
constexpr char HEADER_MAGIC[] = "DEVTODO:";
const char X_MAGIC[] = ":x:";
const char Y_MAGIC[] = ":y:";

struct TouchPoint {
    int x, y;
    Uint32 timestamp;
    bool active;
};

std::vector<TouchPoint> points;
int points_in_last_second = 0;
int points_in_current_second = 0;
Uint32 last_second_timestamp = 0;

// https://stackoverflow.com/questions/41558908/how-can-i-use-getline-without-blocking-for-input
bool getline_async(std::istream& is, std::string& str, char delim = '\n') {

    static std::string lineSoFar;
    char inChar;
    int charsRead = 0;
    bool lineRead = false;
    str = "";

    do {
        charsRead = is.readsome(&inChar, 1);
        if (charsRead == 1) {
            // if the delimiter is read then return the string so far
            if (inChar == delim) {
                str = lineSoFar;
                lineSoFar = "";
                lineRead = true;
            } else {  // otherwise add it to the string so far
                lineSoFar.append(1, inChar);
            }
        }
    } while (charsRead != 0 && !lineRead);

    return lineRead;
}

bool getline(std::istream& is, std::string& str, bool isasync) {
    if (isasync) {
        return getline_async(is, str);
    } else {
        std::getline(is, str);
        return !str.empty();
    }
}

bool parseLine(const std::string& line, int& x, int& y) {
    if (line.substr(0, sizeof(HEADER_MAGIC) - 1) != "DEVTODO:") { // '\0' count 1 in sizeof
        return false;
    }

    size_t xPos = line.find(X_MAGIC);
    size_t yPos = line.find(Y_MAGIC);

    if (xPos == std::string::npos || yPos == std::string::npos) return false;

    try {
        x = std::stoi(line.substr(xPos + sizeof(X_MAGIC) - 1)); // '\0' count 1 in sizeof
        y = std::stoi(line.substr(yPos + sizeof(Y_MAGIC) - 1)); // '\0' count 1 in sizeof
    } catch (...) {
        std::cerr << "ERROR: format stoi fail" << std::endl;
        return false;
    }
    return true;
}

void addPoint(int x, int y, Uint32 timestamp) {
    // Convert coordinates to screen space (assuming input is 0-1000 range)
    if (x < 0 || x >= SCREEN_WIDTH) {
        std::cout << "ERROR: x coordinate " << x << " out of SCREEN_WIDTH range [0, " << SCREEN_WIDTH << "]" ;
        std::cout << " {x: " << x << ", y:" << y << "}" << std::endl;
    }
    if (y < 0 || y >= SCREEN_HEIGHT) {
        std::cout << "ERROR: y coordinate " << y << " out of SCREEN_HEIGHT range [0, " << SCREEN_HEIGHT << "]" ;
        std::cout << " {x: " << x << ", y:" << y << "}" << std::endl;
    }

    // int screenX = std::clamp(x * WINDOW_WIDTH / 1000, 0, WINDOW_WIDTH - 1);
    // int screenY = std::clamp(y * WINDOW_HEIGHT / 1000, 0, WINDOW_HEIGHT - 1);
    // Convert coordinates to window space
    int windowX = std::clamp(static_cast<int>(static_cast<float>(x) * WINDOW_WIDTH / SCREEN_WIDTH), 0, WINDOW_WIDTH - 1);
    int windowY = std::clamp(static_cast<int>(static_cast<float>(y) * WINDOW_HEIGHT / SCREEN_HEIGHT), 0, WINDOW_HEIGHT - 1);

    points.push_back({windowX, windowY, timestamp, true});

    // Update PPS counter
    if (timestamp - last_second_timestamp >= 1000) {
        last_second_timestamp = timestamp;
        points_in_last_second = points_in_current_second;
        points_in_current_second = 1;
    } else {
        points_in_current_second++;
    }

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

void render(SDL_Renderer* renderer, TTF_Font* font) {
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

    // Render PPS counter
    if (font) {
        std::string pps_text = "PPS: " + std::to_string(points_in_last_second);
        SDL_Color white = {255, 255, 255, 255};
        SDL_Surface* surface = TTF_RenderText_Solid(font, pps_text.c_str(), white);
        if (surface) {
            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
            if (texture) {
                SDL_Rect rect = {WINDOW_WIDTH - surface->w - 10, 10, surface->w, surface->h};
                SDL_RenderCopy(renderer, texture, NULL, &rect);
                SDL_DestroyTexture(texture);
            }
            SDL_FreeSurface(surface);
        }
    }

    SDL_RenderPresent(renderer);
}


struct Option {
    bool print = false;
    std::string font = "";
    bool async = false;
};

int main(int argc, char* argv[]) {
    Option option {};
    for (int i = 0; i < argc; i++) {
        std::string arg {argv[i]};

        auto shift = [&]() {
            if(i < argc) {
                i++;
                arg = argv[i];
            } else {
                std::cerr << "ERROR: Fail to parse option with arguement, not enough argv";
                exit(1);
            }
        };

        if ((arg == "--print") || (arg == "-p")) {
            option.print = true;
        } else if ((arg == "--font") || (arg == "-f")) {
            shift();
            option.font = argv[i];
        } else if ((arg == "--async") || (arg == "-a")) {
            option.async = true;
        }
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    if (TTF_Init() == -1) {
        std::cerr << "TTF could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
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
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font *font = nullptr;
    if (option.font != "" ) {
        font = TTF_OpenFont(option.font.c_str(), 24);
        if (!font) {
            std::cerr << "Failed to load font! Using fallback. TTF_Error: " << TTF_GetError() << std::endl;
            // Continue without font - we just won't show the PPS counter
        }
    }

    bool quit = false;
    SDL_Event e;
    Uint32 last_update = SDL_GetTicks();
    last_second_timestamp = SDL_GetTicks();

    // Async must set this to work
    // https://stackoverflow.com/questions/41558908/how-can-i-use-getline-without-blocking-for-input
    std::ios_base::sync_with_stdio(false);

    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        // Read input from stdin
        std::string line;
        if (getline(std::cin, line, option.async)) {
            int x, y;
            if (parseLine(line, x, y)) {
                addPoint(x, y, SDL_GetTicks());
            } else if (option.print) { // parse error and option.print is true
                std::cout << line << std::endl;
            } else {
                // do nothing pass
            }
        }

        // Update and render at ~60 FPS
        Uint32 current_time = SDL_GetTicks();
        if (current_time - last_update > 16) {
            updatePoints(current_time);
            render(renderer, font);
            last_update = current_time;
        } else {
            SDL_Delay(1);
        }
    }

    if (font) {
        TTF_CloseFont(font);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
