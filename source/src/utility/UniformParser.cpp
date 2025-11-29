/**
 * @file UniformParser.cpp
 * @brief Implementation of shader annotation parser.
 */

#include "utility/UniformParser.h"
#include "utility/AnnotationParser.h"
#include "utility/Logger.h"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace Uniforms {

//------------------------------------------------------------------------------
// Helper: Trim whitespace
//------------------------------------------------------------------------------
static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

//------------------------------------------------------------------------------
// Main parse function
//------------------------------------------------------------------------------
UniformCollection UniformParser::parse(const std::string& shaderSource) {
    UniformCollection collection;
    
    // Regex to find annotation comments followed by uniform declarations
    // Matches: // @annotation(...)\n uniform type name;
    std::regex annotationRegex(
        R"(//\s*@(\w+)\s*\(([^)]*)\)\s*\n\s*uniform\s+(\w+)\s+(\w+)\s*;)",
        std::regex::ECMAScript
    );
    
    auto begin = std::sregex_iterator(shaderSource.begin(), shaderSource.end(), annotationRegex);
    auto end = std::sregex_iterator();
    
    for (auto it = begin; it != end; ++it) {
        std::smatch match = *it;
        
        std::string annotationType = match[1].str();  // e.g., "slider"
        std::string params = match[2].str();          // e.g., "min=0.0, max=1.0"
        std::string uniformType = match[3].str();     // e.g., "float"
        std::string uniformName = match[4].str();     // e.g., "uSpeed"
        
        // Use the professional parser
        auto parsedParams = AnnotationParser::parse(params);
        std::optional<UniformVariant> uniform;
        
        // Route to appropriate parser based on annotation type
        if (annotationType == "slider") {
            uniform = parseSlider(uniformType, uniformName, parsedParams);
        }
        else if (annotationType == "color") {
            uniform = parseColor(uniformType, uniformName, parsedParams);
        }
        else if (annotationType == "checkbox") {
            uniform = parseCheckbox(uniformType, uniformName, parsedParams);
        }
        else if (annotationType == "vec2" || annotationType == "vec3" || annotationType == "vec4") {
            uniform = parseVec(uniformType, uniformName, parsedParams);
        }
        else {
            Logger::Warn("UniformParser", "Unknown annotation type: @" + annotationType, {"shader", "annotation"});
        }
        
        if (uniform.has_value()) {
            collection.uniforms.push_back(uniform.value());
        }
    }
    
    if (!collection.empty()) {
        Logger::Info("UniformParser", "Parsed " + std::to_string(collection.size()) + " annotated uniform(s)", {"shader", "annotation"});
    }
    return collection;
}

//------------------------------------------------------------------------------
// Type and name extraction
//------------------------------------------------------------------------------
std::string UniformParser::extractType(const std::string& decl) {
    std::regex typeRegex(R"(uniform\s+(\w+))");
    std::smatch match;
    if (std::regex_search(decl, match, typeRegex)) {
        return match[1].str();
    }
    return "";
}

std::string UniformParser::extractName(const std::string& decl) {
    std::regex nameRegex(R"(uniform\s+\w+\s+(\w+))");
    std::smatch match;
    if (std::regex_search(decl, match, nameRegex)) {
        return match[1].str();
    }
    return "";
}

//------------------------------------------------------------------------------
// Convert variable name to display name
// "uMySpeed" -> "My Speed", "speed_factor" -> "Speed Factor"
//------------------------------------------------------------------------------
std::string UniformParser::toDisplayName(const std::string& name) {
    std::string result;
    
    // Skip common prefixes
    size_t start = 0;
    if (name.length() > 1 && (name[0] == 'u' || name[0] == '_') && std::isupper(name[1])) {
        start = 1;
    }
    
    for (size_t i = start; i < name.length(); ++i) {
        char c = name[i];
        
        if (c == '_') {
            result += ' ';
        }
        else if (i > start && std::isupper(c) && !std::isupper(name[i-1])) {
            // CamelCase: insert space before uppercase
            result += ' ';
            result += c;
        }
        else if (i == start) {
            result += std::toupper(c);
        }
        else {
            result += c;
        }
    }
    
    return result;
}

//------------------------------------------------------------------------------
// Value parsers
//------------------------------------------------------------------------------
float UniformParser::parseFloat(const std::string& str, float defaultVal) {
    try {
        return std::stof(trim(str));
    } catch (...) {
        return defaultVal;
    }
}

