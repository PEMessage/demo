#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "CMSDK_CM3.h" // for SystemCoreClockUpdate();
extern int stdout_init (void);

void * __attribute__((optimize("O0"))) _malloc_r (struct _reent *, size_t) _NOTHROW;

#define MSG "Hello World!\n"
#define MSG_DONE "Done"

int main() {

    SystemCoreClockUpdate();
    stdout_init();

    char *message = (char *)malloc(sizeof(MSG));
    strcpy(message, MSG);
    printf("%s\n", message);

    char *message_done = (char *)malloc(sizeof(MSG_DONE));
    strcpy(message_done, MSG_DONE);
    printf("%s\n", message_done);

    free(message);
    free(message_done);

    while(1) {
        // printf("Now we are in SVC_Handler_Main\n");
    }

}
