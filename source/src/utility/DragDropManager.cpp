/**
 * @file DragDropManager.cpp
 * @brief Implementation of drag-and-drop manager
 */

#include "utility/DragDropManager.h"
#include "utility/Logger.h"
#include <imgui.h>
#include <algorithm>

// ============================================================================
// DroppedFileInfo Implementation
// ============================================================================

DroppedFileInfo::DroppedFileInfo(const std::string& filePath) 
    : path(filePath) {
    
    std::filesystem::path fsPath(filePath);
    filename = fsPath.filename().string();
    extension = fsPath.extension().string();
    isDirectory = std::filesystem::is_directory(filePath);
    
    // Normalize extension to lowercase
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
}

// ============================================================================
// DragDropManager Implementation
// ============================================================================

DragDropManager& DragDropManager::getInstance() {
    static DragDropManager instance;
    return instance;
}

void DragDropManager::registerHandler(const std::string& extension, FileDropHandler handler) {
    std::string ext = extension;
    // Ensure extension starts with '.'
    if (!ext.empty() && ext[0] != '.') {
        ext = "." + ext;
    }
    // Normalize to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    extensionHandlers_[ext] = std::move(handler);
    Logger::Debug("DragDropManager", "Registered handler for: " + ext, {"dragdrop", "init"});
}

void DragDropManager::registerDirectoryHandler(FileDropHandler handler) {
    directoryHandler_ = std::move(handler);
    Logger::Debug("DragDropManager", "Registered directory handler", {"dragdrop", "init"});
}

void DragDropManager::onDragEnter(const std::vector<std::string>& paths) {
    updateDraggedFiles(paths);
    state_ = DragDropState::DragOver;
}

void DragDropManager::onDrop(const std::vector<std::string>& paths) {
    updateDraggedFiles(paths);
    state_ = DragDropState::Dropped;
    handleDroppedFiles();
    
    // Clear state after handling
    draggedFiles_.clear();
    state_ = DragDropState::Idle;
}

void DragDropManager::onDragLeave() {
    state_ = DragDropState::Idle;
    draggedFiles_.clear();
}

void DragDropManager::updateDraggedFiles(const std::vector<std::string>& paths) {
    draggedFiles_.clear();
    draggedFiles_.reserve(paths.size());
    
    for (const auto& path : paths) {
        if (std::filesystem::exists(path)) {
            draggedFiles_.emplace_back(path);
        }
    }
}

void DragDropManager::handleDroppedFiles() {
    for (const auto& fileInfo : draggedFiles_) {
        bool handled = false;
        
        // Handle directories
        if (fileInfo.isDirectory) {
            if (directoryHandler_) {
                handled = directoryHandler_(fileInfo);
                if (handled) {
                    Logger::Info("DragDropManager", "Loaded directory: " + fileInfo.filename, {"dragdrop", "io"});
                }
            } else {
                Logger::Warn("DragDropManager", "No handler for directories: " + fileInfo.filename, {"dragdrop"});
            }
            continue;
        }
        
        // Handle files by extension
        auto it = extensionHandlers_.find(fileInfo.extension);
        if (it != extensionHandlers_.end()) {
            handled = it->second(fileInfo);
            if (handled) {
                Logger::Info("DragDropManager", "Loaded file: " + fileInfo.filename, {"dragdrop", "io"});
            } else {
                Logger::Warn("DragDropManager", "Handler failed for: " + fileInfo.filename, {"dragdrop"});
            }
        } else {
            Logger::Warn("DragDropManager", "Unsupported file type: " + fileInfo.extension + " (" + fileInfo.filename + ")", {"dragdrop"});
        }
    }
}

bool DragDropManager::isFileSupported(const std::string& path) const {
    DroppedFileInfo info(path);
    
    if (info.isDirectory) {
        return directoryHandler_ != nullptr;
    }
    
    return extensionHandlers_.find(info.extension) != extensionHandlers_.end();
}

void DragDropManager::renderOverlay() {
    if (state_ != DragDropState::DragOver || draggedFiles_.empty()) {
        return;
    }
    
    // Get viewport for full-screen overlay
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    // Set up overlay window
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    ImGuiWindowFlags flags = 
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav;
    
    // Push overlay style
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, OVERLAY_ALPHA));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    
    if (ImGui::Begin("##DragDropOverlay", nullptr, flags)) {
        // Center content
        ImVec2 windowSize = ImGui::GetWindowSize();
        
        // Determine if files are supported
        bool allSupported = true;
        for (const auto& file : draggedFiles_) {
            if (!isFileSupported(file.path)) {
                allSupported = false;
                break;
            }
        }
        
        // Color scheme based on support
        ImVec4 borderColor = allSupported ? ImVec4(0.3f, 0.8f, 0.3f, 1.0f) : ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
        ImVec4 textColor = allSupported ? ImVec4(0.8f, 1.0f, 0.8f, 1.0f) : ImVec4(1.0f, 0.6f, 0.6f, 1.0f);
        
        // Draw border
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const float borderThickness = 4.0f;
        const float borderPadding = 20.0f;
        ImVec2 borderMin = ImVec2(viewport->Pos.x + borderPadding, viewport->Pos.y + borderPadding);
        ImVec2 borderMax = ImVec2(viewport->Pos.x + windowSize.x - borderPadding, viewport->Pos.y + windowSize.y - borderPadding);
        drawList->AddRect(borderMin, borderMax, ImGui::ColorConvertFloat4ToU32(borderColor), 8.0f, 0, borderThickness);
        
        // Center text vertically and horizontally
        ImGui::SetCursorPos(ImVec2(windowSize.x * 0.5f, windowSize.y * 0.5f - 60.0f));
        
        // Main message
        std::string message;
        if (draggedFiles_.size() == 1) {
            if (allSupported) {
                message = "Drop to load:";
            } else {
                message = "Unsupported file type:";
            }
        } else {
            if (allSupported) {
                message = "Drop to load " + std::to_string(draggedFiles_.size()) + " files";
            } else {
                message = "Some files not supported";
            }
        }
        
        // Calculate text size for centering
        ImVec2 messageSize = ImGui::CalcTextSize(message.c_str());
        ImGui::SetCursorPosX((windowSize.x - messageSize.x) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        ImGui::Text("%s", message.c_str());
        ImGui::PopStyleColor();
        
        // File names
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20.0f);
        
        for (const auto& file : draggedFiles_) {
            std::string displayText = file.filename;
            if (file.isDirectory) {
                displayText += " (folder)";
            }
            
            ImVec2 filenameSize = ImGui::CalcTextSize(displayText.c_str());
            ImGui::SetCursorPosX((windowSize.x - filenameSize.x) * 0.5f);
            
            bool supported = isFileSupported(file.path);
            ImVec4 fileColor = supported ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, fileColor);
            ImGui::Text("%s", displayText.c_str());
            ImGui::PopStyleColor();
            
            // Show extension/type info
            if (!supported && !file.isDirectory) {
                std::string unsupportedText = "(" + file.extension + " not supported)";
                ImVec2 unsupportedSize = ImGui::CalcTextSize(unsupportedText.c_str());
                ImGui::SetCursorPosX((windowSize.x - unsupportedSize.x) * 0.5f);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::TextUnformatted(unsupportedText.c_str());
                ImGui::PopStyleColor();
            }
        }
    }
    ImGui::End();
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

