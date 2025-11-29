/**
 * @file UniformParser.h
 * @brief Service for parsing shader annotations and extracting uniform metadata.
 *
 * Parses GLSL shader source code looking for specially formatted comments
 * that describe uniform properties (like Unity's property attributes).
 *
 * Supported annotations:
 *   // @slider(min=0.0, max=1.0, default=0.5)
 *   uniform float uMyFloat;
 *
 *   // @color(default=1.0,0.5,0.0)
 *   uniform vec3 uTint;
 *
 *   // @checkbox(default=true)
 *   uniform bool uEnabled;
 */

#pragma once

#include <string>
#include <regex>
#include <optional>
#include "utility/UniformTypes.h"
#include "utility/AnnotationParser.h"

namespace Uniforms {

/**
 * @brief Parser service that extracts annotated uniforms from shader source.
 */
class UniformParser {
public:
    /**
     * @brief Parse shader source and extract all annotated uniforms.
     * @param shaderSource The GLSL shader source code.
     * @return Collection of parsed uniforms.
     */
    static UniformCollection parse(const std::string& shaderSource);

private:
    /**
     * @brief Parse a single annotation and its following uniform declaration.
     * @param annotation The annotation string (e.g., "@slider(min=0, max=1)")
     * @param uniformDecl The uniform declaration (e.g., "uniform float uSpeed;")
     * @return Parsed uniform variant, or nullopt if parsing failed.
     */
    static std::optional<UniformVariant> parseAnnotatedUniform(
        const std::string& annotation,
        const std::string& uniformDecl
    );

    /**
     * @brief Extract the uniform type from a declaration.
     * @param decl The uniform declaration string.
     * @return The GLSL type (e.g., "float", "vec3").
     */
    static std::string extractType(const std::string& decl);

    /**
     * @brief Extract the uniform name from a declaration.
     * @param decl The uniform declaration string.
     * @return The variable name.
     */
    static std::string extractName(const std::string& decl);

    /**
     * @brief Convert variable name to display name (e.g., "uMySpeed" -> "My Speed").
     * @param name The variable name.
     * @return Human-readable display name.
     */
    static std::string toDisplayName(const std::string& name);

    /**
     * @brief Parse annotation parameters into a key-value map.
     * @param params The parameters string (e.g., "min=0.0, max=1.0").
     * @return Map of parameter names to values.
     */
    static std::unordered_map<std::string, std::string> parseParams(const std::string& params);

    /**
     * @brief Parse a float value from string.
     */
    static float parseFloat(const std::string& str, float defaultVal = 0.0f);

    /**
     * @brief Parse an int value from string.
     */
    static int parseInt(const std::string& str, int defaultVal = 0);

    /**
     * @brief Parse a bool value from string.
     */
    static bool parseBool(const std::string& str, bool defaultVal = false);

    /**
     * @brief Parse a vec2 from comma-separated string.
     */
    static glm::vec2 parseVec2(const std::string& str, glm::vec2 defaultVal = glm::vec2(0.0f));

    /**
     * @brief Parse a vec3 from comma-separated string.
     */
    static glm::vec3 parseVec3(const std::string& str, glm::vec3 defaultVal = glm::vec3(0.0f));

    /**
     * @brief Parse a vec4 from comma-separated string.
     */
    static glm::vec4 parseVec4(const std::string& str, glm::vec4 defaultVal = glm::vec4(0.0f));

    /**
     * @brief Parse color from hex string (e.g., "#FF8800" or "FF8800").
     */
    static glm::vec4 parseHexColor(const std::string& str);

    // Annotation type handlers
    static std::optional<UniformVariant> parseSlider(
        const std::string& type,
        const std::string& name,
        const ParamMap& params
    );

    static std::optional<UniformVariant> parseColor(
        const std::string& type,
        const std::string& name,
        const ParamMap& params
    );

    static std::optional<UniformVariant> parseCheckbox(
        const std::string& type,
        const std::string& name,
        const ParamMap& params
    );

    static std::optional<UniformVariant> parseVec(
        const std::string& type,
        const std::string& name,
        const ParamMap& params
    );
};

} // namespace Uniforms

