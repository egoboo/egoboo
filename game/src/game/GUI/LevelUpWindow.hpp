#pragma once

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

/// @file game/GUI/LevelUpWindow.hpp
/// @details LevelUpWindow
/// @author Johan Jansen
#pragma once

#include "game/GUI/InternalWindow.hpp"

class Object;

namespace Ego
{
namespace GUI
{

class LevelUpWindow : public InternalWindow
{
    public:
        LevelUpWindow(const std::shared_ptr<Object> &object);
        ~LevelUpWindow();
        
    private:
        std::shared_ptr<Object> _character;
};

} //GUI
} //Ego
