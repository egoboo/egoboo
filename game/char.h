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

#define SPINRATE            200                     // How fast spinners spin
#define WATCHMIN            0.01f                     //
#define PITDEPTH            -30                     // Depth to kill character
#define VOLMIN          -4000            // Minumum Volume level

#define BORETIME                        (rand()&255)+120
#define CAREFULTIME                     50

#define REEL                            7600.0f      // Dampen for melee knock back
#define REELBASE                        0.35f         //

#define DONTFLASH 255                               //
#define SEEKURSEAND         31                      // Blacking flash
#define RIPPLEAND           15                      // How often ripples spawn

#define RIPPLETOLERANCE     60                      // For deep water
#define SPLASHTOLERANCE     10                      //

// Throwing
#define THROWFIX            30.0f                    // To correct thrown velocities
#define MINTHROWVELOCITY    15.0f                    //
#define MAXTHROWVELOCITY    45.0f                    //

// Inventory
#define MAXNUMINPACK        6                       // Max number of items to carry in pack
#define PACKDELAY           25                      // Time before inventory rotate again
#define GRABDELAY           25                      // Time before grab again

// Z velocity
#define FLYDAMPEN           0.001f                    // Levelling rate for flyers
#define JUMPINFINITE        255                     // Flying character
#define SLIDETOLERANCE      10                      // Stick to ground better
#define PLATTOLERANCE       50 //5 //10             // Platform tolerance...
#define PLATADD             -10                     // Height add...
#define PLATASCEND          0.10f                     // Ascension rate
#define PLATKEEP            0.90f                     // Retention rate

