#pragma once

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "logger/logger.h"

class stderr_logger : public Logger {
    void log(int level, const char *fmt, ...) override {
        va_list args;
        va_start(args, fmt);
        auto len = std::vsnprintf(nullptr, 0, fmt, args);
        va_end(args);

        char *str = static_cast<char *>(std::malloc(1 + len * sizeof(char)));
        va_start(args, fmt);
        std::vsnprintf(str, 1 + len, fmt, args);
        va_end(args);

        std::cerr << "    libinfinite: " << str;
        free(str);
    }
};
