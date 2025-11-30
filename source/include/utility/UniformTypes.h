/**
 * @file UniformTypes.h
 * @brief Data classes representing shader uniforms with metadata.
 *
 * These classes store information about shader uniforms parsed from
 * annotation comments, including type, constraints, and current values.
 */

#pragma once

#include <string>
#include <variant>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace Uniforms {

/**
 * @brief Enum representing the type of uniform control to render.
 */
enum class ControlType {
    Slider,         // Float/Int slider
    DragFloat,      // Float drag (no hard limits)
    DragInt,        // Int drag
    ColorPicker,    // RGB/RGBA color picker
    Checkbox,       // Boolean toggle
    Vec2,           // 2D vector
    Vec3,           // 3D vector
    Vec4,           // 4D vector
    Dropdown,       // Enum/selection
    None            // Unknown/unsupported
};

/**
 * @brief Base information common to all uniform types.
 */
struct UniformBase {
    std::string name;           // Uniform variable name (e.g., "uSpeed")
    std::string displayName;    // Human-readable name (e.g., "Speed")
    std::string tooltip;        // Optional tooltip text
    std::string group;          // Group name for UI organization (empty = ungrouped)
    ControlType controlType = ControlType::None;
    int location = -1;          // OpenGL uniform location (set at runtime)
    
    virtual ~UniformBase() = default;
};

/**
 * @brief Float uniform with slider control.
 * 
 * Annotation: // @slider(min=0.0, max=1.0, default=0.5)
 */
struct FloatUniform : public UniformBase {
    float value = 0.0f;
    float defaultValue = 0.0f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float step = 0.01f;         // Drag speed / step size
    
    FloatUniform() { controlType = ControlType::Slider; }
};

/**
 * @brief Integer uniform with slider control.
 * 
 * Annotation: // @slider(min=0, max=100, default=50)
 */
struct IntUniform : public UniformBase {
    int value = 0;
    int defaultValue = 0;
    int minValue = 0;
    int maxValue = 100;
    
    IntUniform() { controlType = ControlType::Slider; }
};

/**
 * @brief Boolean uniform with checkbox control.
 * 
 * Annotation: // @checkbox(default=true)
 */
struct BoolUniform : public UniformBase {
    bool value = false;
    bool defaultValue = false;
    
    BoolUniform() { controlType = ControlType::Checkbox; }
};

/**
 * @brief Vec2 uniform with drag controls.
 * 
 * Annotation: // @vec2(default=0.5,0.5)
 */
struct Vec2Uniform : public UniformBase {
    glm::vec2 value{0.0f};
    glm::vec2 defaultValue{0.0f};
    float minValue = -FLT_MAX;
    float maxValue = FLT_MAX;
    float step = 0.01f;
    
    Vec2Uniform() { controlType = ControlType::Vec2; }
};

/**
 * @brief Vec3 uniform with drag controls.
 * 
 * Annotation: // @vec3(default=1.0,0.0,0.0)
 */
struct Vec3Uniform : public UniformBase {
    glm::vec3 value{0.0f};
    glm::vec3 defaultValue{0.0f};
    float minValue = -FLT_MAX;
    float maxValue = FLT_MAX;
    float step = 0.01f;
    
    Vec3Uniform() { controlType = ControlType::Vec3; }
};

/**
 * @brief Vec4 uniform with drag controls.
 * 
 * Annotation: // @vec4(default=1.0,1.0,1.0,1.0)
 */
struct Vec4Uniform : public UniformBase {
    glm::vec4 value{0.0f};
    glm::vec4 defaultValue{0.0f};
    float minValue = -FLT_MAX;
    float maxValue = FLT_MAX;
    float step = 0.01f;
    
    Vec4Uniform() { controlType = ControlType::Vec4; }
};

/**
 * @brief Color uniform (vec3 or vec4) with color picker.
 * 
 * Annotation: // @color(default=1.0,0.5,0.0) or // @color(default=#FF8800)
 */
struct ColorUniform : public UniformBase {
    glm::vec4 value{1.0f};      // Always stored as vec4 (alpha = 1 if RGB)
    glm::vec4 defaultValue{1.0f};
    bool hasAlpha = false;      // true for vec4 colors
    
    ColorUniform() { controlType = ControlType::ColorPicker; }
};

/**
 * @brief Dropdown/enum uniform for integer selection.
 * 
 * Annotation: // @dropdown(options=["Low","Medium","High"], default=1)
 */
struct DropdownUniform : public UniformBase {
    int value = 0;
    int defaultValue = 0;
    std::vector<std::string> options;
    
    DropdownUniform() { controlType = ControlType::Dropdown; }
};

/**
 * @brief Type alias for a uniform that can be any of the supported types.
 */
using UniformVariant = std::variant<
    FloatUniform,
    IntUniform,
    BoolUniform,
    Vec2Uniform,
    Vec3Uniform,
    Vec4Uniform,
    ColorUniform,
    DropdownUniform
>;

/**
 * @brief Container for all parsed uniforms from a shader.
 */
struct UniformCollection {
    std::vector<UniformVariant> uniforms;
    
    void clear() { uniforms.clear(); }
    bool empty() const { return uniforms.empty(); }
    size_t size() const { return uniforms.size(); }
};

} // namespace Uniforms

