#pragma once

#include "egolib/Script/TextFile.hpp"
#include "egolib/vfs.h"

namespace Ego
{
    namespace Script
    {

        /**
         * @brief
         *  A text input file supports reading single characters from a file.
         * @author
         *  Michael Heilmann
         */
        template <typename Traits = Traits<char>>
        struct TextInputFile : public TextFile<Traits>
        {

        private:

            /**
             * @brief
             *  The backing VFS file.
             */
            vfs_FILE *_file;

            /**
             * @brief
             *  The current extended character.
             */
            typename Traits::ExtendedType _current;

        public:

            /**
             * @brief
             *  Construct this text input file with the given file name.
             * @param fileName
             *  the file name
             * @remark
             *  The text input file is in its initial state w.r.t. the given file name.
             *  That is, the current extended character is _Traits::startOfInput() and
             *  advance() must be called to advance to the first extend character.
             */
            TextInputFile(const std::string& fileName) :
                TextFile(fileName, Mode::Read),
                _file(nullptr),
                _current(Traits::startOfInput())
            {
                _file = vfs_openReadB(fileName.c_str());
            }

            /**
             * @brief
             *  Destruct this text input file.
             */
            virtual ~TextInputFile()
            {
                if (_file)
                {
                    vfs_close(_file);
                    _file = nullptr;
                }
            }

            /**
             * @brief
             *  Get if the file is opened.
             * @return
             *  @a true if the file is open, @a false otherwise
             */
            bool isOpen() const
            {
                return nullptr != _file;
            }

            /**
             * @brief
             *  Advance the input cursor by one character.
             * @remark
             *  If the end of the input or an error was encountered,
             *  this function does nothing.
             */
            virtual void advance()
            {
                // (1) If an error or the end of the input was encountered ...
                if (_current == Traits::error() || _current == Traits::endOfInput())
                {
                    // ... do nothing.
                    return;
                }
                // (2) If the backing VFS file is not opened ...
                if (!_file)
                {
                    // ... raise an error.
                    _current = Traits::error();
                    return;
                }
                // (3) Otherwise: Read a single Byte.
                uint8_t byte;
                size_t size = vfs_read(&byte, 1, 1, _file);
                if (1 != size)
                {
                    if (vfs_error(_file))
                    {
                        _current = Traits::error();
                        return;
                    }
                    else if (vfs_eof(_file))
                    {
                        _current = Traits::endOfInput();
                        return;
                    }
                    else
                    {
                        std::ostringstream message;
                        message << __FILE__ << ":" << __LINE__ << ": "
                            << "inconsistent state of file object  of file `" << getFileName() << "`";
                        throw std::runtime_error(message.str());
                    }
                }
                // (4) Verify that it is a Byte the represents the starting Byte of a UTF-8 character sequence of length 1.
                if (byte > 0x7F)
                {
                    _current = Traits::error();
                    return;
                }
                // (5) Verify that it is not the zero terminator.
                if ('\0' == byte)
                {
                    _current = Traits::error();
                    return;
                }
                // (6) Propage the Byte to an extended character and store it.
                _current = (Traits::ExtendedType)byte;
            }

            /**
             * @brief
             *  Get the current extended character.
             * @return
             *  the current extended character
             */
            typename Traits::ExtendedType get()
            {
                return _current;
            }

        };
    }
}