/* Egoboo - gltexture.c
 * Loads BMP files into OpenGL textures.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
*/

#include "egoboo.h" // GAC - Needed for Win32 stuff
#include "gltexture.h"
#include "graphic.h" 


/********************> GLTexture_Load() <*****/
void    GLTexture_Load( GLTexture *texture, const char *filename )
{

  SDL_Surface  *tempSurface, *imageSurface;

  /* Load the bitmap into an SDL_Surface */
  imageSurface = SDL_LoadBMP( filename );

  /* Make sure a valid SDL_Surface was returned */
  if ( imageSurface != NULL )
  {
    /* Generate an OpenGL texture */
    glGenTextures( 1, &texture->textureID );

    /* Set up some parameters for the format of the OpenGL texture */
    glBindTexture( GL_TEXTURE_2D, texture->textureID );          /* Bind Our Texture */

    // Check out which type of filter to use
    switch ( texturefilter )
    {
        // Linear filtered
      case 1:
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        break;

        // Bilinear filtered
      case 2:
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
        break;

        // Trilinear filtered
      case 3:
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
        break;

        // Anisotropic filtered!
      case 4:
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy );
        break;
    }

    /* Set the original image's size (incase it's not an exact square of a power of two) */
    texture->imgHeight = imageSurface->h;
    texture->imgWidth = imageSurface->w;

    /* Determine the correct power of two greater than or equal to the original image's size */
    texture->txDimensions = 2;
    while ( ( texture->txDimensions < texture->imgHeight ) && ( texture->txDimensions < texture->imgWidth ) )
      texture->txDimensions *= 2;

    /* Set the texture's alpha */
    texture->alpha = 1;

    /* Create a blank SDL_Surface (of the smallest size to fit the image) & copy imageSurface into it*/
    if ( imageSurface->format->Gmask )
    {
      tempSurface = SDL_CreateRGBSurface( SDL_SWSURFACE, texture->txDimensions, texture->txDimensions, 24, imageSurface->format->Rmask, imageSurface->format->Gmask, imageSurface->format->Bmask, imageSurface->format->Amask );
    }
    else
    {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
      tempSurface = SDL_CreateRGBSurface( SDL_SWSURFACE, texture->txDimensions, texture->txDimensions, 24, 0xFF0000, 0x00FF00, 0x0000FF, 0x000000 );
#else
      tempSurface = SDL_CreateRGBSurface( SDL_SWSURFACE, texture->txDimensions, texture->txDimensions, 24, 0x0000FF, 0x00FF00, 0xFF0000, 0x000000 );

#endif
    }
    SDL_BlitSurface( imageSurface, &imageSurface->clip_rect, tempSurface, &imageSurface->clip_rect );

    /* actually create the OpenGL textures */
    if ( texturefilter > 2 )  // Draw with mipmaps?
    {
      if ( imageSurface->format->BytesPerPixel > 1 )
      {
        // Bitmaps come in BGR format by default...
        gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGB, tempSurface->w, tempSurface->h, GL_BGR_EXT, GL_UNSIGNED_BYTE, tempSurface->pixels );
      }
      else
      {
        gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGB, tempSurface->w, tempSurface->h, GL_RGB, GL_UNSIGNED_BYTE, tempSurface->pixels );
      }
    }
    else      // Draw without mipmaps...
    {
      if ( imageSurface->format->BytesPerPixel > 1 )
      {
        // Bitmaps come in BGR format by default...
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, tempSurface->w, tempSurface->h, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, tempSurface->pixels );
      }
      else
      {
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, tempSurface->w, tempSurface->h, 0, GL_RGB, GL_UNSIGNED_BYTE, tempSurface->pixels );
      }
    }

    /* get rid of our SDL_Surfaces now that we're done with them */
    SDL_FreeSurface( tempSurface );
    SDL_FreeSurface( imageSurface );
  }

}

