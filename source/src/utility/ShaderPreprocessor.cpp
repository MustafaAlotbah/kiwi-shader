/**
 * @file ShaderPreprocessor.cpp
 * @brief Implementation of shader preprocessor.
 */

#include "utility/ShaderPreprocessor.h"
#include "utility/Logger.h"
#include <fstream>
#include <sstream>
#include <regex>

namespace ShaderPreprocessing {

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------
PreprocessResult ShaderPreprocessor::process(const std::string& mainFilePath) {
    PreprocessResult result;
    
    // Check if file exists
    if (!std::filesystem::exists(mainFilePath)) {
        result.success = false;
        result.errorMessage = "Shader file not found: " + mainFilePath;
        Logger::Error("ShaderPreprocessor", result.errorMessage, {"shader", "io"});
        return result;
    }
    
    // Get base directory from main file path
    std::string baseDir = std::filesystem::path(mainFilePath).parent_path().string();
    
    // Load main file
    std::ifstream file(mainFilePath);
    if (!file.is_open()) {
        result.success = false;
        result.errorMessage = "Failed to open shader file: " + mainFilePath;
        Logger::Error("ShaderPreprocessor", result.errorMessage, {"shader", "io"});
        return result;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();
    
    // Process
    ShaderPreprocessor preprocessor(baseDir);
    std::string processed = preprocessor.processRecursive(source, mainFilePath);
    
    if (!preprocessor.errorMessage_.empty()) {
        result.success = false;
        result.errorMessage = preprocessor.errorMessage_;
        return result;
    }
    
    result.success = true;
    result.source = processed;
    result.dependencies = preprocessor.dependencies_;
    
    return result;
}

PreprocessResult ShaderPreprocessor::processSource(const std::string& source, const std::string& baseDirectory) {
    PreprocessResult result;
    
    ShaderPreprocessor preprocessor(baseDirectory);
    std::string processed = preprocessor.processRecursive(source, "<source>");
    
    if (!preprocessor.errorMessage_.empty()) {
        result.success = false;
        result.errorMessage = preprocessor.errorMessage_;
        return result;
    }
    
    result.success = true;
    result.source = processed;
    result.dependencies = preprocessor.dependencies_;
    
    return result;
}

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------
bool ShaderPreprocessor::isIncludeDirective(const std::string& line) {
    // Match: #include "path" or #include <path>
    std::regex includeRegex(R"(^\s*#\s*include\s+[\"<])");
    return std::regex_search(line, includeRegex);
}

std::string ShaderPreprocessor::parseIncludePath(const std::string& line) {
    // Extract path from: #include "path/to/file.glsl" or #include <path/to/file.glsl>
    std::regex pathRegex(R"(#\s*include\s+[\"<]([^\"<>]+)[\">])");
    std::smatch match;
    
    if (std::regex_search(line, match, pathRegex) && match.size() > 1) {
        return match[1].str();
    }
    
    return "";
}

std::string ShaderPreprocessor::resolveIncludePath(const std::string& includePath, const std::string& currentFile) {
    namespace fs = std::filesystem;
    
    // If current file is not a real file (e.g., "<source>"), use base directory
    if (currentFile == "<source>" || currentFile.empty()) {
        fs::path resolved = fs::path(baseDirectory_) / includePath;
        return resolved.string();
    }
    
    // Try relative to current file
    fs::path currentDir = fs::path(currentFile).parent_path();
    fs::path relativePath = currentDir / includePath;
    
    if (fs::exists(relativePath)) {
        return fs::absolute(relativePath).string();
    }
    
    // Try relative to base directory
    fs::path basePath = fs::path(baseDirectory_) / includePath;
    if (fs::exists(basePath)) {
        return fs::absolute(basePath).string();
    }
    
    return "";
}

std::string ShaderPreprocessor::loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

//------------------------------------------------------------------------------
// Recursive processing
//------------------------------------------------------------------------------
std::string ShaderPreprocessor::processRecursive(const std::string& source, const std::string& currentFile) {
    std::stringstream result;
    std::istringstream stream(source);
    std::string line;
    int lineNumber = 0;
    
    while (std::getline(stream, line)) {
        lineNumber++;
        
        // Check if this is an #include directive
        if (isIncludeDirective(line)) {
            std::string includePath = parseIncludePath(line);
            
            if (includePath.empty()) {
                errorMessage_ = "Invalid #include syntax at line " + std::to_string(lineNumber) + 
                               " in " + currentFile + ": " + line;
                Logger::Error("ShaderPreprocessor", errorMessage_, {"shader", "preprocessor"});
                return "";
            }
            
            // Resolve the include path
            std::string resolvedPath = resolveIncludePath(includePath, currentFile);
            
            if (resolvedPath.empty()) {
                errorMessage_ = "Include file not found: " + includePath + 
                               " (referenced in " + currentFile + " at line " + std::to_string(lineNumber) + ")";
                Logger::Error("ShaderPreprocessor", errorMessage_, {"shader", "preprocessor", "io"});
                return "";
            }
            
            // Normalize path for comparison
            std::string normalizedPath = std::filesystem::absolute(resolvedPath).string();
            
            // Check for circular includes
            if (processedFiles_.find(normalizedPath) != processedFiles_.end()) {
                errorMessage_ = "Circular include detected: " + includePath + 
                               " (in " + currentFile + " at line " + std::to_string(lineNumber) + ")";
                Logger::Error("ShaderPreprocessor", errorMessage_, {"shader", "preprocessor"});
                return "";
            }
            
            // Mark as processed
            processedFiles_.insert(normalizedPath);
            dependencies_.push_back(normalizedPath);
            
            // Load and process the included file
            std::string includedSource = loadFile(resolvedPath);
            if (includedSource.empty()) {
                errorMessage_ = "Failed to read include file: " + resolvedPath;
                Logger::Error("ShaderPreprocessor", errorMessage_, {"shader", "preprocessor", "io"});
                return "";
            }
            
            // Add a comment showing where this include came from
            result << "// BEGIN INCLUDE: " << includePath << "\n";
            
            // Recursively process the included file
            std::string processed = processRecursive(includedSource, resolvedPath);
            if (!errorMessage_.empty()) {
                return "";  // Error occurred in recursive call
            }
            
            result << processed;
            result << "// END INCLUDE: " << includePath << "\n";
            
            // Unmark to allow re-inclusion from different paths (optional)
            // processedFiles_.erase(normalizedPath);
        }
        else {
            // Regular line, pass through
            result << line << "\n";
        }
    }
    
    return result.str();
}

} // namespace ShaderPreprocessing

