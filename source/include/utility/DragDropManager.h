/**
 * @file DragDropManager.h
 * @brief Professional drag-and-drop file handling system
 * 
 * Features:
 * - Visual overlay during drag operations
 * - File type validation
 * - Extensible handler system for different file types
 * - Modular and reusable design
 * 
 * Note: GLFW only provides drop callbacks (not drag-enter/leave).
 * The onDragEnter/onDragLeave methods are provided for future platform-specific
 * implementations that can detect when files are being dragged over the window.
 * Currently, files are handled immediately on drop with status bar feedback.
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <map>
#include <filesystem>

/**
 * @brief Drag-drop operation state
 */
enum class DragDropState {
    Idle,           // No drag operation
    DragOver,       // File being dragged over window
    Dropped         // File was dropped (transitioning to Idle)
};

/**
 * @brief Information about a dragged/dropped file
 */
struct DroppedFileInfo {
    std::string path;
    std::string filename;
    std::string extension;
    bool isDirectory;
    
    DroppedFileInfo(const std::string& filePath);
};

/**
 * @brief Handler function for dropped files
 * Returns true if the file was handled, false otherwise
 */
using FileDropHandler = std::function<bool(const DroppedFileInfo&)>;

/**
 * @brief Professional drag-and-drop manager
 * 
 * Singleton class that manages file drag-and-drop operations
 * with visual feedback and extensible file type handlers.
 */
class DragDropManager {
public:
    static DragDropManager& getInstance();
    
    // Delete copy constructor and assignment
    DragDropManager(const DragDropManager&) = delete;
    DragDropManager& operator=(const DragDropManager&) = delete;
    
    /**
     * @brief Register a handler for a specific file extension
     * @param extension File extension (e.g., ".glsl", ".frag", ".vert")
     * @param handler Function to handle the dropped file
     */
    void registerHandler(const std::string& extension, FileDropHandler handler);
    
    /**
     * @brief Register a handler for directories
     * @param handler Function to handle the dropped directory
     */
    void registerDirectoryHandler(FileDropHandler handler);
    
    /**
     * @brief Process files being dragged over the window
     * Called from GLFW drag callback
     */
    void onDragEnter(const std::vector<std::string>& paths);
    
    /**
     * @brief Process dropped files
     * Called from GLFW drop callback
     */
    void onDrop(const std::vector<std::string>& paths);
    
    /**
     * @brief Clear drag state (when drag leaves window)
     */
    void onDragLeave();
    
    /**
     * @brief Render the drag-drop overlay
     * Should be called every frame after other UI
     */
    void renderOverlay();
    
    /**
     * @brief Get current drag-drop state
     */
    DragDropState getState() const { return state_; }
    
    /**
     * @brief Check if a file type is supported
     */
    bool isFileSupported(const std::string& path) const;
    
private:
    DragDropManager() = default;
    ~DragDropManager() = default;
    
    void updateDraggedFiles(const std::vector<std::string>& paths);
    void handleDroppedFiles();
    
    DragDropState state_ = DragDropState::Idle;
    std::vector<DroppedFileInfo> draggedFiles_;
    std::map<std::string, FileDropHandler> extensionHandlers_;
    FileDropHandler directoryHandler_;
    
    // Visual settings
    static constexpr float OVERLAY_ALPHA = 0.85f;
    static constexpr float ICON_SIZE = 64.0f;
};

