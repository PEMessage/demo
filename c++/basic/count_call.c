

#include <stdio.h>

void count_calls() {
    static int cnt = 0;  // Static local variable (retains value between calls)
    cnt++;
    if (cnt % 1000 != 0) { 
        printf("nr: %d\n", cnt); // Prints current count
        return ;
    }
    printf("This is %d!!!\n", cnt); // Prints current count
    cnt = 0;
}

int main() {
    // Call the function 1000 times
    for (int i = 0; i < 1000; i++) {
        count_calls();
    }
    return 0;
}


