/**
 * @file UniformEditor.h
 * @brief Service for rendering ImGui controls for shader uniforms.
 *
 * Takes parsed uniform metadata and generates appropriate ImGui widgets
 * for live editing. Also handles binding values to shader programs.
 */

#pragma once

#include <glad/glad.h>
#include <imgui.h>
#include "utility/UniformTypes.h"

namespace Uniforms {

/**
 * @brief Editor service that renders ImGui controls and binds uniform values.
 */
class UniformEditor {
public:
    /**
     * @brief Render ImGui controls for all uniforms in the collection.
     * @param collection The collection of uniforms to render.
     * @return true if any value was changed.
     */
    static bool renderControls(UniformCollection& collection);

    /**
     * @brief Bind all uniform values to the currently active shader program.
     * @param collection The collection of uniforms to bind.
     * @param programId The OpenGL shader program ID.
     */
    static void bindUniforms(UniformCollection& collection, unsigned int programId);

    /**
     * @brief Reset all uniforms to their default values.
     * @param collection The collection of uniforms to reset.
     */
    static void resetToDefaults(UniformCollection& collection);

    /**
     * @brief Update uniform locations for a new shader program.
     * @param collection The collection of uniforms.
     * @param programId The OpenGL shader program ID.
     */
    static void updateLocations(UniformCollection& collection, unsigned int programId);

private:
    // Individual control renderers - return true if value changed
    static bool renderFloat(FloatUniform& u);
    static bool renderInt(IntUniform& u);
    static bool renderBool(BoolUniform& u);
    static bool renderVec2(Vec2Uniform& u);
    static bool renderVec3(Vec3Uniform& u);
    static bool renderVec4(Vec4Uniform& u);
    static bool renderColor(ColorUniform& u);
    static bool renderDropdown(DropdownUniform& u);

    // Uniform binding helpers
    static void bindFloat(const FloatUniform& u);
    static void bindInt(const IntUniform& u);
    static void bindBool(const BoolUniform& u);
    static void bindVec2(const Vec2Uniform& u);
    static void bindVec3(const Vec3Uniform& u);
    static void bindVec4(const Vec4Uniform& u);
    static void bindColor(const ColorUniform& u);
    static void bindDropdown(const DropdownUniform& u);
};

} // namespace Uniforms

