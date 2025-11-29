/**
 * @file Logger.cpp
 * @brief Implementation of professional logging system.
 */

#include "utility/Logger.h"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <cstdlib>
#include <iostream>

//==============================================================================
// LogMessage Implementation
//==============================================================================

LogMessage::LogMessage(LogLevel lvl, std::string src, std::string msg, std::vector<std::string> tagList)
    : timestamp(std::chrono::system_clock::now())
    , level(lvl)
    , source(std::move(src))
    , message(std::move(msg))
    , tags(std::move(tagList))
{
    // Generate timestamp strings
    auto time_t_now = std::chrono::system_clock::to_time_t(timestamp);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()) % 1000;
    
    // Full timestamp: 2024-11-29 14:32:15.123
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();
    timestampStr = ss.str();
    
    // Short timestamp: 14:32:15.123
    std::stringstream ss2;
    ss2 << std::put_time(std::localtime(&time_t_now), "%H:%M:%S");
    ss2 << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();
    shortTimestampStr = ss2.str();
}

ImVec4 LogMessage::getColor() const {
    switch (level) {
        case LogLevel::TRACE:  return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);  // Gray
        case LogLevel::DEBUG:  return ImVec4(0.4f, 0.7f, 1.0f, 1.0f);  // Light Blue
        case LogLevel::INFO:   return ImVec4(0.8f, 0.8f, 0.8f, 1.0f);  // White
        case LogLevel::WARN:   return ImVec4(1.0f, 0.8f, 0.0f, 1.0f);  // Yellow/Orange
        case LogLevel::ERR:    return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);  // Red
        default:               return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

const char* LogMessage::getLevelName() const {
    switch (level) {
        case LogLevel::TRACE:  return "TRACE";
        case LogLevel::DEBUG:  return "DEBUG";
        case LogLevel::INFO:   return "INFO ";  // Padded to 5 chars
        case LogLevel::WARN:   return "WARN ";  // Padded to 5 chars
        case LogLevel::ERR:    return "ERROR";
        default:               return "UNKNOWN";
    }
}

const char* LogMessage::getLevelIcon() const {
    switch (level) {
        case LogLevel::TRACE:  return ".";  // Dot
        case LogLevel::DEBUG:  return "*";  // Asterisk
        case LogLevel::INFO:   return "i";  // Info
        case LogLevel::WARN:   return "!";  // Exclamation
        case LogLevel::ERR:    return "X";  // X/Cross
        default:               return "?";
    }
}

//==============================================================================
// Logger Implementation
//==============================================================================

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::addLogMessage(LogLevel level, const std::string& source, const std::string& message, const std::vector<std::string>& tags) {
    auto& inst = getInstance();
    
    // Check min log level
    if (level < inst.minLogLevel_) {
        return;
    }
    
    // Add to buffer
    if (inst.messages_.size() >= inst.maxBufferSize_) {
        inst.messages_.erase(inst.messages_.begin());
    }
    
    inst.messages_.emplace_back(level, source, message, tags);
    
    // Update caches
    inst.allSources_.insert(source);
    for (const auto& tag : tags) {
        inst.allTags_.insert(tag);
    }
    
    // Update stats
    inst.updateStats();
}

//==============================================================================
// Public Logging API
//==============================================================================

void Logger::Trace(const std::string& source, const std::string& message, const std::vector<std::string>& tags) {
    getInstance().addLogMessage(LogLevel::TRACE, source, message, tags);
}

void Logger::Debug(const std::string& source, const std::string& message, const std::vector<std::string>& tags) {
    getInstance().addLogMessage(LogLevel::DEBUG, source, message, tags);
}

void Logger::Info(const std::string& source, const std::string& message, const std::vector<std::string>& tags) {
    getInstance().addLogMessage(LogLevel::INFO, source, message, tags);
}

void Logger::Warn(const std::string& source, const std::string& message, const std::vector<std::string>& tags) {
    getInstance().addLogMessage(LogLevel::WARN, source, message, tags);
}

void Logger::Error(const std::string& source, const std::string& message, const std::vector<std::string>& tags) {
    getInstance().addLogMessage(LogLevel::ERR, source, message, tags);
}

// Legacy API (backwards compatibility)
void Logger::Log(const std::string& message) {
    Info("Application", message);
}

void Logger::Warn(const std::string& message) {
    Warn("Application", message);
}

void Logger::Error(const std::string& message) {
    Error("Application", message);
}

//==============================================================================
// Configuration
//==============================================================================

