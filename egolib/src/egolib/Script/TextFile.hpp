#pragma once

#include "egolib/Script/Traits.hpp"

namespace Ego
{
    namespace Script
    {
        /**
         * @brief
         *  A text file.
         */
        template <typename Traits = Traits<char>>
        struct TextFile
        {

            /**
             * @brief
             *  The possible modes a text file can be opened with.
             */
            enum class Mode
            {
                /**
                 * @brief
                 *  The text file is opened for reading.
                 */
                Read,
                /**
                 * @brief
                 *  The text file is opened for writing.
                 */
                Write,
            };

            /**
             * @brief
             *  The traits used.
             */
            typedef typename Traits Traits;

        private:

            /**
             * @brief
             *  The file name of the file.
             */
            std::string _fileName;

            /**
             * @brief
             *  The mode the file was opened in.
             */
            Mode _mode;

        protected:

            /**
             * @brief
             *  Construct this text file with the given file name and mode.
             * @param fileName
             *  the file name
             * @param mode
             *  the mode
             */
            TextFile(const std::string& fileName, Mode mode) :
                _fileName(fileName), _mode(mode)
            {
            }

            /**
             * @brief
             *  Destruct this text file.
             */
            virtual ~TextFile()
            {
            }

        public:

            /**
             * @brief
             *  Get the file name of this text file.
             * @return
             *  the file name of this text file
             */
            const std::string& getFileName() const
            {
                return _fileName;
            }

            /**
             * @brief
             *  The mode this text file was opened with.
             * @return
             *  the mode
             */
            Mode getMode() const
            {
                return _mode;
            }

        };

    }
}