#include "ScriptMigrator/scanner.hpp"

scanner::scanner(const id::c::location& location, const char *start, const char *end, const char *current) :
    m_location(location), m_start(start), m_end(end), m_current(current)
{
    if (nullptr == start) throw id::null_error(__FILE__, __LINE__, "start");
    if (nullptr == end) throw id::null_error(__FILE__, __LINE__, "end");
    if (nullptr == current) throw id::null_error(__FILE__, __LINE__, "current");
}

scanner::~scanner()
{}

std::vector<token> scanner::run()
{
    std::vector<token> result;
    if (m_current == m_start)
    {
        result.push_back(token(token_kind::start_of_input, m_location, ""));
    }
    while (m_current != m_end)
    {
        // ' ' or '\t'
        if (*m_current == ' ' || *m_current == '\t')
        {
            // Parse the prefix of the line.
            m_buffer.clear();
            do
            {
                m_buffer.push_back(' ');
                m_current++;
            } while (m_current != m_end && *m_current == ' ' && *m_current == '\t');
            std::to_string(m_buffer.size());
            result.push_back(token(token_kind::indent, m_location, std::to_string(m_buffer.size())));
        }
        else
        {
            result.push_back(token(token_kind::indent, m_location, std::to_string(0)));

        }
        // Parse the infix of the line.
        m_buffer.clear();
        while (m_current != m_end && *m_current != '\0' && *m_current != '\n' && *m_current != '\r')
        {
            m_buffer.push_back(*m_current);
            m_current++;
        }
        if (*m_current == '\0')
        {
            result.push_back(token(token_kind::error, m_location, std::to_string(m_buffer.size())));
            return result;
        }
        else
        {
            result.push_back(token(token_kind::line, m_location, std::string(m_buffer.cbegin(), m_buffer.cend())));
        }
        // Parse the suffix of the line.
        m_buffer.clear();
        if (m_current != m_end && (*m_current == '\n' || *m_current == '\r'))
        {
            char old = *m_current;
            m_current++;
            if (m_current != m_end && *m_current != old && (*m_current == '\n' || *m_current == '\r'))
            {
                m_current++;
            }
        }
    }
    result.push_back(token(token_kind::end_of_input, m_location, ""));
    return result;
}
