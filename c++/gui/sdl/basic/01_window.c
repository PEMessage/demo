// q-gcc: `pkg-config --cflags --libs sdl2` --
//
// Basic:
// https://thenumb.at/cpp-course/sdl2/01/01.html
#include <SDL2/SDL.h>
#include <assert.h>

int main(int argc, char *argv[])
{
    assert( SDL_Init( SDL_INIT_EVERYTHING ) >= 0);


    // 1. window
    // SDL_Window is a opaque, platform-independent windowing API, just like WinMain in windows
    SDL_Window* win = SDL_CreateWindow( "my window", 100, 100, 640, 480, SDL_WINDOW_SHOWN );
    assert(win);

    // 2. surface
    // Once you have created a window, you need a way to draw to it
    // SDL abstracts any area you can draw to—including loaded images—as a "surface."
    SDL_Surface* winSurface = SDL_GetWindowSurface( win );
    assert(winSurface);

    SDL_FillRect( winSurface, NULL, SDL_MapRGB( winSurface->format, 255, 90, 120 ));

    // the results can be seen in the window by calling SDL_UpdateWindowSurface()
    SDL_UpdateWindowSurface( win );

    // 3. wait for user input
    getchar();

    SDL_DestroyWindow( win );
    SDL_Quit();
    return 0;
}
