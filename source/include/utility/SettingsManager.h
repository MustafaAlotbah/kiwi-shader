/**
 * @file SettingsManager.h
 * @brief Application settings manager using JSON storage
 * 
 * Handles persistent storage of application settings including:
 * - Recent files list
 * - Last opened shader
 * - User preferences
 */

#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

/**
 * @brief Singleton class for managing application settings
 * 
 * Automatically loads settings on first access and provides
 * methods to save/load configuration data.
 */
class SettingsManager {
public:
    // Singleton access
    static SettingsManager& getInstance();
    
    // Delete copy constructor and assignment operator
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;
    
    // Settings file management
    void load();
    void save();
    
    // Recent files management
    void addRecentFile(const std::string& path);
    std::vector<std::string> getRecentFiles() const;
    void clearRecentFiles();
    
    // Last shader
    void setLastShader(const std::string& path);
    std::string getLastShader() const;
    
    // General preferences
    void setBool(const std::string& key, bool value);
    bool getBool(const std::string& key, bool defaultValue = false) const;
    
    void setInt(const std::string& key, int value);
    int getInt(const std::string& key, int defaultValue = 0) const;
    
    void setFloat(const std::string& key, float value);
    float getFloat(const std::string& key, float defaultValue = 0.0f) const;
    
    void setString(const std::string& key, const std::string& value);
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    
private:
    SettingsManager();
    ~SettingsManager();
    
    std::string getSettingsFilePath() const;
    void ensureDefaultSettings();
    
    json data_;
    std::string settingsPath_;
    bool loaded_;
    
    static const size_t MAX_RECENT_FILES = 10;
};

