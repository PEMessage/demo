// cl.exe main.cpp user32.lib

// See: https://learn.microsoft.com/en-us/windows/win32/learnwin32/working-with-strings
//      alwyas define both of them
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <assert.h>
#include <stdio.h>

// HINSTANCE hInstance: handle to current exe
// HINSTANCE hPrevInstance: always NULL in win32
// LPSTR lpCmdLine: cmdline args(not include program name)
// int nCmdShow: pass to `ShowWindow`
//
// `WINAPI` must follow after `return type declare`

#ifndef UNICODE
// NOTICE: we could still use WinMain at UNICODE program, use GetCommandLine
// to get unicode version of cmdline args
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    assert(hPrevInstance == NULL);
    MessageBox(NULL, "Goodbye, cruel world!", "Note", MB_OK);
    return 0;
}
#else
// See: https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nCmdShow)
{
    assert(hPrevInstance == NULL);
    MessageBox(NULL, L"Goodbye, cruel world!", L"Note", MB_OK);
    return 0;
}
#endif
