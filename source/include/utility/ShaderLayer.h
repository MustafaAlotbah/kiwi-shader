/**
 * @file ShaderLayer.h
 * @brief A layer for rendering custom fragment shaders with hot-reload support.
 *
 * This layer renders a fullscreen quad with a custom fragment shader,
 * providing Shadertoy-style uniforms and hot-reload capability.
 *
 * @author Mustafa Alotbah
 * @version 0.2
 */

#pragma once

#include <string>
#include <memory>
#include <filesystem>
#include <chrono>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include "utility/Layer2D.h"
#include "utility/UniformTypes.h"
#include "utility/UniformParser.h"
#include "utility/UniformEditor.h"

/**
 * @brief Result of a shader compilation attempt.
 */
struct ShaderCompileResult {
    bool success = false;
    std::string errorLog;
    unsigned int programId = 0;
};

/**
 * @brief A dummy camera for ShaderLayer (shaders don't need camera transforms).
 */
class ShaderCamera : public Camera {
public:
    void setAspectRatio(float width, float height) override {
        aspectRatio_ = width / height;
    }
    [[nodiscard]] float getAspectRatio() const { return aspectRatio_; }
private:
    float aspectRatio_ = 1.0f;
};

/**
 * @brief A layer that renders a fullscreen quad with a custom fragment shader.
 *
 * Provides hot-reload capability and Shadertoy-compatible uniforms:
 * - iTime: shader playback time (in seconds)
 * - iResolution: viewport resolution (in pixels)
 * - iMouse: mouse position and click state
 * - iTimeDelta: time since last frame
 *
 * Also supports annotated custom uniforms parsed from shader source.
 */
class ShaderLayer : public KiwiLayer {
public:
    ShaderLayer();
    ~ShaderLayer() override;

    // KiwiLayer interface
    void render(float windowWidth, float windowHeight, double time, double deltaTime) override;
    Camera& getCamera() override { return camera_; }
    void updateMousePosition(glm::vec2 normalizedPosition) override;
    void handleMouseEvent(MouseEvent mouseEvent) override;

    /**
     * @brief Load a fragment shader from file.
     * @param fragmentPath Path to the fragment shader file.
     * @return true if compilation succeeded, false otherwise.
     */
    bool loadShader(const std::string& fragmentPath);

    /**
     * @brief Check if the shader file has been modified and reload if necessary.
     * @return true if shader was reloaded successfully, false if unchanged or error.
     */
    bool checkAndReload();

    /**
     * @brief Force reload the current shader file.
     * @return true if reload succeeded, false otherwise.
     */
    bool forceReload();

    /**
     * @brief Get the last compilation error (if any).
     */
    [[nodiscard]] const std::string& getLastError() const { return lastError_; }

    /**
     * @brief Check if a shader is currently loaded and valid.
     */
    [[nodiscard]] bool hasValidShader() const { return shaderProgram_ != 0; }

    /**
     * @brief Get the current shader file path.
     */
    [[nodiscard]] const std::string& getShaderPath() const { return shaderPath_; }

    /**
     * @brief Set whether auto-reload is enabled.
     */
    void setAutoReload(bool enabled) { autoReload_ = enabled; }
    [[nodiscard]] bool isAutoReloadEnabled() const { return autoReload_; }

    /**
     * @brief Get mouse position in world/normalized coordinates.
     */
    [[nodiscard]] glm::vec2 getMousePosition() const { return mousePosition_; }

    /**
     * @brief Check if mouse button is currently down.
     */
    [[nodiscard]] bool isMouseDown() const { return mouseDown_; }

    /**
     * @brief Get the collection of parsed custom uniforms.
     */
    [[nodiscard]] Uniforms::UniformCollection& getUniforms() { return uniforms_; }
    [[nodiscard]] const Uniforms::UniformCollection& getUniforms() const { return uniforms_; }

    /**
     * @brief Reset all custom uniforms to their default values.
     */
    void resetUniforms();

    /**
     * @brief Get the current shader program ID.
     */
    [[nodiscard]] unsigned int getProgramId() const { return shaderProgram_; }
    
    /**
     * @brief Get list of shader dependencies (included files).
     */
    [[nodiscard]] const std::vector<std::string>& getDependencies() const { return shaderDependencies_; }

private:
    // Shader compilation
    ShaderCompileResult tryCompileShader(const std::string& vertexSrc, const std::string& fragmentSrc);
    std::string loadFileContents(const std::string& path);
    std::filesystem::file_time_type getFileModTime(const std::string& path);

    // Setup fullscreen quad
    void setupFullscreenQuad();

    // Default vertex shader for fullscreen quad
    static const char* getDefaultVertexShader();

private:
    // OpenGL resources
    unsigned int shaderProgram_ = 0;
    unsigned int quadVAO_ = 0;
    unsigned int quadVBO_ = 0;

    // Shader file tracking
    std::string shaderPath_;
    std::string shaderSource_;  // Cached source for parsing
    std::filesystem::file_time_type lastModTime_;
    std::string lastError_;
    bool autoReload_ = true;
    
    // Include file tracking for hot-reload
    std::vector<std::string> shaderDependencies_;
    std::unordered_map<std::string, std::filesystem::file_time_type> dependencyModTimes_;

    // Custom uniforms parsed from annotations
    Uniforms::UniformCollection uniforms_;

    // Camera (minimal implementation)
    ShaderCamera camera_;

    // Input state
    glm::vec2 mousePosition_{0.0f};
    glm::vec2 mouseClickPosition_{0.0f};
    bool mouseDown_ = false;

    // Resolution tracking
    glm::vec2 resolution_{1.0f, 1.0f};
};
