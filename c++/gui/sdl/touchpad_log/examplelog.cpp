#include <iostream>
#include <cmath>
#include <algorithm> // for std::clamp
#include <unistd.h>  // for usleep (Linux/Mac)
#include <array>
#include <utility>

// Function that returns (x, y) coordinates based on time
std::pair<double, double> func(double time) {
    // Circular motion parameters
    const double radius = 1.0;
    const double angular_speed = 1.0;  // radians per second

    // Calculate coordinates on a circle
    double x = radius * cos(angular_speed * time);
    double y = radius * sin(angular_speed * time);

    return std::make_pair(x, y);
}

constexpr double lerp(double a, double b, double t) {
    return a + t * (b - a);
}

constexpr double scale_lerp(
    double value,
    double in_min, double in_max,
    double out_min, double out_max
) {
    value = std::clamp(value, in_min, in_max);
    double normalized = (value - in_min) / (in_max - in_min);
    return lerp(out_min, out_max, normalized);
}



int main(int argc, char *argv[]) {
    double time = 0.0;
    const double time_increment = 0.01; // Time step in seconds
    const int sleep_duration = 10000; // 100ms in microseconds

    while (true) {
        // Get coordinates from the function
        auto coordinates = func(time);
        double x = coordinates.first;
        double y = coordinates.second;

        int scaled_x = scale_lerp(x, -1, 1, 0, 720);
        int scaled_y = scale_lerp(y, -1, 1, 0, 1680);

        // Output in the specified format
        std::cout << "DEVTODO:x:" << scaled_x << ":y:" << scaled_y << std::endl;

        // Increment time
        time += time_increment;

        // Sleep for a while to control the output rate
        usleep(sleep_duration);
    }

    return 0;
}



