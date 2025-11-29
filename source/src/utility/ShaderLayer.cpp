/**
 * @file ShaderLayer.cpp
 * @brief Implementation of ShaderLayer for custom fragment shader rendering.
 */

#include "utility/ShaderLayer.h"
#include "utility/ShaderPreprocessor.h"
#include "utility/common.h"
#include "utility/Logger.h"

#include <fstream>
#include <sstream>
#include <map>
#include <type_traits>

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
    
    // Create GPU timer queries for performance profiling
    glGenQueries(2, gpuTimerQueries_);
    
    Logger::Info("ShaderLayer", "Initialized with GPU profiling", {"graphics", "shader"});
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
    if (gpuTimerQueries_[0] != 0) {
        glDeleteQueries(2, gpuTimerQueries_);
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
        Logger::Error("ShaderLayer", lastError_, {"shader", "io"});
        return false;
    }

    // Preprocess shader (handles #include directives)
    auto preprocessResult = ShaderPreprocessing::ShaderPreprocessor::process(fragmentPath);
    
    if (!preprocessResult.success) {
        lastError_ = "Preprocessing failed: " + preprocessResult.errorMessage;
        Logger::Error("ShaderLayer", lastError_, {"shader", "preprocessor"});
        return false;
    }
    
    std::string fragmentSrc = preprocessResult.source;
    
    // Store dependencies for hot-reload tracking
    shaderDependencies_ = preprocessResult.dependencies;

    // Try to compile
    ShaderCompileResult result = tryCompileShader(getDefaultVertexShader(), fragmentSrc);
    
    if (!result.success) {
        lastError_ = result.errorLog;
        Logger::Error("ShaderLayer", "Compilation failed:\n" + lastError_, {"shader", "compile"});
        return false;
    }

    // Success! Delete old shader and use new one
    if (shaderProgram_ != 0) {
        glDeleteProgram(shaderProgram_);
    }
    shaderProgram_ = result.programId;
    shaderSource_ = fragmentSrc;
    lastModTime_ = getFileModTime(fragmentPath);
    
    // Update dependency mod times
    dependencyModTimes_.clear();
    for (const auto& dep : shaderDependencies_) {
        dependencyModTimes_[dep] = getFileModTime(dep);
    }

    // Save current uniform values before parsing new ones
    std::map<std::string, Uniforms::UniformVariant> savedValues;
    for (const auto& uniformVariant : uniforms_.uniforms) {
        std::visit([&savedValues, &uniformVariant](const auto& uniform) {
            savedValues[uniform.name] = uniformVariant;
        }, uniformVariant);
    }
    
    // Parse annotated uniforms from preprocessed source
    uniforms_ = Uniforms::UniformParser::parse(fragmentSrc);
    
    // Restore previous values for uniforms that still exist (preserve user tweaks!)
    for (auto& uniformVariant : uniforms_.uniforms) {
        std::visit([&savedValues, &uniformVariant](auto& uniform) {
            auto it = savedValues.find(uniform.name);
            if (it != savedValues.end()) {
                // Check if types match before restoring
                if (uniformVariant.index() == it->second.index()) {
                    // Copy the saved variant value by visiting it
                    std::visit([&uniform](const auto& savedUniform) {
                        using T = std::decay_t<decltype(savedUniform)>;
                        using U = std::decay_t<decltype(uniform)>;
                        if constexpr (std::is_same_v<T, U>) {
                            uniform.value = savedUniform.value;
                        }
                    }, it->second);
                    Logger::Debug("ShaderLayer", "Preserved value for uniform: " + uniform.name, {"shader", "hotreload"});
                } else {
                    Logger::Debug("ShaderLayer", "Type changed for uniform: " + uniform.name + ", using default", {"shader", "hotreload"});
                }
            }
        }, uniformVariant);
    }
    
    // Update uniform locations for the new shader program
    Uniforms::UniformEditor::updateLocations(uniforms_, shaderProgram_);

    std::filesystem::path shaderFilename(fragmentPath);
    Logger::Info("ShaderLayer", "Shader loaded: " + shaderFilename.filename().string(), {"shader", "io"});
    
    if (!shaderDependencies_.empty()) {
        Logger::Debug("ShaderLayer", "Dependencies: " + std::to_string(shaderDependencies_.size()) + " file(s)", {"shader", "preprocessor"});
        for (const auto& dep : shaderDependencies_) {
            std::filesystem::path depPath(dep);
            Logger::Trace("ShaderLayer", "  Include: " + depPath.filename().string(), {"shader", "preprocessor"});
        }
        Logger::Debug("ShaderLayer", "Hot-reload enabled for all dependencies", {"shader"});
    }
    return true;
}

