/**
 * @file ShaderLayer.cpp
 * @brief Implementation of ShaderLayer for custom fragment shader rendering.
 */

#include "utility/ShaderLayer.h"
#include "utility/common.h"
#include "utility/Logger.h"

#include <fstream>
#include <sstream>

//------------------------------------------------------------------------------
// Default vertex shader for fullscreen quad
//------------------------------------------------------------------------------
const char* ShaderLayer::getDefaultVertexShader() {
    return R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        
        out vec2 fragCoord;
        
        void main() {
            fragCoord = aPos * 0.5 + 0.5;  // Convert from [-1,1] to [0,1]
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";
}

//------------------------------------------------------------------------------
// Constructor / Destructor
//------------------------------------------------------------------------------
ShaderLayer::ShaderLayer() {
    setupFullscreenQuad();
    Logger::Log("ShaderLayer initialized");
}

ShaderLayer::~ShaderLayer() {
    if (shaderProgram_ != 0) {
        glDeleteProgram(shaderProgram_);
    }
    if (quadVAO_ != 0) {
        glDeleteVertexArrays(1, &quadVAO_);
    }
    if (quadVBO_ != 0) {
        glDeleteBuffers(1, &quadVBO_);
    }
}

//------------------------------------------------------------------------------
// Fullscreen quad setup
//------------------------------------------------------------------------------
void ShaderLayer::setupFullscreenQuad() {
    // Fullscreen quad vertices (two triangles)
    float quadVertices[] = {
        // First triangle
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        // Second triangle
        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    GL_TRY(glGenVertexArrays(1, &quadVAO_));
    GL_TRY(glGenBuffers(1, &quadVBO_));

    GL_TRY(glBindVertexArray(quadVAO_));
    GL_TRY(glBindBuffer(GL_ARRAY_BUFFER, quadVBO_));
    GL_TRY(glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW));

    GL_TRY(glEnableVertexAttribArray(0));
    GL_TRY(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr));

    GL_TRY(glBindVertexArray(0));
}

//------------------------------------------------------------------------------
// Shader compilation with error capture
//------------------------------------------------------------------------------
ShaderCompileResult ShaderLayer::tryCompileShader(const std::string& vertexSrc, const std::string& fragmentSrc) {
    ShaderCompileResult result;
    
    // Compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vSrc = vertexSrc.c_str();
    glShaderSource(vertexShader, 1, &vSrc, nullptr);
    glCompileShader(vertexShader);

    int success;
    char infoLog[1024];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 1024, nullptr, infoLog);
        result.errorLog = "VERTEX SHADER ERROR:\n" + std::string(infoLog);
        glDeleteShader(vertexShader);
        return result;
    }

    // Compile fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fSrc = fragmentSrc.c_str();
    glShaderSource(fragmentShader, 1, &fSrc, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 1024, nullptr, infoLog);
        result.errorLog = "FRAGMENT SHADER ERROR:\n" + std::string(infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return result;
    }

    // Link program
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 1024, nullptr, infoLog);
        result.errorLog = "SHADER LINK ERROR:\n" + std::string(infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        return result;
    }

    // Cleanup shaders (they're now part of the program)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    result.success = true;
    result.programId = program;
    return result;
}

