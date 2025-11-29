/**
 * @file AnnotationLexer.h
 * @brief Tokenizer for shader annotation syntax.
 *
 * Converts raw annotation strings into a stream of tokens for parsing.
 */

#pragma once

#include <string>
#include <vector>
#include <optional>

namespace Uniforms {

/**
 * @brief Token types in the annotation syntax.
 */
enum class TokenType {
    Identifier,     // word (e.g., "slider", "min", "default")
    Number,         // numeric literal (e.g., "0.5", "-10", "3.14")
    String,         // quoted string (e.g., "Hello")
    Equals,         // =
    Comma,          // ,
    ParenOpen,      // (
    ParenClose,     // )
    BracketOpen,    // [
    BracketClose,   // ]
    Hash,           // # (for hex colors)
    EndOfInput,
    Invalid
};

/**
 * @brief A single token with type and value.
 */
struct Token {
    TokenType type;
    std::string value;
    size_t position;  // Position in source string
    
    Token(TokenType t, std::string v, size_t pos) 
        : type(t), value(std::move(v)), position(pos) {}
};

/**
 * @brief Lexer that tokenizes annotation strings.
 */
class AnnotationLexer {
public:
    /**
     * @brief Tokenize an annotation parameter string.
     * @param input The parameter string (e.g., "min=0.0, max=1.0, default=0.5,0.3,0.1")
     * @return Vector of tokens in order.
     */
    static std::vector<Token> tokenize(const std::string& input);

private:
    static bool isWhitespace(char c);
    static bool isDigit(char c);
    static bool isAlpha(char c);
    static bool isAlphaNumeric(char c);
    
    static std::optional<Token> readNumber(const std::string& input, size_t& pos);
    static std::optional<Token> readIdentifier(const std::string& input, size_t& pos);
    static std::optional<Token> readString(const std::string& input, size_t& pos);
};

} // namespace Uniforms

