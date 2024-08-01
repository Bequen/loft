//
// Created by martin on 8/1/24.
//

#pragma once

#ifndef LOFT_DEBUG_HPP
#define LOFT_DEBUG_HPP

#include <string>

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

    typedef void(*lft_log_callback)(LogMessageSeverity, LogMessageType, const char *__restrict __format, ...);

    extern lft::dbg::lft_log_callback g_logCallback;
}

namespace lft::log {
    inline void info(const char* __format, ...) {
#if LFT_DEBUG_LEVEL_3
        va_list list;
        va_start(list, __format);
        lft::dbg::g_logCallback(lft::dbg::LogMessageSeverity::info, lft::dbg::LogMessageType::general, __format, list);
        va_end(list);
#endif
    }

    inline void warn(const char* __format, ...) {
#if LFT_DEBUG_LEVEL_2 || LFT_DEBUG_LEVEL_3
        va_list list;
        va_start(list, __format);
        lft::dbg::g_logCallback(lft::dbg::LogMessageSeverity::warning, lft::dbg::LogMessageType::general, __format, list);
        va_end(list);
#endif
    }

    inline void fail(const char* __format, ...) {
#if LFT_DEBUG_LEVEL_1 || LFT_DEBUG_LEVEL_2 || LFT_DEBUG_LEVEL_3
        va_list list;
        va_start(list, __format);
        lft::dbg::g_logCallback(lft::dbg::LogMessageSeverity::error, lft::dbg::LogMessageType::general, __format, list);
        va_end(list);
#endif
    }
}


#endif //LOFT_DEBUG_HPP
