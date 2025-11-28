//
// Created by Musta on 14/02/2024.
//

#include "utility/Logger.h"
#include <chrono>
#include <iomanip>  // for std::put_time
#include <sstream>  // for std::stringstream

LogMessage::LogMessage(const std::string& msg, const ImVec4& col) : message(msg), color(col)
{
    // Capture current time as timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();
    timestamp = ss.str();
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::Log(const std::string& message) {
    getInstance().addLogMessage(message, ImVec4(0.8f, 0.8f, 0.8f, 1.0f)); // White color
}

[[maybe_unused]] void Logger::Warn(const std::string& message) {
    getInstance().addLogMessage(message, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow color
}

void Logger::Error(const std::string& message) {
    getInstance().addLogMessage(message, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red color
}

void Logger::onDraw() {
    getInstance().draw();
}

void Logger::addLogMessage(const std::string& message, const ImVec4& color) {
    if (messages.size() >= maxBufferSize) {
        messages.erase(messages.begin()); // Remove the oldest message
    }
    messages.emplace_back(message, color);
}

void Logger::draw() {
    if (ImGui::Begin("Logger", nullptr, ImGuiWindowFlags_NoCollapse)) {
        // Custom styling (can be adjusted as needed)
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.01f, 0.01f, 0.9f)); // Dark background
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10)); // Padding
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f); // Rounded corners


        // Display messages with timestamps
        for (const auto& message : messages) {
            ImGui::PushStyleColor(ImGuiCol_Text, message.color);
            ImGui::TextWrapped("[%s] %s", message.timestamp.c_str(), message.message.c_str());
            ImGui::PopStyleColor();
        }

        // Ensure newest messages are visible
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        // Pop style variables and colors
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(1);

        // End of logger window
        ImGui::End();
    }
}