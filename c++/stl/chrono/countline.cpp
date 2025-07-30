#include <iostream>
#include <string>
#include <chrono>

using namespace std;
using namespace std::chrono;

const string HEADER = "DEVTODO:";

int main() {
    int lines_in_current_second = 0;
    int lines_in_last_second = 0;
    auto last_second_timestamp = steady_clock::now();


    string line;
    while (getline(cin, line)) {
        if (line.substr(0, HEADER.size()) != HEADER) {
            continue;
        }
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
