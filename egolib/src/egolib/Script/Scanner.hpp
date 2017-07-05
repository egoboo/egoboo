#pragma once

#include "egolib/Script/Traits.hpp"
#include "idlib/parsing_expressions/include.hpp"
#include "egolib/Script/TextInputFile.hpp"

#pragma push_macro("ERROR")
#undef ERROR

namespace Ego {
namespace Script {

/// @brief An iterator decorating an input with start of input and end of input symbols.
template <typename Symbol, Symbol StartOfInput, Symbol EndOfInput, typename UI>
struct input_iterator
{
public:
    // -1 invalid, 0 before start, 1 after end
    int m_counter;
    UI m_current, m_start, m_end;

    input_iterator() :
        m_counter(-1), m_current(), m_start(), m_end()
    {}

    input_iterator(const input_iterator& other) :
        m_counter(other.m_counter),
        m_current(other.m_current), m_start(other.m_start), m_end(other.m_end)
    {}

    static input_iterator make_begin(UI start, UI end)
    {
        return input_iterator(0, start, start, end);
    }

    static input_iterator make_end(UI start, UI end)
    {
        return input_iterator(-1, end, start, end);
    }

protected:
    input_iterator(int counter, UI current, UI start, UI end) :
        m_counter(counter),
        m_current(current), m_start(start), m_end(end)
    {}

public:
    bool operator==(const input_iterator& other) const
    {
        return m_counter == other.m_counter
            && m_current == other.m_current;
    }

    bool operator!=(const input_iterator& other) const
    {
        return !(*this == other);
    }

    input_iterator& operator++() { increment(); return *this; }
    input_iterator operator++(int) { auto it = *this; ++(*this); return it; }

    void increment()
    {
        assert(m_counter != -1);
        if (m_counter == 0)
        {
            m_counter++;
        }
        else if (m_counter == 2)
        {
            m_counter = -1;
        }
        else if (m_counter == 1)
        {
            if (m_current == m_end)
            {
                m_counter++;
            }
            else
            {
                m_current++;
                if (m_current == m_end)
                {
                    m_counter = 2;
                }
            }
        }
        else
        {
            throw std::runtime_error("invalid iterator");
        }
    }

    Symbol current() const
    {
        if (m_counter == -1)
        {
            throw std::runtime_error("invalid iterator");
        }
        if (m_counter == 0)
        {
            return StartOfInput;
        }
        else if (m_counter == 2)
        {
            return EndOfInput;
        }
        else if (m_counter == 1)
        {
            return *m_current;
        }
        else
        {
            throw std::runtime_error("internal error");
        }
    }

    Symbol operator*() const { return current(); }
}; // InputIterator

/// @brief An adapter for a "begin" and "end" iterator pair.
template <typename Target, typename Source>
struct input_adapter
{
public:
    using source = Source;
    using target = Target;
private:
    source m_source_begin, m_source_end;
public:
    input_adapter() : 
        m_source_begin(), m_source_end()
    {}
    input_adapter(source source_begin, source source_end) :
        m_source_begin(source_begin), m_source_end(source_end)
    {}

    target cbegin() const
    {
        return target::make_begin(m_source_begin, m_source_end);
    }

    target cend() const
    {
        return target::make_end(m_source_begin, m_source_end);
    }
};

/// @brief A scanner.
/// @tparam TraitsArg the type of the traits
template <typename TraitsArg = Traits<char>>
struct Scanner
{
private:
    /// @brief The line number.
    size_t m_line_number;

    //// @brief The file name.
    std::string m_file_name;

public:
    using Traits = TraitsArg;
    using SymbolType = typename Traits::Type;
    using ExtendedSymbolType = typename Traits::ExtendedType;

    using source_iterator_type = typename std::vector<char>::const_iterator;
    using target_iterator_type = input_iterator<ExtendedSymbolType, Traits::startOfInput(), Traits::endOfInput(), source_iterator_type>; 
    using input_adapter_type = input_adapter<target_iterator_type, source_iterator_type>;
    
private:
    /// @brief The lexeme accumulation buffer.
    std::vector<char> m_buffer;
    /// @brief The input buffer.
    std::vector<char> m_input_buffer;
    /// @brief The input view.
    input_adapter_type m_input_adapter;
    target_iterator_type m_begin, m_end, m_current;

protected:
    /// @brief Construct this scanner.
    /// @param file_name the filename
    /// @throw id::runtime_error the file can not be read
    /// @post The scanner is in its initial state w.r.t. the specified input if no exception is raised.
    Scanner(const std::string& file_name) :
        m_file_name(file_name), m_line_number(1), m_input_buffer(),
        m_buffer(), m_input_adapter()
    {
        vfs_readEntireFile
        (
            file_name,
            [this](size_t number_of_bytes, const char *bytes)
            {
                m_input_buffer.insert(m_input_buffer.end(), bytes, bytes + number_of_bytes);
            }
        );
        //
        m_input_adapter = input_adapter_type(m_input_buffer.cbegin(), m_input_buffer.cend());
        m_begin = m_input_adapter.cbegin();
        m_end = m_input_adapter.cend();
        m_current = m_input_adapter.cbegin();
    }

