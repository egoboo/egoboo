#pragma once

#include "egolib/Script/Traits.hpp"
#include "idlib/parsing_expressions/include.hpp"
#include "egolib/Script/TextInputFile.hpp"

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
    size_t _lineNumber;

    //// @brief The file name.
    std::string _fileName;

public:
    using Traits = TraitsArg;
    using SymbolType = typename Traits::Type;
    using ExtendedSymbolType = int;
    using ScannerInputViewType = ScannerInputView<ExtendedSymbolType, Traits::startOfInput(), Traits::endOfInput(), Traits::error(), std::vector<char>::const_iterator>;

private:
    /// @brief The lexeme accumulation buffer.
    std::vector<char> _buffer;
    /// @brief The input buffer.
    std::vector<char> _inputBuffer;
    /// @brief The input view.
    ScannerInputViewType _inputView;

protected:
    /// @brief Construct this scanner.
    /// @param fileName the filename
    /// @throw RuntimeErrorException if the file can not be read
    /// @post The scanner is in its initial state w.r.t. the specified input if no exception is raised.
    Scanner(const std::string& fileName) :
        _fileName(fileName), _inputBuffer(),
        _buffer(), _inputView(_inputBuffer.cbegin(), _inputBuffer.cend()),
        _lineNumber(1)
    {
        vfs_readEntireFile
        (
            fileName,
            [this](size_t numberOfBytes, const char *bytes)
        {
            _inputBuffer.insert(_inputBuffer.end(), bytes, bytes + numberOfBytes);
        }
        );
        _inputView = ScannerInputViewType(_inputBuffer.cbegin(), _inputBuffer.cend());
    }

    /// @brief Set the input.
    /// @param fileName the filename
    /// @post The scanner is in its initial state w.r.t. the specified input if no exception is raised.
    /// If an exception is raised, the scanner retains its state.
    void setInput(const std::string& fileName)
    {
        std::vector<char> temporaryBuffer;
        std::string temporaryFileName = fileName;
        // If this succeeds, then we're set.
        vfs_readEntireFile(fileName, [&temporaryBuffer](size_t numberOfBytes, const char *bytes)
        {
            temporaryBuffer.insert(temporaryBuffer.end(), bytes, bytes + numberOfBytes);
        });
        _lineNumber = 1;
        _fileName.swap(temporaryFileName);
        _inputBuffer.swap(temporaryBuffer);
        _inputView = ScannerInputViewType(_inputBuffer.cbegin(), _inputBuffer.cend());
    }

    /// @brief Destruct this scanner.
    virtual ~Scanner()
    {}

public:
    /// @brief Get the file name.
    /// @return the file name
    const std::string& getFileName() const
    {
        return _fileName;
    }

    /// @brief Get the line number.
    /// @return the line number
    size_t getLineNumber() const
    {
        return _lineNumber;
    }

    /// @brief Get the lexeme text.
    /// @return the lexeme text
    std::string getLexemeText() const
    {
        return std::string(_buffer.cbegin(), _buffer.cend());
    }

    /// @brief Clear the lexeme text.
    void clearLexemeText()
    {
        _buffer.clear();
    }

public:
    /// @brief Get the current input symbol.
    /// @return the current input symbol
    ExtendedSymbolType current() const
    {
        return *_inputView;
    }

public:
    /// @brief Advance to the next input symbol.
    void next()
    {
        _inputView++;
    }

    /// @brief Write the specified symbol.
    /// @param symbol the symbol
    inline void write(const ExtendedSymbolType& symbol)
    {
        assert(Traits::is_pua_bmp(symbol) || Traits::is_zt(symbol));
        _buffer.push_back(static_cast<SymbolType>(symbol));
    }

    /// @brief Write the specified symbol and advance to the next symbol.
    /// @param symbol the symbol
    inline void writeAndNext(const ExtendedSymbolType& symbol)
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
    inline void saveAndNext()
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

    /// @brief Get if the current symbol is a whitespace symbol.
    /// @return @a true if the current symbol is a whitespace symbol, @a false otherwise
    inline bool isWhiteSpace() const
    {
        static const auto p = id::parsing_expressions::whitespace<ExtendedSymbolType>();
        auto at = this->_inputView.get_current(),
            end = this->_inputView.get_end();
        return p(at, end);
    }

    /// @brief Get if the current symbol is a new line symbol.
    /// @return @a true if the current symbol is a new line symbol, @a false otherwise
    inline bool isNewLine() const
    {
        static const auto p = id::parsing_expressions::newline<ExtendedSymbolType>();
        auto at = this->_inputView.get_current(),
            end = this->_inputView.get_end();
        return p(at, end);
    }

    /// @brief Get if the current symbol is an alphabetic symbol.
    /// @return @a true if the current symbol is an alphabetic symbol, @a false otherwise
    inline bool isAlpha() const
    {
        static const auto p = id::parsing_expressions::alpha<ExtendedSymbolType>();
        auto at = this->_inputView.get_current(),
            end = this->_inputView.get_end();
        return p(at, end);
    }

    /// @brief Get if the current symbol is a digit symbol.
    /// @return @a true if the current symbol is a digit symbol, @a false otherwise
    inline bool isDigit() const
    {
        static const auto p = id::parsing_expressions::digit<ExtendedSymbolType>();
        auto at = this->_inputView.get_current(),
            end = this->_inputView.get_end();
        return p(at, end);
    }

    /// @brief Get if the current symbol is a start of input symbol.
    /// @return @a true if the current symbol is a start of input symbol, @a false otherwise
    inline bool isStartOfInput() const
    {
        return is(Traits::startOfInput());
    }

    /// @brief Get if the current symbol is an end of input symbol.
    /// @return @a true if the current symbol is an end of input symbol, @a false otherwise
    inline bool isEndOfInput() const
    {
        return is(Traits::endOfInput());
    }

    /// @brief Get if the current symbol is an error symbol.
    /// @return @a true if the current symbol is an error symbol, @a false otherwise
    inline bool isError() const
    {
        return is(Traits::error());
    }

public:
    void newLine()
    {
        if (isNewLine())
        {
            auto old = current();
            writeAndNext('\n');
            if (isNewLine() && old != current())
            {
                next();
            }
            _lineNumber++;
        }
    }

    /// @brief Skip zero or one newline sequences.
    /// @remark Proper line counting is performed.
    void skipNewLine()
    {
        if (isNewLine())
        {
            auto old = current();
            next();
            if (isNewLine() && old != current())
            {
                next();
            }
            _lineNumber++;
        }
    }

    void newLines()
    {
        while (isNewLine())
        {
            auto old = current();
            writeAndNext('\n');
            if (isNewLine() && old != current())
            {
                next();
            }
            _lineNumber++;
        }
    }

    /// @brief Skip zero or more newline sequences.
    /// @remark Proper line counting is performed.
    void skipNewLines()
    {
        while (isNewLine())
        {
            auto old = current();
            next();
            if (isNewLine() && old != current())
            {
                next();
            }
            _lineNumber++;
        }
    }

};

} // namespace Script
} // namespace Ego
