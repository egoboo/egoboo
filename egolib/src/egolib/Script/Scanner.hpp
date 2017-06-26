#pragma once

#include "egolib/Script/Traits.hpp"
#include "idlib/parsing_expressions/include.hpp"
#include "egolib/Script/TextInputFile.hpp"

#pragma push_macro("ERROR")
#undef ERROR

namespace Ego {
namespace Script {

/// @brief A view of a scanner on some input.
/// @detail A bidirectional iterator wrapping a bidirectional iterator pair. Maps input symbols to
/// output symbols while iterating. It can neither be decremented past the beginning nor past the
/// ending of the input. Instead a scanner input view will remain before the first input symbol /
/// behind the last input symbol, returning the special symbols @a StartOfInput/@a EndOfInput.
/// @tparam SymbolType the type of a symbol returned by this view
/// @tparam StartOfInput the start of input symbol
/// @tparam EndOfInput the end of input symbol
/// @tparam Error the error symbol
/// @tparam IteratorType the iterator type
/// @todo Assert this is truly
template <typename SymbolType, SymbolType StartOfInput, SymbolType EndOfInput, SymbolType Error,
          typename IteratorType>
struct ScannerInputView
{
private:
    enum class State
    {
        StartOfInputState,
        EndOfInputState,
        InputState,
    };

private:
    State m_state;
    IteratorType m_begin, m_current, m_end;

public:
    /// @brief Construct this scanner input view.
    /// @param begin bidirectional iterator denoting the beginning of the input
    /// @param end bidirectional iterator denoting the ending of the input
    ScannerInputView(IteratorType begin, IteratorType end) :
        m_state(State::StartOfInputState),
        m_begin(begin), m_current(begin), m_end(end)
    {}

private:
    void decrement()
    {
        switch (m_state)
        {
            case State::StartOfInputState:
                return; // Ignore.
            case State::EndOfInputState:
                assert(m_current == m_end);
                // If the input is empty ...
                if (m_current == m_begin)
                {
                    // ... where at the start of the input.
                    m_state = State::StartOfInputState;
                }
                else
                {
                    --m_current;
                    m_state = State::InputState;
                }
            case State::Input:
            default:
                if (m_current == m_begin)
                {
                    m_state = State::StartOfInputState;
                }
                else
                {
                    --m_current;
                }
                break;
        }
    }
    void increment()
    {
        switch (m_state)
        {
            case State::StartOfInputState:
                m_state = State::InputState;
                if (m_current == m_end)
                {
                    m_state = State::EndOfInputState;
                }
                break;
            case State::EndOfInputState:
                return; // Ignore.
            case State::InputState:
            default:
                if (m_current == m_end)
                {
                    m_state = State::EndOfInputState;
                }
                else
                {
                    m_current++;
                    if (m_current == m_end)
                    {
                        m_state = State::EndOfInputState;
                    }
                }
                break;
        }
    }

    bool equal(ScannerInputView other) const
    {
        m_state == other.m_state && m_current == other.m_current;;
    }

    /// @brief Get the current symbol.
    /// @return the current symbol
    SymbolType current() const
    {
        switch (m_state)
        {
            case State::StartOfInputState:
                return StartOfInput;
            case State::EndOfInputState:
                return EndOfInput;
            case State::InputState:
            default:
                return SymbolType(*m_current);
        }
    }

public:
    IteratorType get_begin() const
    {
        return m_begin;
    }
    IteratorType get_current() const
    {
        return m_current;
    }
    IteratorType get_end() const
    {
        return m_end;
    }

    ScannerInputView& operator--() { decrement(); return *this; }
    ScannerInputView operator--(int) { auto it = *this; --(*this); return it; }

    ScannerInputView& operator++() { increment(); return *this; }
    ScannerInputView operator++(int) { auto it = *this; ++(*this); return it; }

    bool operator==(ScannerInputView other) const { return equal(other); }
    bool operator!=(ScannerInputView other) const { return !(*this == other); }

    SymbolType operator*() const { return current(); }
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
    using ScannerInputViewType = ScannerInputView<ExtendedSymbolType, Traits::startOfInput(), Traits::endOfInput(), Traits::error(), std::vector<char>::const_iterator>;

private:
    /// @brief The lexeme accumulation buffer.
    std::vector<char> m_buffer;
    /// @brief The input buffer.
    std::vector<char> m_input_buffer;
    /// @brief The input view.
    ScannerInputViewType m_input_view;

protected:
    /// @brief Construct this scanner.
    /// @param file_name the filename
    /// @throw id::runtime_error the file can not be read
    /// @post The scanner is in its initial state w.r.t. the specified input if no exception is raised.
    Scanner(const std::string& file_name) :
        m_file_name(file_name), m_line_number(1), m_input_buffer(),
        m_buffer(), m_input_view(m_input_buffer.cbegin(), m_input_buffer.cend())
    {
        vfs_readEntireFile
        (
            file_name,
            [this](size_t number_of_bytes, const char *bytes)
        {
            m_input_buffer.insert(m_input_buffer.end(), bytes, bytes + number_of_bytes);
        }
        );
        m_input_view = ScannerInputViewType(m_input_buffer.cbegin(), m_input_buffer.cend());
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
        m_input_view = ScannerInputViewType(m_input_buffer.cbegin(), m_input_buffer.cend());
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
        return *m_input_view;
    }

public:
    /// @brief Advance to the next input symbol.
    void next()
    {
        m_input_view++;
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
        auto at = m_input_view.get_current(),
             end = m_input_view.get_end();
        return e(at, end);
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
