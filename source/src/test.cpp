
#include <format>

#include "utility/Layer2D.h"
#include "utility/ShaderLayer.h"
#include "utility/UniformEditor.h"
#include "utility/Logger.h"


// ShaderTest class - demonstrates the ShaderLayer with hot-reload and uniform controls
class ShaderTest : public KiwiCore {

    std::shared_ptr<ShaderLayer> shaderLayer = std::make_shared<ShaderLayer>();
    
    // UI state
    char shaderPathBuffer[512] = "";
    int selectedShader = 0;
    const char* shaderOptions[4] = {
        "default.frag",
        "plasma.frag",
        "raymarching.frag",
        "annotated_demo.frag"
    };

    void onLoad() override {
        addLayer(shaderLayer);
        
        // Set default shader path - use the annotated demo to show off the feature
        std::string defaultPath = std::string(ASSETS_PATH) + "/shaders/annotated_demo.frag";
        strncpy(shaderPathBuffer, defaultPath.c_str(), sizeof(shaderPathBuffer) - 1);
        selectedShader = 3;  // annotated_demo.frag
        
        // Load the default shader
        shaderLayer->loadShader(shaderPathBuffer);
    }

    void onUpdate(float time, float deltaTime) override {
        // ShaderLayer handles its own updates including hot-reload
    }

    void onUpdateUI() override {
        // ===== Shader Controls =====
        ImGui::Text("Shader Playground");
        ImGui::Separator();
        
        // Preset shader selector
        ImGui::Text("Preset Shaders:");
        if (ImGui::Combo("##preset", &selectedShader, shaderOptions, 4)) {
            std::string path = std::string(ASSETS_PATH) + "/shaders/" + shaderOptions[selectedShader];
            strncpy(shaderPathBuffer, path.c_str(), sizeof(shaderPathBuffer) - 1);
            shaderLayer->loadShader(shaderPathBuffer);
        }
        
        ImGui::Spacing();
        
        // Custom path input
        ImGui::Text("Custom Shader Path:");
        ImGui::InputText("##shaderpath", shaderPathBuffer, sizeof(shaderPathBuffer));
        
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
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
        
        // ===== Custom Uniform Controls =====
        auto& uniforms = shaderLayer->getUniforms();
        if (!uniforms.empty()) {
            ImGui::Spacing();
            ImGui::Separator();
            
            if (ImGui::CollapsingHeader("Shader Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
                // Reset button
                if (ImGui::Button("Reset All to Defaults")) {
                    shaderLayer->resetUniforms();
                }
                
                ImGui::Spacing();
                
                // Render uniform controls
                Uniforms::UniformEditor::renderControls(uniforms);
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
        
        // ===== Debug Info =====
        if (ImGui::CollapsingHeader("Debug Info")) {
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
};


// Registering ShaderTest app with the KiwiAppFactory
static bool isMyKiwiAppRegistered = KiwiAppFactory::getInstance()
        .registerApp("MyKiwiApp", []() -> KiwiCore* { return new ShaderTest(); });
