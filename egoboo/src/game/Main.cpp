//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************
/// @author Johan Jansen
/// @author Michael Heilmann

#include "egolib/game/Core/GameEngine.hpp"
#include "egolib/game/GUI/UIManager.hpp"

/**
 * @brief
 *  The entry point of the program.
 * @param argc
 *  the number of command-line arguments (number of elements in the array pointed by @a argv)
 * @param argv
 *  the command-line arguments (a static constant array of @a argc pointers to static constant zero-terminated strings)
 * @return
 *  EXIT_SUCCESS upon regular termination, EXIT_FAILURE otherwise
 */
int main(int argc, char **argv)
{
    try
    {
        Ego::Core::System::initialize(std::string(argv[0]));
        try
        {
            _gameEngine = std::make_unique<GameEngine>();

            _gameEngine->start();
        }
        catch (...)
        {
            Ego::Core::System::uninitialize();
            std::rethrow_exception(std::current_exception());
		}
		Ego::Core::System::uninitialize();
    }
    catch (const idlib::exception& ex)
    {
        std::cerr << "unhandled exception: " << std::endl
                  << ex.to_string() << std::endl;

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                 "Unhandled Exception",
                                 ex.to_string().c_str(),
                                 nullptr);

        return EXIT_FAILURE;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "unhandled exception: " << std::endl
                  << ex.what() << std::endl;

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                 "Unhandled asException",
                                 ex.what(),
                                 nullptr);

        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "unhandled exception" << std::endl;

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                 "Unhandled Exception",
                                 "Unknown exception type",
                                 nullptr);

        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
