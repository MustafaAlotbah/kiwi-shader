
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
        
        Logger::Info("ShaderTest", "Opening shader from file dialog: " + path, {"ui", "io"});
        shaderLayer->loadShader(path);
    }
    
    void addToRecent(const std::string& path) {
        SettingsManager::getInstance().addRecentFile(path);
        SettingsManager::getInstance().setLastShader(path);
    }
    
    void onUpdateUI() override {
        // ===== Shader Controls Window =====
        {
            ImGui::Text("Shader Playground");
            ImGui::Separator();
            
        // Preset shader selector
        ImGui::Text("Preset Shaders:");
        if (ImGui::Combo("##preset", &selectedShader, shaderOptions, 5)) {
            std::string path = std::string(ASSETS_PATH) + "/shaders/" + shaderOptions[selectedShader];
            strncpy(shaderPathBuffer, path.c_str(), sizeof(shaderPathBuffer) - 1);
            addToRecent(path);
            Logger::Info("ShaderTest", "Switching to preset: " + std::string(shaderOptions[selectedShader]), {"ui", "shader"});
            shaderLayer->loadShader(shaderPathBuffer);
        }
            
            ImGui::Spacing();
            
            // Custom path input
            ImGui::Text("Custom Shader Path:");
            ImGui::InputText("##shaderpath", shaderPathBuffer, sizeof(shaderPathBuffer));
            
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            addToRecent(std::string(shaderPathBuffer));
            shaderLayer->loadShader(shaderPathBuffer);
        }
            
            // Reload button
            if (ImGui::Button("Force Reload")) {
                shaderLayer->forceReload();
            }
            
            // Auto-reload toggle
            bool autoReload = shaderLayer->isAutoReloadEnabled();
            if (ImGui::Checkbox("Auto-Reload on File Change", &autoReload)) {
                shaderLayer->setAutoReload(autoReload);
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            
            // ===== Status =====
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
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "● Compiled");
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "● Error");
                }
                
                ImGui::SameLine();
                if (shaderLayer->isAutoReloadEnabled()) {
                    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "◉ Auto-reload");
                }
                
                // Quick actions
                ImGui::Spacing();
                if (ImGui::Button("Reload")) {
                    shaderLayer->forceReload();
                }
                
                ImGui::SameLine();
                if (ImGui::Button("Reveal in Explorer")) {
                    std::string command = "explorer /select,\"" + path + "\"";
                    system(command.c_str());
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
                        shaderLayer->loadShader(file);
                        Logger::Info("ShaderTest", "Loaded shader from recent: " + filename, {"ui", "shader"});
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
