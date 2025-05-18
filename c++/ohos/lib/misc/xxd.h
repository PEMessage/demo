#ifndef _XXD_H_
#define _XXD_H_
#include <stdio.h>
#include <ctype.h>


#define CHAR unsigned char

void inline xxd_color2(int len, CHAR* buf) {
    do {
        unsigned int i = 0;
        // Larger buffer to accommodate color codes (each char could need up to ~10 bytes for color codes)
        char str[17 * 10]; // Enough space for color codes in each character
        char *str_ptr = str;

        printf(" Offset    0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");

        for (i = 0; i < len; i++) {
            CHAR c = *((CHAR*)buf + i);

            // Print offset at start of each line
            if (i % 16 == 0) {
                printf("%08x  ", i/16);
            }

            // Apply color to hex value
            if (c > 31 && c < 127) {
                printf("\033[32m"); // COLOR_GREEN
            } else if (c == 9 || c == 10 || c == 13) {
                printf("\033[33m"); // COLOR_YELLOW
            } else if (c == 0) {
                printf("\033[37m"); // COLOR_WHITE
            } else if (c == 255) {
                printf("\033[34m"); // COLOR_BLUE
            } else {
                printf("\033[31m"); // COLOR_RED
            }

            // Print the byte
            printf("%02x\033[0m ", c);

            // Build the ASCII representation with colors
            if (isprint(c) != 0) {
                if (c > 31 && c < 127) {
                    str_ptr += sprintf(str_ptr, "\033[32m%c\033[0m", c); // GREEN
                } else if (c == 9 || c == 10 || c == 13) {
                    str_ptr += sprintf(str_ptr, "\033[33m%c\033[0m", c); // YELLOW
                } else {
                    str_ptr += sprintf(str_ptr, "\033[31m%c\033[0m", c); // RED
                }
            } else {
                if (c == 0) {
                    str_ptr += sprintf(str_ptr, "\033[37m.\033[0m"); // WHITE
                } else if (c == 255) {
                    str_ptr += sprintf(str_ptr, "\033[34m.\033[0m"); // BLUE
                } else {
                    str_ptr += sprintf(str_ptr, "\033[31m.\033[0m"); // RED
                }
            }

            // Print ASCII representation at end of line
            if (i % 16 == 15) {
                printf(" |%s|", str);
                str_ptr = str; // Reset string buffer
                printf("\n");
            }
        }

        // Handle partial last line
        if (len % 16 != 0) {
            for (i = 0; i < (16 - (len % 16)); i++) {
                printf("   ");
            }
            printf(" |%s|", str);
            printf("\n");
        }

        // printf("\n");
    } while(0);
}

#endif