int UniformParser::parseInt(const std::string& str, int defaultVal) {
    try {
        return std::stoi(trim(str));
    } catch (...) {
        return defaultVal;
    }
}

bool UniformParser::parseBool(const std::string& str, bool defaultVal) {
    std::string s = trim(str);
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "true" || s == "1" || s == "yes") return true;
    if (s == "false" || s == "0" || s == "no") return false;
    return defaultVal;
}

glm::vec2 UniformParser::parseVec2(const std::string& str, glm::vec2 defaultVal) {
    std::vector<float> values;
    std::stringstream ss(str);
    std::string item;
    
    while (std::getline(ss, item, ',')) {
        values.push_back(parseFloat(item, 0.0f));
    }
    
    if (values.size() >= 2) {
        return glm::vec2(values[0], values[1]);
    }
    return defaultVal;
}

glm::vec3 UniformParser::parseVec3(const std::string& str, glm::vec3 defaultVal) {
    std::vector<float> values;
    std::stringstream ss(str);
    std::string item;
    
    while (std::getline(ss, item, ',')) {
        values.push_back(parseFloat(item, 0.0f));
    }
    
    if (values.size() >= 3) {
        return glm::vec3(values[0], values[1], values[2]);
    }
    return defaultVal;
}

glm::vec4 UniformParser::parseVec4(const std::string& str, glm::vec4 defaultVal) {
    std::vector<float> values;
    std::stringstream ss(str);
    std::string item;
    
    while (std::getline(ss, item, ',')) {
        values.push_back(parseFloat(item, 0.0f));
    }
    
    if (values.size() >= 4) {
        return glm::vec4(values[0], values[1], values[2], values[3]);
    }
    if (values.size() >= 3) {
        return glm::vec4(values[0], values[1], values[2], 1.0f);
    }
    return defaultVal;
}

