//
// Created by Nathan Tormaschy on 4/19/23.
//

#ifndef ARGUS_SETTINGS_H
#define ARGUS_SETTINGS_H

//#define DEBUGGING
#define ARGUS_RUNTIME_ASSERT
#define ARGUS_STRIP
#define ARGUS_BROKER_ACCOUNT_TRACKING
//#define ARGUS_HISTORY

static double constexpr ARGUS_PORTFOLIO_MAX_LEVERAGE  = 2;
static double constexpr ARGUS_MP_PORTFOLIO_MAX_LEVERAGE = 1.75;

#include <stdexcept>
#include <string>

class RuntimeError : public std::runtime_error {
public:
    RuntimeError(const std::string& message, const char* file, int line)
        : std::runtime_error(message + " (" + file + ":" + std::to_string(line) + ")")
    {}
};

#define ARGUS_RUNTIME_ERROR(msg) throw RuntimeError(msg, __FILE__, __LINE__)

#endif //ARGUS_SETTINGS_H
