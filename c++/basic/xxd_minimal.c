#include <stdio.h>
#include <ctype.h>
#include <stdint.h>

#define PRINTF_INFO(fmt, ...) \
    printf("[INFO]: " fmt, ##__VA_ARGS__)

// Feature:
// 1. only use one printf perline
// 2. fully comply standard
void xxd_minimal(size_t len, void* buf)
{
    if (!buf || len == 0) {
        PRINTF_INFO("Empty buffer\n");
        return;
    }
    unsigned char* ptr = (unsigned char*)buf;

    PRINTF_INFO(" Offset    0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");

    for (size_t i = 0; i < len; i += 16)
    {
        char line[80];  // 足够容纳一行所有内容
        int pos = 0;

        // Part1: 写入偏移量
        pos += sprintf(line + pos, "%08zx  ", i);

        // Part2: 写入十六进制
        for (size_t j = 0; j < 16; j++)
        {
            if (i + j < len)
                pos += sprintf(line + pos, "%02x ", ptr[i + j]);
            else
                pos += sprintf(line + pos, "   ");
        }

        // Part3: 写入ASCII
        pos += sprintf(line + pos, " |");
        for (size_t j = 0; j < 16 && i + j < len; j++)
        {
            unsigned char c = ptr[i + j];
            pos += sprintf(line + pos, "%c", isprint(c) ? c : '.');
        }
        pos += sprintf(line + pos, "|");

        PRINTF_INFO("%s\n", line);
    }
}


int main(int argc, char *argv[])
{
    int a[] = {
        1, 2 ,3 ,4 ,5,
    };


    xxd_minimal(sizeof(a), a);
    return 0;
}
