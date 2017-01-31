#pragma once

#include "egolib/Script/Traits.hpp"
#include "egolib/Script/Buffer.hpp"
#include "egolib/Script/TextInputFile.hpp"

namespace Ego {
namespace Script {

/// @brief A scanner.
template <typename TraitsArg = Traits<char>>
struct AbstractReader {
private:
    /// @brief The line number.
    size_t _lineNumber;

    //// @brief The file name.
    std::string _fileName;

public:
    using Traits = TraitsArg;
    using SymbolType = typename Traits::Type;
    using ExtendedSymbolType = typename Traits::ExtendedType;

    /// @brief The lexeme accumulation buffer.
    Buffer _buffer;

private:
    /// @brief The input buffer.
    Buffer _inputBuffer;

    /// -1 before the first character, inputLength after the last character.
    long long _inputIndex;

protected:
    /// @brief Construct this reader.
    /// @param fileName the filename
    /// @throw RuntimeErrorException if the file can not be read
    /// @post The reader is in its initial state w.r.t. the specified input if no exception is raised.
    AbstractReader(const std::string& fileName) :
        _fileName(fileName), _inputBuffer(), _inputIndex(-1),
        _buffer(),
        _lineNumber(1) {
        vfs_readEntireFile
            (
                fileName,
                [this](size_t numberOfBytes, const char *bytes) {
                    _inputBuffer.append(bytes, numberOfBytes); 
                }
            );
    }

    /// @brief Set the input.
    /// @param fileName the filename
    /// @post The reader is in its initial state w.r.t. the specified input if no exception is raised.
    /// If an exception is raised, the reader retains its state.
    void SetInput(const std::string& fileName) {
        Buffer temporaryBuffer;
        std::string temporaryFileName = fileName;
        // If this succeeds, then we're set.
        vfs_readEntireFile(fileName, [&temporaryBuffer](size_t numberOfBytes, const char *bytes) {
            temporaryBuffer.append(bytes, numberOfBytes);
            });
        _lineNumber = 1;
        _inputIndex = -1;
        _fileName.swap(temporaryFileName);
        _inputBuffer.swap(temporaryBuffer);
    }

    /// @brief Destruct this reader.
    virtual ~AbstractReader() {
    }

public:
    /// @brief Get the file name.
    /// @return the file name
    const std::string& getFileName() const {
        return _fileName;
    }

    /// @brief Get the line number.
    /// @return the line number
    size_t getLineNumber() const {
        return _lineNumber;
    }

public:
    /// @brief Get the current extended character.
    /// @return the current extended character
    ExtendedSymbolType current() const {
        if (_inputIndex == -1) {
            return Traits::startOfInput();
        } else if (_inputIndex == _inputBuffer.getSize()) {
            return Traits::endOfInput();
        }
        return _inputBuffer.get(_inputIndex);
    }

public:
    /// @brief Advance to the next extended character.
    void next() {
        if (_inputIndex == _inputBuffer.getSize()) {
            return;
        }
        _inputIndex++;
    }

    /// @brief Write the specified extended character.
    /// @param echr the extended character
    inline void write(const ExtendedSymbolType& echr) {
        assert(Traits::isValid(echr));
        _buffer.append(static_cast<SymbolType>(echr));
    }

    /// @brief Write the specified extended character and advance to the next extended character.
    /// @param echr the extended character
    inline void writeAndNext(const ExtendedSymbolType& echr) {
        write(echr);
        next();
    }

    /// @brief Save the current extended character.
    inline void save() {
        write(current());
    }

    /// @brief Save the current extended character and advance to the next extended character.
    inline void saveAndNext() {
        save();
        next();
    }

    /// @brief Convert the contents of the lexeme accumulation buffer to a string value.
    /// @return the string value
    std::string toString() const {
        return _buffer.toString();
    }

public:
    /// @brief Get if the current extended character equals another extended character.
    /// @param other the other extended character
    /// @return @a true if the current extended character equals the other extended character, @a false otherwise
    inline bool is(const ExtendedSymbolType& echr) const {
        return echr == current();
    }

    /// @brief Get if the current extended character is a whitespace character.
    /// @return @a true if the current extended character is a whitespace character, @a false otherwise
    inline bool isWhiteSpace() const {
        return Traits::isWhiteSpace(current());
    }

    /// @brief Get if the current extended character is a new line character.
    /// @return @a true if the current extended character is a new line character, @a false otherwise
    inline bool isNewLine() const {
        return Traits::isNewLine(current());
    }

    /// @brief Get if the current extended character is an alphabetic character.
    /// @return @a true if the current extended character is an alphabetic character, @a false otherwise
    inline bool isAlpha() const {
        return Traits::isAlphabetic(current());
    }

    /// @brief Get if the current extended character is a digit character.
    /// @return @a true if the current extended character is a digit character, @a false otherwise
    inline bool isDigit() const {
        return Traits::isDigit(current());
    }

public:
    void newLine() {
        if (isNewLine()) {
            auto old = current();
            writeAndNext('\n');
            if (isNewLine() && old != current()) {
                next();
            }
            _lineNumber++;
        }
    }

    /// @brief Skip zero or one newline sequences.
    /// @remark Proper line counting is performed.
    void skipNewLine() {
        if (isNewLine()) {
            auto old = current();
            next();
            if (isNewLine() && old != current()) {
                next();
            }
            _lineNumber++;
        }
    }

    void newLines() {
        while (isNewLine()) {
            auto old = current();
            writeAndNext('\n');
            if (isNewLine() && old != current()) {
                next();
            }
            _lineNumber++;
        }
    }

    /// @brief Skip zero or more newline sequences.
    /// @remark Proper line counting is performed.
    void skipNewLines() {
        while (isNewLine()) {
            auto old = current();
            next();
            if (isNewLine() && old != current()) {
                next();
            }
            _lineNumber++;
        }
    }

};

} // namespace Script
} // namespace Ego
