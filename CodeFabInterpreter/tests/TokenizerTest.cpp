#include <gtest/gtest.h>
#include "../Tokenizer.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

// ──────────────────────────────────────────────
// Fixture (R6: Tokenizer 인스턴스 공유, R7: extractTypes 헬퍼)
// ──────────────────────────────────────────────
class TokenizerFixture : public ::testing::Test
{
protected:
    Tokenizer tokenizer;

    std::vector<TokenType> extractTypes(const std::vector<Token>& tokens)
    {
        std::vector<TokenType> types(tokens.size());
        std::transform(tokens.begin(), tokens.end(), types.begin(),
            [](const Token& t) { return t.type; });
        return types;
    }
};

// ──────────────────────────────────────────────
// 통합: 지원하는 모든 TokenType이 올바른 순서로 인식된다 (literal 검증은 TC-07~09 참조)
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, TokenizesAllTokenTypesInOrder)
{
    const std::string source =
        "( ) { } , . ; + - * / ! = > < "
        "identifier \"hello\" 123 "
        "var print if else for true false and or";

    const std::vector<TokenType> expectedTypes = {
        TokenType::LEFT_PAREN,
        TokenType::RIGHT_PAREN,
        TokenType::LEFT_BRACE,
        TokenType::RIGHT_BRACE,
        TokenType::COMMA,
        TokenType::DOT,
        TokenType::SEMICOLON,
        TokenType::PLUS,
        TokenType::MINUS,
        TokenType::STAR,
        TokenType::SLASH,
        TokenType::BANG,
        TokenType::EQUAL,
        TokenType::GREATER,
        TokenType::LESS,
        TokenType::IDENTIFIER,
        TokenType::STRING,
        TokenType::NUMBER,
        TokenType::VAR,
        TokenType::PRINT,
        TokenType::IF,
        TokenType::ELSE,
        TokenType::FOR,
        TokenType::TRUE,
        TokenType::FALSE,
        TokenType::AND,
        TokenType::OR,
        TokenType::EOF_TOKEN
    };

    EXPECT_EQ(extractTypes(tokenizer.tokenize(source)), expectedTypes);
}

// ──────────────────────────────────────────────
// 경계: 빈 소스를 입력하면 EOF_TOKEN 하나만 반환한다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, EmptySourceReturnsOnlyEof)
{
    const auto tokens = tokenizer.tokenize("");

    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::EOF_TOKEN);
}

// ──────────────────────────────────────────────
// 경계: 소스가 무엇이든 마지막 토큰은 항상 EOF_TOKEN이다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, LastTokenIsAlwaysEof)
{
    const auto tokens = tokenizer.tokenize("x");

    ASSERT_FALSE(tokens.empty());
    EXPECT_EQ(tokens.back().type, TokenType::EOF_TOKEN);
}

// ──────────────────────────────────────────────
// 공백: 스페이스와 탭은 토큰을 생성하지 않는다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, WhitespaceIsSkipped)
{
    const auto tokens = tokenizer.tokenize("  +  \t  -  ");

    // PLUS, MINUS, EOF 총 3개
    ASSERT_EQ(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type, TokenType::PLUS);
    EXPECT_EQ(tokens[1].type, TokenType::MINUS);
    EXPECT_EQ(tokens[2].type, TokenType::EOF_TOKEN);
}

// ──────────────────────────────────────────────
// lexeme: 단일 문자 토큰의 lexeme이 소스 문자와 일치한다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, SingleCharTokensHaveCorrectLexeme)
{
    const std::vector<std::string> cases = {"(", ")", "+", "-", "*", "/"};
    for (const auto& src : cases)
    {
        const auto tokens = tokenizer.tokenize(src);
        ASSERT_GE(tokens.size(), 1u) << "input: " << src;
        EXPECT_EQ(tokens[0].lexeme, src) << "input: " << src;
    }
}

// ──────────────────────────────────────────────
// lexeme: 식별자 토큰의 lexeme이 소스 단어와 일치한다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, IdentifierHasCorrectLexeme)
{
    const auto tokens = tokenizer.tokenize("myVar");

    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type,   TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].lexeme, "myVar");
}

// ──────────────────────────────────────────────
// literal: 문자열 리터럴의 literal 값이 따옴표를 제외한 내용이다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, StringLiteralHasCorrectValue)
{
    const auto tokens = tokenizer.tokenize("\"hello\"");

    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::STRING);
    EXPECT_EQ(std::get<std::string>(tokens[0].literal), "hello");
}

// ──────────────────────────────────────────────
// literal: 정수 숫자 리터럴의 literal 값이 double로 저장된다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, IntegerLiteralHasCorrectValue)
{
    const auto tokens = tokenizer.tokenize("123");

    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::NUMBER);
    EXPECT_DOUBLE_EQ(std::get<double>(tokens[0].literal), 123.0);
}

// ──────────────────────────────────────────────
// literal: 소수점 숫자 리터럴의 literal 값이 double로 정확히 저장된다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, FloatLiteralHasCorrectValue)
{
    const auto tokens = tokenizer.tokenize("3.14");

    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::NUMBER);
    EXPECT_DOUBLE_EQ(std::get<double>(tokens[0].literal), 3.14);
}

