#pragma once

#include "ogl_include.h"

#define FOV                             40          // Field of view
#define CAMKEYTURN                      5           // Keyboard camera rotation
#define CAMJOYTURN                      5           // Joystick camera rotation

// Multi cam
#define MINZOOM                         100         // Camera distance
#define MAXZOOM                         600         //
#define MINZADD                         100         // Camera height
#define MAXZADD                         3000

//Camera control stuff

typedef struct camera_t
{
  int       swing;           // Camera swingin'
  int       swingrate;       //
  float     swingamp;        //
  vect3     pos;             // Camera position
  float     zoom;            // Distance from the trackee
  vect3     trackvel;        // Change in trackee position
  vect3     centerpos;       // Move character to side before tracking
  vect3     trackpos;        // Trackee position
  float     tracklevel;      //
  float     zadd;            // Camera height above terrain
  float     zaddgoto;        // Desired z position
  float     zgoto;           //
  float     turn_lr;         // Camera rotations
  float     turn_lr_one;
  float     turnadd;         // Turning rate
  float     sustain;         // Turning rate falloff
  float     roll;            //
  GLmatrix  mView;           // View Matrix
  GLmatrix  mProjection;     // Projection Matrix
  GLmatrix  mProjectionBig;  // Larger projection matrix for frustum culling
} CAMERA;

extern CAMERA GCamera;

extern float                   cornerx[4];                 // Render area corners
extern float                   cornery[4];                 //
extern int                     cornerlistlowtohighy[4];    // Ordered list
extern int                     cornerlowx;                 // Render area extremes
extern int                     cornerhighx;                //
extern int                     cornerlowy;                 //
extern int                     cornerhighy;                //

