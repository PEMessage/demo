#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef USING_CHAR
typedef char CHAR;          // this have fmt error sometime
#else 
typedef unsigned char CHAR; // this work fine 
#endif

#define XXD(len,buf,fmt,args...)\
	do{\
	    uint i = 0;\
	    CHAR str[17];\
	    printf("[%s:%d]: "fmt"\n",__FILE__,__LINE__, ##args );\
	    printf(" Offset    0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n" );\
	    for (i = 0; i < (len) ; i++)\
	    {\
	        if (i % 16 == 0)\
	            printf("%08x  ", i/16);\
	        printf("%02x ", *((CHAR*)buf+i));\
	        if(isprint(*((CHAR*)buf+i)) != 0)\
	            str[i%16] = *((CHAR*)buf+i); \
	        else\
	            str[i%16] = '.';\
	        if (i % 16 == 15)\
	        {\
	            printf(" |");\
	            str[16] = '\0';\
	            printf( "%s", str);\
	            printf("|");\
	            printf("\n");\
	        }\
	    }\
	    if ((len) % 16 != 0 )\
	    {\
	        for (i = 0; i < (16 - ((len) % 16)); i++)\
	            printf("   ");\
	        printf( " |");\
	        str[(len) % 16+1] = '\0';\
	        printf( "%s", str);\
	        printf("|");\
	        printf("\n");\
	    }\
	    printf("\n");\
	}while(0)


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    fseek(file, 0L, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0L, SEEK_SET);

    char *buf = (char*) malloc(fsize + 1);
    if (!buf) {
        perror("Memory allocation failed");
        fclose(file);
        return EXIT_FAILURE;
    }

    fread(buf, 1, fsize, file);
    buf[fsize] = '\0'; // null terminate the buffer for safety

    XXD(fsize, buf, "Hex dump of");

    free(buf);
    fclose(file);

    return EXIT_SUCCESS;
}
