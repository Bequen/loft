//
// Created by marti on 7/1/2024.
//

#ifndef LOFT_LOG_H
#define LOFT_LOG_H

#include <cstdarg>
#include <cstdio>

#define LOG_ENABLE 1

class Log {
private:
    FILE *output;

public:
#if LOG_ENABLE
    Log() {
        output = stdout;
    }

    inline void fail(const char* format, ...) const {
        va_list args;
        va_start(args, format);
        fwrite("[FAIL]: ", 1, 8, output);
        vfprintf(output, format, args);
        fwrite("\n", 1, 1, output);
    }

    inline void warn(const char* format, ...) const {
        va_list args;
        va_start(args, format);
        vfprintf(output, format, args);
        fwrite("\n", 1, 1, output);
    }

    inline void done(const char* format, ...) const {
        va_list args;
        va_start(args, format);
        vfprintf(output, format, args);
        fwrite("\n", 1, 1, output);
    }

    inline void mesg(const char* format, ...) const {
        va_list args;
        va_start(args, format);
        vfprintf(output, format, args);
        fwrite("\n", 1, 1, output);
    }
#else
    inline void fail(const char* format, ...) const {}

    inline void warn(const char* format, ...) const {}

    inline void done(const char* format, ...) const {}

    inline void mesg(const char* format, ...) const {}
#endif
};

static Log log = Log();

#define FAIL(fmt, ...) log.fail(fmt, ##__VA_ARGS__)

#endif //LOFT_LOG_H
