
#include "Tokenizer.h"
#include <unordered_map>
#include <cctype>

static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"var",   TokenType::VAR},
    {"print", TokenType::PRINT},
    {"if",    TokenType::IF},
    {"else",  TokenType::ELSE},
    {"for",   TokenType::FOR},
    {"true",  TokenType::TRUE},
    {"false", TokenType::FALSE},
    {"and",   TokenType::AND},
    {"or",    TokenType::OR},
};

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

        switch (c)
        {
            case '(': tokens.push_back({TokenType::LEFT_PAREN,  "(", line, {}}); ++pos; continue;
            case ')': tokens.push_back({TokenType::RIGHT_PAREN, ")", line, {}}); ++pos; continue;
            case '{': tokens.push_back({TokenType::LEFT_BRACE,  "{", line, {}}); ++pos; continue;
            case '}': tokens.push_back({TokenType::RIGHT_BRACE, "}", line, {}}); ++pos; continue;
            case ',': tokens.push_back({TokenType::COMMA,       ",", line, {}}); ++pos; continue;
            case '.': tokens.push_back({TokenType::DOT,         ".", line, {}}); ++pos; continue;
            case ';': tokens.push_back({TokenType::SEMICOLON,   ";", line, {}}); ++pos; continue;
            case '+': tokens.push_back({TokenType::PLUS,        "+", line, {}}); ++pos; continue;
            case '-': tokens.push_back({TokenType::MINUS,       "-", line, {}}); ++pos; continue;
            case '*': tokens.push_back({TokenType::STAR,        "*", line, {}}); ++pos; continue;
            case '/': tokens.push_back({TokenType::SLASH,       "/", line, {}}); ++pos; continue;
            case '=': tokens.push_back({TokenType::EQUAL,       "=", line, {}}); ++pos; continue;
            case '>': tokens.push_back({TokenType::GREATER,     ">", line, {}}); ++pos; continue;
            case '<': tokens.push_back({TokenType::LESS,        "<", line, {}}); ++pos; continue;
            case '!': tokens.push_back({TokenType::BANG,        "!", line, {}}); ++pos; continue;
        }

        if (c == '"')
        {
            ++pos;
            size_t start = pos;
            while (pos < source.size() && source[pos] != '"')
            {
                if (source[pos] == '\n') ++line;
                ++pos;
            }
            std::string value  = source.substr(start, pos - start);
            std::string lexeme = "\"" + value + "\"";
            tokens.push_back({TokenType::STRING, lexeme, line, value});
            ++pos;
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c)))
        {
            size_t start = pos;
            while (pos < source.size() && std::isdigit(static_cast<unsigned char>(source[pos])))
                ++pos;
            if (pos < source.size() && source[pos] == '.' &&
                pos + 1 < source.size() && std::isdigit(static_cast<unsigned char>(source[pos + 1])))
            {
                ++pos;
                while (pos < source.size() && std::isdigit(static_cast<unsigned char>(source[pos])))
                    ++pos;
            }
            std::string numStr = source.substr(start, pos - start);
            tokens.push_back({TokenType::NUMBER, numStr, line, std::stod(numStr)});
            continue;
        }

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
        {
            size_t start = pos;
            while (pos < source.size() &&
                   (std::isalnum(static_cast<unsigned char>(source[pos])) || source[pos] == '_'))
                ++pos;
            std::string word = source.substr(start, pos - start);
            auto it = KEYWORDS.find(word);
            TokenType type = (it != KEYWORDS.end()) ? it->second : TokenType::IDENTIFIER;
            tokens.push_back({type, word, line, {}});
            continue;
        }

        ++pos;
    }

    tokens.push_back({TokenType::EOF_TOKEN, "", line, {}});
    return tokens;
}
