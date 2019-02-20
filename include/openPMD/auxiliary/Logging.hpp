/* Copyright 2019 Fabian Koller
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#if openPMD_HAVE_LOGGING
#   include <spdlog/spdlog.h>
#endif

namespace openPMD
{
namespace auxiliary
{
namespace log
{

enum class Level
{
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL,
    OFF
}; // Level

#if openPMD_HAVE_LOGGING
#   define LOG_SET_LEVEL(LEVEL) {                                             \
    [](openPMD::auxiliary::log::Level level) {                                \
        switch( level ) {                                                     \
            case openPMD::auxiliary::log::Level::TRACE:                       \
                spdlog::set_level(spdlog::level::trace);                      \
                return;                                                       \
            case openPMD::auxiliary::log::Level::DEBUG:                       \
                spdlog::set_level(spdlog::level::debug);                      \
                return;                                                       \
            case openPMD::auxiliary::log::Level::INFO:                        \
                spdlog::set_level(spdlog::level::info);                       \
                return;                                                       \
            case openPMD::auxiliary::log::Level::WARN:                        \
                spdlog::set_level(spdlog::level::warn);                       \
                return;                                                       \
            case openPMD::auxiliary::log::Level::ERROR:                       \
                spdlog::set_level(spdlog::level::err);                        \
                return;                                                       \
            case openPMD::auxiliary::log::Level::CRITICAL:                    \
                spdlog::set_level(spdlog::level::critical);                   \
                return;                                                       \
            case openPMD::auxiliary::log::Level::OFF:                         \
                spdlog::set_level(spdlog::level::off);                        \
                return;                                                       \
            default:                                                          \
                throw std::runtime_error("Logging level not supported!");     \
        }                                                                     \
    }((LEVEL));                                                               \
}
#   define LOG_TRACE(...) { spdlog::trace(__VA_ARGS__); }
#   define LOG_DEBUG(...) { spdlog::debug(__VA_ARGS__); }
#   define LOG_INFO(...) { spdlog::info(__VA_ARGS__); }
#   define LOG_WARN(...) { spdlog::warn(__VA_ARGS__); }
#   define LOG_ERROR(...) { spdlog::error(__VA_ARGS__); }
#   define LOG_CRITICAL(...) { spdlog::critical(__VA_ARGS__); }
#else
#   define NOOP do{ (void)sizeof(void*); } while( 0 )
#   define LOG_SET_LEVEL(LEVEL) NOOP
#   define LOG_TRACE(...) NOOP
#   define LOG_DEBUG(...) NOOP
#   define LOG_INFO(...) NOOP
#   define LOG_WARN(...) NOOP
#   define LOG_ERROR(...) NOOP
#   define LOG_CRITICAL(...) NOOP
#endif
} // log
} // auxiliary
} // openPMD
