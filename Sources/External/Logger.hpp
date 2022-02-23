#pragma once

#define FMT_HEADER_ONLY
#include <fmt/format-inl.h>
#include <fmt/color.h>

#include <sstream>
#include <thread>
#include <atomic>
#include <iomanip>

class Logger
{
public:
    static std::size_t get_thread_id() noexcept {
        static std::atomic<std::size_t> thread_idx{0};
        thread_local std::size_t id = thread_idx;
        thread_idx++;
        return id;
    }

    template<typename ... Args>
    static void Info(const std::string& format, const Args&... args)
    {
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "[AR] ");
        fmt::print(fg(fmt::color::deep_pink) | fmt::emphasis::bold, "[Info] ");

        std::stringstream thread;
        thread << get_thread_id();

        fmt::print(fg(fmt::color::coral) | fmt::emphasis::bold, fmt::format("[🧵 {}] ", thread.str()));
        fmt::print(format, args...);
        fmt::print("\n");
    }

    template<typename ... Args>
    static void Error(const std::string& format, const Args&... args)
    {
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "[AR] ");
        fmt::print(fg(fmt::color::red) | fmt::emphasis::bold, "[Error] ");
        fmt::print(format, args...);
        fmt::print("\n");
    }

    template<typename ... Args>
    static void Resolve(const std::string& from, const std::string& to)
    {
        fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::bold, "[AR] ");
        fmt::print(fg(fmt::color::medium_purple) | fmt::emphasis::bold, "[Resolve]\n");
        fmt::print("\t{}", from);
        fmt::print(fg(fmt::color::medium_purple) | fmt::emphasis::bold, " ->\n");
        fmt::print("\t{}", to);
        fmt::print("\n");
    }
};