bool ShaderLayer::checkAndReload() {
    if (!autoReload_ || shaderPath_.empty()) {
        return false;
    }

    // Check main shader file
    auto currentModTime = getFileModTime(shaderPath_);
    if (currentModTime != lastModTime_) {
        std::filesystem::path shaderFilename(shaderPath_);
        Logger::Info("ShaderLayer", "File modified: " + shaderFilename.filename().string(), {"shader", "hotreload"});
        return loadShader(shaderPath_);
    }
    
    // Check all dependencies (included files)
    for (const auto& dep : shaderDependencies_) {
        auto it = dependencyModTimes_.find(dep);
        if (it != dependencyModTimes_.end()) {
            auto currentDepModTime = getFileModTime(dep);
            if (currentDepModTime != it->second) {
                std::filesystem::path depPath(dep);
                Logger::Info("ShaderLayer", "Include modified: " + depPath.filename().string(), {"shader", "hotreload"});
                Logger::Debug("ShaderLayer", "  Path: " + dep, {"shader", "hotreload"});
                return loadShader(shaderPath_);
            }
        }
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

void ShaderLayer::resetUniforms() {
    Uniforms::UniformEditor::resetToDefaults(uniforms_);
}

//------------------------------------------------------------------------------
// Rendering
//------------------------------------------------------------------------------
void ShaderLayer::render(float windowWidth, float windowHeight, double time, double deltaTime) {
    resolution_ = glm::vec2(windowWidth, windowHeight);
    
    // Update camera aspect ratio
    cameraController_.setAspectRatio(windowWidth / windowHeight);

    // Check for hot reload
    checkAndReload();

    // If no valid shader, just clear to a dark color
    if (shaderProgram_ == 0) {
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        gpuFrameTime_ = 0.0;
        return;
    }
    
    // Read results from previous frame (non-blocking)
    int previousQuery = 1 - currentQuery_;
    GLint available = 0;
    glGetQueryObjectiv(gpuTimerQueries_[previousQuery], GL_QUERY_RESULT_AVAILABLE, &available);
    if (available) {
        GLuint64 timeElapsed = 0;
        glGetQueryObjectui64v(gpuTimerQueries_[previousQuery], GL_QUERY_RESULT, &timeElapsed);
        gpuFrameTime_ = static_cast<double>(timeElapsed) / 1000000.0; // Convert nanoseconds to milliseconds
    }
    
    // Start GPU timer for this frame
    glBeginQuery(GL_TIME_ELAPSED, gpuTimerQueries_[currentQuery_]);

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

    // Bind custom annotated uniforms
    Uniforms::UniformEditor::bindUniforms(uniforms_, shaderProgram_);
    
    // Set camera uniforms (if shader uses them)
    cameraController_.setShaderUniforms(shaderProgram_);

    // Draw fullscreen quad
    GL_TRY(glBindVertexArray(quadVAO_));
    GL_TRY(glDrawArrays(GL_TRIANGLES, 0, 6));
    GL_TRY(glBindVertexArray(0));
    
    // End GPU timer
    glEndQuery(GL_TIME_ELAPSED);
    
    // Swap query buffers for next frame
    currentQuery_ = 1 - currentQuery_;
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

