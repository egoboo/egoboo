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

/* Egoboo - input.c
 * Keyboard, mouse, and joystick handling code.
 */

#include "input.h"

#if defined(USE_LUA_CONSOLE)
#    include "lua_console.h"
#else
#    include "egoboo_console.h"
#endif

#include "ui.h"
#include "log.h"
#include "network.h"

#include "egoboo_fileutil.h"
#include "egoboo.h"


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint32 input_device_count = 0;

int       scantag_count = 0;
scantag_t scantag[MAXTAG];

mouse_t          mous;
keyboard_t        keyb;
device_joystick_t joy[MAXJOYSTICK];

device_controls_t controls[INPUT_DEVICE_COUNT + MAXJOYSTICK];

int              cursorx = 0;
int              cursory = 0;
bool_t           pressed = bfalse;
bool_t           clicked = bfalse;
bool_t           pending_click = bfalse;
bool_t           mouse_wheel_event = bfalse;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void input_read_mouse();
static void input_read_keyboard();
static void input_read_joystick(Uint16 which);

static void   scantag_reset();
static bool_t scantag_read_one( FILE *fileread );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void input_init()
{
    // BB > initialize the inputs

    int i;

    log_info( "Intializing SDL Joystick... " );
    if ( SDL_InitSubSystem( SDL_INIT_JOYSTICK ) < 0 )
    {
        log_message( "Failed!\n" );
    }
    else
    {
        log_message( "Succeess!\n" );
    }

    // init the keyboard info
    init_scancodes();
    keyb.on        = btrue;
    keyb.count     = 0;
    keyb.state_ptr = NULL;

    // init the mouse info
    memset( &mous, 0, sizeof(mouse_t) );
    mous.on      = btrue;
    mous.sense   = 6;
    mous.sustain = 0.50f;
    mous.cover   = 0.50f;

    // init the joystick info
    for (i = 0; i < MAXJOYSTICK; i++)
    {
        memset( joy + i, 0, sizeof(device_joystick_t) );
        if (i < SDL_NumJoysticks() )
        {
            joy[i].sdl_ptr = SDL_JoystickOpen( i );
            joy[i].on      = (NULL != joy[i].sdl_ptr);
        }
    }
}

//--------------------------------------------------------------------------------------------
void input_read_mouse()
{
    int x, y, b;
    if ( menuactive )
        b = SDL_GetMouseState( &x, &y );
    else
        b = SDL_GetRelativeMouseState( &x, &y );

    mous.x = x; // mous.x and mous.y are the wrong type to use in above call
    mous.y = y;
    mous.button[0] = ( b & SDL_BUTTON( 1 ) ) ? 1 : 0;
    mous.button[1] = ( b & SDL_BUTTON( 3 ) ) ? 1 : 0;
    mous.button[2] = ( b & SDL_BUTTON( 2 ) ) ? 1 : 0; // Middle is 2 on SDL
    mous.button[3] = ( b & SDL_BUTTON( 4 ) ) ? 1 : 0;

    // Mouse mask
    mous.b = ( mous.button[3] << 3 ) | ( mous.button[2] << 2 ) | ( mous.button[1] << 1 ) | ( mous.button[0] << 0 );
}

//--------------------------------------------------------------------------------------------
void input_read_keyboard()
{
    keyb.state_ptr = SDL_GetKeyState( &keyb.count );
}

//--------------------------------------------------------------------------------------------
void input_read_joystick(Uint16 which)
{
    int dead_zone = 0x8000 / 10;
    int i, button_count, x, y;
    device_joystick_t * pjoy;
    if ( which + INPUT_DEVICE_JOY > input_device_count ) return;
    if ( !joy[which].on ) return;

    pjoy = joy + which;

    // get the raw values
    x = SDL_JoystickGetAxis( pjoy->sdl_ptr, 0 );
    y = SDL_JoystickGetAxis( pjoy->sdl_ptr, 1 );

    // make a dead zone
    if ( x > dead_zone ) x -= dead_zone;
    else if ( x < -dead_zone ) x += dead_zone;
    else x = 0;
    if ( y > dead_zone ) y -= dead_zone;
    else if ( y < -dead_zone ) y += dead_zone;
    else y = 0;

    // store the values
    pjoy->x = x / (float)(0x8000 - dead_zone);
    pjoy->y = y / (float)(0x8000 - dead_zone);

    // get buttons
    button_count = SDL_JoystickNumButtons( pjoy->sdl_ptr );
    button_count = MIN(JOYBUTTON, button_count);
    for ( i = 0; i < button_count; i++ )
    {
        pjoy->button[i] = SDL_JoystickGetButton( pjoy->sdl_ptr, i );
    }

    // buttonmask mask
    pjoy->b = 0;
    for ( i = 0; i < button_count; i++ )
    {
        pjoy->b |= ( pjoy->button[i] << i );
    }

    return;
}

