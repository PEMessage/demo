#include <iostream>
#include <unistd.h>
#include "timer.h"

#include<chrono>
#include<thread>


using namespace OHOS::Utils;
using namespace OHOS;
using namespace std;

int main (int argc, char *argv[]) {
    Timer timer("timer_test");
    Timer::TimerCallback func1 = [](){
        static int count = 0;
        cout << "Hi from func1, count " << count <<  endl;
        count ++;
    };
    Timer::TimerCallback func2 = [](){
        static int count = 0;
        cout << "Hi from func2, count " << count <<  endl;
        count ++;
    };

    timer.Setup();
    uint32_t timer1Id = timer.Register(func1, 1000);
    uint32_t timer2Id = timer.Register(func2, 2000);
    // while(1);
    this_thread::sleep_for(chrono::seconds(300));
    timer.Shutdown(true);

    return 0;
}

