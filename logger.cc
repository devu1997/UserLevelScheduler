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
    // Get current time including milliseconds
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto value = now_ms.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(value).count();

    // Convert milliseconds to seconds and milliseconds
    auto seconds = millis / 1000;
    auto milliseconds = millis % 1000;

    // Get local time
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&now_time);

    // Format date and time with milliseconds
    char timeBuffer[80];
    std::strftime(timeBuffer, sizeof(timeBuffer), "[%Y-%m-%d %H:%M:%S:", localTime);

    // Append milliseconds to the formatted time
    char milliBuffer[4]; // Buffer for milliseconds
    std::sprintf(milliBuffer, "%03d", static_cast<int>(milliseconds));
    std::strcat(timeBuffer, milliBuffer);
    std::strcat(timeBuffer, "] "); // Add closing bracket and space

    // Write date and time with milliseconds to output stream
    outputStream << timeBuffer;

    // Write log level to output stream
    outputStream << levelString;

    // Format additional arguments
    char messageBuffer[1024]; // Adjust buffer size as needed
    std::vsprintf(messageBuffer, format, args);

    // Write formatted message to output stream
    outputStream << messageBuffer << std::endl;
}