//--------------------------------------------------------------------------------------------
void input_read()
{
    // ZZ> This function gets all the current player input states
    int cnt;
    SDL_Event evt;

    // Run through SDL's event loop to get info in the way that we want
    // it for the Gui code
    while ( SDL_PollEvent( &evt ) )
    {

        if ( gDevMode )
        {
            if ( NULL == egoboo_console_handle_events( &evt ) )
            {
                continue;
            }
        }

        ui_handleSDLEvent( &evt );

        switch ( evt.type )
        {
            case SDL_MOUSEBUTTONDOWN:
                if (evt.button.button == SDL_BUTTON_WHEELUP)
                {
                    mous.z++;
                    mouse_wheel_event = btrue;
                }
                else if (evt.button.button == SDL_BUTTON_WHEELDOWN)
                {
                    mous.z--;
                    mouse_wheel_event = btrue;
                }
                else
                {
                    pending_click = btrue;
                }
                break;

            case SDL_MOUSEBUTTONUP:
                pending_click = bfalse;
                break;

                // use this loop to grab any console-mode entry from the keyboard
            case SDL_KEYDOWN:
                {
                    Uint32 kmod;
                    bool_t is_alt, is_shift;

                    kmod = SDL_GetModState();

                    is_alt   = ( 0 != (kmod & (KMOD_ALT | KMOD_CTRL) ) );
                    is_shift = ( 0 != (kmod & KMOD_SHIFT) );
                    if ( console_mode )
                    {
                        if ( !is_alt )
                        {
                            if ( SDLK_RETURN == evt.key.keysym.sym || SDLK_KP_ENTER == evt.key.keysym.sym )
                            {
                                keyb.buffer[keyb.buffer_count] = '\0';
                                console_mode = bfalse;
                                console_done = btrue;
                                SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_DELAY);
                            }
                            else if ( SDLK_ESCAPE == evt.key.keysym.sym )
                            {
                                // reset the keyboard buffer
                                console_mode = bfalse;
                                console_done = bfalse;
                                keyb.buffer_count = 0;
                                keyb.buffer[0] = '\0';
                                SDL_EnableKeyRepeat(0, SDL_DEFAULT_REPEAT_DELAY);
                            }
                            else if ( SDLK_BACKSPACE == evt.key.keysym.sym )
                            {
                                if (keyb.buffer_count > 0)
                                {
                                    keyb.buffer_count--;
                                }
                                keyb.buffer[keyb.buffer_count] = '\0';
                            }
                            else if ( keyb.buffer_count < KEYB_BUFFER_SIZE )
                            {
                                if ( is_shift )
                                {
                                    keyb.buffer[keyb.buffer_count++] = scancode_to_ascii_shift[evt.key.keysym.sym];
                                }
                                else
                                {
                                    keyb.buffer[keyb.buffer_count++] = scancode_to_ascii[evt.key.keysym.sym];
                                }
                                keyb.buffer[keyb.buffer_count] = '\0';
                            }
                        }
                    }
                }
                break;
        }
    }

    // Get immediate mode state for the rest of the game
    input_read_keyboard();
    input_read_mouse();

    SDL_JoystickUpdate();
    for ( cnt = 0; cnt < MAXJOYSTICK; cnt++ )
    {
        input_read_joystick(cnt);
    }
}

//--------------------------------------------------------------------------------------------
Uint32 input_get_buttonmask( Uint32 idevice )
{
    Uint32 buttonmask = 0;
    Uint32 which_device;

    // make sure the idevice is valid
    if ( idevice > input_device_count || idevice > INPUT_DEVICE_COUNT + MAXJOYSTICK ) return 0;
    which_device = controls[idevice].device;
    if ( which_device >= INPUT_DEVICE_JOY )
    {
        // joysticks
        buttonmask = joy[which_device - INPUT_DEVICE_JOY].b;
    }
    else
    {
        switch ( controls[idevice].device )
        {
            case INPUT_DEVICE_KEYBOARD: buttonmask = 0; break;
            case INPUT_DEVICE_MOUSE:    buttonmask = mous.b; break;
        }
    }

    return buttonmask;
}

//--------------------------------------------------------------------------------------------
// Tag Reading---------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void scantag_reset()
{
    // ZZ> This function resets the tags
    scantag_count = 0;
}