/********************> GLTexture_LoadA() <*****/
void    GLTexture_LoadA( GLTexture *texture, const char *filename, Uint32  key )
{

  /* The key param indicates which color to set alpha to 0.  All other values are 0xFF. */
  SDL_Surface  *tempSurface, *imageSurface;
  Sint16  x, y;
  Uint32   *p;

  /* Load the bitmap into an SDL_Surface */
  imageSurface = SDL_LoadBMP( filename );

  /* Make sure a valid SDL_Surface was returned */
  if ( imageSurface != NULL )
  {
    /* Generate an OpenGL texture */
    glGenTextures( 1, &texture->textureID );

    /* Set up some parameters for the format of the OpenGL texture */
    glBindTexture( GL_TEXTURE_2D, texture->textureID );          /* Bind Our Texture */

    // Check out which type of filter to use
    switch ( texturefilter )
    {
        // Linear filtered
      case 1:
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        break;

        // Bilinear filtered
      case 2:
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
        break;

        // Trilinear filtered
      case 3:
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
        break;

        // Anisotropic filtered!
      case 4:
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy );
        break;
    }

    /* Set the original image's size (incase it's not an exact square of a power of two) */
    texture->imgHeight = imageSurface->h;
    texture->imgWidth = imageSurface->w;

    /* Determine the correct power of two greater than or equal to the original image's size */
    texture->txDimensions = 2;
    while ( ( texture->txDimensions < texture->imgHeight ) && ( texture->txDimensions < texture->imgWidth ) )
      texture->txDimensions *= 2;

    /* Set the texture's alpha */
    texture->alpha = 1;

    /* Create a blank SDL_Surface (of the smallest size to fit the image) & copy imageSurface into it*/
    // SDL_SetColorKey(imageSurface, SDL_SRCCOLORKEY,0);
    // cvtSurface = SDL_ConvertSurface(imageSurface, &fmt, SDL_SWSURFACE);

    if ( imageSurface->format->Gmask )
    {
      tempSurface = SDL_CreateRGBSurface( SDL_SWSURFACE, texture->txDimensions, texture->txDimensions, 32, imageSurface->format->Rmask, imageSurface->format->Gmask, imageSurface->format->Bmask, imageSurface->format->Amask );
    }
    else
    {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
      tempSurface = SDL_CreateRGBSurface( SDL_SWSURFACE, texture->txDimensions, texture->txDimensions, 32, 0x0000FF, 0x00FF00, 0xFF0000, 0x000000 );
#else
      tempSurface = SDL_CreateRGBSurface( SDL_SWSURFACE, texture->txDimensions, texture->txDimensions, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x00000000 );
#endif
    }
    // SDL_BlitSurface( cvtSurface, &cvtSurface->clip_rect, tempSurface, &cvtSurface->clip_rect );
    SDL_BlitSurface( imageSurface, &imageSurface->clip_rect, tempSurface, &imageSurface->clip_rect );

    /* Fix the alpha values */
    SDL_LockSurface( tempSurface );
    p = tempSurface->pixels;
    for ( y = ( texture->txDimensions - 1 ) ;y >= 0; y-- )
    {
      for ( x = ( texture->txDimensions - 1 ); x >= 0; x-- )
      {
        if ( p[x+y*texture->txDimensions] != key )
        {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
          p[x+y*texture->txDimensions] = p[x+y*texture->txDimensions] | 0xFF000000;
#else
          p[x+y*texture->txDimensions] = p[x+y*texture->txDimensions] | 0x000000FF;
#endif
        }
      }
    }
    SDL_UnlockSurface( tempSurface );

    /* actually create the OpenGL textures */
    if ( texturefilter > 2 ) gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGBA, tempSurface->w, tempSurface->h, GL_RGBA, GL_UNSIGNED_BYTE, tempSurface->pixels );  // With mipmapping
    else glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, tempSurface->w, tempSurface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tempSurface->pixels );          // Without mipmapping

    /* get rid of our SDL_Surfaces now that we're done with them */
    SDL_FreeSurface( tempSurface );
    SDL_FreeSurface( imageSurface );
  }

}

/********************> GLTexture_GetTextureID() <*****/
GLuint    GLTexture_GetTextureID( GLTexture *texture )
{

  return texture->textureID;

}

/********************> GLTexture_GetImageHeight() <*****/
GLsizei    GLTexture_GetImageHeight( GLTexture *texture )
{

  return texture->imgHeight;

}

/********************> GLTexture_GetImageWidth() <*****/
GLsizei    GLTexture_GetImageWidth( GLTexture *texture )
{

  return texture->imgWidth;

}

/********************> GLTexture_GetDimensions() <*****/
GLsizei    GLTexture_GetDimensions( GLTexture *texture )
{

  return texture->txDimensions;

}

/********************> GLTexture_SetAlpha() <*****/
void    GLTexture_SetAlpha( GLTexture *texture, GLfloat alpha )
{

  texture->alpha = alpha;

}

/********************> GLTexture_GetAlpha() <*****/
GLfloat    GLTexture_GetAlpha( GLTexture *texture )
{

  return texture->alpha;

}

/********************> GLTexture_Release() <*****/
void    GLTexture_Release( GLTexture *texture )
{

  /* Delete the OpenGL texture */
  glDeleteTextures( 1, &texture->textureID );

  /* Reset the other data */
  texture->imgHeight = texture->imgWidth = texture->txDimensions = 0;

}
