#include <iostream>
#include <string>
#include <chrono>

using namespace std;
using namespace std::chrono;

// const string HEADER = "DEVTODO:";

int main() {
    int lines_in_current_second = 0;
    int lines_in_last_second = 0;
    auto last_second_timestamp = steady_clock::now();


    string line;
    while (getline(cin, line)) {
        // if (line.substr(0, HEADER.size()) != HEADER) {
        //     continue;
        // }
        auto current_time = steady_clock::now();
        auto elapsed_ms = duration_cast<milliseconds>(current_time - last_second_timestamp).count();

        if (elapsed_ms >= 1000) {
            // New second started
            lines_in_last_second = lines_in_current_second;
            lines_in_current_second = 1;
            last_second_timestamp = current_time;

            cout << "Lines in last second: " << lines_in_last_second << endl;
        } else {
            // Still in same second
            lines_in_current_second++;
        }
    }

    // Print final count
    cout << "Final count - Lines in last second: " << lines_in_last_second << endl;

    return 0;
}

// For NDK See: https://developer.android.com/ndk/guides/other_build_systems?hl=zh-cn
// Must add -static-libstdc++ for old version of android: https://stackoverflow.com/questions/55184167/clang-linking-so-library-libc-shared-so
// /opt/android-sdk/ndk/25.1.8937393/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi21-clang++ countline.cpp -o countline  -static-libstdc++
