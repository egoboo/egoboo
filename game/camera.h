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

#include "egoboo_typedef.h"
#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define TRACKXAREALOW     100
#define TRACKXAREAHIGH    180
#define TRACKYAREAMINLOW  320
#define TRACKYAREAMAXLOW  460
#define TRACKYAREAMINHIGH 460
#define TRACKYAREAMAXHIGH 600

#define FOV                             60           // Field of view
#define CAMJOYTURN                      (0.01f*3)    // Joystick camera rotation
#define CAMKEYTURN                      (10*3)       // Keyboard camera rotation
#define FARTRACK                        1200         // For outside modules...
#define EDGETRACK                       800          // Camtrack bounds
#define TURNTIME                        16           // Smooth turn

// Multi cam (uses macro to switch between old and new camera
#ifndef OLD_CAMERA_MODE
#    define MINZOOM                         800         // Camera distance
#    define MAXZOOM                         700         //
#    define MINZADD                         800         // Camera height
#    define MAXZADD                         2500        //
#    define MINUPDOWN                       (0.24f*PI)    // Camera updown angle
#    define MAXUPDOWN                       (0.10f*PI)
#else
#    define MINZOOM                         500         // Camera distance
#    define MAXZOOM                         600         //
#    define MINZADD                         800         // Camera height
#    define MAXZADD                         1500  //1000        //
#    define MINUPDOWN                       (0.24f*PI)    // Camera updown angle
#    define MAXUPDOWN                       (0.18f*PI)//(0.15f*PI) // (0.18f*PI)
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_camera
{
    glMatrix mWorld;                       // World Matrix
    glMatrix mView;                        // View Matrix
    glMatrix mViewSave;                    // View Matrix initial state
    glMatrix mProjection;                  // Projection Matrix

    int    swing;                   // Camera swingin'
    int    swingrate;               //
    float  swingamp;                //
    float  x;                       // Camera position
    float  y;                       //
    float  z;                       // 500-1000
    float  zoom;                    // Distance from the trackee
    float  trackxvel;               // Change in trackee position
    float  trackyvel;               //
    float  trackzvel;               //
    float  centerx;                 // Move character to side before tracking
    float  centery;                 //
    float  trackx;                  // Trackee position
    float  tracky;                  //
    float  trackz;                  //
    float  tracklevel;              //
    float  zadd;                    // Camera height above terrain
    float  zaddgoto;                // Desired z position
    float  zgoto;                   //
    float  turnleftright;           // Camera rotations
    float  turnleftrightone;
    Uint16 turnleftrightshort;
    float  turnadd;                 // Turning rate
    float  sustain;                 // Turning rate falloff
    float  turnupdown;
    float  roll;                    //
};

typedef struct s_camera camera_t;

extern camera_t gCamera;

//Function prototypes
camera_t * camera_new( camera_t * pcam );

void reset_camera( camera_t * pcam );
void adjust_camera_angle( camera_t * pcam, float height );
void move_camera( camera_t * pcam );
void make_camera_matrix( camera_t * pcam );
void camera_look_at( camera_t * pcam, float x, float y );
