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

/* Egoboo - Ui.c
 * A basic library for implementing user interfaces, based off of Casey Muratori's
 * IMGUI.  (https:// mollyrocket.com/forums/viewtopic.php?t=134)
 */

#include "ui.h"
#include "graphic.h"
#include "egoboo.h"

#include "ogl_debug.h"
#include "SDL_extensions.h"

#include <string.h>
#include <SDL_opengl.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct UiContext
{
    // Tracking control focus stuff
    ui_id_t active;
    ui_id_t hot;

    // Basic mouse state
    int mouseX, mouseY;
    int mouseReleased;
    int mousePressed;

    Font *defaultFont;
    Font *activeFont;
};

static struct UiContext ui_context;

GLfloat ui_white_color[]  = {1.00f, 1.00f, 1.00f, 1.00f};

GLfloat ui_active_color[]  = {0.00f, 0.00f, 0.90f, 0.60f};
GLfloat ui_hot_color[]     = {0.54f, 0.00f, 0.00f, 1.00f};
GLfloat ui_normal_color[]  = {0.66f, 0.00f, 0.00f, 0.60f};

GLfloat ui_active_color2[] = {0.00f, 0.45f, 0.45f, 0.60f};
GLfloat ui_hot_color2[]    = {0.00f, 0.28f, 0.28f, 1.00f};
GLfloat ui_normal_color2[] = {0.33f, 0.00f, 0.33f, 0.60f};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Core functions
int ui_initialize( const char *default_font, int default_font_size )
{
    // initialize the font handler
    fnt_init();

    memset( &ui_context, 0, sizeof( ui_context ) );

    ui_context.active = ui_context.hot = UI_Nothing;
    ui_context.defaultFont = fnt_loadFont( default_font, default_font_size );

    return 1;
}

//--------------------------------------------------------------------------------------------
void ui_shutdown()
{
    if ( ui_context.defaultFont )
    {
        fnt_freeFont( ui_context.defaultFont );
    }

    memset( &ui_context, 0, sizeof( ui_context ) );
}

//--------------------------------------------------------------------------------------------
void ui_Reset()
{
    ui_context.active = ui_context.hot = UI_Nothing;
}

//--------------------------------------------------------------------------------------------
void ui_handleSDLEvent( SDL_Event *evt )
{
    if ( evt )
    {
        switch ( evt->type )
        {
            case SDL_MOUSEBUTTONDOWN:
                ui_context.mouseReleased = 0;
                ui_context.mousePressed = 1;

                break;

            case SDL_MOUSEBUTTONUP:
                ui_context.mousePressed = 0;
                ui_context.mouseReleased = 1;

                break;

            case SDL_MOUSEMOTION:
                ui_context.mouseX = evt->motion.x;
                ui_context.mouseY = evt->motion.y;

                break;
        }
    }
}

