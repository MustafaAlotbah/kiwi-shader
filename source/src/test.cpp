
#include <format>
#include <filesystem>
#include <cstdlib>
#include <vector>
#include <string>
#include <algorithm>

#include "utility/Layer2D.h"
#include "utility/ShaderLayer.h"
#include "utility/UniformEditor.h"
#include "utility/Logger.h"
#include "utility/SettingsManager.h"
#include "utility/StatusBar.h"
#include "utility/DragDropManager.h"

// ShaderTest class - demonstrates the ShaderLayer with hot-reload and uniform controls
class ShaderTest : public KiwiCore {

    std::shared_ptr<ShaderLayer> shaderLayer = std::make_shared<ShaderLayer>();
    
    // UI state
    char shaderPathBuffer[512] = "";
    int selectedShader = 0;
    const char* shaderOptions[5] = {
        "default.frag",
        "plasma.frag",
        "raymarching.frag",
        "annotated_demo.frag",
        "example_with_includes.frag"
    };
    bool showShaderParameters = true;
    bool showProject = true;

    void onLoad() override {
        addLayer(shaderLayer);
        
        // Log startup with various levels
        Logger::Info("ShaderTest", "Application started", {"app", "startup"});
        Logger::Debug("ShaderTest", "Using assets path: " + std::string(ASSETS_PATH), {"app", "config"});
        
        // Try to load last shader from settings
        std::string lastShader = SettingsManager::getInstance().getLastShader();
        
        if (!lastShader.empty() && std::filesystem::exists(lastShader)) {
            // Load last used shader
            strncpy(shaderPathBuffer, lastShader.c_str(), sizeof(shaderPathBuffer) - 1);
            Logger::Info("ShaderTest", "Loading last shader: " + lastShader, {"app", "shader"});
            
            // Try to match with presets
            for (int i = 0; i < 5; ++i) {
                if (lastShader.find(shaderOptions[i]) != std::string::npos) {
                    selectedShader = i;
                    break;
                }
            }
        } else {
            // Set default shader path - use the annotated demo to show off the feature
            std::string defaultPath = std::string(ASSETS_PATH) + "/shaders/annotated_demo.frag";
            strncpy(shaderPathBuffer, defaultPath.c_str(), sizeof(shaderPathBuffer) - 1);
            selectedShader = 3;  // annotated_demo.frag
            Logger::Info("ShaderTest", "Loading default shader", {"app", "shader"});
        }
        
        // Load the shader
        shaderLayer->loadShader(shaderPathBuffer);
        
        // Initialize status bar widgets
        setupStatusBar();
        
        // Set initial status
        StatusBar::getInstance().setState(StatusBarState::Idle);
        StatusBar::getInstance().setMessage("Ready");
        
        // Setup drag-and-drop handlers
        setupDragDropHandlers();
    }
    
    void setupDragDropHandlers() {
        auto& dragDrop = DragDropManager::getInstance();
        
        // Handler for GLSL shader files (.glsl, .frag, .vert, .comp, .geom, .tesc, .tese)
        auto shaderHandler = [this](const DroppedFileInfo& file) -> bool {
            Logger::Info("ShaderTest", "Loading shader from drag-drop: " + file.filename, {"dragdrop", "shader"});
            
            // Update status bar
            StatusBar::getInstance().setState(StatusBarState::Compiling);
            StatusBar::getInstance().setMessage("Loading shader: " + file.filename);
            
            // Load the shader
            strncpy(shaderPathBuffer, file.path.c_str(), sizeof(shaderPathBuffer) - 1);
            shaderPathBuffer[sizeof(shaderPathBuffer) - 1] = '\0';
            shaderLayer->loadShader(file.path);
            
            // Add to recent files
            addToRecent(file.path);
            
            // Update status based on result
            updateShaderStatus();
            
            return true;
        };
        
        // Register all shader extensions
        dragDrop.registerHandler(".glsl", shaderHandler);
        dragDrop.registerHandler(".frag", shaderHandler);
        dragDrop.registerHandler(".vert", shaderHandler);
        dragDrop.registerHandler(".comp", shaderHandler);
        dragDrop.registerHandler(".geom", shaderHandler);
        dragDrop.registerHandler(".tesc", shaderHandler);
        dragDrop.registerHandler(".tese", shaderHandler);
        
        Logger::Info("ShaderTest", "Drag-and-drop handlers registered", {"dragdrop", "init"});
    }
    
