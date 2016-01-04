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

/// @file game/Shop.hpp
/// @brief Shop interaction
#pragma once

#include "game/egoboo.h"

struct Shop {
private:
    static bool buy(const std::shared_ptr<Object>& buyer, const std::shared_ptr<Object>& item);
    static bool steal(const std::shared_ptr<Object>& thief, const std::shared_ptr<Object>& item);
    static bool canGrabItem(ObjectRef igrabber, ObjectRef iitem);
public:
    static bool drop(const std::shared_ptr<Object>& dropper, const std::shared_ptr<Object>& item);
    static bool canGrabItem(const std::shared_ptr<Object>& grabber, const std::shared_ptr<Object>& item);
};
