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

/// @file egolib/network.h
/// @brief Skeleton for Egoboo networking

#pragma once

#include "egolib/typedef.h"

#define MAXLAG (1 << 6)
#define LAGAND (MAXLAG - 1)
#define SHORTLATCH 1024.0f

    /// A latch with a time attached
    /// @details This is recieved over the network, or inserted into the list by the local system to simulate
    ///  network traffic
    struct time_latch_t
    {
        float   x;
        float   y;
        Uint32  button;
        Uint32  time;
    };

    void tlatch_ary_init(time_latch_t ary[], size_t len);