

#include <stdio.h>

void count_calls_simple(int n) {
    static int cnt = 0;  // Static local variable (retains value between calls)
    cnt++;
    if (cnt % n != 0) { 
        printf("nr: %d\n", cnt); // Prints current count
        return ;
    }
    printf("This is %d!!!\n", cnt); // Prints current count
    cnt = 0;
}

int main() {
    // Call the function 1000 times
    for (int i = 0; i < 5; i++) {
        count_calls_simple(5);
    }
    return 0;
}


