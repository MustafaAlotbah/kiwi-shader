/**
 * @file ShaderPreprocessor.h
 * @brief Preprocessor for shader source code with #include support.
 *
 * Handles #include directives, resolves paths, tracks dependencies,
 * and detects circular includes.
 */

#pragma once

#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <filesystem>

namespace ShaderPreprocessing {

/**
 * @brief Result of preprocessing a shader.
 */
struct PreprocessResult {
    bool success = false;
    std::string source;              // Final preprocessed source
    std::string errorMessage;
    std::vector<std::string> dependencies;  // List of included files
};

/**
 * @brief Preprocessor that handles #include directives.
 */
class ShaderPreprocessor {
public:
    /**
     * @brief Preprocess a shader file.
     * @param mainFilePath Path to the main shader file
     * @return Preprocessed result with expanded includes
     */
    static PreprocessResult process(const std::string& mainFilePath);
    
    /**
     * @brief Preprocess shader source with includes.
     * @param source Shader source code
     * @param baseDirectory Base directory for resolving relative includes
     * @return Preprocessed result
     */
    static PreprocessResult processSource(const std::string& source, const std::string& baseDirectory);

private:
    std::string baseDirectory_;
    std::set<std::string> processedFiles_;  // Track to prevent circular includes
    std::vector<std::string> dependencies_;
    std::string errorMessage_;
    
    ShaderPreprocessor(std::string baseDir) : baseDirectory_(std::move(baseDir)) {}
    
    /**
     * @brief Recursively process source code and expand includes.
     */
    std::string processRecursive(const std::string& source, const std::string& currentFile);
    
    /**
     * @brief Parse a single #include directive.
     * @return Path to the included file, or empty if invalid
     */
    std::string parseIncludePath(const std::string& line);
    
    /**
     * @brief Resolve include path relative to current file or base directory.
     */
    std::string resolveIncludePath(const std::string& includePath, const std::string& currentFile);
    
    /**
     * @brief Load file contents.
     */
    std::string loadFile(const std::string& path);
    
    /**
     * @brief Check if a line is an #include directive.
     */
    static bool isIncludeDirective(const std::string& line);
};

} // namespace ShaderPreprocessing

