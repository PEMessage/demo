// cl.exe main.cpp user32.lib
#include <windows.h>
#include <assert.h>
#include <stdio.h>

// HINSTANCE hInstance: handle to current exe
// HINSTANCE hPrevInstance: always NULL in win32
// LPSTR lpCmdLine: cmdline args(not include program name)
// int nCmdShow: pass to `ShowWindow`
//
// `WINAPI` must follow after `return type declare`
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    assert(hPrevInstance == NULL);
    MessageBox(NULL, "Goodbye, cruel world!", "Note", MB_OK);
    return 0;
}