// ──────────────────────────────────────────────
// 예약어: 모든 키워드가 IDENTIFIER가 아닌 고유 TokenType으로 인식된다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, KeywordsAreNotIdentifiers)
{
    const std::vector<TokenType> expectedTypes = {
        TokenType::VAR,
        TokenType::PRINT,
        TokenType::IF,
        TokenType::ELSE,
        TokenType::FOR,
        TokenType::TRUE,
        TokenType::FALSE,
        TokenType::AND,
        TokenType::OR,
        TokenType::EOF_TOKEN
    };

    EXPECT_EQ(
        extractTypes(tokenizer.tokenize("var print if else for true false and or")),
        expectedTypes
    );
}

// ──────────────────────────────────────────────
// 예약어: 예약어로 시작하지만 더 긴 단어는 IDENTIFIER로 인식된다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, IdentifierStartingWithKeywordIsIdentifier)
{
    // "variable" ⊃ "var", "iffy" ⊃ "if", "forge" ⊃ "for"
    const auto tokens = tokenizer.tokenize("variable iffy forge");

    ASSERT_GE(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].type,   TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].lexeme, "variable");
    EXPECT_EQ(tokens[1].type,   TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].lexeme, "iffy");
    EXPECT_EQ(tokens[2].type,   TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[2].lexeme, "forge");
}

// ──────────────────────────────────────────────
// lexeme: ! 는 BANG 타입으로 인식되고 lexeme이 "!"이다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, BangTokenHasCorrectLexeme)
{
    const auto tokens = tokenizer.tokenize("!");

    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type,   TokenType::BANG);
    EXPECT_EQ(tokens[0].lexeme, "!");
}

// ──────────────────────────────────────────────
// line: 개행 문자를 만날 때마다 line 번호가 1씩 증가한다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, NewlineIncrementsLineNumber)
{
    // +는 1번 줄, -는 2번 줄, *는 3번 줄에 위치한다
    const auto tokens = tokenizer.tokenize("+\n-\n*");

    ASSERT_GE(tokens.size(), 3u);
    EXPECT_EQ(tokens[0].line, 1); // +
    EXPECT_EQ(tokens[1].line, 2); // -
    EXPECT_EQ(tokens[2].line, 3); // *
}

// ──────────────────────────────────────────────
// 에러: 종결되지 않은 문자열 리터럴은 runtime_error를 throw한다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, UnterminatedStringLiteralThrows)
{
    ASSERT_THROW(tokenizer.tokenize("\"hello"), std::runtime_error);
}

// ──────────────────────────────────────────────
// line: 멀티라인 문자열 토큰의 line은 닫는 " 줄이 아닌 시작 줄이다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, MultilineStringHasStartLineNumber)
{
    // "line1\nline2" → 1번 줄에서 시작, 2번 줄에서 닫힘
    // token.line == 1 (시작 줄)
    const auto tokens = tokenizer.tokenize("\"line1\nline2\"");

    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::STRING);
    EXPECT_EQ(tokens[0].line, 1);
}

// ──────────────────────────────────────────────
// 에러: 알 수 없는 문자는 runtime_error를 throw한다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, UnknownCharacterThrows)
{
    ASSERT_THROW(tokenizer.tokenize("@"), std::runtime_error);
}

// ──────────────────────────────────────────────
// 에러(T-1): 소스 중간에 알 수 없는 문자가 있으면 runtime_error를 throw한다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, UnknownCharMidSourceThrows)
{
    // 유효한 토큰(var, x) 사이에 '@' 가 끼어있는 경우
    ASSERT_THROW(tokenizer.tokenize("var @ x"), std::runtime_error);
}

// ──────────────────────────────────────────────
// 복합: 여러 줄 소스의 type과 line이 동시에 올바르다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, MultilineSourceHasCorrectTypeAndLine)
{
    // var\nx\n=\n1 → VAR(1) IDENTIFIER(2) EQUAL(3) NUMBER(4)
    const auto tokens = tokenizer.tokenize("var\nx\n=\n1");

    ASSERT_GE(tokens.size(), 4u);
    EXPECT_EQ(tokens[0].type, TokenType::VAR);
    EXPECT_EQ(tokens[0].line, 1);
    EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].line, 2);
    EXPECT_EQ(tokens[2].type, TokenType::EQUAL);
    EXPECT_EQ(tokens[2].line, 3);
    EXPECT_EQ(tokens[3].type, TokenType::NUMBER);
    EXPECT_EQ(tokens[3].line, 4);
}

// ──────────────────────────────────────────────
// lexeme: 문자열 토큰의 lexeme은 따옴표를 포함한다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, StringLexemeIncludesQuotes)
{
    const auto tokens = tokenizer.tokenize("\"hello\"");

    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type,   TokenType::STRING);
    EXPECT_EQ(tokens[0].lexeme, "\"hello\"");
}

// ──────────────────────────────────────────────
// lexeme: 숫자 토큰의 lexeme은 소스 문자열과 일치한다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, NumberLexemeMatchesSource)
{
    const auto tokens = tokenizer.tokenize("3.14");

    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type,   TokenType::NUMBER);
    EXPECT_EQ(tokens[0].lexeme, "3.14");
}

// ──────────────────────────────────────────────
// lexeme: 밑줄(_)로 시작하는 식별자를 IDENTIFIER로 인식한다
// ──────────────────────────────────────────────
TEST_F(TokenizerFixture, UnderscoreIdentifierIsRecognized)
{
    const auto tokens = tokenizer.tokenize("_count");

    ASSERT_GE(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type,   TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].lexeme, "_count");
}
