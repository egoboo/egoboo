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

/// @file game/GUI/CharacterStatus.hpp
/// @author Johan Jansen

#pragma once

#include "game/GUI/GUIComponent.hpp"

class Object;
namespace GUI { class ProgressBar; }

class CharacterStatus : public GUIComponent
{
public:
    CharacterStatus(const std::shared_ptr<Object> &object);

    virtual void draw() override;

    std::shared_ptr<Object> getObject() const { return _object.lock(); }

private:
    std::weak_ptr<Object> _object;
    std::shared_ptr<GUI::ProgressBar> _chargeBar;
};
