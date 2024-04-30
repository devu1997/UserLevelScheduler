#include "logger.h"

Logger::Logger(std::ostream& output) : outputStream(output) {}

void Logger::trace(const char* format, ...) {
    #if defined(LOG_LEVEL_TRACE)
    va_list args;
    va_start(args, format);
    log("[TRACE] ", format, args);
    va_end(args);
    #endif
}

void Logger::info(const char* format, ...) {
    #if defined(LOG_LEVEL_TRACE) || defined(LOG_LEVEL_INFO)
    va_list args;
    va_start(args, format);
    log("[INFO] ", format, args);
    va_end(args);
    #endif
}

void Logger::error(const char* format, ...) {
    #if defined(LOG_LEVEL_TRACE) || defined(LOG_LEVEL_INFO) || defined(LOG_LEVEL_ERROR)
    va_list args;
    va_start(args, format);
    log("[ERROR] ", format, args);
    va_end(args);
    #endif
}

void Logger::log(const char* levelString, const char* format, va_list args) {
    // Get current date and time
    std::time_t now = std::time(nullptr);
    std::tm* localTime = std::localtime(&now);

    // Format date and time
    char timeBuffer[80];
    std::strftime(timeBuffer, 80, "[%Y-%m-%d %H:%M:%S] ", localTime);

    // Write date and time to output stream
    outputStream << timeBuffer;

    // Write log level to output stream
    outputStream << levelString;

    // Format additional arguments
    char messageBuffer[1024]; // Adjust buffer size as needed
    std::vsprintf(messageBuffer, format, args);

    // Write formatted message to output stream
    outputStream << messageBuffer << std::endl;
}
