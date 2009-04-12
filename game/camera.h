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
#define CAMJOYTURN                      (0.01f*3)         // Joystick camera rotation
#define CAMKEYTURN                      (10*3)          // Keyboard camera rotation
#define FARTRACK            1200                    // For outside modules...
#define EDGETRACK           800                     // Camtrack bounds
#define TURNTIME 16              // Smooth turn

// Multi cam (uses macro to switch between old and new camera
#ifndef OLD_CAMERA_MODE
#define MINZOOM                         800         // Camera distance
#define MAXZOOM                         700         //
#define MINZADD                         800         // Camera height
#define MAXZADD                         2500        //
#define MINUPDOWN                       (0.24f*PI)    // Camera updown angle
#define MAXUPDOWN                       (0.10f*PI)
#else
#define MINZOOM                         500         // Camera distance
#define MAXZOOM                         600         //
#define MINZADD                         800         // Camera height
#define MAXZADD                         1500  //1000        //
#define MINUPDOWN                       (0.24f*PI)    // Camera updown angle
#define MAXUPDOWN                       (0.18f*PI)//(0.15f*PI) // (0.18f*PI)
#endif
// Camera control stuff
extern int                     camswing;                   // Camera swingin'
extern int                     camswingrate;               //
extern float                   camswingamp;                //
extern float                   camx;                       // Camera position
extern float                   camy;                       //
extern float                   camz;                       // 500-1000
extern float                   camzoom;                    // Distance from the trackee
extern float                   camtrackxvel;               // Change in trackee position
extern float                   camtrackyvel;               //
extern float                   camtrackzvel;               //
extern float                   camcenterx;                 // Move character to side before tracking
extern float                   camcentery;                 //
extern float                   camtrackx;                  // Trackee position
extern float                   camtracky;                  //
extern float                   camtrackz;                  //
extern float                   camtracklevel;              //
extern float                   camzadd;                    // Camera height above terrain
extern float                   camzaddgoto;                // Desired z position
extern float                   camzgoto;                   //
extern float                   camturnleftright;           // Camera rotations
extern float                   camturnleftrightone;
extern Uint16                  camturnleftrightshort;
extern float                   camturnadd;                 // Turning rate
extern float                   camsustain;                 // Turning rate falloff
extern float                   camturnupdown;
extern float                   camroll;                    //

extern float                   cornerx[4];                 // Render area corners
extern float                   cornery[4];                 //
extern int                     cornerlistlowtohighy[4];    // Ordered list
extern float                   cornerlowx;                 // Render area extremes
extern float                   cornerhighx;                //
extern float                   cornerlowy;                 //
extern float                   cornerhighy;                //

//Function prototypes
void project_view();

void reset_camera();
void adjust_camera_angle( float height );
void move_camera();
void make_camera_matrix();
void camera_look_at( float x, float y );