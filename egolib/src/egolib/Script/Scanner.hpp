#pragma once

#include "egolib/Script/Traits.hpp"
#include "egolib/Script/Buffer.hpp"
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
    using ExtendedSymbolType = typename Traits::ExtendedType;
    using BufferType = Buffer;
    using ScannerInputViewType = ScannerInputView<ExtendedSymbolType, Traits::startOfInput(), Traits::endOfInput(), Traits::error(), Buffer::const_iterator>;

    /// @brief The lexeme accumulation buffer.
    BufferType _buffer;

private:
    /// @brief The input buffer.
    BufferType _inputBuffer;
    ScannerInputViewType _inputBufferView;

protected:
    /// @brief Construct this scanner.
    /// @param fileName the filename
    /// @throw RuntimeErrorException if the file can not be read
    /// @post The scanner is in its initial state w.r.t. the specified input if no exception is raised.
    Scanner(const string& fileName) :
        _fileName(fileName), _inputBuffer(),
        _buffer(), _inputBufferView(_inputBuffer.cbegin(), _inputBuffer.cend()),
        _lineNumber(1)
    {
        vfs_readEntireFile
        (
            fileName,
            [this](size_t numberOfBytes, const char *bytes)
        {
            _inputBuffer.append(bytes, numberOfBytes);
        }
        );
        _inputBufferView = ScannerInputViewType(_inputBuffer.cbegin(), _inputBuffer.cend());
    }

    /// @brief Set the input.
    /// @param fileName the filename
    /// @post The scanner is in its initial state w.r.t. the specified input if no exception is raised.
    /// If an exception is raised, the scanner retains its state.
    void setInput(const string& fileName)
    {
        BufferType temporaryBuffer;
        string temporaryFileName = fileName;
        // If this succeeds, then we're set.
        vfs_readEntireFile(fileName, [&temporaryBuffer](size_t numberOfBytes, const char *bytes)
        {
            temporaryBuffer.append(bytes, numberOfBytes);
        });
        _lineNumber = 1;
        _fileName.swap(temporaryFileName);
        _inputBuffer.swap(temporaryBuffer);
        _inputBufferView = ScannerInputViewType(_inputBuffer.cbegin(), _inputBuffer.cend());
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

public:
    /// @brief Get the current extended character.
    /// @return the current extended character
    ExtendedSymbolType current() const
    {
        return *_inputBufferView;
    }

public:
    /// @brief Advance to the next extended character.
    void next()
    {
        _inputBufferView++;
    }

    /// @brief Write the specified extended character.
    /// @param echr the extended character
    inline void write(const ExtendedSymbolType& echr)
    {
        assert(Traits::isValid(echr));
        _buffer.append(static_cast<SymbolType>(echr));
    }

    /// @brief Write the specified extended character and advance to the next extended character.
    /// @param echr the extended character
    inline void writeAndNext(const ExtendedSymbolType& echr)
    {
        write(echr);
        next();
    }

    /// @brief Save the current extended character.
    inline void save()
    {
        write(current());
    }

    /// @brief Save the current extended character and advance to the next extended character.
    inline void saveAndNext()
    {
        save();
        next();
    }

    /// @brief Convert the contents of the lexeme accumulation buffer to a string value.
    /// @return the string value
    std::string toString() const
    {
        return _buffer.toString();
    }

public:
    /// @brief Get if the current extended character equals another extended character.
    /// @param other the other extended character
    /// @return @a true if the current extended character equals the other extended character, @a false otherwise
    inline bool is(const ExtendedSymbolType& echr) const
    {
        return echr == current();
    }

    /// @brief Get if the current extended character is a whitespace character.
    /// @return @a true if the current extended character is a whitespace character, @a false otherwise
    inline bool isWhiteSpace() const
    {
        return Traits::isWhiteSpace(current());
    }

    /// @brief Get if the current extended character is a new line character.
    /// @return @a true if the current extended character is a new line character, @a false otherwise
    inline bool isNewLine() const
    {
        return Traits::isNewLine(current());
    }

    /// @brief Get if the current extended character is an alphabetic character.
    /// @return @a true if the current extended character is an alphabetic character, @a false otherwise
    inline bool isAlpha() const
    {
        return Traits::isAlphabetic(current());
    }

    /// @brief Get if the current extended character is a digit character.
    /// @return @a true if the current extended character is a digit character, @a false otherwise
    inline bool isDigit() const
    {
        return Traits::isDigit(current());
    }

    /// @brief Get if the current extended character is a start of input character.
    /// @return @a true if the current extended character is a start of input character, @a false otherwise
    inline bool isStartOfInput() const
    {
        return is(Traits::startOfInput());
    }

    /// @brief Get if the current extended character is an end of input character.
    /// @return @a true if the current extended character is an end of input character, @a false otherwise
    inline bool isEndOfInput() const
    {
        return is(Traits::endOfInput());
    }

    /// @brief Get if the current extended character is an error character.
    /// @return @a true if the current extended character is an error character, @a false otherwise
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
