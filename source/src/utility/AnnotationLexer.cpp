/**
 * @file AnnotationLexer.cpp
 * @brief Implementation of annotation tokenizer.
 */

#include "utility/AnnotationLexer.h"
#include <cctype>

namespace Uniforms {

//------------------------------------------------------------------------------
// Character classification helpers
//------------------------------------------------------------------------------
bool AnnotationLexer::isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool AnnotationLexer::isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool AnnotationLexer::isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool AnnotationLexer::isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

//------------------------------------------------------------------------------
// Token readers
//------------------------------------------------------------------------------
std::optional<Token> AnnotationLexer::readNumber(const std::string& input, size_t& pos) {
    size_t start = pos;
    std::string value;
    
    // Optional negative sign
    if (input[pos] == '-') {
        value += input[pos++];
    }
    
    // Integer part
    while (pos < input.length() && isDigit(input[pos])) {
        value += input[pos++];
    }
    
    // Decimal part
    if (pos < input.length() && input[pos] == '.') {
        value += input[pos++];
        while (pos < input.length() && isDigit(input[pos])) {
            value += input[pos++];
        }
    }
    
    // Scientific notation (e.g., 1e-5)
    if (pos < input.length() && (input[pos] == 'e' || input[pos] == 'E')) {
        value += input[pos++];
        if (pos < input.length() && (input[pos] == '+' || input[pos] == '-')) {
            value += input[pos++];
        }
        while (pos < input.length() && isDigit(input[pos])) {
            value += input[pos++];
        }
    }
    
    return Token(TokenType::Number, value, start);
}

std::optional<Token> AnnotationLexer::readIdentifier(const std::string& input, size_t& pos) {
    size_t start = pos;
    std::string value;
    
    while (pos < input.length() && isAlphaNumeric(input[pos])) {
        value += input[pos++];
    }
    
    return Token(TokenType::Identifier, value, start);
}

std::optional<Token> AnnotationLexer::readString(const std::string& input, size_t& pos) {
    size_t start = pos;
    char quote = input[pos++];  // " or '
    std::string value;
    
    while (pos < input.length() && input[pos] != quote) {
        if (input[pos] == '\\' && pos + 1 < input.length()) {
            // Escape sequence
            pos++;
            switch (input[pos]) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                case '\'': value += '\''; break;
                default: value += input[pos];
            }
            pos++;
        } else {
            value += input[pos++];
        }
    }
    
    if (pos < input.length()) {
        pos++;  // Skip closing quote
    }
    
    return Token(TokenType::String, value, start);
}

//------------------------------------------------------------------------------
// Main tokenization
//------------------------------------------------------------------------------
std::vector<Token> AnnotationLexer::tokenize(const std::string& input) {
    std::vector<Token> tokens;
    size_t pos = 0;
    
    while (pos < input.length()) {
        char c = input[pos];
        
        // Skip whitespace
        if (isWhitespace(c)) {
            pos++;
            continue;
        }
        
        // Single-character tokens
        switch (c) {
            case '=':
                tokens.emplace_back(TokenType::Equals, "=", pos);
                pos++;
                continue;
            case ',':
                tokens.emplace_back(TokenType::Comma, ",", pos);
                pos++;
                continue;
            case '(':
                tokens.emplace_back(TokenType::ParenOpen, "(", pos);
                pos++;
                continue;
            case ')':
                tokens.emplace_back(TokenType::ParenClose, ")", pos);
                pos++;
                continue;
            case '[':
                tokens.emplace_back(TokenType::BracketOpen, "[", pos);
                pos++;
                continue;
            case ']':
                tokens.emplace_back(TokenType::BracketClose, "]", pos);
                pos++;
                continue;
            case '#':
                tokens.emplace_back(TokenType::Hash, "#", pos);
                pos++;
                continue;
        }
        
        // Number (digit or negative sign followed by digit)
        if (isDigit(c) || (c == '-' && pos + 1 < input.length() && isDigit(input[pos + 1]))) {
            auto token = readNumber(input, pos);
            if (token) tokens.push_back(*token);
            continue;
        }
        
        // Identifier (word)
        if (isAlpha(c)) {
            auto token = readIdentifier(input, pos);
            if (token) tokens.push_back(*token);
            continue;
        }
        
        // String
        if (c == '"' || c == '\'') {
            auto token = readString(input, pos);
            if (token) tokens.push_back(*token);
            continue;
        }
        
        // Invalid character
        tokens.emplace_back(TokenType::Invalid, std::string(1, c), pos);
        pos++;
    }
    
    tokens.emplace_back(TokenType::EndOfInput, "", pos);
    return tokens;
}

} // namespace Uniforms

