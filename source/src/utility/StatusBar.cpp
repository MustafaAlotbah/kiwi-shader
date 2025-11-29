/**
 * @file StatusBar.cpp
 * @brief Implementation of status bar
 */

#include "utility/StatusBar.h"
#include <algorithm>

StatusBar& StatusBar::getInstance() {
    static StatusBar instance;
    return instance;
}

void StatusBar::setState(StatusBarState state) {
    state_ = state;
}

void StatusBar::setMessage(const std::string& message) {
    message_ = message;
}

void StatusBar::addWidget(const std::string& id, std::function<void()> renderFunc) {
    // Check if widget already exists
    auto it = std::find_if(widgets_.begin(), widgets_.end(),
        [&id](const StatusBarWidget& w) { return w.id == id; });
    
    if (it != widgets_.end()) {
        // Update existing widget
        it->render = std::move(renderFunc);
    } else {
        // Add new widget
        widgets_.emplace_back(id, std::move(renderFunc));
    }
}

void StatusBar::removeWidget(const std::string& id) {
    auto it = std::find_if(widgets_.begin(), widgets_.end(),
        [&id](const StatusBarWidget& w) { return w.id == id; });
    
    if (it != widgets_.end()) {
        widgets_.erase(it);
    }
}

void StatusBar::clearWidgets() {
    widgets_.clear();
}

ImVec4 StatusBar::getBackgroundColor() const {
    // Always dark gray
    return ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
}

void StatusBar::render() {
    // Position status bar at the absolute bottom of the work area
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 workPos = viewport->WorkPos;
    ImVec2 workSize = viewport->WorkSize;
    
    // Position status bar at the very bottom with small padding (dockspace reserves space for us)
    const float bottomPadding = 10.0f;
    ImVec2 statusBarPos = ImVec2(workPos.x, workPos.y + workSize.y - HEIGHT - bottomPadding);
    ImVec2 statusBarSize = ImVec2(workSize.x, HEIGHT);
    
    // Set window position and size
    ImGui::SetNextWindowPos(statusBarPos);
    ImGui::SetNextWindowSize(statusBarSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    // Window flags for status bar behavior
    ImGuiWindowFlags flags = 
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoDocking;
    
    // Push background color based on state
    ImGui::PushStyleColor(ImGuiCol_WindowBg, getBackgroundColor());
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    
    if (ImGui::Begin("##StatusBar", nullptr, flags)) {
        // Left section: Status message
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", message_.c_str());
        
        // Right section: Widgets (buttons, stats, etc.)
        if (!widgets_.empty()) {
            // Calculate total width needed for right widgets
            float rightWidgetsStartX = ImGui::GetWindowWidth();
            
            // Render widgets from right to left
            for (auto it = widgets_.rbegin(); it != widgets_.rend(); ++it) {
                // Render widget to measure its size
                float cursorStart = ImGui::GetCursorPosX();
                
                ImGui::SameLine();
                ImGui::PushID(it->id.c_str());
                it->render();
                ImGui::PopID();
            }
        }
    }
    ImGui::End();
    
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();
}

