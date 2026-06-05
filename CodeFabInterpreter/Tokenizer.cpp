
#include "Tokenizer.h"
#include <unordered_map>
#include <cctype>
#include <stdexcept>

static bool isDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)); }
static bool isAlpha(char c) { return std::isalpha(static_cast<unsigned char>(c)); }
static bool isAlNum(char c) { return std::isalnum(static_cast<unsigned char>(c)); }

static const std::unordered_map<char, TokenType> SINGLE_CHAR = {
    {'(', TokenType::LEFT_PAREN},  {')', TokenType::RIGHT_PAREN},
    {'{', TokenType::LEFT_BRACE},  {'}', TokenType::RIGHT_BRACE},
    {',', TokenType::COMMA},       {'.', TokenType::DOT},
    {';', TokenType::SEMICOLON},   {'+', TokenType::PLUS},
    {'-', TokenType::MINUS},       {'*', TokenType::STAR},
    {'/', TokenType::SLASH},       {'=', TokenType::EQUAL},
    {'>', TokenType::GREATER},     {'<', TokenType::LESS},
    {'!', TokenType::BANG},
    {'[', TokenType::LEFT_BRACKET}, {']', TokenType::RIGHT_BRACKET},
};

static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"var",   TokenType::VAR},
    {"print", TokenType::PRINT},
    {"if",    TokenType::IF},
    {"else",  TokenType::ELSE},
    {"for",   TokenType::FOR},
    {"true",  TokenType::TRUE},
    {"false", TokenType::FALSE},
    {"and",    TokenType::AND},
    {"or",     TokenType::OR},
    {"func",   TokenType::FUNC},
    {"Func",   TokenType::FUNC},
    {"return", TokenType::RETURN},
};

Token Tokenizer::scanString(const std::string& source, size_t& pos, int& line)
{
    int startLine = line;
    ++pos; // opening "
    size_t start = pos;
    while (pos < source.size() && source[pos] != '"')
    {
        if (source[pos] == '\n') ++line;
        ++pos;
    }
    if (pos >= source.size())
        throw std::runtime_error(
            std::string("[") + std::to_string(startLine) + "번째 줄] 종결되지 않은 문자열 리터럴");
    std::string value  = source.substr(start, pos - start);
    std::string lexeme = "\"" + value + "\"";
    ++pos; // closing "
    return {TokenType::STRING, lexeme, startLine, value};
}

Token Tokenizer::scanNumber(const std::string& source, size_t& pos, int line)
{
    size_t start = pos;
    while (pos < source.size() && isDigit(source[pos]))
        ++pos;
    if (pos < source.size() && source[pos] == '.' &&
        pos + 1 < source.size() && isDigit(source[pos + 1]))
    {
        ++pos;
        while (pos < source.size() && isDigit(source[pos]))
            ++pos;
    }
    std::string numStr = source.substr(start, pos - start);
    return {TokenType::NUMBER, numStr, line, std::stod(numStr)};
}

Token Tokenizer::scanWord(const std::string& source, size_t& pos, int line)
{
    size_t start = pos;
    while (pos < source.size() && (isAlNum(source[pos]) || source[pos] == '_'))
        ++pos;
    std::string word = source.substr(start, pos - start);
    auto it = KEYWORDS.find(word);
    TokenType type = (it != KEYWORDS.end()) ? it->second : TokenType::IDENTIFIER;
    return {type, word, line, {}};
}

std::vector<Token> Tokenizer::tokenize(const std::string& source)
{
    std::vector<Token> tokens;
    size_t pos  = 0;
    int    line = 1;

    while (pos < source.size())
    {
        char c = source[pos];

        if (c == '\n') { ++line; ++pos; continue; }
        if (c == ' ' || c == '\r' || c == '\t') { ++pos; continue; }

        auto it = SINGLE_CHAR.find(c);
        if (it != SINGLE_CHAR.end())
        {
            tokens.push_back({it->second, std::string(1, c), line, {}});
            ++pos;
            continue;
        }

        if (c == '"')    { tokens.push_back(scanString(source, pos, line)); continue; }
        if (isDigit(c))  { tokens.push_back(scanNumber(source, pos, line)); continue; }
        if (isAlpha(c) || c == '_') { tokens.push_back(scanWord(source, pos, line)); continue; }

        throw std::runtime_error(
            std::string("[") + std::to_string(line) + "번째 줄] 알 수 없는 문자: '" + c + "'");
    }

    tokens.push_back({TokenType::EOF_TOKEN, "", line, {}});
    return tokens;
}
