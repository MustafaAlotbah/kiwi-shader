/**
 * @file FullscreenQuad.h
 * @brief Renders a texture to a fullscreen quad using OpenGL.
 * 
 * Used for fullscreen mode to bypass ImGui and render the framebuffer
 * directly to the screen.
 */

#pragma once

#include <glad/glad.h>
#include <string>

/**
 * @brief Renders a texture to fill the entire screen.
 * 
 * This class provides a simple way to display a framebuffer texture
 * in fullscreen mode without using ImGui.
 */
class FullscreenQuad {
public:
    FullscreenQuad();
    ~FullscreenQuad();
    
    // Delete copy constructor and assignment
    FullscreenQuad(const FullscreenQuad&) = delete;
    FullscreenQuad& operator=(const FullscreenQuad&) = delete;
    
    /**
     * @brief Initialize the quad VAO, VBO, and shader.
     * @return true if initialization succeeded
     */
    bool initialize();
    
    /**
     * @brief Render a texture to fill the entire screen.
     * @param textureId The OpenGL texture ID to render
     * @param screenWidth Current screen width
     * @param screenHeight Current screen height
     */
    void render(GLuint textureId, int screenWidth, int screenHeight);
    
    /**
     * @brief Render hint text on the screen.
     * @param text The text to display
     * @param x X position (0-1 normalized)
     * @param y Y position (0-1 normalized)
     */
    void renderHintText(const std::string& text, float x, float y);
    
    /**
     * @brief Check if the quad is initialized.
     */
    bool isInitialized() const { return initialized_; }
    
private:
    bool createShader();
    bool createQuad();
    
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint shaderProgram_ = 0;
    GLint textureLoc_ = -1;
    bool initialized_ = false;
};

