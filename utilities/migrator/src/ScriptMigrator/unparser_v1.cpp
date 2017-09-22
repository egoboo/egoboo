#include "ScriptMigrator/unparser_v1.hpp"
#include "ScriptMigrator/tree_v1.hpp"

unparser_v1::unparser_v1() :
    m_indent(0)
{}

unparser_v1::~unparser_v1()
{}

void unparser_v1::empty_line(const std::shared_ptr<::empty_line_v1>& empty_line)
{
    for (size_t i = 0, n = m_indent * 2; i < n; ++i)
    {
        write(' ');
    }
    write('\n');
}

void unparser_v1::word(const std::shared_ptr<::word_v1>& word)
{
    write(word->get_text().cbegin(), word->get_text().cend());
}

void unparser_v1::line(const std::shared_ptr<::line_v1>& line)
{
    for (size_t i = 0, n = m_indent * 2; i < n; ++i)
    {
        write(' ');
    }
    auto it = line->get_words().cbegin();
    if (it != line->get_words().cend())
    {
        this->word(*it);
        ++it;
        for (; it != line->get_words().cend(); ++it)
        {
            write(' ');
            this->word(*it);
        }
    }
    write('\n');
}

void unparser_v1::statement(const std::shared_ptr<::statement_v1>& statement)
{
    switch (statement->id::category_element<statement_kind_v1, statement_kind_v1::unknown>::category())
    {
        case ::statement_kind_v1::line:
            this->line(std::dynamic_pointer_cast<::line_v1>(statement));
        break;
        case ::statement_kind_v1::empty_line:
            this->empty_line(std::dynamic_pointer_cast<::empty_line_v1>(statement));
        break;
        case ::statement_kind_v1::statement_list:
            this->statement_list(std::dynamic_pointer_cast<::statement_list_v1>(statement));
        break;
        case ::statement_kind_v1::block:
            this->block(std::dynamic_pointer_cast<::block_v1>(statement));
        break;
        case ::statement_kind_v1::unknown:
        default:
            // Do nothing.
        break;
    };
}

void unparser_v1::block(const std::shared_ptr<::block_v1>& block)
{
    this->remove_empty_lines_at_end_of_block(block);
    for (size_t i = 0, n = m_indent * 2; i < n; ++i)
    { write(' '); }
    write('{');
    write('\n');
    m_indent++;
    this->statement_list(block->get_statement_list());
    m_indent--;
    for (size_t i = 0, n = m_indent * 2; i < n; ++i)
    { write(' '); }
    write('}');
    write('\n');
}

void unparser_v1::statement_list(const std::shared_ptr<::statement_list_v1>& statement_list)
{
    for (const auto& statement : (*statement_list))
    {
        this->statement(statement);
    }
}

void unparser_v1::program(const std::shared_ptr<::program_v1>& program)
{
    this->statement_list(program->get_statement_list());
}

void unparser_v1::run(const std::shared_ptr<::program_v1>& program, std::function<void(char)> write)
{ 
    this->m_write = write;
    this->m_indent = 0;
    this->program(program); 
}

void unparser_v1::remove_empty_lines_at_end_of_block(const std::shared_ptr<::block_v1>& block)
{
    static const auto pred = [](auto x) { return x->id::category_element<statement_kind_v1, statement_kind_v1::unknown>::is_one_of(statement_kind_v1::empty_line); };
    auto& statements = block->get_statement_list()->get_statements();
    auto it = std::find_if_not(statements.rbegin(), statements.rend(), pred).base();
    statements.erase(it, statements.end());
}
