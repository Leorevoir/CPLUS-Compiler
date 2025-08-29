#pragma once

#include <CPlus/Compiler/Interface.hpp>
#include <CPlus/Parser/Token.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace cplus {

namespace lx {

/**
* @brief LexicalAnalyzer
* @details converts source code into tokens
*
* @input std::pair<std::string, std::string> (filename, source code)
* @output std::vector<Token>
*/
class LexicalAnalyzer : public CompilerPass<FileContent, std::vector<Token>>
{
    public:
        constexpr LexicalAnalyzer() = default;
        constexpr ~LexicalAnalyzer() = default;

        std::vector<Token> run(const FileContent &source) override;

    private:
        std::string _source;
        cstr _module;
        u64 _position = 0;
        u64 _line = 1;
        u64 _column = 1;

        std::vector<Token> _tokens;

        void _scan_token();
        char _advance();
        bool _match(const char expected);

        char _peek() const;
        char _peek_next() const;
        bool _is_at_end() const;

        void _skip_line_comment();
        void _skip_block_comment();

        void _scan_number();
        void _scan_identifier();
        void _scan_string();
        void _scan_character();

        void _add_token(const TokenKind kind, const std::string_view lexeme);
};

}// namespace lx

}// namespace cplus
