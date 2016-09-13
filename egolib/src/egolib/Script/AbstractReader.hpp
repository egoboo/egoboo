#pragma once

#include "egolib/Script/Traits.hpp"
#include "egolib/Script/Buffer.hpp"
#include "egolib/Script/TextInputFile.hpp"

namespace Ego {
namespace Script {

/**
 * @brief A scanner.
 */
template <typename Traits = Traits<char>>
struct AbstractReader {
private:
    /**
     * @brief The line number.
     */
    size_t _lineNumber;
    /**
     * @brief The file name.
     */
    std::string _fileName;

public:
    /**
     * @brief The lexeme accumulation buffer.
     */
    Buffer _buffer;

private:
    /// @brief A pointer to an array of at least @a _inputLength Bytes.
    char *_input;

    /// @brief The first @a _inputLength Bytes of the array pointed to by @a _input contain the input.
    size_t _inputLength;

    /// -1 before the first character, inputLength after the last character.
    long long _inputIndex;

protected:
    /**
     * @brief Construct this reader.
     * @param fileName the filename
     * @param initialBufferCapacity the initial capacity of the lexeme accumulation buffer
     * @throw RuntimeErrorException if the file can not be read
     */
    AbstractReader(const std::string& fileName, size_t initialBufferCapacity) :
        _fileName(fileName), _input(nullptr), _inputLength(0), _inputIndex(-1),
        _buffer(initialBufferCapacity),
        _lineNumber(1) {
        char *input; size_t inputLength;
        if (!vfs_readEntireFile(fileName, &input, &inputLength)) {
            throw RuntimeErrorException(__FILE__, __LINE__, "unable to read input `" + fileName + "`");
        }
        _input = input;
        _inputLength = inputLength;
    }

    void SetInput(const std::string& fileName) {
        char *input; size_t inputLength;
        if (!vfs_readEntireFile(fileName, &input, &inputLength)) {
            throw RuntimeErrorException(__FILE__, __LINE__, "unable to read input `" + fileName + "`");
        }
        if (_input) {
            delete[] _input;
            _input = nullptr;
        }
        _fileName = fileName;
        _input = input;
        _inputLength = inputLength;
        _lineNumber = 1;
        _inputIndex = -1;
    }

    /**
     * @brief Destruct this reader.
     */
    virtual ~AbstractReader() {
        if (_input) {
            delete[] _input;
            _input = nullptr;
        }
    }

public:
    /**
     * @brief Get the file name.
     * @return the file name
     */
    const std::string& getFileName() const {
        return _fileName;
    }

    /**
     * @brief Get the line number.
     * @return the line number
     */
    size_t getLineNumber() const {
        return _lineNumber;
    }

public:
    /**
     * @brief Get the current extended character.
     * @return the current extended character
     */
    typename Traits::ExtendedType current() const {
        if (_inputIndex == -1) {
            return Traits::startOfInput();
        } else if (_inputIndex == _inputLength) {
            return Traits::endOfInput();
        }
        return _input[_inputIndex];
    }

public:
    /**
     * @brief Advance to the next extended character.
     */
    void next() {
        if (_inputIndex == _inputLength) {
            return;
        }
        _inputIndex++;
    }

    /**
     * @brief Write the specified extended character.
     * @param echr the extended character
     */
    inline void write(const typename Traits::ExtendedType& echr) {
        assert(Traits::isValid(echr));
        _buffer.append(static_cast<typename Traits::Type>(echr));
    }

    /**
     * @brief Write the specified extended character and advance to the next extended character.
     * @param echr the extended character
     */
    inline void writeAndNext(const typename Traits::ExtendedType& echr) {
        write(echr);
        next();
    }

    /**
     * @brief Save the current extended character.
     */
    inline void save() {
        write(current());
    }

    /**
     * @brief Save the current extended character and advance to the next extended character.
     */
    inline void saveAndNext() {
        save();
        next();
    }

    /**
     * @brief Convert the contents of the lexeme accumulation buffer to a string value.
     * @return the string value
     */
    std::string toString() const {
        return _buffer.toString();
    }

public:
    /**
     * @brief Get if the current extended character equals another extended character.
     * @param other the other extended character
     * @return @a true if the current extended character equals the other extended character, @a false otherwise
     */
    inline bool is(const typename Traits::ExtendedType& echr) const {
        return echr == current();
    }

    /**
     * @brief Get if the current extended character is a whitespace character.
     * @return @a true if the current extended character is a whitespace character, @a false otherwise
     */
    inline bool isWhiteSpace() const {
        return Traits::isWhiteSpace(current());
    }

    /**
     * @brief Get if the current extended character is a new line character.
     * @return @a true if the current extended character is a new line character, @a false otherwise
     */
    inline bool isNewLine() const {
        return Traits::isNewLine(current());
    }

    /**
     * @brief Get if the current extended character is an alphabetic character.
     * @return @a true if the current extended character is an alphabetic character, @a false otherwise
     */
    inline bool isAlpha() const {
        return Traits::isAlphabetic(current());
    }

    /**
     * @brief Get if the current extended character is a digit character.
     * @return @a true if the current extended character is a digit character, @a false otherwise
     */
    inline bool isDigit() const {
        return Traits::isDigit(current());
    }

public:
    void newLine() {
        if (isNewLine()) {
            typename Traits::ExtendedType old = current();
            writeAndNext('\n');
            if (isNewLine() && old != current()) {
                next();
            }
            _lineNumber++;
        }
    }
    /**
     * @brief Skip zero or one newline sequences.
     * @remark Proper line counting is performed.
     */
    void skipNewLine() {
        if (isNewLine()) {
            typename Traits::ExtendedType old = current();
            next();
            if (isNewLine() && old != current()) {
                next();
            }
            _lineNumber++;
        }
    }

    void newLines() {
        while (isNewLine()) {
            typename Traits::ExtendedType old = current();
            writeAndNext('\n');
            if (isNewLine() && old != current()) {
                next();
            }
            _lineNumber++;
        }
    }
    /**
     * @brief Skip zero or more newline sequences.
     * @remark Proper line counting is performed.
     */
    void skipNewLines() {
        while (isNewLine()) {
            typename Traits::ExtendedType old = current();
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
