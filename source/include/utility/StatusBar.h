/**
 * @file StatusBar.h
 * @brief Status bar component
 * 
 * Features:
 * - State-based background colors (Idle, Compiling, Error, Success)
 * - Left-aligned status text
 * - Right-aligned widgets (buttons, stats, info)
 * - Modular and extensible design
 */

#pragma once

#include <string>
#include <functional>
#include <vector>
#include <imgui.h>

/**
 * @brief Status bar states with corresponding colors
 */
enum class StatusBarState {
    Idle,       // Black background
    Compiling,  // Dark blue background
    Error,      // Dark red background
    Success     // Dark green background
};

/**
 * @brief Right-aligned widget in the status bar
 */
struct StatusBarWidget {
    std::string id;
    std::function<void()> render;
    
    StatusBarWidget(const std::string& widgetId, std::function<void()> renderFunc)
        : id(widgetId), render(std::move(renderFunc)) {}
};

/**
 * @brief Professional status bar component
 * 
 * Singleton class that provides a VSCode-style status bar at the
 * bottom of the application window.
 */
class StatusBar {
public:
    static StatusBar& getInstance();
    
    // Delete copy constructor and assignment
    StatusBar(const StatusBar&) = delete;
    StatusBar& operator=(const StatusBar&) = delete;
    
    /**
     * @brief Render the status bar
     */
    void render();
    
    /**
     * @brief Set the current state (affects background color)
     */
    void setState(StatusBarState state);
    
    /**
     * @brief Set the status message (left-aligned text)
     */
    void setMessage(const std::string& message);
    
    /**
     * @brief Add or update a right-aligned widget
     * @param id Unique identifier for the widget
     * @param renderFunc Function that renders the widget
     */
    void addWidget(const std::string& id, std::function<void()> renderFunc);
    
    /**
     * @brief Remove a widget by ID
     */
    void removeWidget(const std::string& id);
    
    /**
     * @brief Clear all widgets
     */
    void clearWidgets();
    
    /**
     * @brief Get background color for current state
     */
    ImVec4 getBackgroundColor() const;
    
    /**
     * @brief Get the height of the status bar (for layout calculations)
     */
    static constexpr float getHeight() { return HEIGHT; }
    
private:
    StatusBar() = default;
    ~StatusBar() = default;
    
    StatusBarState state_ = StatusBarState::Idle;
    std::string message_ = "Ready";
    std::vector<StatusBarWidget> widgets_;
    
    static constexpr float HEIGHT = 24.0f;
};

