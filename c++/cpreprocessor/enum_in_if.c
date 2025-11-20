

typedef enum {
    MODE_TIMER,
    MODE_EVENT
} mode_t;

#define MODE MODE_TIMER



#if MODE == MODE_TIMER
    #error "Not work as expect"
#endif

// See: https://stackoverflow.com/a/34677270
// There are no macros called
//      MODE_TIMER or MODE_EVENT
//
// so on your #if line,
// A and B get replaced by 0, so you actually have:
// #if 0 == 0

int main(int argc, char *argv[])
{
    return 0;
}
