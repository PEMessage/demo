
// 【不依赖三方库和框架，直接操纵像素在 CPU 上跑 Shader | Tsoding】
// 【精准空降到 01:20】
//  https://www.bilibili.com/video/BV1NSC5BtEem/?share_source=copy_web&vd_source=ba726eaf572f03aca4ba3d79f0118159&t=80
#include <stdio.h>
#include <string.h>

#include <libgen.h> // for dirname, not show in manpage??
#include <sys/stat.h> // for mkdir
#include <unistd.h> // for access
#include <stdlib.h> // for free

#include <assert.h>

#define WIDTH 400
#define HEIGHT 300

#define OUTPUT_DIR "output"



int mkdir_recursive(const char *path, mode_t mode) {
    // If directory exists, return success
    if (access(path, F_OK) == 0) return 0;

    char *tmp = strdup(path);
    char *parent = dirname(tmp);

    if (strcmp(parent, ".") != 0 && strcmp(parent, "/") != 0) {
        mkdir_recursive(parent, mode);
    }

    mkdir(path, mode);

    free(tmp);
    return 0;
}

int main(int argc, char *argv[])
{
    mkdir_recursive(OUTPUT_DIR, 0755);
    FILE* f = fopen(OUTPUT_DIR "/1.ppm" , "wb");
    assert(f);

    fprintf(f, "P6\n");
    fprintf(f, "%d %d\n", WIDTH, HEIGHT);
    fprintf(f, "255\n");

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            fputc(0xFF, f); // RED
            fputc(0x0, f); // GREEN
            fputc(0x0, f); // BLUE
        }
    }


    fclose(f);
    return 0;
}
