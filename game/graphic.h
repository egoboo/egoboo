#pragma once

#include "ogl_texture.h"

int draw_string( GLtexture * pfnt, float x, float y, GLfloat tint[], char * szFormat, ... );
bool_t draw_texture_box( GLtexture * ptx, FRect * tx_rect, FRect * sc_rect );

void BeginText( GLtexture * pfnt );
void EndText( void );

void Begin2DMode( void );
void End2DMode( void );

