

#include <format>

#include "utility/Layer2D.h"
#include "utility/ShaderLayer.h"
#include "utility/Logger.h"


// ShaderTest class - demonstrates the ShaderLayer with hot-reload
class ShaderTest : public KiwiCore {

    std::shared_ptr<ShaderLayer> shaderLayer = std::make_shared<ShaderLayer>();
    
    // UI state
    char shaderPathBuffer[512] = "";
    int selectedShader = 0;
    const char* shaderOptions[3] = {
        "default.frag",
        "plasma.frag",
        "raymarching.frag"
    };

    void onLoad() override {
        addLayer(shaderLayer);
        
        // Set default shader path
        std::string defaultPath = std::string(ASSETS_PATH) + "/shaders/default.frag";
        strncpy(shaderPathBuffer, defaultPath.c_str(), sizeof(shaderPathBuffer) - 1);
        
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
        if (ImGui::Combo("##preset", &selectedShader, shaderOptions, 3)) {
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
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        
        // ===== Help =====
        if (ImGui::CollapsingHeader("Shader Uniforms Help")) {
            ImGui::BulletText("iTime - playback time (seconds)");
            ImGui::BulletText("iTimeDelta - frame delta time");
            ImGui::BulletText("iResolution - viewport size (vec3)");
            ImGui::BulletText("iMouse - mouse state (vec4)");
            ImGui::BulletText("fragCoord - UV coords [0,1]");
            
            ImGui::Spacing();
            ImGui::TextWrapped(
                "Edit the shader file externally and save - "
                "it will hot-reload automatically!"
            );
        }
    }
};


// Registering ShaderTest app with the KiwiAppFactory
static bool isMyKiwiAppRegistered = KiwiAppFactory::getInstance()
        .registerApp("MyKiwiApp", []() -> KiwiCore* { return new ShaderTest(); });