glm::vec4 UniformParser::parseHexColor(const std::string& str) {
    std::string hex = trim(str);
    
    // Remove # prefix if present
    if (!hex.empty() && hex[0] == '#') {
        hex = hex.substr(1);
    }
    
    // Parse hex values
    unsigned int r = 0, g = 0, b = 0, a = 255;
    
    if (hex.length() == 6) {
        sscanf(hex.c_str(), "%02x%02x%02x", &r, &g, &b);
    }
    else if (hex.length() == 8) {
        sscanf(hex.c_str(), "%02x%02x%02x%02x", &r, &g, &b, &a);
    }
    
    return glm::vec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

//------------------------------------------------------------------------------
// Annotation type handlers
//------------------------------------------------------------------------------
std::optional<UniformVariant> UniformParser::parseSlider(
    const std::string& type,
    const std::string& name,
    const ParamMap& params
) {
    if (type == "float") {
        FloatUniform u;
        u.name = name;
        u.displayName = toDisplayName(name);
        u.controlType = ControlType::Slider;
        
        u.minValue = static_cast<float>(AnnotationParser::getNumber(params, "min", 0.0));
        u.maxValue = static_cast<float>(AnnotationParser::getNumber(params, "max", 1.0));
        u.defaultValue = static_cast<float>(AnnotationParser::getNumber(params, "default", 0.0));
        u.value = u.defaultValue;
        u.step = static_cast<float>(AnnotationParser::getNumber(params, "step", 0.01));
        
        return u;
    }
    else if (type == "int") {
        IntUniform u;
        u.name = name;
        u.displayName = toDisplayName(name);
        u.controlType = ControlType::Slider;
        
        u.minValue = static_cast<int>(AnnotationParser::getNumber(params, "min", 0.0));
        u.maxValue = static_cast<int>(AnnotationParser::getNumber(params, "max", 100.0));
        u.defaultValue = static_cast<int>(AnnotationParser::getNumber(params, "default", 0.0));
        u.value = u.defaultValue;
        
        return u;
    }
    
    Logger::Warn("UniformParser", "@slider not supported for type: " + type, {"shader", "annotation"});
    return std::nullopt;
}

std::optional<UniformVariant> UniformParser::parseColor(
    const std::string& type,
    const std::string& name,
    const ParamMap& params
) {
    if (type != "vec3" && type != "vec4") {
        Logger::Warn("UniformParser", "@color requires vec3 or vec4 type, got: " + type, {"shader", "annotation"});
        return std::nullopt;
    }
    
    ColorUniform u;
    u.name = name;
    u.displayName = toDisplayName(name);
    u.hasAlpha = (type == "vec4");
    
    // Get default color value (can be array of numbers)
    auto defaultArr = AnnotationParser::getNumberArray(params, "default");
    if (!defaultArr.empty()) {
        if (defaultArr.size() >= 4) {
            u.defaultValue = glm::vec4(defaultArr[0], defaultArr[1], defaultArr[2], defaultArr[3]);
        } else if (defaultArr.size() >= 3) {
            u.defaultValue = glm::vec4(defaultArr[0], defaultArr[1], defaultArr[2], 1.0f);
        } else {
            u.defaultValue = glm::vec4(1.0f);
        }
        u.value = u.defaultValue;
    }
    
    return u;
}

std::optional<UniformVariant> UniformParser::parseCheckbox(
    const std::string& type,
    const std::string& name,
    const ParamMap& params
) {
    // Note: GLSL doesn't have bool uniforms, we use int (0 or 1)
    if (type != "int" && type != "bool") {
        Logger::Warn("UniformParser", "@checkbox requires int or bool type, got: " + type, {"shader", "annotation"});
        return std::nullopt;
    }
    
    BoolUniform u;
    u.name = name;
    u.displayName = toDisplayName(name);
    u.defaultValue = AnnotationParser::getBool(params, "default", false);
    u.value = u.defaultValue;
    
    return u;
}

std::optional<UniformVariant> UniformParser::parseVec(
    const std::string& type,
    const std::string& name,
    const ParamMap& params
) {
    if (type == "vec2") {
        Vec2Uniform u;
        u.name = name;
        u.displayName = toDisplayName(name);
        
        auto defaultArr = AnnotationParser::getNumberArray(params, "default");
        if (defaultArr.size() >= 2) {
            u.defaultValue = glm::vec2(defaultArr[0], defaultArr[1]);
            u.value = u.defaultValue;
        }
        
        u.minValue = static_cast<float>(AnnotationParser::getNumber(params, "min", -FLT_MAX));
        u.maxValue = static_cast<float>(AnnotationParser::getNumber(params, "max", FLT_MAX));
        u.step = static_cast<float>(AnnotationParser::getNumber(params, "step", 0.01));
        
        return u;
    }
    else if (type == "vec3") {
        Vec3Uniform u;
        u.name = name;
        u.displayName = toDisplayName(name);
        
        auto defaultArr = AnnotationParser::getNumberArray(params, "default");
        if (defaultArr.size() >= 3) {
            u.defaultValue = glm::vec3(defaultArr[0], defaultArr[1], defaultArr[2]);
            u.value = u.defaultValue;
        }
        
        u.minValue = static_cast<float>(AnnotationParser::getNumber(params, "min", -FLT_MAX));
        u.maxValue = static_cast<float>(AnnotationParser::getNumber(params, "max", FLT_MAX));
        u.step = static_cast<float>(AnnotationParser::getNumber(params, "step", 0.01));
        
        return u;
    }
    else if (type == "vec4") {
        Vec4Uniform u;
        u.name = name;
        u.displayName = toDisplayName(name);
        
        auto defaultArr = AnnotationParser::getNumberArray(params, "default");
        if (defaultArr.size() >= 4) {
            u.defaultValue = glm::vec4(defaultArr[0], defaultArr[1], defaultArr[2], defaultArr[3]);
            u.value = u.defaultValue;
        } else if (defaultArr.size() >= 3) {
            u.defaultValue = glm::vec4(defaultArr[0], defaultArr[1], defaultArr[2], 1.0f);
            u.value = u.defaultValue;
        }
        
        u.minValue = static_cast<float>(AnnotationParser::getNumber(params, "min", -FLT_MAX));
        u.maxValue = static_cast<float>(AnnotationParser::getNumber(params, "max", FLT_MAX));
        u.step = static_cast<float>(AnnotationParser::getNumber(params, "step", 0.01));
        
        return u;
    }
    
    Logger::Warn("UniformParser", "Unsupported vector type: " + type, {"shader", "annotation"});
    return std::nullopt;
}

} // namespace Uniforms

