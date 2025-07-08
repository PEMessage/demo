#include <string>
#include <stdio.h>

using namespace std;
int main (int argc, char *argv[]) {
    string s = "123445667777";
    for (auto x : s) {
        printf("Ascii: %c -> Int: %d\n", x, x);
    }
    return 0;
}
