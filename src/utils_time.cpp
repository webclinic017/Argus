//
// Created by Nathan Tormaschy on 4/23/23.
//

#include <string>
#include <chrono>
#include <ctime>

std::string nanosecond_epoch_time_to_string(long long ns_epoch_time) {
    // Define the epoch time for the system clock
    std::chrono::system_clock::time_point sys_epoch_time;

    // Calculate the duration since the epoch time
    std::chrono::nanoseconds duration_since_epoch(ns_epoch_time);

    // Convert the nanoseconds duration to a system clock duration
    auto sys_duration = std::chrono::duration_cast<std::chrono::system_clock::duration>(duration_since_epoch);

    // Calculate the system clock time by adding the duration to the epoch time
    std::chrono::system_clock::time_point sys_time = sys_epoch_time + sys_duration;

    // Convert the system clock time to a C-style time
    std::time_t time = std::chrono::system_clock::to_time_t(sys_time);

    // Convert the C-style time to a string
    char buffer[80];
    std::strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", std::localtime(&time));

    // Convert the character array to a string
    std::string str(buffer);

    return str;
}
