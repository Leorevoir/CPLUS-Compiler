#pragma once

#include <CPlus/Compiler/Interface.hpp>
#include <CPlus/Parser/Token.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace cplus {

class LexicalAnalyzer : public CompilerPass<std::string, std::vector<Token>>
{
    public:
        constexpr LexicalAnalyzer() = default;
        constexpr ~LexicalAnalyzer() = default;

        std::vector<Token> run(const std::string &source) override;

    private:
        std::string _source;
        u64 _position = 0;
        u64 _line = 1;
        u64 _column = 1;

        std::vector<Token> _tokens;

        void scan_token();
        char advance();
        bool match(const char expected);

        char peek() const;
        char peek_next() const;
        bool is_at_end() const;

        void skip_line_comment();
        void skip_block_comment();

        void scan_number();
        void scan_identifier();

        void add_token(const TokenKind kind, const std::string_view lexeme);
};

}// namespace cplus
