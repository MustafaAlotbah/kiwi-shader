/**
 * @file AnnotationParser.cpp
 * @brief Implementation of annotation parser.
 */

#include "utility/AnnotationParser.h"
#include "utility/Logger.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cstdio>

namespace Uniforms {

//------------------------------------------------------------------------------
// Token navigation
//------------------------------------------------------------------------------
const Token& AnnotationParser::current() const {
    if (pos_ >= tokens_.size()) {
        return tokens_.back();  // EndOfInput token
    }
    return tokens_[pos_];
}

const Token& AnnotationParser::peek(size_t offset) const {
    size_t index = pos_ + offset;
    if (index >= tokens_.size()) {
        return tokens_.back();
    }
    return tokens_[index];
}

bool AnnotationParser::check(TokenType type) const {
    return current().type == type;
}

bool AnnotationParser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

void AnnotationParser::advance() {
    if (pos_ < tokens_.size()) {
        pos_++;
    }
}

void AnnotationParser::expect(TokenType type, const std::string& message) {
    if (!match(type)) {
        Logger::Error("AnnotationParser", "Parser error: " + message + " (got '" + current().value + "')", {"shader", "parser"});
        throw std::runtime_error(message);
    }
}

//------------------------------------------------------------------------------
// Parsing methods
//------------------------------------------------------------------------------
ParamMap AnnotationParser::parseParams() {
    ParamMap params;
    
    // Empty parameters
    if (check(TokenType::EndOfInput) || check(TokenType::ParenClose)) {
        return params;
    }
    
    // Parse first parameter
    while (!check(TokenType::EndOfInput) && !check(TokenType::ParenClose)) {
        // Expect: identifier = value
        if (!check(TokenType::Identifier)) {
            Logger::Warn("AnnotationParser", "Expected parameter name, got: " + current().value, {"shader", "parser"});
            advance();
            continue;
        }
        
        std::string key = current().value;
        advance();
        
        expect(TokenType::Equals, "Expected '=' after parameter name");
        
        ParamValue value = parseValue();
        params[key] = value;
        
        // Optional comma separator
        if (match(TokenType::Comma)) {
            // Continue to next parameter
            continue;
        }
        
        // No comma, must be end
        break;
    }
    
    return params;
}

ParamValue AnnotationParser::parseValue() {
    // Hex color: #RRGGBB or #RRGGBBAA
    if (check(TokenType::Hash)) {
        return parseHexColor();
    }
    
    // String literal
    if (check(TokenType::String)) {
        std::string value = current().value;
        advance();
        return value;
    }
    
    // Number or number array
    if (check(TokenType::Number)) {
        // Check if it's an array (numbers separated by commas with no spaces)
        // Peek ahead to see if there's a comma immediately followed by a number
        if (peek().type == TokenType::Comma && peek(2).type == TokenType::Number) {
            return parseArray();
        }
        
        // Single number
        double value = std::stod(current().value);
        advance();
        return value;
    }
    
    // Identifier (like "true", "false", or enum values)
    if (check(TokenType::Identifier)) {
        std::string value = current().value;
        advance();
        return value;
    }
    
    // Array in brackets
    if (check(TokenType::BracketOpen)) {
        return parseArray();
    }
    
    Logger::Warn("AnnotationParser", "Unexpected token in value: " + current().value, {"shader", "parser"});
    advance();
    return std::string("");
}

ParamValue AnnotationParser::parseArray() {
    std::vector<double> numbers;
    std::vector<std::string> strings;
    bool isNumericArray = true;
    
    // Check if we start with a bracket
    bool hasBrackets = match(TokenType::BracketOpen);
    
    while (!check(TokenType::EndOfInput) && !check(TokenType::BracketClose)) {
        if (check(TokenType::Number)) {
            numbers.push_back(std::stod(current().value));
            advance();
        }
        else if (check(TokenType::String) || check(TokenType::Identifier)) {
            isNumericArray = false;
            strings.push_back(current().value);
            advance();
        }
        else {
            break;
        }
        
        // Comma separator
        if (match(TokenType::Comma)) {
            continue;
        }
        
        // No comma, end of array
        break;
    }
    
    if (hasBrackets) {
        expect(TokenType::BracketClose, "Expected ']' after array");
    }
    
    if (isNumericArray && !numbers.empty()) {
        return numbers;
    }
    if (!strings.empty()) {
        return strings;
    }
    
    return std::vector<double>{};
}

ParamValue AnnotationParser::parseHexColor() {
    expect(TokenType::Hash, "Expected '#' for hex color");
    
    if (!check(TokenType::Identifier) && !check(TokenType::Number)) {
        Logger::Error("AnnotationParser", "Expected hex digits after '#'", {"shader", "parser"});
        return std::vector<double>{1.0, 1.0, 1.0};
    }
    
    std::string hex = current().value;
    advance();
    
    // Parse hex color
    unsigned int r = 0, g = 0, b = 0, a = 255;
    
    if (hex.length() == 6) {
        sscanf(hex.c_str(), "%02x%02x%02x", &r, &g, &b);
    }
    else if (hex.length() == 8) {
        sscanf(hex.c_str(), "%02x%02x%02x%02x", &r, &g, &b, &a);
    }
    else {
        Logger::Warn("AnnotationParser", "Invalid hex color format: #" + hex, {"shader", "parser"});
    }
    
    return std::vector<double>{r / 255.0, g / 255.0, b / 255.0, a / 255.0};
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------
ParamMap AnnotationParser::parse(const std::vector<Token>& tokens) {
    AnnotationParser parser(tokens);
    try {
        return parser.parseParams();
    } catch (const std::exception& e) {
        Logger::Error("AnnotationParser", "Parse error: " + std::string(e.what()), {"shader", "parser"});
        return {};
    }
}

ParamMap AnnotationParser::parse(const std::string& input) {
    auto tokens = AnnotationLexer::tokenize(input);
    return parse(tokens);
}

//------------------------------------------------------------------------------
// Helper getters
//------------------------------------------------------------------------------
double AnnotationParser::getNumber(const ParamMap& params, const std::string& key, double defaultVal) {
    auto it = params.find(key);
    if (it == params.end()) return defaultVal;
    
    if (std::holds_alternative<double>(it->second)) {
        return std::get<double>(it->second);
    }
    
    if (std::holds_alternative<std::string>(it->second)) {
        try {
            return std::stod(std::get<std::string>(it->second));
        } catch (...) {
            return defaultVal;
        }
    }
    
    return defaultVal;
}

std::string AnnotationParser::getString(const ParamMap& params, const std::string& key, const std::string& defaultVal) {
    auto it = params.find(key);
    if (it == params.end()) return defaultVal;
    
    if (std::holds_alternative<std::string>(it->second)) {
        return std::get<std::string>(it->second);
    }
    
    if (std::holds_alternative<double>(it->second)) {
        return std::to_string(std::get<double>(it->second));
    }
    
    return defaultVal;
}

std::vector<double> AnnotationParser::getNumberArray(const ParamMap& params, const std::string& key) {
    auto it = params.find(key);
    if (it == params.end()) return {};
    
    if (std::holds_alternative<std::vector<double>>(it->second)) {
        return std::get<std::vector<double>>(it->second);
    }
    
    // Single number -> array of 1
    if (std::holds_alternative<double>(it->second)) {
        return {std::get<double>(it->second)};
    }
    
    return {};
}

std::vector<std::string> AnnotationParser::getStringArray(const ParamMap& params, const std::string& key) {
    auto it = params.find(key);
    if (it == params.end()) return {};
    
    if (std::holds_alternative<std::vector<std::string>>(it->second)) {
        return std::get<std::vector<std::string>>(it->second);
    }
    
    // Single string -> array of 1
    if (std::holds_alternative<std::string>(it->second)) {
        return {std::get<std::string>(it->second)};
    }
    
    return {};
}

bool AnnotationParser::getBool(const ParamMap& params, const std::string& key, bool defaultVal) {
    auto str = getString(params, key, "");
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    
    if (str == "true" || str == "1" || str == "yes") return true;
    if (str == "false" || str == "0" || str == "no") return false;
    
    return defaultVal;
}

} // namespace Uniforms

