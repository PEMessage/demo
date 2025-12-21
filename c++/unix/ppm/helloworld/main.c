
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <libgen.h> // for dirname, not show in manpage??

#include <stdlib.h> // for free

#define WIDTH 800
#define HEIGHT 600

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
    return 0;
}
