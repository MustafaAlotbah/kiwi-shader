/**
 * @file UniformEditor.cpp
 * @brief Implementation of ImGui uniform editor.
 */

#include "utility/UniformEditor.h"
#include <format>
#include <map>
#include <vector>

namespace Uniforms {

//------------------------------------------------------------------------------
// Main render function with group support
//------------------------------------------------------------------------------
bool UniformEditor::renderControls(UniformCollection& collection) {
    bool anyChanged = false;
    
    // Helper lambda to render a single uniform (has access to private members)
    auto renderUniform = [](UniformVariant& uniform) -> bool {
        return std::visit([](auto& u) -> bool {
            using T = std::decay_t<decltype(u)>;
            
            if constexpr (std::is_same_v<T, FloatUniform>) {
                return renderFloat(u);
            }
            else if constexpr (std::is_same_v<T, IntUniform>) {
                return renderInt(u);
            }
            else if constexpr (std::is_same_v<T, BoolUniform>) {
                return renderBool(u);
            }
            else if constexpr (std::is_same_v<T, Vec2Uniform>) {
                return renderVec2(u);
            }
            else if constexpr (std::is_same_v<T, Vec3Uniform>) {
                return renderVec3(u);
            }
            else if constexpr (std::is_same_v<T, Vec4Uniform>) {
                return renderVec4(u);
            }
            else if constexpr (std::is_same_v<T, ColorUniform>) {
                return renderColor(u);
            }
            else if constexpr (std::is_same_v<T, DropdownUniform>) {
                return renderDropdown(u);
            }
            return false;
        }, uniform);
    };
    
    // Organize uniforms by group (preserving order)
    std::vector<std::string> groupOrder; // Track order of groups
    std::map<std::string, std::vector<size_t>> groupedIndices; // Group name -> uniform indices
    
    for (size_t i = 0; i < collection.uniforms.size(); ++i) {
        std::string groupName;
        std::visit([&groupName](const auto& u) {
            groupName = u.group;
        }, collection.uniforms[i]);
        
        // Track group order (first occurrence)
        if (groupedIndices.find(groupName) == groupedIndices.end()) {
            groupOrder.push_back(groupName);
        }
        groupedIndices[groupName].push_back(i);
    }
    
    // Render groups in order
    for (const auto& groupName : groupOrder) {
        const auto& indices = groupedIndices[groupName];
        
        if (groupName.empty()) {
            // Ungrouped uniforms - render directly
            for (size_t idx : indices) {
                anyChanged |= renderUniform(collection.uniforms[idx]);
            }
        } else {
            // Grouped uniforms - render in collapsible header
            if (ImGui::CollapsingHeader(groupName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Indent(10.0f);
                for (size_t idx : indices) {
                    anyChanged |= renderUniform(collection.uniforms[idx]);
                }
                ImGui::Unindent(10.0f);
            }
        }
    }
    
    return anyChanged;
}

//------------------------------------------------------------------------------
// Bind all uniforms to shader
//------------------------------------------------------------------------------
void UniformEditor::bindUniforms(UniformCollection& collection, unsigned int programId) {
    glUseProgram(programId);
    
    for (auto& uniform : collection.uniforms) {
        std::visit([](auto& u) {
            using T = std::decay_t<decltype(u)>;
            
            if (u.location == -1) return;  // Skip if no valid location
            
            if constexpr (std::is_same_v<T, FloatUniform>) {
                bindFloat(u);
            }
            else if constexpr (std::is_same_v<T, IntUniform>) {
                bindInt(u);
            }
            else if constexpr (std::is_same_v<T, BoolUniform>) {
                bindBool(u);
            }
            else if constexpr (std::is_same_v<T, Vec2Uniform>) {
                bindVec2(u);
            }
            else if constexpr (std::is_same_v<T, Vec3Uniform>) {
                bindVec3(u);
            }
            else if constexpr (std::is_same_v<T, Vec4Uniform>) {
                bindVec4(u);
            }
            else if constexpr (std::is_same_v<T, ColorUniform>) {
                bindColor(u);
            }
            else if constexpr (std::is_same_v<T, DropdownUniform>) {
                bindDropdown(u);
            }
        }, uniform);
    }
}

//------------------------------------------------------------------------------
// Reset all to defaults
//------------------------------------------------------------------------------
void UniformEditor::resetToDefaults(UniformCollection& collection) {
    for (auto& uniform : collection.uniforms) {
        std::visit([](auto& u) {
            u.value = u.defaultValue;
        }, uniform);
    }
}

//------------------------------------------------------------------------------
// Update uniform locations from shader program
//------------------------------------------------------------------------------
void UniformEditor::updateLocations(UniformCollection& collection, unsigned int programId) {
    for (auto& uniform : collection.uniforms) {
        std::visit([programId](auto& u) {
            u.location = glGetUniformLocation(programId, u.name.c_str());
        }, uniform);
    }
}

//------------------------------------------------------------------------------
// Individual control renderers
//------------------------------------------------------------------------------
bool UniformEditor::renderFloat(FloatUniform& u) {
    ImGui::PushID(u.name.c_str());
    
    bool changed = false;
    
    // Slider with min/max
    changed = ImGui::SliderFloat(
        u.displayName.c_str(),
        &u.value,
        u.minValue,
        u.maxValue,
        "%.3f"
    );
    
    // Context menu for reset
    if (ImGui::BeginPopupContextItem("float_context")) {
        if (ImGui::MenuItem("Reset to Default")) {
            u.value = u.defaultValue;
            changed = true;
        }
        ImGui::Text("Range: [%.2f, %.2f]", u.minValue, u.maxValue);
        ImGui::Text("Default: %.3f", u.defaultValue);
        ImGui::EndPopup();
    }
    
    ImGui::PopID();
    return changed;
}

bool UniformEditor::renderInt(IntUniform& u) {
    ImGui::PushID(u.name.c_str());
    
    bool changed = ImGui::SliderInt(
        u.displayName.c_str(),
        &u.value,
        u.minValue,
        u.maxValue
    );
    
    if (ImGui::BeginPopupContextItem("int_context")) {
        if (ImGui::MenuItem("Reset to Default")) {
            u.value = u.defaultValue;
            changed = true;
        }
        ImGui::EndPopup();
    }
    
    ImGui::PopID();
    return changed;
}

bool UniformEditor::renderBool(BoolUniform& u) {
    ImGui::PushID(u.name.c_str());
    
    bool changed = ImGui::Checkbox(u.displayName.c_str(), &u.value);
    
    if (ImGui::BeginPopupContextItem("bool_context")) {
        if (ImGui::MenuItem("Reset to Default")) {
            u.value = u.defaultValue;
            changed = true;
        }
        ImGui::EndPopup();
    }
    
    ImGui::PopID();
    return changed;
}

bool UniformEditor::renderVec2(Vec2Uniform& u) {
    ImGui::PushID(u.name.c_str());
    
    bool changed = ImGui::DragFloat2(
        u.displayName.c_str(),
        &u.value[0],
        u.step,
        u.minValue,
        u.maxValue,
        "%.3f"
    );
    
    if (ImGui::BeginPopupContextItem("vec2_context")) {
        if (ImGui::MenuItem("Reset to Default")) {
            u.value = u.defaultValue;
            changed = true;
        }
        ImGui::EndPopup();
    }
    
    ImGui::PopID();
    return changed;
}

bool UniformEditor::renderVec3(Vec3Uniform& u) {
    ImGui::PushID(u.name.c_str());
    
    bool changed = ImGui::DragFloat3(
        u.displayName.c_str(),
        &u.value[0],
        u.step,
        u.minValue,
        u.maxValue,
        "%.3f"
    );
    
    if (ImGui::BeginPopupContextItem("vec3_context")) {
        if (ImGui::MenuItem("Reset to Default")) {
            u.value = u.defaultValue;
            changed = true;
        }
        ImGui::EndPopup();
    }
    
    ImGui::PopID();
    return changed;
}

bool UniformEditor::renderVec4(Vec4Uniform& u) {
    ImGui::PushID(u.name.c_str());
    
    bool changed = ImGui::DragFloat4(
        u.displayName.c_str(),
        &u.value[0],
        u.step,
        u.minValue,
        u.maxValue,
        "%.3f"
    );
    
    if (ImGui::BeginPopupContextItem("vec4_context")) {
        if (ImGui::MenuItem("Reset to Default")) {
            u.value = u.defaultValue;
            changed = true;
        }
        ImGui::EndPopup();
    }
    
    ImGui::PopID();
    return changed;
}

bool UniformEditor::renderColor(ColorUniform& u) {
    ImGui::PushID(u.name.c_str());
    
    bool changed;
    if (u.hasAlpha) {
        changed = ImGui::ColorEdit4(
            u.displayName.c_str(),
            &u.value[0],
            ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf
        );
    } else {
        changed = ImGui::ColorEdit3(
            u.displayName.c_str(),
            &u.value[0]
        );
    }
    
    if (ImGui::BeginPopupContextItem("color_context")) {
        if (ImGui::MenuItem("Reset to Default")) {
            u.value = u.defaultValue;
            changed = true;
        }
        ImGui::EndPopup();
    }
    
    ImGui::PopID();
    return changed;
}

bool UniformEditor::renderDropdown(DropdownUniform& u) {
    ImGui::PushID(u.name.c_str());
    
    // Build combo items
    std::vector<const char*> items;
    for (const auto& opt : u.options) {
        items.push_back(opt.c_str());
    }
    
    bool changed = ImGui::Combo(
        u.displayName.c_str(),
        &u.value,
        items.data(),
        static_cast<int>(items.size())
    );
    
    if (ImGui::BeginPopupContextItem("dropdown_context")) {
        if (ImGui::MenuItem("Reset to Default")) {
            u.value = u.defaultValue;
            changed = true;
        }
        ImGui::EndPopup();
    }
    
    ImGui::PopID();
    return changed;
}

//------------------------------------------------------------------------------
// Uniform binding helpers
//------------------------------------------------------------------------------
void UniformEditor::bindFloat(const FloatUniform& u) {
    glUniform1f(u.location, u.value);
}

void UniformEditor::bindInt(const IntUniform& u) {
    glUniform1i(u.location, u.value);
}

void UniformEditor::bindBool(const BoolUniform& u) {
    glUniform1i(u.location, u.value ? 1 : 0);
}

void UniformEditor::bindVec2(const Vec2Uniform& u) {
    glUniform2f(u.location, u.value.x, u.value.y);
}

void UniformEditor::bindVec3(const Vec3Uniform& u) {
    glUniform3f(u.location, u.value.x, u.value.y, u.value.z);
}

void UniformEditor::bindVec4(const Vec4Uniform& u) {
    glUniform4f(u.location, u.value.x, u.value.y, u.value.z, u.value.w);
}

void UniformEditor::bindColor(const ColorUniform& u) {
    if (u.hasAlpha) {
        glUniform4f(u.location, u.value.x, u.value.y, u.value.z, u.value.w);
    } else {
        glUniform3f(u.location, u.value.x, u.value.y, u.value.z);
    }
}

void UniformEditor::bindDropdown(const DropdownUniform& u) {
    glUniform1i(u.location, u.value);
}

} // namespace Uniforms