//--------------------------------------------------------------------------------------------
bool_t scantag_read_one( FILE *fileread )
{
    // ZZ> This function finds the next tag, returning btrue if it found one

    bool_t retval;

    retval = goto_colon_yesno( fileread ) && (scantag_count < MAXTAG);
    if ( retval )
    {
        fscanf( fileread, "%s%d", scantag[scantag_count].name, &scantag[scantag_count].value );
        scantag_count++;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
void scantag_read_all(  const char *szFilename )
{
    // ZZ> This function reads the scancode.txt file
    FILE* fileread;

    scantag_reset();
    fileread = fopen( szFilename, "r" );
    if ( fileread )
    {
        while ( scantag_read_one( fileread ) );

        fclose( fileread );
    }
}

//--------------------------------------------------------------------------------------------
int scantag_get_value(  const char *string )
{
    // ZZ> This function matches the string with its tag, and returns the value...
    //     It will return 255 if there are no matches.
    int cnt;

    cnt = 0;

    while ( cnt < scantag_count )
    {
        if ( 0 == strcmp( string, scantag[cnt].name ) )
        {
            // They match
            return scantag[cnt].value;
        }

        cnt++;
    }

    // No matches
    return 255;
}

//--------------------------------------------------------------------------------------------
char* scantag_get_string( Sint32 device, Sint32 tag, bool_t is_key )
{
    //ZF> This translates a input tag value to a string
    int cnt;
    if ( device >= INPUT_DEVICE_JOY ) device = INPUT_DEVICE_JOY;
    if ( device == INPUT_DEVICE_KEYBOARD ) is_key = btrue;

    for ( cnt = 0; cnt < scantag_count; cnt++ )
    {
        // do not search invalid keys
        if ( is_key )
        {
            if ( 'K' != scantag[cnt].name[0] ) continue;
        }
        else
        {
            switch ( device )
            {
                case INPUT_DEVICE_MOUSE:
                    if ( 'M' != scantag[cnt].name[0] ) continue;
                    break;

                case INPUT_DEVICE_JOY:
                    if ( 'J' != scantag[cnt].name[0] ) continue;
                    break;
            }
        };
        if ( tag == scantag[cnt].value)
        {
            return scantag[cnt].name;
        }
    }

    // No matches
    return "N/A";
}

//--------------------------------------------------------------------------------------------
bool_t control_is_pressed( Uint32 idevice, Uint8 icontrol )
{
    // ZZ> This function returns btrue if the given icontrol is pressed...

    bool_t retval = bfalse;

    device_controls_t * pdevice;
    control_t         * pcontrol;

    // make sure the idevice is valid
    if ( idevice > input_device_count || idevice > INPUT_DEVICE_COUNT + MAXJOYSTICK ) return bfalse;
    pdevice = controls + idevice;

    // make sure the icontrol is within range
    if ( pdevice->count < icontrol ) return retval;
    pcontrol = pdevice->control + icontrol;
    if ( INPUT_DEVICE_KEYBOARD == idevice || pcontrol->is_key )
    {
        retval = SDLKEYDOWN( pcontrol->tag );
    }
    else
    {
        retval = ( input_get_buttonmask( idevice ) == pcontrol->tag );
    }

    return retval;
}


//--------------------------------------------------------------------------------------------
void reset_players()
{
    // ZZ> This function clears the player list data
    int cnt, tnc;

    // Reset the local data stuff
    local_seekurse     = bfalse;
    local_senseenemies = MAXCHR;
    local_seeinvisible = bfalse;
    local_allpladead    = bfalse;

    // Reset the initial player data and latches
    for ( cnt = 0; cnt < MAXPLAYER; cnt++ )
    {
        PlaList[cnt].valid = bfalse;
        PlaList[cnt].index = 0;
        PlaList[cnt].latchx = 0;
        PlaList[cnt].latchy = 0;
        PlaList[cnt].latchbutton = 0;

        PlaList[cnt].tlatch_count = 0;
        for ( tnc = 0; tnc < MAXLAG; tnc++ )
        {
            PlaList[cnt].tlatch[tnc].x      = 0;
            PlaList[cnt].tlatch[tnc].y      = 0;
            PlaList[cnt].tlatch[tnc].button = 0;
            PlaList[cnt].tlatch[tnc].time   = 0;
        }

        PlaList[cnt].device = INPUT_BITS_NONE;
    }

    numpla = 0;
    nexttimestamp = ((Uint32)~0);
    numplatimes   = 0;
}
