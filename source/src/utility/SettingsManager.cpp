/**
 * @file SettingsManager.cpp
 * @brief Implementation of settings manager
 */

#include "utility/SettingsManager.h"
#include "utility/Logger.h"
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

SettingsManager::SettingsManager() 
    : loaded_(false)
{
    settingsPath_ = getSettingsFilePath();
    load();
}

SettingsManager::~SettingsManager() {
    save();
}

SettingsManager& SettingsManager::getInstance() {
    static SettingsManager instance;
    return instance;
}

std::string SettingsManager::getSettingsFilePath() const {
    // Store settings in the executable directory
    fs::path exePath = fs::current_path();
    return (exePath / "settings.json").string();
}

void SettingsManager::ensureDefaultSettings() {
    if (!data_.contains("recent_files")) {
        data_["recent_files"] = json::array();
    }
    if (!data_.contains("last_shader")) {
        data_["last_shader"] = "";
    }
    if (!data_.contains("preferences")) {
        data_["preferences"] = json::object();
    }
}

void SettingsManager::load() {
    if (loaded_) {
        return;
    }
    
    try {
        if (fs::exists(settingsPath_)) {
            std::ifstream file(settingsPath_);
            if (file.is_open()) {
                file >> data_;
                file.close();
                Logger::Info("SettingsManager", "Settings loaded from: " + settingsPath_, {"settings", "io"});
            } else {
                Logger::Warn("SettingsManager", "Could not open settings file: " + settingsPath_, {"settings", "io"});
                data_ = json::object();
            }
        } else {
            Logger::Info("SettingsManager", "No settings file found, using defaults", {"settings"});
            data_ = json::object();
        }
    } catch (const std::exception& e) {
        Logger::Error("SettingsManager", "Error loading settings: " + std::string(e.what()), {"settings", "error"});
        data_ = json::object();
    }
    
    ensureDefaultSettings();
    loaded_ = true;
}

void SettingsManager::save() {
    try {
        std::ofstream file(settingsPath_);
        if (file.is_open()) {
            file << data_.dump(4); // Pretty print with 4-space indent
            file.close();
            Logger::Debug("SettingsManager", "Settings saved to: " + settingsPath_, {"settings", "io"});
        } else {
            Logger::Error("SettingsManager", "Could not save settings to: " + settingsPath_, {"settings", "io"});
        }
    } catch (const std::exception& e) {
        Logger::Error("SettingsManager", "Error saving settings: " + std::string(e.what()), {"settings", "error"});
    }
}

void SettingsManager::addRecentFile(const std::string& path) {
    if (path.empty()) {
        return;
    }
    
    auto& recentFiles = data_["recent_files"];
    
    // Convert to vector for easier manipulation
    std::vector<std::string> files;
    if (recentFiles.is_array()) {
        files = recentFiles.get<std::vector<std::string>>();
    }
    
    // Remove if already exists
    auto it = std::find(files.begin(), files.end(), path);
    if (it != files.end()) {
        files.erase(it);
    }
    
    // Add to front
    files.insert(files.begin(), path);
    
    // Trim to max size
    if (files.size() > MAX_RECENT_FILES) {
        files.resize(MAX_RECENT_FILES);
    }
    
    // Save back to JSON
    data_["recent_files"] = files;
    
    Logger::Debug("SettingsManager", "Added to recent files: " + path, {"settings"});
    save();
}

std::vector<std::string> SettingsManager::getRecentFiles() const {
    if (data_.contains("recent_files") && data_["recent_files"].is_array()) {
        return data_["recent_files"].get<std::vector<std::string>>();
    }
    return {};
}

void SettingsManager::clearRecentFiles() {
    data_["recent_files"] = json::array();
    Logger::Info("SettingsManager", "Recent files cleared", {"settings"});
    save();
}

void SettingsManager::setLastShader(const std::string& path) {
    data_["last_shader"] = path;
    save();
}

std::string SettingsManager::getLastShader() const {
    if (data_.contains("last_shader") && data_["last_shader"].is_string()) {
        return data_["last_shader"].get<std::string>();
    }
    return "";
}

void SettingsManager::setBool(const std::string& key, bool value) {
    data_["preferences"][key] = value;
    save();
}

bool SettingsManager::getBool(const std::string& key, bool defaultValue) const {
    if (data_.contains("preferences") && data_["preferences"].contains(key)) {
        return data_["preferences"][key].get<bool>();
    }
    return defaultValue;
}

void SettingsManager::setInt(const std::string& key, int value) {
    data_["preferences"][key] = value;
    save();
}

int SettingsManager::getInt(const std::string& key, int defaultValue) const {
    if (data_.contains("preferences") && data_["preferences"].contains(key)) {
        return data_["preferences"][key].get<int>();
    }
    return defaultValue;
}

void SettingsManager::setFloat(const std::string& key, float value) {
    data_["preferences"][key] = value;
    save();
}

float SettingsManager::getFloat(const std::string& key, float defaultValue) const {
    if (data_.contains("preferences") && data_["preferences"].contains(key)) {
        return data_["preferences"][key].get<float>();
    }
    return defaultValue;
}

void SettingsManager::setString(const std::string& key, const std::string& value) {
    data_["preferences"][key] = value;
    save();
}

std::string SettingsManager::getString(const std::string& key, const std::string& defaultValue) const {
    if (data_.contains("preferences") && data_["preferences"].contains(key)) {
        return data_["preferences"][key].get<std::string>();
    }
    return defaultValue;
}

