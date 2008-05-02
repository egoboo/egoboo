#pragma once

#include "egoboo_types.inl"

#include <SDL.h>

typedef enum control_list_e
{
  KEY_JUMP = 0,
  KEY_LEFT_USE,
  KEY_LEFT_GET,
  KEY_LEFT_PACK,
  KEY_RIGHT_USE,
  KEY_RIGHT_GET,
  KEY_RIGHT_PACK,
  KEY_MESSAGE,
  KEY_CAMERA_LEFT,
  KEY_CAMERA_RIGHT,
  KEY_CAMERA_IN,
  KEY_CAMERA_OUT,
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,

  MOS_JUMP,
  MOS_LEFT_USE,
  MOS_LEFT_GET,
  MOS_LEFT_PACK,
  MOS_RIGHT_USE,
  MOS_RIGHT_GET,
  MOS_RIGHT_PACK,
  MOS_CAMERA,

  JOA_JUMP,
  JOA_LEFT_USE,
  JOA_LEFT_GET,
  JOA_LEFT_PACK,
  JOA_RIGHT_USE,
  JOA_RIGHT_GET,
  JOA_RIGHT_PACK,
  JOA_CAMERA,

  JOB_JUMP,
  JOB_LEFT_USE,
  JOB_LEFT_GET,
  JOB_LEFT_PACK,
  JOB_RIGHT_USE,
  JOB_RIGHT_GET,
  JOB_RIGHT_PACK,
  JOB_CAMERA,
  CONTROL_LIST_COUNT,

  // !!!! OMG !!!! - these aliases have to be last or they mess up the automatic ordering
  KEY_FIRST = KEY_JUMP,
  MOS_FIRST = MOS_JUMP,
  JOA_FIRST = JOA_JUMP,
  JOB_FIRST = JOB_JUMP,

  KEY_LAST = MOS_FIRST,
  MOS_LAST = JOA_FIRST,
  JOA_LAST = JOB_FIRST,
  JOB_LAST = CONTROL_LIST_COUNT

} CONTROL_LIST;

typedef enum input_type_e
{
  INPUT_MOUS = 0,
  INPUT_KEYB,
  INPUT_JOYA,
  INPUT_JOYB,
  INPUT_COUNT
} INPUT_TYPE;

typedef enum input_bits_e
{
  INBITS_NONE  =               0,                         //
  INBITS_MOUS  = 1 << INPUT_MOUS,                         // Input devices
  INBITS_KEYB  = 1 << INPUT_KEYB,                         //
  INBITS_JOYA  = 1 << INPUT_JOYA,                         //
  INBITS_JOYB  = 1 << INPUT_JOYB                          //
} INPUT_BITS;



typedef enum control_type_e
{
  CONTROL_JUMP = 0,
  CONTROL_LEFT_USE,
  CONTROL_LEFT_GET,
  CONTROL_LEFT_PACK,
  CONTROL_RIGHT_USE,
  CONTROL_RIGHT_GET,
  CONTROL_RIGHT_PACK,
  CONTROL_MESSAGE,
  CONTROL_CAMERA_LEFT,
  CONTROL_CAMERA_RIGHT,
  CONTROL_CAMERA_IN,
  CONTROL_CAMERA_OUT,
  CONTROL_UP,
  CONTROL_DOWN,
  CONTROL_LEFT,
  CONTROL_RIGHT,
  CONTROL_COUNT,

  CONTROL_CAMERA = CONTROL_MESSAGE
} CONTROL;


typedef struct latch_t
{
  float    x;        // x value
  float    y;        // y value
  Uint32   b;        // button(s) mask
} LATCH;

#define MAXPLAYER   (1<<3)                          // 2 to a power...  2^3

extern int               numpla;                                 // Number of players

typedef struct player_t
{
  bool_t            valid;                    // Player used?
  CHR_REF           chr;                    // Which character?
  LATCH             latch;                   // Local latches
  Uint8             device;                   // Input device
} PLAYER;

extern PLAYER PlaList[MAXPLAYER];
CHR_REF pla_get_character( PLA_REF iplayer );

#define VALID_PLA(XX) ( ((XX)>=0) && ((XX)<MAXPLAYER) && PlaList[XX].valid )

#define MOUSEBUTTON         4

typedef struct mouse_t
{
  bool_t   on;                   // Is the mouse alive?
  float    sense;                // Sensitivity threshold
  float    sustain;              // Falloff rate for old movement
  float    cover;                // For falloff
  LATCH    latch;
  LATCH    latch_old;            // For sustain
  LATCH    dlatch;
  Sint32   z;                    // Mouse wheel movement counter
  Uint8    button[MOUSEBUTTON];  // Mouse button states
} MOUSE;

extern MOUSE mous;

#define JOYBUTTON           8                       // Maximum number of joystick buttons

typedef struct joystick_t
{
  SDL_Joystick *sdl_device;
  bool_t        on;                     // Is the holy joystick alive?
  LATCH         latch;                  //
  Uint8         button[JOYBUTTON];      //
} JOYSTICK;

extern JOYSTICK joy[2];

// SDL specific declarations
typedef struct keyboard_t
{
  //char     buffer[256];        // Keyboard key states
  //char     press[256];         // Keyboard new hits
  bool_t   on ;                // Is the keyboard alive?
  Uint8   *state;
  LATCH    latch;
} KEYBOARD;

extern KEYBOARD keyb;

#define SDLKEYDOWN(k) (NULL!=keyb.state && 0!=keyb.state[k])


//Tags
void read_all_tags( char *szFilename );
void read_controls( char *szFilename );

bool_t control_key_is_pressed( CONTROL control );
bool_t control_mouse_is_pressed( CONTROL control );
bool_t control_joy_is_pressed( int joy_num, CONTROL control );

void input_setup();

bool_t read_mouse(MOUSE * pm);

bool_t read_key(KEYBOARD * pk);
bool_t reset_press(KEYBOARD * pk);

bool_t read_joystick(JOYSTICK * pj);