//------------------------------------------------------------------------------
// File operations
//------------------------------------------------------------------------------
std::string ShaderLayer::loadFileContents(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::filesystem::file_time_type ShaderLayer::getFileModTime(const std::string& path) {
    try {
        return std::filesystem::last_write_time(path);
    } catch (...) {
        return std::filesystem::file_time_type{};
    }
}

//------------------------------------------------------------------------------
// Shader loading
//------------------------------------------------------------------------------
bool ShaderLayer::loadShader(const std::string& fragmentPath) {
    shaderPath_ = fragmentPath;
    lastError_.clear();

    // Check if file exists
    if (!std::filesystem::exists(fragmentPath)) {
        lastError_ = "File not found: " + fragmentPath;
        Logger::Error(lastError_);
        return false;
    }

    // Load fragment shader source
    std::string fragmentSrc = loadFileContents(fragmentPath);
    if (fragmentSrc.empty()) {
        lastError_ = "Failed to read file: " + fragmentPath;
        Logger::Error(lastError_);
        return false;
    }

    // Try to compile
    ShaderCompileResult result = tryCompileShader(getDefaultVertexShader(), fragmentSrc);
    
    if (!result.success) {
        lastError_ = result.errorLog;
        Logger::Error("Shader compilation failed:\n" + lastError_);
        return false;
    }

    // Success! Delete old shader and use new one
    if (shaderProgram_ != 0) {
        glDeleteProgram(shaderProgram_);
    }
    shaderProgram_ = result.programId;
    lastModTime_ = getFileModTime(fragmentPath);

    Logger::Log("Shader loaded successfully: " + fragmentPath);
    return true;
}

bool ShaderLayer::checkAndReload() {
    if (!autoReload_ || shaderPath_.empty()) {
        return false;
    }

    auto currentModTime = getFileModTime(shaderPath_);
    if (currentModTime != lastModTime_) {
        Logger::Log("Shader file changed, reloading...");
        return loadShader(shaderPath_);
    }
    return false;
}

bool ShaderLayer::forceReload() {
    if (shaderPath_.empty()) {
        lastError_ = "No shader path set";
        return false;
    }
    return loadShader(shaderPath_);
}

//------------------------------------------------------------------------------
// Rendering
//------------------------------------------------------------------------------
void ShaderLayer::render(float windowWidth, float windowHeight, double time, double deltaTime) {
    resolution_ = glm::vec2(windowWidth, windowHeight);

    // Check for hot reload
    checkAndReload();

    // If no valid shader, just clear to a dark color
    if (shaderProgram_ == 0) {
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }

    // Use shader and set uniforms
    GL_TRY(glUseProgram(shaderProgram_));

    // Shadertoy-compatible uniforms
    GLint loc;
    
    loc = glGetUniformLocation(shaderProgram_, "iTime");
    if (loc != -1) glUniform1f(loc, static_cast<float>(time));

    loc = glGetUniformLocation(shaderProgram_, "iTimeDelta");
    if (loc != -1) glUniform1f(loc, static_cast<float>(deltaTime));

    loc = glGetUniformLocation(shaderProgram_, "iResolution");
    if (loc != -1) glUniform3f(loc, windowWidth, windowHeight, 1.0f);

    // iMouse: xy = current pos (if down), zw = click pos
    loc = glGetUniformLocation(shaderProgram_, "iMouse");
    if (loc != -1) {
        glm::vec2 pixelPos = (mousePosition_ * 0.5f + 0.5f) * resolution_;
        glm::vec2 clickPixelPos = (mouseClickPosition_ * 0.5f + 0.5f) * resolution_;
        if (mouseDown_) {
            glUniform4f(loc, pixelPos.x, pixelPos.y, clickPixelPos.x, clickPixelPos.y);
        } else {
            glUniform4f(loc, pixelPos.x, pixelPos.y, -clickPixelPos.x, -clickPixelPos.y);
        }
    }

    // Draw fullscreen quad
    GL_TRY(glBindVertexArray(quadVAO_));
    GL_TRY(glDrawArrays(GL_TRIANGLES, 0, 6));
    GL_TRY(glBindVertexArray(0));
}

//------------------------------------------------------------------------------
// Input handling
//------------------------------------------------------------------------------
void ShaderLayer::updateMousePosition(glm::vec2 normalizedPosition) {
    mousePosition_ = normalizedPosition;
}

void ShaderLayer::handleMouseEvent(MouseEvent mouseEvent) {
    if (mouseEvent.type == MouseEvent::Type::Click) {
        mouseDown_ = true;
        mouseClickPosition_ = mouseEvent.position;
    } else if (mouseEvent.type == MouseEvent::Type::Release) {
        mouseDown_ = false;
    }
}

