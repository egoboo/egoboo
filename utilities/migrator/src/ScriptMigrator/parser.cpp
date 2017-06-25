#include "ScriptMigrator/parser.hpp"

parser::parser(scanner *scanner) :
    m_scanner(scanner)
{
    if (nullptr == scanner) throw id::null_error(__FILE__, __LINE__, "scanner");
}

parser::~parser()
{}

enum class ast_node_kind
{
    error = 0, ///< The error node is the default node.
    program,
};

class ast
{
private:
    ast_node_kind m_kind;
public:
    ast(ast_node_kind kind) :
        m_kind(kind)
    {}
    ast(const ast&) = delete;
    ast& operator=(const ast&) = delete;
};

void parser::run()
{
    auto ast = std::make_shared<::ast>(ast_node_kind::program);
    auto tokens = m_scanner->run();
    size_t index = 0;
    if (!tokens[index].is_one_of(token_kind::start_of_input))
    {
        return;
    }
    index++;
    while (!tokens[index].is_one_of(token_kind::end_of_input))
    { 
        index++;
    }
    if (!tokens[index].is_one_of(token_kind::end_of_input))
    {
        return;
    }
}
