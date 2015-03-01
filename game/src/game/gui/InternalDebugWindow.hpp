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

/// @file game/gui/InternalDebugWindow.hpp
/// @details InternalDebugWindow
/// @author Johan Jansen
#pragma once

#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include "game/gui/GUIComponent.hpp"

class InternalDebugWindow : public GUIComponent
{
    public:
        InternalDebugWindow(const std::string &title);

        virtual void draw() override;

        void addWatchVariable(const std::string &variableName, std::function<std::string()> lambda);

        bool notifyMouseMoved(const int x, const int y) override;
        bool notifyMouseClicked(const int InternalDebugWindow, const int x, const int y) override;

        //Disable copying class
        InternalDebugWindow(const InternalDebugWindow& copy) = delete;
        InternalDebugWindow& operator=(const InternalDebugWindow&) = delete;

    private:
        bool _mouseOver;
        bool _mouseOverCloseButton;
        bool _isDragging;
        std::string _title;

        std::unordered_map<std::string, std::function<std::string()>> _watchedVariables;
};
