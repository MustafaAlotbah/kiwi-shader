/**
 * @file Logger.h
 * @brief Professional logging system with structured log messages.
 * 
 * Features:
 * - Multiple log levels (TRACE, DEBUG, INFO, WARN, ERROR)
 * - Structured messages with source, category, tags
 * - Filtering by level, source, tag
 * - Search functionality
 * - Export capabilities
 */

#pragma once

#include <string>
#include <vector>
#include <set>
#include <imgui.h>
#include <chrono>

/**
 * @brief Log severity levels
 */
enum class LogLevel {
    TRACE,      // Detailed diagnostic info
    DEBUG,      // Debug information
    INFO,       // General information
    WARN,       // Warning messages
    ERR         // Error messages (renamed from ERROR to avoid Windows macro conflict)
};

/**
 * @brief Structured log message with metadata
 */
struct LogMessage {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::string source;         // Class/module/component name
    std::string message;        // Main log message
    std::vector<std::string> tags;  // Categories/labels (e.g., "shader", "gpu", "io")
    
    // Cached strings for display
    std::string timestampStr;
    std::string shortTimestampStr;  // HH:MM:SS format
    
    LogMessage(LogLevel lvl, std::string src, std::string msg, std::vector<std::string> tagList = {});
    
    // Get color for this log level
    ImVec4 getColor() const;
    
    // Get level name as string
    const char* getLevelName() const;
    
    // Get level icon/badge
    const char* getLevelIcon() const;
};

/**
 * @brief Professional logging system (Singleton)
 */
class Logger {
public:
    // Deleted copy constructor and assignment operator (Singleton)
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // =========================================================================
    // Logging API
    // =========================================================================
    
    static void Trace(const std::string& source, const std::string& message, const std::vector<std::string>& tags = {});
    static void Debug(const std::string& source, const std::string& message, const std::vector<std::string>& tags = {});
    static void Info(const std::string& source, const std::string& message, const std::vector<std::string>& tags = {});
    static void Warn(const std::string& source, const std::string& message, const std::vector<std::string>& tags = {});
    static void Error(const std::string& source, const std::string& message, const std::vector<std::string>& tags = {});
    
    // Legacy API (backwards compatibility)
    static void Log(const std::string& message);
    static void Warn(const std::string& message);
    static void Error(const std::string& message);
    
    // =========================================================================
    // UI Rendering
    // =========================================================================
    
    static void onDraw();
    
    // =========================================================================
    // Configuration
    // =========================================================================
    
    static void setMaxBufferSize(size_t size);
    static void setMinLogLevel(LogLevel level);
    static void clear();
    static std::vector<LogMessage> getAllMessages();
    
    // =========================================================================
    // Font Management
    // =========================================================================
    
    static void loadMonospaceFont();
    static bool tryLoadFont(const std::string& path, float size);
    
private:
    Logger() = default;
    static Logger& getInstance();
    
    void addLogMessage(LogLevel level, const std::string& source, const std::string& message, const std::vector<std::string>& tags);
    void draw();
    
    // Log storage
    std::vector<LogMessage> messages_;
    size_t maxBufferSize_ = 1000;
    LogLevel minLogLevel_ = LogLevel::TRACE;
    
    // UI state
    bool autoScroll_ = true;
    bool showTimestamp_ = true;
    bool showSource_ = true;
    bool showTags_ = true;
    bool useShortTimestamp_ = true;
    
    // Filtering
    LogLevel filterLevel_ = LogLevel::TRACE;
    char searchBuffer_[256] = "";
    std::string selectedSource_ = "All";
    std::string selectedTag_ = "All";
    
    // Cached sets for dropdowns
    std::set<std::string> allSources_;
    std::set<std::string> allTags_;
    
    // Stats
    int countTrace_ = 0;
    int countDebug_ = 0;
    int countInfo_ = 0;
    int countWarn_ = 0;
    int countError_ = 0;
    
    void updateStats();
    void rebuildFilterCache();
    bool passesFilter(const LogMessage& msg) const;
    
    // Font
    ImFont* monoFont_ = nullptr;
    bool fontLoadAttempted_ = false;
};
