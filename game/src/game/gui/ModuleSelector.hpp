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

/// @file game/gui/ModuleSelector.cpp
/// @details GUI widget to select which module to play
/// @author Johan Jansen
#pragma once

#include "game/gui/GUIComponent.hpp"
#include "game/gui/ComponentContainer.hpp"

class ModuleSelector : public GUIComponent, public ComponentContainer
{
    public:
        ModuleSelector();

        void draw() override { drawAll(); }

        //Disable copying class
        ModuleSelector(const ModuleSelector& copy) = delete;
        ModuleSelector& operator=(const ModuleSelector&) = delete;

    protected:
	    void drawContainer() override;

    private:
        size_t _startIndex;
};
