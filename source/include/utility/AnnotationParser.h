/**
 * @file AnnotationParser.h
 * @brief Parser for shader annotation syntax.
 *
 * Parses tokenized annotations into structured parameter maps.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include "utility/AnnotationLexer.h"

namespace Uniforms {

/**
 * @brief Represents a parsed annotation value.
 */
using ParamValue = std::variant<
    double,                     // Single number
    std::string,                // String or identifier
    std::vector<double>,        // Array of numbers (e.g., RGB values)
    std::vector<std::string>    // Array of strings (e.g., options)
>;

/**
 * @brief Map of parameter names to their values.
 */
using ParamMap = std::unordered_map<std::string, ParamValue>;

/**
 * @brief Parser for annotation parameter lists.
 * 
 * Grammar:
 *   params     := param (',' param)*
 *   param      := IDENT '=' value
 *   value      := number | string | array | hex_color
 *   array      := number (',' number)*  // No spaces between elements
 *   hex_color  := '#' HEX_DIGITS
 */
class AnnotationParser {
public:
    /**
     * @brief Parse annotation parameters from token stream.
     * @param tokens Token stream from lexer
     * @return Map of parameter names to values
     */
    static ParamMap parse(const std::vector<Token>& tokens);
    
    /**
     * @brief Parse annotation parameters from raw string.
     * @param input Raw parameter string
     * @return Map of parameter names to values
     */
    static ParamMap parse(const std::string& input);
    
    // Helper getters for extracting typed values
    static double getNumber(const ParamMap& params, const std::string& key, double defaultVal);
    static std::string getString(const ParamMap& params, const std::string& key, const std::string& defaultVal);
    static std::vector<double> getNumberArray(const ParamMap& params, const std::string& key);
    static std::vector<std::string> getStringArray(const ParamMap& params, const std::string& key);
    static bool getBool(const ParamMap& params, const std::string& key, bool defaultVal);

private:
    size_t pos_ = 0;
    std::vector<Token> tokens_;
    
    AnnotationParser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}
    
    ParamMap parseParams();
    ParamValue parseValue();
    ParamValue parseArray();
    ParamValue parseHexColor();
    
    const Token& current() const;
    const Token& peek(size_t offset = 1) const;
    bool match(TokenType type);
    bool check(TokenType type) const;
    void advance();
    void expect(TokenType type, const std::string& message);
};

} // namespace Uniforms