void Logger::setMaxBufferSize(size_t size) {
    getInstance().maxBufferSize_ = size;
}

void Logger::setMinLogLevel(LogLevel level) {
    getInstance().minLogLevel_ = level;
}

void Logger::clear() {
    auto& inst = getInstance();
    inst.messages_.clear();
    inst.allSources_.clear();
    inst.allTags_.clear();
    inst.updateStats();
}

std::vector<LogMessage> Logger::getAllMessages() {
    return getInstance().messages_;
}

void Logger::onDraw() {
    getInstance().draw();
}

//==============================================================================
// Internal Methods
//==============================================================================

void Logger::updateStats() {
    countTrace_ = 0;
    countDebug_ = 0;
    countInfo_ = 0;
    countWarn_ = 0;
    countError_ = 0;
    
    for (const auto& msg : messages_) {
        switch (msg.level) {
            case LogLevel::TRACE: countTrace_++; break;
            case LogLevel::DEBUG: countDebug_++; break;
            case LogLevel::INFO:  countInfo_++;  break;
            case LogLevel::WARN:  countWarn_++;  break;
            case LogLevel::ERR:   countError_++; break;
        }
    }
}

bool Logger::passesFilter(const LogMessage& msg) const {
    // Level filter
    if (msg.level < filterLevel_) {
        return false;
    }
    
    // Source filter
    if (selectedSource_ != "All" && msg.source != selectedSource_) {
        return false;
    }
    
    // Tag filter
    if (selectedTag_ != "All") {
        bool hasTag = std::find(msg.tags.begin(), msg.tags.end(), selectedTag_) != msg.tags.end();
        if (!hasTag) {
            return false;
        }
    }
    
    // Search filter
    if (searchBuffer_[0] != '\0') {
        std::string search = searchBuffer_;
        std::transform(search.begin(), search.end(), search.begin(), ::tolower);
        
        std::string msgLower = msg.message;
        std::transform(msgLower.begin(), msgLower.end(), msgLower.begin(), ::tolower);
        
        std::string srcLower = msg.source;
        std::transform(srcLower.begin(), srcLower.end(), srcLower.begin(), ::tolower);
        
        if (msgLower.find(search) == std::string::npos && 
            srcLower.find(search) == std::string::npos) {
            return false;
        }
    }
    
    return true;
}

//==============================================================================
// Font Management
//==============================================================================

bool Logger::tryLoadFont(const std::string& path, float size) {
    namespace fs = std::filesystem;
    
    if (!fs::exists(path)) {
        return false;
    }
    
    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF(path.c_str(), size);
    
    if (font) {
        getInstance().monoFont_ = font;
        getInstance().fontLoadAttempted_ = true;
        // Log after ImGui is fully initialized
        std::cout << "[Logger] Loaded monospace font: " << path << std::endl;
        return true;
    }
    
    return false;
}

void Logger::loadMonospaceFont() {
    auto& inst = getInstance();
    
    if (inst.fontLoadAttempted_) {
        return;
    }
    
    // Font size
    const float fontSize = 16.0f;
    
    // Get Windows Fonts directory
    const char* windir = std::getenv("WINDIR");
    std::string systemFonts = windir ? std::string(windir) + "\\Fonts\\" : "C:\\Windows\\Fonts\\";
    
    // Get user fonts directory
    const char* localappdata = std::getenv("LOCALAPPDATA");
    std::string userFonts = localappdata ? std::string(localappdata) + "\\Microsoft\\Windows\\Fonts\\" : "";
    
    // Priority list of monospace fonts to try
    std::vector<std::string> fontsToTry = {
        // Cascadia Code variants
        "CascadiaCode.ttf",
        "CascadiaCodePL.ttf",
        "CascadiaMono.ttf",
        "CascadiaMonoPL.ttf",
        
        // Fira Code
        "FiraCode-Regular.ttf",
        "FiraCode-Medium.ttf",
        
        // JetBrains Mono
        "JetBrainsMono-Regular.ttf",
        
        // Consolas (built-in Windows)
        "consola.ttf",
        "consolab.ttf",
    };
    
    // Try user fonts first, then system fonts
    for (const auto& fontName : fontsToTry) {
        // Try user fonts
        if (!userFonts.empty()) {
            if (tryLoadFont(userFonts + fontName, fontSize)) {
                return;
            }
        }
        
        // Try system fonts
        if (tryLoadFont(systemFonts + fontName, fontSize)) {
            return;
        }
    }
    
    // Mark as attempted even if no font was found
    inst.fontLoadAttempted_ = true;
    std::cout << "[Logger] No monospace font found, using default ImGui font" << std::endl;
}

