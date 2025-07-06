#include <stdio.h>
#include <assert.h>
#include <fcntl.h> // for open
#include <sys/file.h> // for flock
#include <unistd.h> // POSIX API -- sleep


// See:
//   https://jyywiki.cn/pages/OS/2022/demos/flock-demo.c
//   https://www.bilibili.com/video/BV1fY4y137yh/
//   【Android 系统 (Android App 和系统架构；应用后台保活) [南京大学2022操作系统-P31]】
//   【精准空降到 1:23:35】
int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        assert(fd > 0);
        printf("Acquire %s\n", argv[i]);
        flock(fd, LOCK_EX);
        sleep(3);
    }
    printf("All locks acquired!\n");
}