    void setupStatusBar() {
        auto& statusBar = StatusBar::getInstance();
        
        // Widget: Mouse Position (UV and Pixel coords)
        statusBar.addWidget("mouse_pos", [this]() {
            glm::vec2 mouseUV = shaderLayer->getMousePosition();
            glm::vec2 resolution = frameSize();
            glm::vec2 mousePixel = mouseUV * resolution;
            
            // Show UV coords (0-1 range)
            ImGui::Text("UV: %.3f, %.3f", mouseUV.x, mouseUV.y);
            ImGui::SameLine();
            
            // Show pixel coords
            ImGui::Text("| Px: %.0f, %.0f", mousePixel.x, mousePixel.y);
        });
        
        // Widget: GPU Frame Time (smart unit formatting)
        statusBar.addWidget("gpu_time", [this]() {
            ImGui::Text("|");
            ImGui::SameLine();
            double gpuTime = shaderLayer->getGpuFrameTime();
            if (gpuTime > 0.0) {
                // Color-code based on performance (60 FPS = 16.67ms)
                ImVec4 color;
                if (gpuTime < 16.67) {
                    color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); // Green (good)
                } else if (gpuTime < 33.33) {
                    color = ImVec4(1.0f, 0.9f, 0.4f, 1.0f); // Yellow (okay)
                } else {
                    color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); // Red (slow)
                }
                
                // Smart unit formatting: us for < 9ms, ms otherwise
                if (gpuTime < 9.0) {
                    double microSeconds = gpuTime * 1000.0; // Convert ms to μs
                    ImGui::TextColored(color, "GPU: %.0f us", microSeconds);
                } else {
                    ImGui::TextColored(color, "GPU: %.2f ms", gpuTime);
                }
            } else {
                ImGui::Text("GPU: --");
            }
        });
        
        // Widget: Resolution
        statusBar.addWidget("resolution", [this]() {
            ImGui::Text("|");
            ImGui::SameLine();
            ImGui::Text("%.0fx%.0f", frameSize().x, frameSize().y);
        });
        
        // Widget: Reload Button
        statusBar.addWidget("reload_btn", [this]() {
            ImGui::Text("|");
            ImGui::SameLine();
            if (ImGui::SmallButton("Reload Shader")) {
                shaderLayer->forceReload();
            }
        });
    }

    void onUpdate(float time, float deltaTime) override {
        // ShaderLayer handles its own updates including hot-reload
    }

    void loadShaderFromMenu(const std::string& path) override {
        strncpy(shaderPathBuffer, path.c_str(), sizeof(shaderPathBuffer) - 1);
        shaderPathBuffer[sizeof(shaderPathBuffer) - 1] = '\0';
        
        // Find matching preset (if any)
        for (int i = 0; i < 5; ++i) {
            if (path.find(shaderOptions[i]) != std::string::npos) {
                selectedShader = i;
                break;
            }
        }
        
        // Add to recent list
        addToRecent(path);
        
        // Update status bar
        StatusBar::getInstance().setState(StatusBarState::Compiling);
        StatusBar::getInstance().setMessage("Loading shader: " + std::filesystem::path(path).filename().string());
        
        Logger::Info("ShaderTest", "Opening shader from file dialog: " + path, {"ui", "io"});
        shaderLayer->loadShader(path);
        
        // Update status based on result
        updateShaderStatus();
    }
    
    void addToRecent(const std::string& path) {
        SettingsManager::getInstance().addRecentFile(path);
        SettingsManager::getInstance().setLastShader(path);
    }
    
    void updateShaderStatus() {
        if (shaderLayer->hasValidShader()) {
            std::filesystem::path shaderPath(shaderLayer->getShaderPath());
            StatusBar::getInstance().setState(StatusBarState::Success);
            StatusBar::getInstance().setMessage("Shader: " + shaderPath.filename().string());
        } else {
            StatusBar::getInstance().setState(StatusBarState::Error);
            StatusBar::getInstance().setMessage("Shader compilation failed");
        }
    }
    
    void onUpdateUI() override {
        // ===== Shader Controls Window =====
        {
            // FPS Display
            ImGui::Text("Application: %.1f FPS (%.3f ms/frame)",
                        ImGui::GetIO().Framerate,
                        1000.0f / ImGui::GetIO().Framerate);
            
            ImGui::Separator();
            
            // ===== Status =====
            ImGui::Text("Shader Status");
            ImGui::Separator();
            
            ImGui::Text("Status:");
            if (shaderLayer->hasValidShader()) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Shader Active");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "No Valid Shader");
            }
            
            // Show current shader path
            ImGui::Text("Current: %s", shaderLayer->getShaderPath().c_str());
            
            // Show dependencies
            const auto& deps = shaderLayer->getDependencies();
            if (!deps.empty()) {
                ImGui::Text("Includes: %zu file(s)", deps.size());
                if (ImGui::IsItemHovered() && ImGui::BeginTooltip()) {
                    for (const auto& dep : deps) {
                        std::filesystem::path depPath(dep);
                        ImGui::BulletText("%s", depPath.filename().string().c_str());
                    }
                    ImGui::EndTooltip();
                }
            }
            
            // ===== Error Display =====
            const std::string& error = shaderLayer->getLastError();
            if (!error.empty()) {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Compilation Error:");
                
                // Scrollable error text
                ImGui::BeginChild("ErrorLog", ImVec2(0, 150), true);
                ImGui::TextWrapped("%s", error.c_str());
                ImGui::EndChild();
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            
            // ===== View Options =====
            if (ImGui::CollapsingHeader("View Options")) {
                auto& uniforms = shaderLayer->getUniforms();
                
                ImGui::Checkbox("Show Project Window", &showProject);
                
                // Only show if we have parameters
                if (!uniforms.empty()) {
                    ImGui::Checkbox("Show Shader Parameters", &showShaderParameters);
                }
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            
            // ===== Debug Info =====
            if (ImGui::CollapsingHeader("Debug Info")) {
                auto& uniforms = shaderLayer->getUniforms();
                ImGui::Text("Resolution: %.0f x %.0f", frameSize().x, frameSize().y);
                glm::vec2 mouse = shaderLayer->getMousePosition();
                ImGui::Text("Mouse (normalized): %.3f, %.3f", mouse.x, mouse.y);
                ImGui::Text("Mouse Down: %s", shaderLayer->isMouseDown() ? "Yes" : "No");
                ImGui::Text("Parsed Uniforms: %zu", uniforms.size());
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            
            // ===== Help =====
            if (ImGui::CollapsingHeader("Annotation Syntax")) {
                ImGui::TextWrapped("Add annotations before uniform declarations:");
                ImGui::Spacing();
                
                ImGui::BulletText("@slider(min=0.0, max=1.0, default=0.5)");
                ImGui::BulletText("@color(default=1.0,0.5,0.0)");
                ImGui::BulletText("@checkbox(default=true)");
                ImGui::BulletText("@vec2(default=0.5,0.5)");
                ImGui::BulletText("@vec3(default=1.0,0.0,0.0)");
                
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Example:");
                ImGui::TextWrapped("// @slider(min=0.0, max=10.0, default=1.0)\nuniform float uSpeed;");
            }
            
            if (ImGui::CollapsingHeader("Built-in Uniforms")) {
                ImGui::BulletText("iTime - playback time (seconds)");
                ImGui::BulletText("iTimeDelta - frame delta time");
                ImGui::BulletText("iResolution - viewport size (vec3)");
                ImGui::BulletText("iMouse - mouse state (vec4)");
                ImGui::BulletText("fragCoord - UV coords [0,1]");
            }
        }
        
        // ===== Project Window (separate) =====
        renderProjectWindow();
        
        // ===== Shader Parameters Window (separate) =====
        auto& uniforms = shaderLayer->getUniforms();
        if (!uniforms.empty() && showShaderParameters) {
            ImGui::Begin("Shader Parameters", &showShaderParameters);
            
            // Reset button
            if (ImGui::Button("Reset All to Defaults")) {
                shaderLayer->resetUniforms();
            }
            
            ImGui::SameLine();
            
            // Info text
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(%zu parameters)", uniforms.size());
            
            ImGui::Separator();
            ImGui::Spacing();
            
            // Render uniform controls
            Uniforms::UniformEditor::renderControls(uniforms);
            
            ImGui::End();
        }
    }
    
    void renderProjectWindow() {
        if (!showProject) return;
        
        ImGui::Begin("Project", &showProject);
        
        // ===== Preset Shaders =====
        ImGui::Text("Preset Shaders:");
        if (ImGui::Combo("##preset", &selectedShader, shaderOptions, 5)) {
            std::string path = std::string(ASSETS_PATH) + "/shaders/" + shaderOptions[selectedShader];
            strncpy(shaderPathBuffer, path.c_str(), sizeof(shaderPathBuffer) - 1);
            addToRecent(path);
            
            // Update status bar
            StatusBar::getInstance().setState(StatusBarState::Compiling);
            StatusBar::getInstance().setMessage("Loading shader: " + std::string(shaderOptions[selectedShader]));
            
            Logger::Info("ShaderTest", "Switching to preset: " + std::string(shaderOptions[selectedShader]), {"ui", "shader"});
            shaderLayer->loadShader(path);
            
            // Update status after load
            updateShaderStatus();
        }
        
        ImGui::Separator();
        
        // ===== Current Shader Section =====
        if (ImGui::CollapsingHeader("Current Shader", ImGuiTreeNodeFlags_DefaultOpen)) {
            const std::string& path = shaderLayer->getShaderPath();
            
            if (!path.empty()) {
                // Extract filename
                std::filesystem::path shaderPath(path);
                std::string filename = shaderPath.filename().string();
                std::string directory = shaderPath.parent_path().string();
                
                ImGui::Text("File:");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", filename.c_str());
                
                ImGui::Text("Path:");
                ImGui::TextWrapped("%s", directory.c_str());
                
                // Status
                ImGui::Spacing();
                if (shaderLayer->hasValidShader()) {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Compiled");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error");
                }
                
                // Actions row: Reload, Reveal, Auto-reload
                ImGui::Spacing();
                if (ImGui::Button("Reload")) {
                    shaderLayer->forceReload();
                }
                ImGui::SameLine();
                if (ImGui::Button("Reveal in Explorer")) {
                    std::string command = "explorer /select,\"" + path + "\"";
                    system(command.c_str());
                }
                ImGui::SameLine();
                bool autoReload = shaderLayer->isAutoReloadEnabled();
                if (ImGui::Checkbox("Auto-Reload", &autoReload)) {
                    shaderLayer->setAutoReload(autoReload);
                }
            } else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No shader loaded");
            }
        }
        
        // ===== Include Files Section =====
        const auto& deps = shaderLayer->getDependencies();
        if (!deps.empty()) {
            if (ImGui::CollapsingHeader("Include Files", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Dependencies: %zu file(s)", deps.size());
                ImGui::Separator();
                
                for (const auto& dep : deps) {
                    std::filesystem::path depPath(dep);
                    std::string filename = depPath.filename().string();
                    
                    ImGui::BulletText("%s", filename.c_str());
                    
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s\nClick to open in default editor", dep.c_str());
                    }
                    
                    if (ImGui::IsItemClicked()) {
                        std::string command = "start \"\" \"" + dep + "\"";
                        system(command.c_str());
                    }
                }
            }
        }
        
        // ===== Shader Statistics =====
        if (shaderLayer->hasValidShader()) {
            if (ImGui::CollapsingHeader("Statistics")) {
                auto& uniforms = shaderLayer->getUniforms();
                
                ImGui::Text("Annotated Uniforms: %zu", uniforms.size());
                ImGui::Text("Include Files: %zu", deps.size());
                
                // Could add more stats: line count, etc.
            }
        }
        
        // ===== Recent Files Section =====
        auto recentFiles = SettingsManager::getInstance().getRecentFiles();
        if (!recentFiles.empty()) {
            if (ImGui::CollapsingHeader("Recent Files")) {
                for (size_t i = 0; i < recentFiles.size(); ++i) {
                    const auto& file = recentFiles[i];
                    std::filesystem::path filePath(file);
                    std::string filename = filePath.filename().string();
                    
                    // Clickable recent file
                    ImGui::PushID(static_cast<int>(i));
                    if (ImGui::Selectable(filename.c_str())) {
                        // Load this shader directly
                        strncpy(shaderPathBuffer, file.c_str(), sizeof(shaderPathBuffer) - 1);
                        shaderPathBuffer[sizeof(shaderPathBuffer) - 1] = '\0';
                        
                        // Update status bar
                        StatusBar::getInstance().setState(StatusBarState::Compiling);
                        StatusBar::getInstance().setMessage("Loading shader: " + filename);
                        
                        shaderLayer->loadShader(file);
                        Logger::Info("ShaderTest", "Loaded shader from recent: " + filename, {"ui", "shader"});
                        
                        // Update status after load
                        updateShaderStatus();
                    }
                    ImGui::PopID();
                    
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s", file.c_str());
                    }
                }
                
                ImGui::Spacing();
                if (ImGui::Button("Clear Recent Files")) {
                    SettingsManager::getInstance().clearRecentFiles();
                }
            }
        }
        
        ImGui::End();
    }
};


// Registering ShaderTest app with the KiwiAppFactory
static bool isMyKiwiAppRegistered = KiwiAppFactory::getInstance()
        .registerApp("MyKiwiApp", []() -> KiwiCore* { return new ShaderTest(); });