//==============================================================================
// UI Rendering
//==============================================================================

void Logger::draw() {
    ImGui::SetNextWindowSize(ImVec2(800, 300), ImGuiCond_FirstUseEver);
    
    bool* p_open = nullptr;  // Could be passed as parameter if we want close button support
    if (!ImGui::Begin("Logger", p_open)) {
        ImGui::End();
        return;
    }
    
    // ===== Toolbar =====
    {
        // Stats
        ImGui::Text("Messages: %zu", messages_.size());
        ImGui::SameLine();
        
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "T:%d", countTrace_);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "D:%d", countDebug_);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "I:%d", countInfo_);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "W:%d", countWarn_);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "E:%d", countError_);
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 180);
        if (ImGui::Button("Clear")) {
            clear();
        }
        
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &autoScroll_);
        
        ImGui::Separator();
        
        // Filters
        ImGui::PushItemWidth(100);
        
        // Level filter
        const char* levels[] = { "TRACE", "DEBUG", "INFO", "WARN", "ERROR" };
        int currentLevel = static_cast<int>(filterLevel_);
        if (ImGui::Combo("##level", &currentLevel, levels, 5)) {
            filterLevel_ = static_cast<LogLevel>(currentLevel);
        }
        
        ImGui::SameLine();
        
        // Source filter
        std::vector<std::string> sources = { "All" };
        sources.insert(sources.end(), allSources_.begin(), allSources_.end());
        
        if (ImGui::BeginCombo("##source", selectedSource_.c_str())) {
            for (const auto& src : sources) {
                bool selected = (selectedSource_ == src);
                if (ImGui::Selectable(src.c_str(), selected)) {
                    selectedSource_ = src;
                }
            }
            ImGui::EndCombo();
        }
        
        ImGui::SameLine();
        
        // Tag filter
        std::vector<std::string> tags = { "All" };
        tags.insert(tags.end(), allTags_.begin(), allTags_.end());
        
        if (ImGui::BeginCombo("##tag", selectedTag_.c_str())) {
            for (const auto& tag : tags) {
                bool selected = (selectedTag_ == tag);
                if (ImGui::Selectable(tag.c_str(), selected)) {
                    selectedTag_ = tag;
                }
            }
            ImGui::EndCombo();
        }
        
        ImGui::SameLine();
        
        // Search
        ImGui::PushItemWidth(200);
        ImGui::InputTextWithHint("##search", "Search...", searchBuffer_, sizeof(searchBuffer_));
        ImGui::PopItemWidth();
        
        ImGui::PopItemWidth();
        
        ImGui::Separator();
    }
    
    // ===== Log Messages =====
    {
        // Use monospace font if available
        if (monoFont_) {
            ImGui::PushFont(monoFont_);
        }
        
        ImGui::BeginChild("LogMessages", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        for (const auto& msg : messages_) {
            if (!passesFilter(msg)) {
                continue;
            }
            
            // Format: [YYYY-MM-DD HH:MM:SS.mmm] [LEVEL] [Source] Message {tags}
            // Each part has its own color like Quarkus
            
            // Timestamp - Green (full date/time/millis)
            if (showTimestamp_) {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.3f, 1.0f), "[%s]", 
                    msg.timestampStr.c_str());
                ImGui::SameLine(0, 5);
            }
            
            // Level name - Color based on level
            ImGui::TextColored(msg.getColor(), "[%s]", msg.getLevelName());
            ImGui::SameLine(0, 5);
            
            // Source - Cyan
            if (showSource_) {
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.9f, 1.0f), "[%s]", msg.source.c_str());
                ImGui::SameLine(0, 5);
            }
            
            // Message - White/default
            ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.95f, 1.0f), "%s", msg.message.c_str());
            
            // Tags - Purple/magenta
            if (showTags_ && !msg.tags.empty()) {
                ImGui::SameLine(0, 5);
                std::string tagStr = "{";
                for (size_t i = 0; i < msg.tags.size(); ++i) {
                    if (i > 0) tagStr += ", ";
                    tagStr += msg.tags[i];
                }
                tagStr += "}";
                ImGui::TextColored(ImVec4(0.8f, 0.5f, 0.9f, 1.0f), "%s", tagStr.c_str());
            }
        }
        
        // Auto-scroll
        if (autoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        
        ImGui::EndChild();
        
        // Pop monospace font if we pushed it
        if (monoFont_) {
            ImGui::PopFont();
        }
    }
    
    ImGui::End();
}
