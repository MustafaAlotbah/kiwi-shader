#pragma once

#include <string>
#include <vector>
#include <imgui.h>

struct LogMessage {
    std::string message;
    ImVec4 color;
    std::string timestamp;

    LogMessage(const std::string& msg, const ImVec4& col);
};

class Logger {
public:
    // Deleted copy constructor and assignment operator (Singleton)
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    [[maybe_unused]] static void Log(const std::string& message);

    [[maybe_unused]] static void Warn(const std::string& message);

    [[maybe_unused]] static void Error(const std::string& message);

    static void onDraw();
private:
    Logger() = default;  // Private Constructor

    static Logger& getInstance();

    void addLogMessage(const std::string& message, const ImVec4& color);

    void draw();

    const size_t maxBufferSize = 100;
    std::vector<LogMessage> messages;
};
