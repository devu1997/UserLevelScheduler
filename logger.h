#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <cstring>
#include <cstdarg>


#define LOG_LEVEL_INFO

enum class LogLevel {
    INFO,
    TRACE,
    ERROR
};

class Logger {
public:
    Logger(std::ostream& output = std::cout);

    void info(const char* format, ...);
    void trace(const char* format, ...);
    void error(const char* format, ...);

private:
    std::ostream& outputStream;

    void log(const char* levelString, const char* format, va_list args);
};

Logger logger;
