#include <stdio.h>

// Thanks to https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
// Core hacks is Duff's device
// 在实际应用中，这种玩具版的协程实现不太可能有用，因为它依赖于 static 变量，因此无法做到重新进入或多线程
#define crBegin static int state=0; switch(state) { case 0:
#define crReturn(i,x) do { state=i; return x; case i:; } while (0)
#define crFinish }

int function(void) {
    static int i;
    crBegin;
    for (i = 0; i < 10; i++)
        crReturn(1, i);
    crFinish;
}

int main(int argc, char *argv[])
{
    int i;
    for ( i = 0 ; i < 10 ; i ++ ) {
        printf("comsumer loop : %d\n", function());
    }
    return 0;
}

// More refine lib
// https://github.com/jsseldenthuis/coroutine
//
// Must read:
// https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