    /// @brief Set the input.
    /// @param file_name the filename
    /// @post The scanner is in its initial state w.r.t. the specified input if no exception is raised.
    /// If an exception is raised, the scanner retains its state.
    void set_input(const std::string& file_name)
    {
        std::vector<char> temporary_input_buffer;
        std::string temporary_file_name = file_name;
        // If this succeeds, then we're set.
        vfs_readEntireFile(file_name, [&temporary_input_buffer](size_t number_of_bytes, const char *bytes)
        {
            temporary_input_buffer.insert(temporary_input_buffer.end(), bytes, bytes + number_of_bytes);
        });
        m_line_number = 1;
        m_file_name.swap(temporary_file_name);
        m_input_buffer.swap(temporary_input_buffer);
        //
        m_input_adapter = input_adapter_type(m_input_buffer.cbegin(), m_input_buffer.cend());
        m_begin = m_input_adapter.cbegin();
        m_end = m_input_adapter.cend();
        m_current = m_input_adapter.cbegin();
    }

    /// @brief Destruct this scanner.
    virtual ~Scanner()
    {}

public:
    /// @brief Get the file name.
    /// @return the file name
    const std::string& get_file_name() const
    {
        return m_file_name;
    }

    /// @brief Get the line number.
    /// @return the line number
    size_t get_line_number() const
    {
        return m_line_number;
    }

    /// @brief Get the location.
    /// @return the location
    id::location get_location() const
    {
        return id::location(get_file_name(), get_line_number());
    }

    /// @brief Get the lexeme text.
    /// @return the lexeme text
    std::string get_lexeme_text() const
    {
        return std::string(m_buffer.cbegin(), m_buffer.cend());
    }

    /// @brief Clear the lexeme text.
    void clear_lexeme_text()
    {
        m_buffer.clear();
    }

public:
    /// @brief Get the current input symbol.
    /// @return the current input symbol
    ExtendedSymbolType current() const
    {
        return *m_current;
    }

public:
    /// @brief Advance to the next input symbol.
    void next()
    {
        m_current++;
    }

    /// @brief Write the specified symbol.
    /// @param symbol the symbol
    inline void write(const ExtendedSymbolType& symbol)
    {
        assert(!Traits::is_pua_bmp(symbol) && !Traits::is_zt(symbol));
        m_buffer.push_back(static_cast<SymbolType>(symbol));
    }

    /// @brief Write the specified symbol and advance to the next symbol.
    /// @param symbol the symbol
    inline void write_and_next(const ExtendedSymbolType& symbol)
    {
        write(symbol);
        next();
    }

    /// @brief Save the current extended character.
    inline void save()
    {
        write(current());
    }

    /// @brief Save the current input symbol and advance to the next input symbol.
    inline void save_and_next()
    {
        save();
        next();
    }

public:
    /// @brief Get if the current symbol equals another symbol.
    /// @param symbol the other extended character
    /// @return @a true if the current symbol equals the other symbol, @a false otherwise
    inline bool is(const ExtendedSymbolType& symbol) const
    {
        return symbol == current();
    }

    static decltype(auto) WHITE_SPACE()
    {
        static const auto p = id::parsing_expressions::whitespace<ExtendedSymbolType>();
        return p;
    }

    static decltype(auto) NEW_LINE()
    {
        static const auto p = id::parsing_expressions::newline<ExtendedSymbolType>();
        return p;
    }

    static decltype(auto) ALPHA()
    {
        static const auto p = id::parsing_expressions::alpha<ExtendedSymbolType>();
        return p;
    }

    static decltype(auto) DIGIT()
    {
        static const auto p = id::parsing_expressions::digit<ExtendedSymbolType>();
        return p;
    }

    static decltype(auto) START_OF_INPUT()
    {
        static const auto p = id::parsing_expressions::sym<ExtendedSymbolType>(Traits::startOfInput());
        return p;
    }

    static decltype(auto) END_OF_INPUT()
    {
        static const auto p = id::parsing_expressions::sym<ExtendedSymbolType>(Traits::endOfInput());
        return p;
    }

    static decltype(auto) ERROR()
    {
        static const auto p = id::parsing_expressions::sym<ExtendedSymbolType>(Traits::error());
        return p;
    }

    template <typename T>
    bool ise(const T& e) const
    {
        return e(m_current, m_end).first;
    }

public:
    /// @code
    /// new_line := NEW_LINE?
    /// @endcode
    void new_line(std::function<void(char)> action)
    {
        if (ise(NEW_LINE()))
        {
            auto old = current();
            if (action)
            { 
                action('\n');
            }
            next();
            if (ise(NEW_LINE()) && old != current())
            {
                next();
            }
            m_line_number++;
        }
    }

    /// @code
    /// new_lines = NEW_LINE*
    /// @endcode
    void new_lines(std::function<void(char)> action)
    {
        while (ise(NEW_LINE()))
        {
            auto old = current();
            if (action)
            {
                action('\n');
            }
            next();
            if (ise(NEW_LINE()) && old != current())
            {
                next();
            }
            m_line_number++;
        }
    }

};

} // namespace Script
} // namespace Ego

#pragma pop_macro("ERROR")