//--------------------------------------------------------------------------------------------
void ui_beginFrame( float deltaTime )
{
    ATTRIB_PUSH( "ui_beginFrame", GL_ENABLE_BIT );
    GL_DEBUG(glDisable)(GL_DEPTH_TEST );
    GL_DEBUG(glDisable)(GL_CULL_FACE );
    GL_DEBUG(glEnable)(GL_TEXTURE_2D );

    GL_DEBUG(glEnable)(GL_BLEND );
    GL_DEBUG(glBlendFunc)(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    GL_DEBUG(glViewport)(0, 0, sdl_scr.x, sdl_scr.y );

    // Set up an ortho projection for the gui to use.  Controls are free to modify this
    // later, but most of them will need this, so it's done by default at the beginning
    // of a frame
    GL_DEBUG(glMatrixMode)(GL_PROJECTION );
    GL_DEBUG(glPushMatrix)();
    GL_DEBUG(glLoadIdentity)();
    GL_DEBUG(glOrtho)(0, sdl_scr.x, sdl_scr.y, 0, -1, 1 );

    GL_DEBUG(glMatrixMode)(GL_MODELVIEW );
    GL_DEBUG(glLoadIdentity)();

    // hotness gets reset at the start of each frame
    ui_context.hot = UI_Nothing;
}

//--------------------------------------------------------------------------------------------
void ui_endFrame()
{
    // Restore the OpenGL matrices to what they were
    GL_DEBUG(glMatrixMode)(GL_PROJECTION );
    GL_DEBUG(glPopMatrix)();

    GL_DEBUG(glMatrixMode)(GL_MODELVIEW );
    GL_DEBUG(glLoadIdentity)();

    // Re-enable any states disabled by gui_beginFrame
    ATTRIB_POP( "ui_endFrame" );

    // Clear input states at the end of the frame
    ui_context.mousePressed = ui_context.mouseReleased = 0;
}

//--------------------------------------------------------------------------------------------
// Utility functions
int ui_mouseInside( int x, int y, int width, int height )
{
    int right, bottom;
    right = x + width;
    bottom = y + height;
    if ( x <= ui_context.mouseX && y <= ui_context.mouseY && ui_context.mouseX <= right && ui_context.mouseY <= bottom )
    {
        return 1;
    }

    return 0;
}

//--------------------------------------------------------------------------------------------
void ui_setactive( ui_id_t id )
{
    ui_context.active = id;
}

//--------------------------------------------------------------------------------------------
void ui_sethot( ui_id_t id )
{
    ui_context.hot = id;
}

//--------------------------------------------------------------------------------------------
void ui_setWidgetactive( ui_Widget_t * pw )
{
    if ( NULL == pw )
    {
        ui_context.active = UI_Nothing;
    }
    else
    {
        ui_context.active = pw->id;

        pw->timeout = SDL_GetTicks() + 100;
        if ( 0 != ( pw->mask & UI_BITS_CLICKED ) )
        {
            // use exclusive or to flip the bit
            pw->state ^= UI_BITS_CLICKED;
        };
    };
}

//--------------------------------------------------------------------------------------------
void ui_setWidgethot( ui_Widget_t * pw )
{
    if ( NULL == pw )
    {
        ui_context.hot = UI_Nothing;
    }
    else if (( ui_context.active == pw->id || ui_context.active == UI_Nothing ) )
    {
        if ( pw->timeout < SDL_GetTicks() )
        {
            pw->timeout = SDL_GetTicks() + 100;

            if ( 0 != ( pw->mask & UI_BITS_MOUSEOVER ) && ui_context.hot != pw->id )
            {
                // use exclusive or to flip the bit
                pw->state ^= UI_BITS_MOUSEOVER;
            };
        };

        // Only allow hotness to be set if this control, or no control is active
        ui_context.hot = pw->id;
    }
}

//--------------------------------------------------------------------------------------------
Font* ui_getFont()
{
    return ( ui_context.activeFont != NULL ) ? ui_context.activeFont : ui_context.defaultFont;
}

//--------------------------------------------------------------------------------------------
// Behaviors
ui_buttonValues ui_buttonBehavior( ui_id_t id, int x, int y, int width, int height )
{
    ui_buttonValues result = BUTTON_NOCHANGE;

    // If the mouse is over the button, try and set hotness so that it can be cursor_clicked
    if ( ui_mouseInside( x, y, width, height ) )
    {
        ui_sethot( id );
    }

    // Check to see if the button gets cursor_clicked on
    if ( ui_context.active == id )
    {
        if ( ui_context.mouseReleased == 1 )
        {
            if ( ui_context.hot == id ) result = BUTTON_UP;

            ui_setactive( UI_Nothing );
        }
    }
    else if ( ui_context.hot == id )
    {
        if ( ui_context.mousePressed == 1 )
        {
            if ( ui_context.hot == id ) result = BUTTON_DOWN;

            ui_setactive( id );
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------
ui_buttonValues ui_WidgetBehavior( ui_Widget_t * pWidget )
{
    ui_buttonValues result = BUTTON_NOCHANGE;

    // If the mouse is over the button, try and set hotness so that it can be cursor_clicked
    if ( ui_mouseInside( pWidget->x, pWidget->y, pWidget->width, pWidget->height ) )
    {
        ui_setWidgethot( pWidget );
    }

    // Check to see if the button gets cursor_clicked on
    if ( ui_context.active == pWidget->id )
    {
        if ( ui_context.mouseReleased == 1 )
        {
            // mouse button up
            if ( ui_context.active == pWidget->id ) result = BUTTON_UP;

            ui_setWidgetactive( NULL );
        }
    }
    else if ( ui_context.hot == pWidget->id )
    {
        if ( ui_context.mousePressed == 1 )
        {
            // mouse button down
            if ( ui_context.hot == pWidget->id ) result = BUTTON_DOWN;

            ui_setWidgetactive( pWidget );
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------
// Drawing
void ui_drawButton( ui_id_t id, int x, int y, int width, int height, GLfloat * pcolor )
{
    GLfloat color_1[4] = { 0.0f, 0.0f, 0.9f, 0.6f };
    GLfloat color_2[4] = { 0.54f, 0.0f, 0.0f, 1.0f };
    GLfloat color_3[4] = { 0.66f, 0.0f, 0.0f, 0.6f };

    // Draw the button
    GL_DEBUG(glDisable)(GL_TEXTURE_2D );

    if ( NULL == pcolor )
    {
        if ( ui_context.active != UI_Nothing && ui_context.active == id && ui_context.hot == id )
        {
            pcolor = color_1;
        }
        else if ( ui_context.hot != UI_Nothing && ui_context.hot == id )
        {
            pcolor = color_2;
        }
        else
        {
            pcolor = color_3;
        }
    }

    GL_DEBUG(glColor4fv)( pcolor );
    GL_DEBUG(glBegin)( GL_QUADS );
    {
        GL_DEBUG(glVertex2f)(x, y );
        GL_DEBUG(glVertex2f)(x, y + height );
        GL_DEBUG(glVertex2f)(x + width, y + height );
        GL_DEBUG(glVertex2f)(x + width, y );
    }
    GL_DEBUG_END();

    GL_DEBUG(glEnable)( GL_TEXTURE_2D );
}

//--------------------------------------------------------------------------------------------
void ui_drawImage( ui_id_t id, oglx_texture *img, int x, int y, int width, int height )
{
    int w, h;
    float x1, y1;
    if ( img )
    {
        if ( width == 0 || height == 0 )
        {
            w = img->imgW;
            h = img->imgH;
        }
        else
        {
            w = width;
            h = height;
        }

        x1 = ( float ) oglx_texture_GetImageWidth( img )  / ( float ) oglx_texture_GetTextureWidth( img );
        y1 = ( float ) oglx_texture_GetImageHeight( img ) / ( float ) oglx_texture_GetTextureHeight( img );

        // Draw the image
        oglx_texture_Bind( img );

        GL_DEBUG(glBegin)( GL_QUADS );
        {
            GL_DEBUG(glTexCoord2f)( 0,  0 );  GL_DEBUG(glVertex2f)(x,     y );
            GL_DEBUG(glTexCoord2f)(x1,  0 );  GL_DEBUG(glVertex2f)(x + w, y );
            GL_DEBUG(glTexCoord2f)(x1, y1 );  GL_DEBUG(glVertex2f)(x + w, y + h );
            GL_DEBUG(glTexCoord2f)( 0, y1 );  GL_DEBUG(glVertex2f)(x,     y + h );
        }
        GL_DEBUG_END();
    }
}

//--------------------------------------------------------------------------------------------
void ui_drawWidgetButton( ui_Widget_t * pw )
{
    GLfloat * pcolor = NULL;
    bool_t bactive, bhot;

    bactive = ui_context.active == pw->id && ui_context.hot == pw->id;
    bactive = bactive || 0 != ( pw->mask & pw->state & UI_BITS_CLICKED );
    bhot    = ui_context.hot == pw->id;
    bhot    = bhot || 0 != ( pw->mask & pw->state & UI_BITS_MOUSEOVER );

    if ( 0 != pw->mask )
    {
        if ( bactive )
        {
            pcolor = ui_normal_color2;
        }
        else if ( bhot )
        {
            pcolor = ui_hot_color;
        }
        else
        {
            pcolor = ui_normal_color;
        }
    }
    else
    {
        if ( bactive )
        {
            pcolor = ui_active_color;
        }
        else if ( bhot )
        {
            pcolor = ui_hot_color;
        }
        else
        {
            pcolor = ui_normal_color;
        }
    }

    ui_drawButton( pw->id, pw->x, pw->y, pw->width, pw->height, pcolor );
}

//--------------------------------------------------------------------------------------------
void ui_drawWidgetImage( ui_Widget_t * pw )
{
    if ( NULL != pw && NULL != pw->img )
    {
        ui_drawImage( pw->id, pw->img, pw->x, pw->y, pw->width, pw->height );
    }
}

//--------------------------------------------------------------------------------------------
/** ui_drawTextBox
 * Draws a text string into a box, splitting it into lines according to newlines in the string.
 * NOTE: Doesn't pay attention to the width/height arguments yet.
 *
 * text    - The text to draw
 * x       - The x position to start drawing at
 * y       - The y position to start drawing at
 * width   - Maximum width of the box (not implemented)
 * height  - Maximum height of the box (not implemented)
 * spacing - Amount of space to move down between lines. (usually close to your font size)
 */
void ui_drawTextBox( const char *text, int x, int y, int width, int height, int spacing )
{
    Font *font = ui_getFont();
    fnt_drawTextBox( font, text, x, y, width, height, spacing );
}

//--------------------------------------------------------------------------------------------
// Controls
ui_buttonValues ui_doButton( ui_id_t id, const char *text, int x, int y, int width, int height )
{
    ui_buttonValues result;
    int text_w, text_h;
    int text_x, text_y;
    Font *font;

    // Do all the logic type work for the button
    result = ui_buttonBehavior( id, x, y, width, height );

    // Draw the button part of the button
    ui_drawButton( id, x, y, width, height, NULL );

    // And then draw the text that goes on top of the button
    font = ui_getFont();
    if ( NULL != font && NULL != text && '\0' != text[0] )
    {
        // find the width & height of the text to be drawn, so that it can be centered inside
        // the button
        fnt_getTextSize( font, text, &text_w, &text_h );

        text_x = ( width - text_w ) / 2 + x;
        text_y = ( height - text_h ) / 2 + y;

        GL_DEBUG(glColor3f)(1, 1, 1 );
        fnt_drawText( font, text_x, text_y, text );
    }

    return result;
}

//--------------------------------------------------------------------------------------------
ui_buttonValues ui_doImageButton( ui_id_t id, oglx_texture *img, int x, int y, int width, int height )
{
    ui_buttonValues result;

    // Do all the logic type work for the button
    result = ui_buttonBehavior( id, x, y, width, height );

    // Draw the button part of the button
    ui_drawButton( id, x, y, width, height, NULL );

    // And then draw the image on top of it
    GL_DEBUG(glColor3f)(1, 1, 1 );
    ui_drawImage( id, img, x + 5, y + 5, width - 10, height - 10 );

    return result;
}

//--------------------------------------------------------------------------------------------
ui_buttonValues ui_doImageButtonWithText( ui_id_t id, oglx_texture *img, const char *text, int x, int y, int width, int height )
{
    ui_buttonValues result;

    Font *font;
    int text_x, text_y;
    int text_w, text_h;

    // Do all the logic type work for the button
    result = ui_buttonBehavior( id, x, y, width, height );

    // Draw the button part of the button
    ui_drawButton( id, x, y, width, height, NULL );

    // Draw the image part
    GL_DEBUG(glColor3f)(1, 1, 1 );
    ui_drawImage( id, img, x + 5, y + 5, 0, 0 );

    // And draw the text next to the image
    // And then draw the text that goes on top of the button
    font = ui_getFont();
    if ( font )
    {
        // find the width & height of the text to be drawn, so that it can be centered inside
        // the button
        fnt_getTextSize( font, text, &text_w, &text_h );

        text_x = img->imgW + 10 + x;
        text_y = ( height - text_h ) / 2 + y;

        GL_DEBUG(glColor3f)(1, 1, 1 );
        fnt_drawText( font, text_x, text_y, text );
    }

    return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ui_buttonValues ui_doWidget( ui_Widget_t * pw )
{
    ui_buttonValues result;

    int text_x, text_y;
    int text_w, text_h;
    int img_w;

    // Do all the logic type work for the button
    result = ui_WidgetBehavior( pw );

    // Draw the button part of the button
    ui_drawWidgetButton( pw );

    // draw any image on the left hand side of the button
    img_w = 0;
    if ( NULL != pw->img )
    {
        ui_Widget_t wtmp;

        // Draw the image part
        GL_DEBUG(glColor3f)(1, 1, 1 );

        ui_shrinkWidget( &wtmp, pw, 5 );
        wtmp.width = wtmp.height;

        ui_drawWidgetImage( &wtmp );

        img_w = pw->img->imgW;
    }

    // And draw the text on the right hand side of any image
    if ( NULL != pw->pfont && NULL != pw->text && '\0' != pw->text[0] )
    {
        GL_DEBUG(glColor3f)(1, 1, 1 );

        // find the pw->width & pw->height of the pw->text to be drawn, so that it can be centered inside
        // the button
        fnt_getTextSize( pw->pfont, pw->text, &text_w, &text_h );

        text_w = MIN(text_w, pw->width );
        text_h = MIN(text_h, pw->height);

        text_x = ( pw->width  - text_w ) / 2 + pw->x;
        text_y = ( pw->height - text_h ) / 2 + pw->y;

        text_x = img_w + ( pw->width - img_w - text_w ) / 2 + pw->x;
        text_y = ( pw->height - text_h ) / 2 + pw->y;

        GL_DEBUG(glColor3f)(1, 1, 1 );
        fnt_drawText( pw->pfont, text_x, text_y, pw->text );
    }

    return result;
}

//--------------------------------------------------------------------------------------------
bool_t ui_copyWidget( ui_Widget_t * pw2, ui_Widget_t * pw1 )
{
    if ( NULL == pw2 || NULL == pw1 ) return bfalse;
    return NULL != memcpy( pw2, pw1, sizeof( ui_Widget_t ) );
}

//--------------------------------------------------------------------------------------------
bool_t ui_shrinkWidget( ui_Widget_t * pw2, ui_Widget_t * pw1, int pixels )
{
    if ( NULL == pw2 || NULL == pw1 ) return bfalse;

    if ( !ui_copyWidget( pw2, pw1 ) ) return bfalse;

    pw2->x += pixels;
    pw2->y += pixels;
    pw2->width  -= 2 * pixels;
    pw2->height -= 2 * pixels;

    if ( pw2->width < 0 )  pw2->width   = 0;
    if ( pw2->height < 0 ) pw2->height = 0;

    return pw2->width > 0 && pw2->height > 0;
}

//--------------------------------------------------------------------------------------------
bool_t ui_initWidget( ui_Widget_t * pw, ui_id_t id, Font * pfont, const char *text, oglx_texture *img, int x, int y, int width, int height )
{
    if ( NULL == pw ) return bfalse;

    pw->id      = id;
    pw->pfont   = pfont;
    pw->text    = text;
    pw->img     = img;
    pw->x       = x;
    pw->y       = y;
    pw->width   = width;
    pw->height  = height;
    pw->state   = 0;
    pw->mask    = 0;
    pw->timeout = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ui_widgetAddMask( ui_Widget_t * pw, Uint32 mbits )
{
    if ( NULL == pw ) return bfalse;

    pw->mask  |= mbits;
    pw->state &= ~mbits;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ui_widgetRemoveMask( ui_Widget_t * pw, Uint32 mbits )
{
    if ( NULL == pw ) return bfalse;

    pw->mask  &= ~mbits;
    pw->state &= ~mbits;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ui_widgetSetMask( ui_Widget_t * pw, Uint32 mbits )
{
    if ( NULL == pw ) return bfalse;

    pw->mask   = mbits;
    pw->state &= ~mbits;

    return btrue;
}
