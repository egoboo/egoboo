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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

#define FOV                             60          // Field of view
#define CAMJOYTURN                      (0.01f*6)         // Joystick camera rotation
#define CAMKEYTURN                      (10*6)          // Keyboard camera rotation
#define FARTRACK            1200                    // For outside modules...
#define EDGETRACK           800                     // Camtrack bounds
#define TURNTIME 16              // Smooth turn


// Multi cam
#define MINZOOM                         500         // Camera distance
#define MAXZOOM                         600         //
#define MINZADD                         800         // Camera height
#define MAXZADD                         1500  //1000        //
#define MINUPDOWN                       (0.24f*PI)    // Camera updown angle
#define MAXUPDOWN                       (0.18f*PI)//(0.15f*PI) // (0.18f*PI)
