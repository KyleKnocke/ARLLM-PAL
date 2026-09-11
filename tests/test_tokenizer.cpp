#include <catch2/catch_test_macros.hpp>
#include <tokenizer/tokenizer.h>
#include <vocabulary/vocabulary.h>
#include <iostream>

TEST_CASE("Tokenizer: Basic arithmetic expression", "[tokenizer]") {
    Vocabulary vocab;
    // Load vocabulary (should be same as in main)
    vocab.load();  // Assumes this loads all opcodes from static data

    Tokenizer tokenizer(vocab);
    std::string input = "a + b * 2";
    
    auto tokens = tokenizer.tokenize(input);

    REQUIRE(tokens.size() == 5);
    REQUIRE(tokens[0].id == vocab.get_token_id("a"));      // identifier
    REQUIRE(tokens[1].id == vocab.get_token_id("+"));     // plus
    REQUIRE(tokens[2].id == vocab.get_token_id("b"));     // identifier
    REQUIRE(tokens[3].id == vocab.get_token_id("*"));     // star
    REQUIRE(tokens[4].id == vocab.get_token_id("2"));     // literal (int)
}

TEST_CASE("Tokenizer: Keywords", "[tokenizer]") {
    Vocabulary vocab;
    vocab.load();

    Tokenizer tokenizer(vocab);
    std::string input = "let x = fn() { return; }";

    auto tokens = tokenizer.tokenize(input);

    REQUIRE(tokens.size() >= 8); // At least: let, id(x), =, fn, (, ), {, return, ;

    // Check specific keywords
    REQUIRE(tokens[0].id == vocab.get_token_id("let"));
    REQUIRE(tokens[3].id == vocab.get_token_id("fn"));
    REQUIRE(tokens[7].id == vocab.get_token_id("return"));
}

TEST_CASE("Tokenizer: Invalid character error", "[tokenizer]") {
    Vocabulary vocab;
    vocab.load();

    Tokenizer tokenizer(vocab);
    std::string input = "a @ b";  // @ is not a valid token

    CHECK_THROWS_AS(tokenizer.tokenize(input), std::runtime_error);
}

TEST_CASE("Tokenizer: Span tracking works", "[tokenizer]") {
    Vocabulary vocab;
    vocab.load();

    Tokenizer tokenizer(vocab);
    std::string input = "x + y\nz * 42";

    auto tokens = tokenizer.tokenize(input);

    REQUIRE(tokens.size() >= 5);

    // First token 'x' should be at line 1, col 0
    REQUIRE(tokens[0].span.start_line == 1);
    REQUIRE(tokens[0].span.start_column == 0);
    
    // '+' should be at line 1, after space
    REQUIRE(tokens[1].span.start_line == 1);
    REQUIRE(tokens[1].span.start_column > 0);

    // 'z' is on second line
    REQUIRE(tokens[3].span.start_line == 2);
}
"