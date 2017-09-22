#include "ScriptMigrator/unparser_v2.hpp"
#include "ScriptMigrator/tree_v2.hpp"

unparser_v2::unparser_v2() :
    m_indent(0)
{}

unparser_v2::~unparser_v2()
{}

void unparser_v2::empty_line(const std::shared_ptr<::empty_line_v2>& empty_line)
{
    for (size_t i = 0, n = m_indent * 2; i < n; ++i)
    {
        m_write(' ');
    }
    m_write('\n');
}

void unparser_v2::call(const std::shared_ptr<word_v2>& call)
{
    write(call->get_text().cbegin(), call->get_text().cend());
}

void unparser_v2::op(const std::shared_ptr<word_v2>& op)
{
    write(op->get_text().cbegin(), op->get_text().cend());
}

void unparser_v2::number(const std::shared_ptr<word_v2>& number)
{
    write(number->get_text().cbegin(), number->get_text().cend());
}

void unparser_v2::string(const std::shared_ptr<word_v2>& string)
{
    write(string->get_text().cbegin(), string->get_text().cend());
}

void unparser_v2::identifier(const std::shared_ptr<word_v2>& identifier)
{
    write(identifier->get_text().cbegin(), identifier->get_text().cend());
}

void unparser_v2::word(const std::shared_ptr<::word_v2>& word)
{
    switch (word->id::category_element<word_kind_v2, word_kind_v2::unknown>::category())
    {
    case word_kind_v2::call:
        this->call(word);
        break;
    case word_kind_v2::identifier:
        this->identifier(word);
        break;
    case word_kind_v2::number:
        this->number(word);
        break;
    case word_kind_v2::string:
        this->string(word);
        break;
    case word_kind_v2::op:
        this->op(word);
        break;
    case word_kind_v2::unknown:
    default:
        break;
    };
}

void unparser_v2::line(const std::shared_ptr<::line_v2>& line)
{
    for (size_t i = 0, n = m_indent * 2; i < n; ++i)
    {
        m_write(' ');
    }
    auto it = line->get_words().cbegin();
    if (it != line->get_words().cend())
    {
        this->word(*it);
        ++it;
        for (; it != line->get_words().cend(); ++it)
        {
            m_write(' ');
            this->word(*it);
        }
    }
    m_write('\n');
}

void unparser_v2::statement(const std::shared_ptr<::statement_v2>& statement)
{
    switch (statement->id::category_element<statement_kind_v2, statement_kind_v2::unknown>::category())
    {
    case ::statement_kind_v2::line:
        this->line(std::dynamic_pointer_cast<::line_v2>(statement));
        break;
    case ::statement_kind_v2::empty_line:
        this->empty_line(std::dynamic_pointer_cast<::empty_line_v2>(statement));
        break;
    case ::statement_kind_v2::statement_list:
        this->statement_list(std::dynamic_pointer_cast<::statement_list_v2>(statement));
        break;
    case ::statement_kind_v2::block:
        this->block(std::dynamic_pointer_cast<::block_v2>(statement));
        break;
    case ::statement_kind_v2::unknown:
    default:
        // Do nothing.
        break;
    };
}

void unparser_v2::block(const std::shared_ptr<::block_v2>& block)
{
    this->remove_empty_lines_at_end_of_block(block);
    for (size_t i = 0, n = m_indent * 2; i < n; ++i)
    {
        m_write(' ');
    }
    m_write('{');
    m_write('\n');
    m_indent++;
    this->statement_list(block->get_statement_list());
    m_indent--;
    for (size_t i = 0, n = m_indent * 2; i < n; ++i)
    {
        m_write(' ');
    }
    m_write('}');
    m_write('\n');
}

void unparser_v2::statement_list(const std::shared_ptr<::statement_list_v2>& statement_list)
{
    for (const auto& statement : (*statement_list))
    {
        this->statement(statement);
    }
}

void unparser_v2::program(const std::shared_ptr<::program_v2>& program)
{
    this->statement_list(program->get_statement_list());
}

void unparser_v2::run(const std::shared_ptr<::program_v2>& program, std::function<void(char)> write)
{
    this->m_write = write;
    this->m_indent = 0;
    this->program(program);
}

void unparser_v2::remove_empty_lines_at_end_of_block(const std::shared_ptr<::block_v2>& block)
{
    static const auto pred = [](auto x) { return x->id::category_element<statement_kind_v2, statement_kind_v2::unknown>::is_one_of(statement_kind_v2::empty_line); };
    auto& statements = block->get_statement_list()->get_statements();
    auto it = std::find_if_not(statements.rbegin(), statements.rend(), pred).base();
    statements.erase(it, statements.end());
}
