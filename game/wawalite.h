#pragma once

#include "egoboo_typedef.h"

#define MAXWATERLAYER 2                             // Maximum water layers

// a wrapper for the wawalite.txt file
struct s_wawalite_water_layer
{
    Uint32  frame_add;      // Speed

    float   z;                // Base height of water
    float   amp;            // Amplitude of waves

    vec2f_t dist;            // For distant backgrounds
    Uint32  light_dir;        // direct  reflectivity 0 - 63
    Uint32  light_add;        // ambient reflectivity 0 - 63

    vec2f_t tx_add;            // Texture movement
    Uint8   alpha;            // Transparency
};
typedef struct s_wawalite_water_layer wawalite_water_layer_t;

struct s_wawalite_water
{
    int                    layer_count;
    wawalite_water_layer_t layer[MAXWATERLAYER];

    float  surface_level;    // Surface level for water striders
    float  douse_level;         // Surface level for torches
    Uint8  spek_start;         // Specular begins at which light value
    Uint8  spek_level;         // General specular amount (0-255)
    bool_t is_water;         // Is it water?  ( Or lava... )
    bool_t overlay_req;
    bool_t background_req;

    bool_t light;            // Is it light ( default is alpha )

    float  foregroundrepeat;
    float  backgroundrepeat;
};
typedef struct s_wawalite_water wawalite_water_t;

struct s_wawalite_physics
{
    float hillslide;
    float slippyfriction;
    float airfriction;
    float waterfriction;
    float noslipfriction;
    float gravity;
};
typedef struct s_wawalite_physics wawalite_physics_t;

struct s_wawalite_animtile
{
    Uint32 update_and;
    Uint32 frame_and;
};
typedef struct s_wawalite_animtile wawalite_animtile_t;

struct s_wawalite_damagetile
{
    Uint32 amount;
    int    type;

    int    parttype;
    Uint32 partand;
    int    sound;
};
typedef struct s_wawalite_damagetile wawalite_damagetile_t;

struct s_wawalite_weather
{
    bool_t over_water;
    int    timer_reset;
};
typedef struct s_wawalite_weather wawalite_weather_t;

struct s_wawalite_graphics
{
    bool_t exploremode;
    bool_t usefaredge;
};
typedef struct s_wawalite_graphics wawalite_graphics_t;

struct s_wawalite_camera
{
    bool_t swing;
    float  swingrate;
    float  swingamp;
};
typedef struct s_wawalite_camera wawalite_camera_t;

struct s_wawalite_fog
{
    bool_t found;
    float  top;
    float  bottom;
    float  red;
    float  grn;
    float  blu;
    bool_t affects_water;
};
typedef struct s_wawalite_fog wawalite_fog_t;

struct s_wawalite_data
{
    Uint32 seed;

    wawalite_water_t      water;
    wawalite_physics_t    phys;
    wawalite_animtile_t   animtile;
    wawalite_damagetile_t damagetile;

    wawalite_weather_t    weather;
    wawalite_graphics_t   graphics;
    wawalite_camera_t     camera;
    wawalite_fog_t        fog;

    float light_x;
    float light_y;
    float light_z;
    float light_a;
};

typedef struct s_wawalite_data wawalite_data_t;

extern wawalite_data_t wawalite_data;

bool_t            write_wawalite( const char *modname, wawalite_data_t * pdata );
wawalite_data_t * read_wawalite ( const char *modname, wawalite_data_t * pdata );
