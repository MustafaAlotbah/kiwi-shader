/**
 * @file FullscreenQuad.cpp
 * @brief Implementation of fullscreen quad renderer.
 */

#include "utility/FullscreenQuad.h"
#include <iostream>

// Simple vertex shader for fullscreen quad
static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

// Simple fragment shader for texture display
static const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;

void main() {
    FragColor = texture(screenTexture, TexCoord);
}
)";

FullscreenQuad::FullscreenQuad() = default;

FullscreenQuad::~FullscreenQuad() {
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
    }
    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
    }
    if (shaderProgram_ != 0) {
        glDeleteProgram(shaderProgram_);
    }
}

bool FullscreenQuad::initialize() {
    if (initialized_) {
        return true;
    }
    
    if (!createShader()) {
        std::cerr << "[FullscreenQuad] Failed to create shader" << std::endl;
        return false;
    }
    
    if (!createQuad()) {
        std::cerr << "[FullscreenQuad] Failed to create quad" << std::endl;
        return false;
    }
    
    initialized_ = true;
    return true;
}

bool FullscreenQuad::createShader() {
    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "[FullscreenQuad] Vertex shader error: " << infoLog << std::endl;
        return false;
    }
    
    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "[FullscreenQuad] Fragment shader error: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return false;
    }
    
    // Link program
    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vertexShader);
    glAttachShader(shaderProgram_, fragmentShader);
    glLinkProgram(shaderProgram_);
    
    glGetProgramiv(shaderProgram_, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram_, 512, nullptr, infoLog);
        std::cerr << "[FullscreenQuad] Shader link error: " << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }
    
    // Cleanup shaders (they're linked now)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Get uniform location
    textureLoc_ = glGetUniformLocation(shaderProgram_, "screenTexture");
    
    return true;
}

bool FullscreenQuad::createQuad() {
    // Fullscreen quad vertices: position (x, y) and texture coords (u, v)
    // Note: texture coords are flipped vertically (0,0 at bottom-left for OpenGL)
    float quadVertices[] = {
        // Position    // TexCoord
        -1.0f,  1.0f,  0.0f, 1.0f,  // Top-left
        -1.0f, -1.0f,  0.0f, 0.0f,  // Bottom-left
         1.0f, -1.0f,  1.0f, 0.0f,  // Bottom-right
        
        -1.0f,  1.0f,  0.0f, 1.0f,  // Top-left
         1.0f, -1.0f,  1.0f, 0.0f,  // Bottom-right
         1.0f,  1.0f,  1.0f, 1.0f   // Top-right
    };
    
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    
    glBindVertexArray(vao_);
    
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    return true;
}

void FullscreenQuad::render(GLuint textureId, int screenWidth, int screenHeight) {
    if (!initialized_) {
        return;
    }
    
    // Set viewport to full screen
    glViewport(0, 0, screenWidth, screenHeight);
    
    // Clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Use our shader
    glUseProgram(shaderProgram_);
    
    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(textureLoc_, 0);
    
    // Draw the quad
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    // Reset state
    glUseProgram(0);
}

void FullscreenQuad::renderHintText(const std::string& text, float x, float y) {
    // Note: For simplicity, we won't render text in pure OpenGL mode.
    // The hint will be shown via ImGui in a minimal overlay if needed.
    // This function is a placeholder for future enhancement.
}

