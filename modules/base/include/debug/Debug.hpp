//
// Created by martin on 8/1/24.
//

#pragma once

#ifndef LOFT_DEBUG_HPP
#define LOFT_DEBUG_HPP

#include <string>
#include <stdarg.h>

namespace lft::dbg {
    enum LogMessageSeverity {
        info = 0,
        warning,
        error
    };

    enum LogMessageType {
        general = 0,
        validation,
        performance
    };

    typedef void(*lft_log_callback)(LogMessageSeverity, LogMessageType, const char *__restrict __format, va_list args);

}
extern lft::dbg::lft_log_callback g_logCallback;

#define LOG_INFO(fmt, ...) g_logCallback(lft::dbg::LogMessageSeverity::info, lft::dbg::LogMessageType::general, fmt, __VA_ARGS__)
#define LOG_WARN(fmt, ...) g_logCallback(lft::dbg::LogMessageSeverity::warning, lft::dbg::LogMessageType::general, fmt, __VA_ARGS__)
#define LOG_FAIL(fmt, ...) g_logCallback(lft::dbg::LogMessageSeverity::error, lft::dbg::LogMessageType::general, fmt, __VA_ARGS__)

namespace lft::log {
    static void info(const char* __format, ...) {
        va_list list;
        va_start(list, __format);
        // g_logCallback(lft::dbg::LogMessageSeverity::info, lft::dbg::LogMessageType::general, __format, list);
        va_end(list);
    }

    static void warn(const char* __format, ...) {
        va_list list;
        va_start(list, __format);
        // g_logCallback(lft::dbg::LogMessageSeverity::warning, lft::dbg::LogMessageType::general, __format, list);
        va_end(list);
    }

    static void fail(const char* __format, ...) {
        va_list list;
        va_start(list, __format);
        // g_logCallback(lft::dbg::LogMessageSeverity::error, lft::dbg::LogMessageType::general, __format, list);
        va_end(list);
    }
}

#endif //LOFT_DEBUG_HPP
