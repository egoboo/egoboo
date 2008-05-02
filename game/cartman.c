#include "cartman.h"

#include "graphic.h"
#include "input.h"
#include "UI.h"
#include "Log.h"
#include "Mesh.h"
#include "graphic.h"
#include "egoboo_types.inl"
#include "egoboo.h"

#include <stdio.h>      // For printf and such
#include "SDL_Pixel.h"
#include <SDL_image.h>

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
  const Uint32 rmask = 0x000000ff;
  const Uint32 gmask = 0x0000ff00;
  const Uint32 bmask = 0x00ff0000;
  const Uint32 amask = 0xff000000;
#else
  const Uint32 rmask = 0xff000000;
  const Uint32 gmask = 0x00ff0000;
  const Uint32 bmask = 0x0000ff00;
  const Uint32 amask = 0x000000ff;
#endif


typedef enum cart_win_type_bits_e
{
  WINNOTHING = 0,             // Window display mode
  WINTILE    = 1 << 0,      //
  WINVERTEX  = 1 << 1,      //
  WINSIDE    = 1 << 2,      //
  WINFX      = 1 << 3        //
};

#define MAXSELECT 2560      // Max points that can be selected

#define NEARLOW         0.0 //16.0    // For autoweld
#define NEARHI        128.0 //112.0    //

#define TINYX 4 //8        // Plan tiles
#define TINYY 4 //8        //
#define SMALLX 31      // Small tiles
#define SMALLY 31      //
#define BIGX 63        // Big tiles
#define BIGY 63        //

#define CHAINEND 0xffffffff    // End of vertex chain
#define VERTEXUNUSED 0         // Check meshvrta to see if used

int   numwritten = 0;
int   numattempt = 0;
int   numlight;
int   addinglight;
int   lightx[MAXLIGHT];
int   lighty[MAXLIGHT];
Uint8 lightlevel[MAXLIGHT];
int   lightradius[MAXLIGHT];
int   ambi = 22;
int   ambicut = 1;
int   direct = 16;

int cart_pos_x = 0;
int cart_pos_y = 0;

char    loadname[80];    // Text
int    SCRX = 640;    // Screen size
int    SCRY = 480;    //
int    OUTX = 640;    // Output size <= Screen size
int    OUTY = 480;    //


int    brushsize = 3;    // Size of raise/lower terrain brush
int    brushamount = 50;  // Amount of raise/lower


GLtexture    imgcursor;    // Cursor image
GLtexture    imgpoint[16];    // Vertex image
GLtexture    imgpointon[16];  // Vertex image ( Selected )
GLtexture    imgref;    // Meshfx images
GLtexture    imgdrawref;    //
GLtexture    imganim;    //
GLtexture    imgwater;    //
GLtexture    imgwall;    //
GLtexture    imgimpass;    //
GLtexture    imgdamage;    //
GLtexture    imgslippy;    //
GLtexture    bmptemp;    // A temporary bitmap
GLtexture    bmpdbuff;    // The double buffer bitmap
SDL_Surface *bmphitemap;    // Heightmap image
GLtexture    bmpsmalltile[MAXTILE];  // Tiles
GLtexture    bmpbigtile[MAXTILE];  //
GLtexture    bmptinysmalltile[MAXTILE];  // Plan tiles
GLtexture    bmptinybigtile[MAXTILE];  //
GLtexture    bmpfanoff;    //
int          numsmalltile = 0;  //
int          numbigtile = 0;    //

int     numpointsonscreen = 0;
Uint32  pointsonscreen[MAXPOINTS];
int     numselect = 0;
Uint32  select[MAXSELECT];


float    debugx = -1;    // Blargh
float    debugy = -1;    //
int      mouseinwin = -1;  // More mouse data
int      mouseinwinx = -1;  //
int      mouseinwiny = -1;  //
Uint16   mouseinwinmode = 0;  // Window mode
Uint32   mouseinwinonfan = 0;  // Fan mouse is on
Uint16   mouseinwintile = 0;  // Tile
Uint16   mouseinwinpresser = 0;  // Random add for tiles
Uint8    mouseinwintype = 0;  // Fan type
int      mouseinwinrect = 0;  // Rectangle drawing
int      mouseinwinrectx;  //
int      mouseinwinrecty;  //
Uint8    mouseinwinfx = MESHFX_NOREFLECT;//

#define MAXWIN 8                 // Number of windows
GLtexture window_tx[MAXWIN];     // Window images
bool_t    windowon[MAXWIN];       // Draw it?
int       windowborderx[MAXWIN]; // Window border size
int       windowbordery[MAXWIN]; //
SDL_Rect  window_rect[MAXWIN];    // Window size
Uint16    windowmode[MAXWIN];     // Window display mode


int    colordepth = 8;    // 256 colors
int    keydelay = 0;    //


Uint32 atvertex = 0;      // Current vertex check for new
Uint32 numfreevertices = 0;    // Number of free vertices
float  meshedgez;      //
Uint8  Mesh_Mem.vrt_a[MAXTOTALMESHVERTICES];  // Vertex base light, 0=unused
Uint32 Mesh_Mem.vrt_next[MAXTOTALMESHVERTICES]; // Next vertex in fan
Uint32 Mesh[MAXMESHTYPE].numline;  // Number of lines to draw
Uint8  Mesh[MAXMESHTYPE].linestart[MAXMESHLINE];
Uint8  Mesh[MAXMESHTYPE].lineend[MAXMESHLINE];



//------------------------------------------------------------------------------
GLfloat * make_color(Uint8 r, Uint8 g, Uint8 b)
{
  static GLfloat color[4];

  color[0] = (r << 3) / 256.0f;
  color[1] = (g << 3) / 256.0f;
  color[2] = (b << 3) / 256.0f;
  color[3] = 1.0f;

  return color;
}

//------------------------------------------------------------------------------
void set_window_viewport( int window )
{
  if(-1 == window)
  {
    glViewport(0, 0, CData.scrx, CData.scry);
  }
  else
  {
    glViewport(window_rect[window].x, window_rect[window].y, window_rect[window].w, window_rect[window].h);
  };
};


//------------------------------------------------------------------------------
void draw_rect(int window, GLfloat color[], float xl, float yt, float xr, float yb )
{
  set_window_viewport( window );

  glBindTexture( GL_TEXTURE_2D, INVALID_TEXTURE );

  if(NULL != color)
  {
    glColor3fv( color );
  }

  glBegin(GL_QUADS);
  {
    glVertex2f( xl, yb );
    glVertex2f( xr, yb );
    glVertex2f( xr, yt );
    glVertex2f( xl, yt );
  }
  glEnd();
};


void draw_line(int window, GLfloat color[], float x1, float y1, float x2, float y2 )
{
  set_window_viewport( window );

  if( NULL != color )
  {
    glColor3fv( color );
  };

  glBegin(GL_LINES);
  {
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
  }
  glEnd();
}


//------------------------------------------------------------------------------
void clear_to_color( int window, GLfloat color[] )
{
  SDL_Rect dst;

  if( -1 == window )
  {
    dst.x = dst.y = 0;
    dst.w = CData.scrx;
    dst.h = CData.scry;
  }
  else
  {
    dst = window_rect[window];
  }

  draw_rect( window, color, dst.x, dst.y, dst.x + dst.w, dst.y + dst.h );

}



//------------------------------------------------------------------------------
void draw_blit_sprite(int window, GLtexture * sprite, int x, int y)
{
  FRect dst;

  set_window_viewport( window );

  dst.left   = x;
  dst.right  = x + sprite->txW;
  dst.top    = y;
  dst.bottom = y + sprite->txH;

  draw_texture_box(sprite, NULL, &dst);
};

//------------------------------------------------------------------------------
void add_light(int x, int y, int radius, int level)
{
  if(numlight >= MAXLIGHT)  numlight = MAXLIGHT-1;

  lightx[numlight] = x;
  lighty[numlight] = y;
  lightradius[numlight] = radius;
  lightlevel[numlight] = level;
  numlight++;
}

//------------------------------------------------------------------------------
void alter_light(int x, int y)
{
  int radius, level;

  numlight--;
  if(numlight < 0)  numlight = 0;

  radius = abs(lightx[numlight] - x);
  level = abs(lighty[numlight] - y);
  if(radius > MAXRADIUS)  radius = MAXRADIUS;
  if(radius < MINRADIUS)  radius = MINRADIUS;
  lightradius[numlight] = radius;
  if(level > MAXHEIGHT) level = MAXHEIGHT;
  if(level < MINLEVEL) level = MINLEVEL;
  lightlevel[numlight] = level;

  numlight++;
}

//------------------------------------------------------------------------------
void draw_circle( int window, float x, float y, float radius, GLfloat color[] )
{
  GLUquadricObj * qobj = gluNewQuadric();

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();

    if(NULL != color)
    {
      glColor4fv( color );
    };

    gluQuadricDrawStyle(qobj, GLU_FILL); /* flat shaded */
    gluQuadricNormals(qobj, GLU_FLAT);

    glTranslatef(x,y,0);

    gluDisk(qobj, 0, 1.0, 20, 4);

  glPopMatrix();
};

//------------------------------------------------------------------------------
void draw_light(int window, int number)
{
  int xdraw, ydraw, radius;
  int icolor;

  xdraw = (lightx[number]/FOURNUM) - cart_pos_x + (window_rect[window].w>>1) - SMALLX;
  ydraw = (lighty[number]/FOURNUM) - cart_pos_y + (window_rect[window].h>>1) - SMALLY;
  radius = abs(lightradius[number])/FOURNUM;
  icolor = lightlevel[number]>>3;

  draw_circle( window, xdraw, ydraw, radius, make_color(icolor, icolor, icolor) );
}

//------------------------------------------------------------------------------
int dist_from_border(int x, int y)
{
  if(x > mesh.edge_x / 2.0f) x = mesh.edge_x-x-1;
  if(x < 0) x = 0;

  if(y > mesh.edge_y / 2.0f) y = mesh.edge_y-y-1;
  if(y < 0) y = 0;

  return ( (x < y) ? x : y );
}

//------------------------------------------------------------------------------
int dist_from_edge(int x, int y)
{
  if(x > (mesh.size_x>>1)) x = mesh.size_x-x-1;
  if(y > (mesh.size_y>>1)) y = mesh.size_y-y-1;

  return ( (x < y) ? x : y );
}

//------------------------------------------------------------------------------
bool_t fan_is_floor(float x, float y)
{
  Uint32 fan;
  vect3 pos = {x,y,0};

  fan = mesh_get_fan( pos );
  if( INVALID_FAN != fan )
  {
    return mesh_has_no_bits(fan, MESHFX_WALL | MESHFX_IMPASS);
  }

  return bfalse;
}

//------------------------------------------------------------------------------
void set_barrier_height(int x, int y)
{
  Uint32 type, fan, vert;
  int cnt, noedges;
  float bestprox, prox, tprox, scale;
  vect3 pos = {x,y,0};

  fan = mesh_get_fan(pos);
  if(fan != -1)
  {
    if( mesh_has_some_bits( fan, MESHFX_WALL ) )
    {
      type = Mesh_Fan[fan].type;
      noedges = btrue;
      vert = Mesh_Fan[fan].vrt_start;
      cnt = 0;
      while(cnt < Mesh_Cmd[type].vrt_count)
      {
        bestprox = 2*(NEARHI-NEARLOW)/3.0;
        if(fan_is_floor(x+1, y))
        {
          prox = NEARHI-Mesh_Cmd[type].u[cnt];
          if(prox < bestprox) bestprox = prox;
          noedges = bfalse;
        }
        if(fan_is_floor(x, y+1))
        {
          prox = NEARHI-Mesh_Cmd[type].v[cnt];
          if(prox < bestprox) bestprox = prox;
          noedges = bfalse;
        }
        if(fan_is_floor(x-1, y))
        {
          prox = Mesh_Cmd[type].u[cnt]-NEARLOW;
          if(prox < bestprox) bestprox = prox;
          noedges = bfalse;
        }
        if(fan_is_floor(x, y-1))
        {
          prox = Mesh_Cmd[type].v[cnt]-NEARLOW;
          if(prox < bestprox) bestprox = prox;
          noedges = bfalse;
        }
        if(noedges)
        {
          // Surrounded by walls on all 4 sides, but it may be a corner piece
          if(fan_is_floor(x+1, y+1))
          {
            prox = NEARHI-Mesh_Cmd[type].u[cnt];
            tprox = NEARHI-Mesh_Cmd[type].v[cnt];
            if(tprox > prox) prox = tprox;
            if(prox < bestprox) bestprox = prox;
          }
          if(fan_is_floor(x+1, y-1))
          {
            prox = NEARHI-Mesh_Cmd[type].u[cnt];
            tprox = Mesh_Cmd[type].v[cnt]-NEARLOW;
            if(tprox > prox) prox = tprox;
            if(prox < bestprox) bestprox = prox;
          }
          if(fan_is_floor(x-1, y+1))
          {
            prox = Mesh_Cmd[type].u[cnt]-NEARLOW;
            tprox = NEARHI-Mesh_Cmd[type].v[cnt];
            if(tprox > prox) prox = tprox;
            if(prox < bestprox) bestprox = prox;
          }
          if(fan_is_floor(x-1, y-1))
          {
            prox = Mesh_Cmd[type].u[cnt]-NEARLOW;
            tprox = Mesh_Cmd[type].v[cnt]-NEARLOW;
            if(tprox > prox) prox = tprox;
            if(prox < bestprox) bestprox = prox;
          }
        }
        scale = window_rect[mouseinwin].h-(mouseinwiny/FOURNUM);
        bestprox = bestprox*scale*BARRIERHEIGHT/window_rect[mouseinwin].h;
        if(bestprox > meshedgez) bestprox = meshedgez;
        if(bestprox < 0) bestprox = 0;
        Mesh_Mem.vrt_z[vert] = bestprox;
        vert = Mesh_Mem.vrt_next[vert];
        cnt++;
      }
    }
  }
}

//------------------------------------------------------------------------------
void fix_walls()
{
  int x, y;

  y = 0;
  while(y < mesh.size_y)
  {
    x = 0;
    while(x < mesh.size_x)
    {
      set_barrier_height(x, y);
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
void impass_edges(int amount)
{
  int x, y;
  Uint32 fan;

  y = 0;
  while(y < mesh.size_y)
  {
    x = 0;
    while(x < mesh.size_x)
    {
      if(dist_from_edge(x, y) < amount)
      {
        fan = mesh_convert_fan(x, y);
        Mesh_Fan[fan].fx |= MESHFX_IMPASS;
      }
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
Uint8 get_twist(int x, int y)
{
  Uint8 twist;


  // x and y should be from -7 to 8
  if(x < -7) x = -7;
  if(x > 8) x = 8;
  if(y < -7) y = -7;
  if(y > 8) y = 8;
  // Now between 0 and 15
  x = x+7;
  y = y+7;
  twist = (y<<4)+x;
  return twist;
}

//------------------------------------------------------------------------------
Uint8 get_fan_twist(Uint32 fan)
{
  int zx, zy, vt0, vt1, vt2, vt3;
  Uint8 twist;

  vt0 = Mesh_Fan[fan].vrt_start;
  vt1 = Mesh_Mem.vrt_next[vt0];
  vt2 = Mesh_Mem.vrt_next[vt1];
  vt3 = Mesh_Mem.vrt_next[vt2];
  zx = (Mesh_Mem.vrt_z[vt0]+Mesh_Mem.vrt_z[vt3]-Mesh_Mem.vrt_z[vt1]-Mesh_Mem.vrt_z[vt2])/SLOPE;
  zy = (Mesh_Mem.vrt_z[vt2]+Mesh_Mem.vrt_z[vt3]-Mesh_Mem.vrt_z[vt0]-Mesh_Mem.vrt_z[vt1])/SLOPE;
  twist = get_twist(zx, zy);


  return twist;
}

//------------------------------------------------------------------------------
//void make_graypal(void)
//{
//  int cnt, level;
//
//
//  cnt = 0;
//  while(cnt < 253)
//  {
//    level = cnt+2; if(level > 255) level = 255;
//    badpal[cnt].r = level>>2;
//    level = cnt+1; if(level > 255) level = 255;
//    badpal[cnt].g = level>>2;
//    badpal[cnt].b = cnt>>2;
//    cnt++;
//  }
//  badpal[253].r = 63;  badpal[253].g = 0;  badpal[253].b = 0;
//  badpal[254].r = 0;  badpal[254].g = 0;  badpal[254].b = 63;
//  badpal[255].r = 63;  badpal[255].g = 0;  badpal[255].b = 63;
//  return;
//}

////------------------------------------------------------------------------------
//int mesh_get_level(int x, int y)
//{
//  Uint32 fan;
//  int z0, z1, z2, z3;         // Height of each fan corner
//  int zleft, zright,zdone;    // Weighted height of each side
//
//
//  if((y>>7) >= mesh.size_y || (x>>7) >= mesh.size_x)
//    return 0;
//  fan = Mesh[y>>7].fanstart+(x>>7);
//  x = x&127;
//  y = y&127;
//  z0 = Mesh_Mem.vrt__start[Mesh_Fan[fan]+0].vrtz;
//  z1 = Mesh_Mem.vrt__start[Mesh_Fan[fan]+1].vrtz;
//  z2 = Mesh_Mem.vrt__start[Mesh_Fan[fan]+2].vrtz;
//  z3 = Mesh_Mem.vrt__start[Mesh_Fan[fan]+3].vrtz;
//
//  zleft = (z0*(128-y)+z3*y)>>7;
//  zright = (z1*(128-y)+z2*y)>>7;
//  zdone = (zleft*(128-x)+zright*x)>>7;
//  return (zdone);
//}

//------------------------------------------------------------------------------
void make_hitemap(void)
{
  int x, y, pixx, pixy, level, fan;

  if(NULL != bmphitemap) SDL_FreeSurface(bmphitemap);
  bmphitemap = SDL_CreateRGBSurface(SDL_SWSURFACE, mesh.size_x<<2, mesh.size_y<<2, 32, rmask, gmask, bmask, amask);
  if(NULL == bmphitemap) return;

  y = 16;
  pixy = 0;
  while(pixy < (mesh.size_y<<2))
  {
    x = 16;
    pixx = 0;
    while(pixx < (mesh.size_x<<2))
    {
      vect3 pos = {x,y,0};

      level=(mesh_get_level(mesh_get_fan(pos), x, y, bfalse)*255/meshedgez);  // level is 0 to 255
      if(level > 252) level = 252;
      fan = Mesh[pixy>>2].fanstart+(pixx>>2);

      if( mesh_has_all_bits(fan, MESHFX_WALL | MESHFX_IMPASS) ) level = 255;   // Both
      else if( mesh_has_all_bits(fan, MESHFX_WALL)   ) level = 253;         // Wall
      else if( mesh_has_all_bits(fan, MESHFX_IMPASS) ) level = 254;         // Impass

      SDL_PutPixel(bmphitemap, pixx, pixy, level);

      x+=32;
      pixx++;
    }
    y+=32;
    pixy++;
  }
  return;
}

//------------------------------------------------------------------------------
GLtexture * tiny_tile_at(int x, int y)
{
  GLtexture * retval = NULL;
  Uint16 tile, basetile;
  Uint8 type, fx;
  Uint32 fan;

  if(x < 0 || x >= mesh.size_x || y < 0 || y >= mesh.size_y)
  {
    return retval;
  }

  fan = mesh_convert_fan(x, y);
  if(Mesh_Fan[fan].tile==INVALID_FAN)
  {
    return NULL;
  }

  tile = Mesh_Fan[fan].tile;
  type = Mesh_Fan[fan].type;
  fx = Mesh_Fan[fan].fx;

  // Animate the tiles
  if ( HAS_SOME_BITS( fx, MESHFX_ANIM ) )
  {
    if ( type >= ( MAXMESHTYPE >> 1 ) )
    {
      // Big tiles
      basetile = tile & GTile_Anim.bigbaseand;// Animation set
      tile += GTile_Anim.frameadd << 1;         // Animated tile
      tile = ( tile & GTile_Anim.bigframeand ) + basetile;
    }
    else
    {
      // Small tiles
      basetile = tile & GTile_Anim.baseand;// Animation set
      tile += GTile_Anim.frameadd;         // Animated tile
      tile = ( tile & GTile_Anim.frameand ) + basetile;
    }
  }


  if(type >= (MAXMESHTYPE>>1))
  {
    retval = &bmptinybigtile[tile];
  }
  else
  {
    retval = &bmptinysmalltile[tile];
  }

  return retval;
}

//------------------------------------------------------------------------------
void make_planmap(void)
{
  int x, y, putx, puty;
  SDL_Surface* bmptemp;


  bmptemp = SDL_CreateRGBSurface(SDL_SWSURFACE, 64, 64, 32, rmask, gmask, bmask, amask);
  if(!bmptemp)  return;

  if(NULL != bmphitemap) SDL_FreeSurface(bmphitemap);
  bmphitemap = SDL_CreateRGBSurface(SDL_SWSURFACE, mesh.size_x*TINYX, mesh.size_y*TINYY, 32, rmask, gmask, bmask, amask);
  if(!bmphitemap) return;


  do_clear();
  puty = 0;
  y = 0;
  while(y < mesh.size_y)
  {
    putx = 0;
    x = 0;
    while(x < mesh.size_x)
    {
      FRect dst = {putx, puty, putx + TINYX, puty + TINYY};
      GLtexture * tmp = tiny_tile_at(x, y);
      draw_texture_box(tmp, NULL, &dst);

      putx+=TINYX;
      x++;
    }
    puty+=TINYY;
    y++;
  }


  SDL_BlitSurface(bmphitemap, &bmphitemap->clip_rect, bmptemp, &bmptemp->clip_rect);

  SDL_FreeSurface(bmphitemap);

  bmphitemap = bmptemp;
}

//------------------------------------------------------------------------------
void draw_cursor_in_window(int win)
{
  int x, y;

  if(mouseinwin!=-1)
  {
    if(windowon[win] && win != mouseinwin)
    {
      if((windowmode[mouseinwin]&WINSIDE) == (windowmode[win]&WINSIDE))
      {
        x = ui_getMouseX()-window_rect[mouseinwin].x-windowborderx[mouseinwin];
        y = ui_getMouseY()-window_rect[mouseinwin].y-windowbordery[mouseinwin];
        draw_blit_sprite(win, &imgpointon[10], x-4, y-4);
      }
    }
  }
  return;
}

//------------------------------------------------------------------------------
int get_vertex(int x, int y, int num)
{
  // ZZ> This function gets a vertex number or -1
  int vert, cnt;
  Uint32 fan;

  vert = -1;
  if(x>=0 && y>=0 && x<mesh.size_x && y<mesh.size_y)
  {
    fan = mesh_convert_fan(x, y);
    if(Mesh_Cmd[Mesh_Fan[fan].type].vrt_count>num)
    {
      vert = Mesh_Fan[fan].vrt_start;
      cnt = 0;
      while(cnt < num)
      {
        vert = Mesh_Mem.vrt_next[vert];
        if(vert==-1)
        {
          log_error("BAD GET_VERTEX NUMBER(2nd), %d at %d, %d...\n" "%d VERTICES ALLOWED...\n\n", num, x, y , Mesh_Cmd[Mesh_Fan[fan].type].vrt_count);
        }
        cnt++;
      }
    }
  }
  return vert;
}

//------------------------------------------------------------------------------
int nearest_vertex(int x, int y, float nearx, float neary)
{
  // ZZ> This function gets a vertex number or -1
  int vert, bestvert, cnt;
  Uint32 fan;
  int num;
  float prox, proxx, proxy, bestprox;


  bestvert = -1;
  if(x>=0 && y>=0 && x<mesh.size_x && y<mesh.size_y)
  {
    fan = mesh_convert_fan(x, y);
    num = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
    vert = Mesh_Fan[fan].vrt_start;
    vert = Mesh_Mem.vrt_next[vert];
    vert = Mesh_Mem.vrt_next[vert];
    vert = Mesh_Mem.vrt_next[vert];
    vert = Mesh_Mem.vrt_next[vert];
    bestprox = 9000;
    cnt = 4;
    while(cnt < num)
    {
      proxx = Mesh[Mesh_Fan[fan].type].commandu[cnt]-nearx;
      proxy = Mesh[Mesh_Fan[fan].type].commandv[cnt]-neary;
      if(proxx < 0) proxx=-proxx;
      if(proxy < 0) proxy=-proxy;
      prox = proxx+proxy;
      if(prox < bestprox)
      {
        bestvert = vert;
        bestprox = prox;
      }
      vert = Mesh_Mem.vrt_next[vert];
      cnt++;
    }
  }
  return bestvert;
}

//------------------------------------------------------------------------------
void weld_select()
{
  // ZZ> This function welds the highlighted vertices
  int cnt, x, y, z, a;
  Uint32 vert;

  if(numselect > 1)
  {
    x = 0;
    y = 0;
    z = 0;
    a = 0;
    cnt = 0;
    while(cnt < numselect)
    {
      vert = select[cnt];
      x+=Mesh_Mem.vrt_x[vert];
      y+=Mesh_Mem.vrt_y[vert];
      z+=Mesh_Mem.vrt_z[vert];
      a+=Mesh_Mem.vrt_a[vert];
      cnt++;
    }
    x+=cnt>>1;  y+=cnt>>1;
    x /= numselect;
    y /= numselect;
    z /= numselect;
    a /= numselect;
    cnt = 0;
    while(cnt < numselect)
    {
      vert = select[cnt];
      Mesh_Mem.vrt_x[vert]=x;
      Mesh_Mem.vrt_y[vert]=y;
      Mesh_Mem.vrt_z[vert]=z;
      Mesh_Mem.vrt_a[vert]=a;
      cnt++;
    }
  }
  return;
}

//------------------------------------------------------------------------------
void add_select(int vert)
{
  // ZZ> This function highlights a vertex
  int cnt, found;


  if(numselect < MAXSELECT && vert >= 0)
  {
    found = bfalse;
    cnt = 0;
    while(cnt < numselect && !found)
    {
      if(select[cnt]==vert)
      {
        found=btrue;
      }
      cnt++;
    }
    if(!found)
    {
      select[numselect] = vert;
      numselect++;
    }
  }

  return;
}

//------------------------------------------------------------------------------
void clear_select(void)
{
  // ZZ> This function unselects all vertices
  numselect = 0;
  return;
}

//------------------------------------------------------------------------------
int vert_selected(int vert)
{
  // ZZ> This function returns btrue if the vertex has been highlighted by user
  int cnt;

  cnt = 0;
  while(cnt < numselect)
  {
    if(vert==select[cnt])
    {
      return btrue;
    }
    cnt++;
  }

  return bfalse;
}

//------------------------------------------------------------------------------
void remove_select(int vert)
{
  // ZZ> This function makes sure the vertex is not highlighted
  int cnt, stillgoing;

  cnt = 0;
  stillgoing = btrue;
  while(cnt < numselect && stillgoing)
  {
    if(vert==select[cnt])
    {
      stillgoing = bfalse;
    }
    cnt++;
  }
  if(stillgoing == bfalse)
  {
    while(cnt < numselect)
    {
      select[cnt-1] = select[cnt];
      cnt++;
    }
    numselect--;
  }



  return;
}

//------------------------------------------------------------------------------
void fan_onscreen(Uint32 fan)
{
  // ZZ> This function flags a fan's points as being "onscreen"
  int cnt;
  Uint32 vert;


  vert = Mesh_Fan[fan].vrt_start;
  cnt = 0;
  while(cnt < Mesh_Cmd[Mesh_Fan[fan].type].vrt_count)
  {
    pointsonscreen[numpointsonscreen] = vert;  numpointsonscreen++;
    vert = Mesh_Mem.vrt_next[vert];
    cnt++;
  }
  return;
}

//------------------------------------------------------------------------------
void make_onscreen(void)
{
  int x, y, cntx, cnty, numx, numy, mapx, mapy, mapxstt, mapystt;
  Uint32 fan;


  numpointsonscreen = 0;
  mapxstt = (cart_pos_x-(ONSIZE>>1))/31;
  mapystt = (cart_pos_y-(ONSIZE>>1))/31;
  numx = (ONSIZE/SMALLX)+3;
  numy = (ONSIZE/SMALLY)+3;
  x = -cart_pos_x+(ONSIZE>>1)-SMALLX;
  y = -cart_pos_y+(ONSIZE>>1)-SMALLY;


  mapy = mapystt;
  cnty = 0;
  while(cnty < numy)
  {
    if(mapy>=0 && mapy<mesh.size_y)
    {
      mapx = mapxstt;
      cntx = 0;
      while(cntx < numx)
      {
        if(mapx>=0 && mapx<mesh.size_x)
        {
          fan = mesh_convert_fan(mapx, mapy);
          fan_onscreen(fan);
        }
        mapx++;
        cntx++;
      }
    }
    mapy++;
    cnty++;
  }

  return;
}

//------------------------------------------------------------------------------
void draw_top_fan(int window, int fan, int x, int y)
{
  // ZZ> This function draws the line drawing preview of the tile type...
  //     A wireframe tile from a vertex connection window
  Uint32 faketoreal[MAXMESHVERTICES];
  int fantype;
  int cnt, stt, end, vert;
  GLfloat * color;
  int size;

  set_window_viewport( window );

  fantype = Mesh_Fan[fan].type;
  color = make_color(16, 16, 31);
  if(fantype>=MAXMESHTYPE/2)
  {
    color = make_color(31, 16, 16);
  }

  vert = Mesh_Fan[fan].vrt_start;
  cnt = 0;
  while(cnt < Mesh_Cmd[fantype].vrt_count)
  {
    faketoreal[cnt] = vert;
    vert = Mesh_Mem.vrt_next[vert];
    cnt++;
  }


  glBegin(GL_LINES);
  glColor3fv(color);
  cnt = 0;
  while(cnt < Mesh[fantype].numline)
  {
    stt = faketoreal[Mesh[fantype].linestart[cnt]];
    end = faketoreal[Mesh[fantype].lineend[cnt]];

    glVertex2f(Mesh_Mem.vrt_x[stt]+x, Mesh_Mem.vrt_y[stt]+y);
    glVertex2f(Mesh_Mem.vrt_x[end]+x, Mesh_Mem.vrt_y[end]+y);

    cnt++;
  }
  glEnd();



  cnt = 0;
  while(cnt < Mesh_Cmd[fantype].vrt_count)
  {
    vert = faketoreal[cnt];
    size = (Mesh_Mem.vrt_z[vert] * 16.0f)/(meshedgez+1);
    if(Mesh_Mem.vrt_z[vert] >= 0)
    {
      if(vert_selected(vert))
      {
        draw_blit_sprite(-1, &imgpointon[size],
          Mesh_Mem.vrt_x[vert]+x-(imgpointon[size].txW>>1),
          Mesh_Mem.vrt_y[vert]+y-(imgpointon[size].txH>>1));
      }
      else
      {
        draw_blit_sprite(-1, &imgpoint[size],
          Mesh_Mem.vrt_x[vert]+x-(imgpoint[size].txW>>1),
          Mesh_Mem.vrt_y[vert]+y-(imgpoint[size].txH>>1));
      }
    }
    cnt++;
  }


  return;
}

//------------------------------------------------------------------------------
void draw_side_fan(int window, int fan, int x, int y)
{
  // ZZ> This function draws the line drawing preview of the tile type...
  //     A wireframe tile from a vertex connection window ( Side view )
  Uint32 faketoreal[MAXMESHVERTICES];
  int fantype;
  int cnt, stt, end, vert;
  GLfloat * color;
  int size;

  set_window_viewport( window );

  fantype = Mesh_Fan[fan].type;
  color = make_color(16, 16, 31);
  if(fantype>=MAXMESHTYPE/2)
  {
    color = make_color(31, 16, 16);
  }

  vert = Mesh_Fan[fan].vrt_start;
  cnt = 0;
  while(cnt < Mesh_Cmd[fantype].vrt_count)
  {
    faketoreal[cnt] = vert;
    vert = Mesh_Mem.vrt_next[vert];
    cnt++;
  }


  glBegin(GL_LINES);
  glColor3fv( color );
  cnt = 0;
  while(cnt < Mesh[fantype].numline)
  {
    stt = faketoreal[Mesh[fantype].linestart[cnt]];
    end = faketoreal[Mesh[fantype].lineend[cnt]];
    if(Mesh_Mem.vrt_z[stt] >= 0 && Mesh_Mem.vrt_z[end] >= 0)
    {
      glVertex2f( Mesh_Mem.vrt_x[stt]+x, -(Mesh_Mem.vrt_z[stt] / 16.0f)+y );
      glVertex2f( Mesh_Mem.vrt_x[end]+x, -(Mesh_Mem.vrt_z[end] / 16.0f)+y );
    }
    cnt++;
  }
  glEnd();


  size = 5;
  cnt = 0;
  while(cnt < Mesh_Cmd[fantype].vrt_count)
  {
    vert = faketoreal[cnt];
    if(Mesh_Mem.vrt_z[vert] >= 0)
    {
      if(vert_selected(vert))
      {
        draw_blit_sprite(window, &imgpointon[size],
          Mesh_Mem.vrt_x[vert]+x-(imgpointon[size].txW>>1),
          -(Mesh_Mem.vrt_z[vert] / 16.0f)+y-(imgpointon[size].txH>>1));
      }
      else
      {
        draw_blit_sprite(window, &imgpoint[size],
          Mesh_Mem.vrt_x[vert]+x-(imgpoint[size].txW>>1),
          -(Mesh_Mem.vrt_z[vert] / 16.0f)+y-(imgpoint[size].txH>>1));
      }
    }
    cnt++;
  }


  return;
}

//------------------------------------------------------------------------------
void draw_schematic(int window, int fantype, int x, int y)
{
  // ZZ> This function draws the line drawing preview of the tile type...
  //     The wireframe on the left side of the screen.
  int cnt, stt, end;
  GLfloat * color;

  color = make_color(16, 16, 31);
  if(mouseinwintype>=MAXMESHTYPE/2)  color = make_color(31, 16, 16);

  glBegin(GL_LINES);
  glColor4fv( color );
  cnt = 0;
  while(cnt < Mesh[fantype].numline)
  {
    stt = Mesh[fantype].linestart[cnt];
    end = Mesh[fantype].lineend[cnt];

    glVertex2f( Mesh_Cmd[fantype].u[stt]+x, Mesh_Cmd[fantype].v[stt]+y );
    glVertex2f( Mesh_Cmd[fantype].u[end]+x, Mesh_Cmd[fantype].v[end]+y );

    cnt++;
  }
  glEnd();


}

//------------------------------------------------------------------------------
void add_line(int fantype, int start, int end)
{
  // ZZ> This function adds a line to the vertex schematic
  int cnt;

  // Make sure line isn't already in list
  cnt = 0;
  while(cnt < Mesh[fantype].numline)
  {
    if((Mesh[fantype].linestart[cnt]==start &&
      Mesh[fantype].lineend[cnt]==end) ||
      (Mesh[fantype].lineend[cnt]==start &&
      Mesh[fantype].linestart[cnt]==end))
    {
      return;
    }
    cnt++;
  }


  // Add it in
  Mesh[fantype].linestart[cnt]=start;
  Mesh[fantype].lineend[cnt]=end;
  Mesh[fantype].numline++;

  return;
}


//------------------------------------------------------------------------------
void free_vertices()
{
  // ZZ> This function sets all vertices to unused
  int cnt;

  cnt = 0;
  while(cnt < MAXTOTALMESHVERTICES)
  {
    Mesh_Mem.vrt_a[cnt] = VERTEXUNUSED;
    cnt++;
  }
  atvertex = 0;
  numfreevertices=MAXTOTALMESHVERTICES;
  return;
}

//------------------------------------------------------------------------------
int get_free_vertex()
{
  // ZZ> This function returns btrue if it can find an unused vertex, and it
  // will set atvertex to that vertex index.  bfalse otherwise.
  int cnt;

  if(numfreevertices!=0)
  {
    cnt = 0;
    while(cnt < MAXTOTALMESHVERTICES && Mesh_Mem.vrt_a[atvertex]!=VERTEXUNUSED)
    {
      atvertex++;
      if(atvertex == MAXTOTALMESHVERTICES)
      {
        atvertex = 0;
      }
      cnt++;
    }
    if(Mesh_Mem.vrt_a[atvertex]==VERTEXUNUSED)
    {
      Mesh_Mem.vrt_a[atvertex]=60;
      return btrue;
    }
  }
  return bfalse;
}

//------------------------------------------------------------------------------
void remove_fan(int fan)
{
  // ZZ> This function removes a fan's vertices from usage and sets the fan
  //     to not be drawn
  int cnt, vert;
  Uint32 numvert;


  numvert = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
  vert = Mesh_Fan[fan].vrt_start;
  cnt = 0;
  while(cnt < numvert)
  {
    Mesh_Mem.vrt_a[vert] = VERTEXUNUSED;
    numfreevertices++;
    vert = Mesh_Mem.vrt_next[vert];
    cnt++;
  }
  Mesh_Fan[fan].type = 0;
  Mesh_Fan[fan].fx = MESHFX_NOREFLECT;
}

//------------------------------------------------------------------------------
int add_fan(int fan, int x, int y)
{
  // ZZ> This function allocates the vertices needed for a fan
  int cnt;
  int numvert;
  Uint32 vertex;
  Uint32 vertexlist[17];


  numvert = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
  if(numfreevertices >= numvert)
  {
    Mesh_Fan[fan].fx = MESHFX_NOREFLECT;
    cnt = 0;
    while(cnt < numvert)
    {
      if(get_free_vertex()==bfalse)
      {
        // Reset to unused
        numvert = cnt;
        cnt = 0;
        while(cnt < numvert)
        {
          Mesh[vertexlist[cnt]].vrta=60;
          cnt++;
        }
        return bfalse;
      }
      vertexlist[cnt] = atvertex;
      cnt++;
    }
    vertexlist[cnt] = CHAINEND;


    cnt = 0;
    while(cnt < numvert)
    {
      vertex = vertexlist[cnt];
      Mesh_Mem.vrt_x[vertex] = x + (Mesh[Mesh_Fan[fan].type].commandu[cnt] / 4.0f);
      Mesh_Mem.vrt_y[vertex] = y + (Mesh[Mesh_Fan[fan].type].commandv[cnt] / 4.0f);
      Mesh_Mem.vrt_z[vertex] = 0;
      Mesh_Mem.vrt_next[vertex] = vertexlist[cnt+1];
      cnt++;
    }
    Mesh_Fan[fan].vrt_start = vertexlist[0];
    numfreevertices-=numvert;
    return btrue;
  }
  return bfalse;
}

//------------------------------------------------------------------------------
void num_free_vertex()
{
  // ZZ> This function counts the unused vertices and sets numfreevertices
  int cnt, num;

  num = 0;
  cnt = 0;
  while(cnt < MAXTOTALMESHVERTICES)
  {
    if(Mesh_Mem.vrt_a[cnt]==VERTEXUNUSED)
    {
      num++;
    }
    cnt++;
  }
  numfreevertices=num;
}

//------------------------------------------------------------------------------
GLtexture * tile_at(int x, int y)
{
  GLtexture * retval = NULL;
  Uint16 tile, basetile;
  Uint8 type, fx;
  Uint32 fan;

  if(x < 0 || x >= mesh.size_x || y < 0 || y >= mesh.size_y)
  {
    return retval;
  }

  fan = mesh_convert_fan(x, y);
  if(Mesh_Fan[fan].tile==INVALID_FAN)
  {
    return NULL;
  }

  tile = Mesh_Fan[fan].tile;
  type = Mesh_Fan[fan].type;
  fx = Mesh_Fan[fan].fx;

  // Animate the tiles
  if ( HAS_SOME_BITS( fx, MESHFX_ANIM ) )
  {
    if ( type >= ( MAXMESHTYPE >> 1 ) )
    {
      // Big tiles
      basetile = tile & GTile_Anim.bigbaseand;// Animation set
      tile += GTile_Anim.frameadd << 1;         // Animated tile
      tile = ( tile & GTile_Anim.bigframeand ) + basetile;
    }
    else
    {
      // Small tiles
      basetile = tile & GTile_Anim.baseand;// Animation set
      tile += GTile_Anim.frameadd;         // Animated tile
      tile = ( tile & GTile_Anim.frameand ) + basetile;
    }
  }

  if(type >= (MAXMESHTYPE>>1))
  {
    retval = &bmpbigtile[tile];
  }
  else
  {
    retval = &bmpsmalltile[tile];
  }

  return retval;
}

//------------------------------------------------------------------------------
int fan_at(int x, int y)
{
  int fan;

  if(x < 0 || x >= mesh.size_x || y < 0 || y >= mesh.size_y)
  {
    return INVALID_FAN;
  }

  fan = mesh_convert_fan(x, y);
  return fan;
}

//------------------------------------------------------------------------------
void weld_0(int x, int y)
{
  clear_select();
  add_select(get_vertex(x, y, 0));
  add_select(get_vertex(x-1, y, 1));
  add_select(get_vertex(x, y-1, 3));
  add_select(get_vertex(x-1, y-1, 2));
  weld_select();
  clear_select();
  return;
}

//------------------------------------------------------------------------------
void weld_1(int x, int y)
{
  clear_select();
  add_select(get_vertex(x, y, 1));
  add_select(get_vertex(x+1, y, 0));
  add_select(get_vertex(x, y-1, 2));
  add_select(get_vertex(x+1, y-1, 3));
  weld_select();
  clear_select();
  return;
}

//------------------------------------------------------------------------------
void weld_2(int x, int y)
{
  clear_select();
  add_select(get_vertex(x, y, 2));
  add_select(get_vertex(x+1, y, 3));
  add_select(get_vertex(x, y+1, 1));
  add_select(get_vertex(x+1, y+1, 0));
  weld_select();
  clear_select();
  return;
}

//------------------------------------------------------------------------------
void weld_3(int x, int y)
{
  clear_select();
  add_select(get_vertex(x, y, 3));
  add_select(get_vertex(x-1, y, 2));
  add_select(get_vertex(x, y+1, 0));
  add_select(get_vertex(x-1, y+1, 1));
  weld_select();
  clear_select();
  return;
}

//------------------------------------------------------------------------------
void weld_cnt(int x, int y, int cnt, Uint32 fan)
{
  if(Mesh[Mesh_Fan[fan].type].commandu[cnt] < NEARLOW+1 ||
    Mesh[Mesh_Fan[fan].type].commandv[cnt] < NEARLOW+1 ||
    Mesh[Mesh_Fan[fan].type].commandu[cnt] > NEARHI-1 ||
    Mesh[Mesh_Fan[fan].type].commandv[cnt] > NEARHI-1)
  {
    clear_select();
    add_select(get_vertex(x, y, cnt));
    if(Mesh[Mesh_Fan[fan].type].commandu[cnt] < NEARLOW+1)
      add_select(nearest_vertex(x-1, y, NEARHI, Mesh[Mesh_Fan[fan].type].commandv[cnt]));
    if(Mesh[Mesh_Fan[fan].type].commandv[cnt] < NEARLOW+1)
      add_select(nearest_vertex(x, y-1, Mesh[Mesh_Fan[fan].type].commandu[cnt], NEARHI));
    if(Mesh[Mesh_Fan[fan].type].commandu[cnt] > NEARHI-1)
      add_select(nearest_vertex(x+1, y, NEARLOW, Mesh[Mesh_Fan[fan].type].commandv[cnt]));
    if(Mesh[Mesh_Fan[fan].type].commandv[cnt] > NEARHI-1)
      add_select(nearest_vertex(x, y+1, Mesh[Mesh_Fan[fan].type].commandu[cnt], NEARLOW));
    weld_select();
    clear_select();
  }
  return;
}

//------------------------------------------------------------------------------
void fix_corners(int x, int y)
{
  Uint32 fan;


  fan = mesh_convert_fan(x, y);
  weld_0(x, y);
  weld_1(x, y);
  weld_2(x, y);
  weld_3(x, y);
}

//------------------------------------------------------------------------------
void fix_vertices(int x, int y)
{
  Uint32 fan;
  int cnt;

  fix_corners(x, y);
  fan = mesh_convert_fan(x, y);
  cnt = 4;
  while(cnt < Mesh_Cmd[Mesh_Fan[fan].type].vrt_count)
  {
    weld_cnt(x, y, cnt, fan);
    cnt++;
  }
}

//------------------------------------------------------------------------------
void fix_mesh(void)
{
  // ZZ> This function corrects corners across entire mesh
  int x, y;

  y = 0;
  while(y < mesh.size_y)
  {
    x = 0;
    while(x < mesh.size_x)
    {
      //      fix_corners(x, y);
      fix_vertices(x, y);
      x++;
    }
    y++;
  }
  return;
}


//------------------------------------------------------------------------------
char tile_is_different(int x, int y, Uint16 tileset,
                       Uint16 tileand)
{
  // ZZ> bfalse if of same set, btrue if different
  Uint32 fan;
  if(x < 0 || x >= mesh.size_x || y < 0 || y >= mesh.size_y)
  {
    return bfalse;
  }

  fan = mesh_convert_fan(x, y);
  if(tileand == 192)
  {
    if(Mesh_Fan[fan].tile >= 48) return bfalse;
  }

  if((Mesh_Fan[fan].tile&tileand) == tileset)
  {
    return bfalse;
  }
  return btrue;
}

//------------------------------------------------------------------------------
Uint16 trim_code(int x, int y, Uint16 tileset)
{
  // ZZ> This function returns the standard tile set value thing...  For
  //     Trimming tops of walls and floors

  Uint16 code;


  if(tile_is_different(x, y-1, tileset, 240))
  {
    // Top
    code = 0;
    if(tile_is_different(x-1, y, tileset, 240))
    {
      // Left
      code = 8;
    }
    if(tile_is_different(x+1, y, tileset, 240))
    {
      // Right
      code = 9;
    }
    return code;
  }
  if(tile_is_different(x, y+1, tileset, 240))
  {
    // Bottom
    code = 1;
    if(tile_is_different(x-1, y, tileset, 240))
    {
      // Left
      code = 10;
    }
    if(tile_is_different(x+1, y, tileset, 240))
    {
      // Right
      code = 11;
    }
    return code;
  }
  if(tile_is_different(x-1, y, tileset, 240))
  {
    // Left
    code = 2;
    return code;
  }
  if(tile_is_different(x+1, y, tileset, 240))
  {
    // Right
    code = 3;
    return code;
  }



  if(tile_is_different(x+1, y+1, tileset, 240))
  {
    // Bottom Right
    code = 4;
    return code;
  }
  if(tile_is_different(x-1, y+1, tileset, 240))
  {
    // Bottom Left
    code = 5;
    return code;
  }
  if(tile_is_different(x+1, y-1, tileset, 240))
  {
    // Top Right
    code = 6;
    return code;
  }
  if(tile_is_different(x-1, y-1, tileset, 240))
  {
    // Top Left
    code = 7;
    return code;
  }


  code = 255;
  return code;
}

//------------------------------------------------------------------------------
Uint16 wall_code(int x, int y, Uint16 tileset)
{
  // ZZ> This function returns the standard tile set value thing...  For
  //     Trimming tops of walls and floors

  Uint16 code;


  if(tile_is_different(x, y-1, tileset, 192))
  {
    // Top
    code = (rand()&2) + 20;
    if(tile_is_different(x-1, y, tileset, 192))
    {
      // Left
      code = 48;
    }
    if(tile_is_different(x+1, y, tileset, 192))
    {
      // Right
      code = 50;
    }
    return code;
  }

  if(tile_is_different(x, y+1, tileset, 192))
  {
    // Bottom
    code = (rand()&2);
    if(tile_is_different(x-1, y, tileset, 192))
    {
      // Left
      code = 52;
    }
    if(tile_is_different(x+1, y, tileset, 192))
    {
      // Right
      code = 54;
    }
    return code;
  }

  if(tile_is_different(x-1, y, tileset, 192))
  {
    // Left
    code = (rand()&2) + 16;
    return code;
  }
  if(tile_is_different(x+1, y, tileset, 192))
  {
    // Right
    code = (rand()&2) + 4;
    return code;
  }


  if(tile_is_different(x+1, y+1, tileset, 192))
  {
    // Bottom Right
    code = 32;
    return code;
  }

  if(tile_is_different(x-1, y+1, tileset, 192))
  {
    // Bottom Left
    code = 34;
    return code;
  }

  if(tile_is_different(x+1, y-1, tileset, 192))
  {
    // Top Right
    code = 36;
    return code;
  }

  if(tile_is_different(x-1, y-1, tileset, 192))
  {
    // Top Left
    code = 38;
    return code;
  }

  code = 255;
  return code;
}

//------------------------------------------------------------------------------
void trim_mesh_tile(Uint16 tileset, Uint16 tileand)
{
  // ZZ> This function trims walls and floors and tops automagically
  Uint32 fan;
  int x, y, code;


  tileset = tileset&tileand;
  y = 0;
  while(y < mesh.size_y)
  {
    x = 0;
    while(x < mesh.size_x)
    {
      fan = mesh_convert_fan(x, y);
      if((Mesh_Fan[fan].tile&tileand) == tileset)
      {
        if(tileand == 192)
        {
          code = wall_code(x, y, tileset);
        }
        else
        {
          code = trim_code(x, y, tileset);
        }
        if(code != 255)
        {
          Mesh_Fan[fan].tile = tileset + code;
        }
      }
      x++;
    }
    y++;
  }
  return;
}

//------------------------------------------------------------------------------
void fx_mesh_tile(Uint16 tileset, Uint16 tileand, Uint8 fx)
{
  // ZZ> This function sets the fx for a group of tiles
  Uint32 fan;
  int x, y;


  tileset = tileset&tileand;
  y = 0;
  while(y < mesh.size_y)
  {
    x = 0;
    while(x < mesh.size_x)
    {
      fan = mesh_convert_fan(x, y);
      if((Mesh_Fan[fan].tile&tileand) == tileset)
      {
        Mesh_Fan[fan].fx = fx;
      }
      x++;
    }
    y++;
  }
  return;
}

//------------------------------------------------------------------------------
void set_mesh_tile(Uint16 tiletoset)
{
  // ZZ> This function sets one tile type to another
  Uint32 fan;
  int x, y;

  y = 0;
  while(y < mesh.size_y)
  {
    x = 0;
    while(x < mesh.size_x)
    {
      fan = mesh_convert_fan(x, y);
      if(Mesh_Fan[fan].tile == tiletoset)
      {
        switch(mouseinwinpresser)
        {
        case 0:
          Mesh_Fan[fan].tile=mouseinwintile;
          break;
        case 1:
          Mesh_Fan[fan].tile=(mouseinwintile&0xfffe)+(rand()&1);
          break;
        case 2:
          Mesh_Fan[fan].tile=(mouseinwintile&0xfffc)+(rand()&3);
          break;
        case 3:
          Mesh_Fan[fan].tile=(mouseinwintile&0xfff0)+(rand()&6);
          break;
        }
      }
      x++;
    }
    y++;
  }
  return;
}

//------------------------------------------------------------------------------
void setup_mesh(void)
{
  // ZZ> This function makes the mesh
  int x, y, fan, tile;


  free_vertices();
  printf("Mesh file not found, so creating a new one...\n");
  printf("Number of tiles in X direction ( 32-512 ):  ");
  scanf("%d", &mesh.size_x);
  printf("Number of tiles in Y direction ( 32-512 ):  ");
  scanf("%d", &mesh.size_y);
  mesh.edge_x = (mesh.size_x*SMALLX)-1;
  mesh.edge_y = (mesh.size_y*SMALLY)-1;
  meshedgez = 180<<4;


  fan = 0;
  y = 0;
  tile = 0;
  while(y < mesh.size_y)
  {
    x = 0;
    while(x < mesh.size_x)
    {
      Mesh_Fan[fan].type = 2+0;
      Mesh_Fan[fan].tile = (((x&1)+(y&1))&1)+62;
      if(add_fan(fan, x*31, y*31)==bfalse)
      {
        log_error("NOT ENOUGH VERTICES!!!\n\n");
      }
      fan++;
      x++;
    }
    y++;
  }

  make_fanstart();
  fix_mesh();
}

//------------------------------------------------------------------------------
void rip_small_tiles(SDL_Surface * bmpload)
{
  SDL_Surface *bmpsmall, *bmptiny;
  int x, y;

  bmpsmall = SDL_CreateRGBSurface(SDL_SWSURFACE, SMALLX, SMALLY, 32, rmask, gmask, bmask, amask);
  bmptiny  = SDL_CreateRGBSurface(SDL_SWSURFACE, TINYX, TINYY, 32, rmask, gmask, bmask, amask);


  y = 0;
  while(y < 256)
  {
    x = 0;
    while(x < 256)
    {
      SDL_Rect src = {x, y, TINYX, TINYY};

      SDL_BlitSurface(bmpload, &src, bmpsmall, &bmpsmall->clip_rect);
      GLTexture_Convert( GL_TEXTURE_2D, &bmpsmalltile[numsmalltile], bmpsmall, INVALID_KEY);

      SDL_BlitSurface(bmpload, &src, bmptiny, &bmptiny->clip_rect);
      GLTexture_Convert( GL_TEXTURE_2D, &bmptinysmalltile[numsmalltile], bmptiny, INVALID_KEY);

      numsmalltile++;
      x+=32;
    }
    y+=32;
  }

  SDL_FreeSurface(bmpsmall);
  SDL_FreeSurface(bmptiny);

  return;
}

//------------------------------------------------------------------------------
void rip_big_tiles(SDL_Surface* bmpload)
{
  SDL_Surface *bmpsmall, *bmptiny;
  int x, y;

  bmpsmall = SDL_CreateRGBSurface(SDL_SWSURFACE, SMALLX, SMALLY, 32, rmask, gmask, bmask, amask);
  bmptiny  = SDL_CreateRGBSurface(SDL_SWSURFACE, TINYX, TINYY, 32, rmask, gmask, bmask, amask);

  y = 0;
  while(y < 256)
  {
    x = 0;
    while(x < 256)
    {
      SDL_Rect src = {x, y, x + BIGX, y + BIGY};

      SDL_BlitSurface(bmpload, &src, bmpsmall, &bmpsmall->clip_rect);
      GLTexture_Convert(GL_TEXTURE_2D, &bmpbigtile[numbigtile], bmpsmall, INVALID_KEY);

      SDL_BlitSurface(bmpload, &src, bmptiny, &bmptiny->clip_rect);
      GLTexture_Convert(GL_TEXTURE_2D, &bmptinybigtile[numbigtile], bmptiny, INVALID_KEY);

      numbigtile++;
      x+=32;
    }
    y+=32;
  }
  SDL_FreeSurface(bmpsmall);
  SDL_FreeSurface(bmptiny);

  return;
}

//------------------------------------------------------------------------------
void rip_tiles(SDL_Surface * bmpload)
{
  rip_small_tiles(bmpload);
  rip_big_tiles(bmpload);
}

////------------------------------------------------------------------------------
//void make_newloadname(char *modname, char *appendname, char *newloadname)
//{
//  // ZZ> This function takes some names and puts 'em together
//  int cnt, tnc;
//  char ctmp;
//
//  cnt = 0;
//  ctmp = ModList[cnt].name;
//  while(ctmp != 0)
//  {
//    newloadname[cnt] = ctmp;
//    cnt++;
//    ctmp = ModList[cnt].name;
//  }
//  tnc = 0;
//  ctmp = appendname[tnc];
//  while(ctmp != 0)
//  {
//    newloadname[cnt] = ctmp;
//    cnt++;
//    tnc++;
//    ctmp = appendname[tnc];
//  }
//  newloadname[cnt] = 0;
//
//  return;
//}

//------------------------------------------------------------------------------
void cart_load_basic_textures(char *modname)
{
  // ZZ> This function loads the standard textures for a module
  char newloadname[256];
  SDL_Surface * surface;

  snprintf(newloadname, sizeof(newloadname), "%s\\gamedat\\tile0.bmp", modname);
  surface = IMG_Load(newloadname);
  rip_tiles(surface);

  snprintf(newloadname, sizeof(newloadname), "%s\\gamedat\\tile1.bmp", modname);
  surface = IMG_Load(newloadname);
  rip_tiles(surface);

  snprintf(newloadname, sizeof(newloadname), "%s\\gamedat\\tile2.bmp", modname);
  surface = IMG_Load(newloadname);
  rip_tiles(surface);

  snprintf(newloadname, sizeof(newloadname), "%s\\gamedat\\tile3.bmp", modname);
  surface = IMG_Load(newloadname);
  rip_tiles(surface);

  //bmpfanoff = SDL_CreateRGBSurface(SDL_SWSURFACE, SMALLX, SMALLY, 32, rmask, gmask, bmask, amask);
  //draw_blit_sprite(bmpfanoff, imgpointon[10], (SMALLX-(*imgpointon[10]).txW)/2, (SMALLY-(*imgpointon[10]).txH)/2);
  return;
}


//------------------------------------------------------------------------------
//void show_name(char *newloadname)
//{
//  textout(screen, font, newloadname, 0, OUTY-16, make_color(31, 31, 31));
//  return;
//}

////------------------------------------------------------------------------------
//void make_twist()
//{
//  Uint32 fan, numfan;
//
//  numfan = mesh.size_x*mesh.size_y;
//  fan = 0;
//  while(fan < numfan)
//  {
//    Mesh_Fan[fan].twist=get_fan_twist(fan);
//    fan++;
//  }
//
//
//  return;
//}

//------------------------------------------------------------------------------
int count_vertices()
{
  int fan, x, y, cnt, num, totalvert;
  Uint32 vert;

  totalvert = 0;
  y = 0;
  while(y < mesh.size_y)
  {
    x = 0;
    while(x < mesh.size_x)
    {
      fan = mesh_convert_fan(x, y);
      num = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
      vert = Mesh_Fan[fan].vrt_start;
      cnt = 0;
      while(cnt < num)
      {
        totalvert++;
        vert = Mesh_Mem.vrt_next[vert];
        cnt++;
      }
      x++;
    }
    y++;
  }
  return totalvert;
}

//------------------------------------------------------------------------------
void save_mesh(char *modname)
{
#define SAVE numwritten+=fwrite(&itmp, 4, 1, filewrite); numattempt++
#define SAVEF numwritten+=fwrite(&ftmp, 4, 1, filewrite); numattempt++
  FILE* filewrite;
  char newloadname[256];
  int itmp;
  float ftmp;
  int fan, x, y, cnt, num;
  Uint32 vert;
  Uint8 ctmp;


  numwritten = 0;
  numattempt = 0;
  make_newloadname(modname, "\\gamedat\\plan.bmp", newloadname);
  make_planmap();
  if(bmphitemap)
  {
    SDL_SaveBMP(bmphitemap, newloadname);
  }


  make_newloadname(modname, "\\gamedat\\level.bmp", newloadname);
  make_hitemap();
  if(bmphitemap)
  {
    SDL_SaveBMP(bmphitemap, newloadname);
  }
  make_twist();


  make_newloadname(modname, "\\gamedat\\level.mpd", newloadname);
  debug_message(1, newloadname);

  filewrite = fopen(newloadname, "wb");
  if(filewrite)
  {
    itmp=MAPID;  SAVE;
    //    This didn't work for some reason...
    //    itmp=MAXTOTALMESHVERTICES-numfreevertices;  SAVE;
    itmp = count_vertices();  SAVE;
    itmp=mesh.size_x;  SAVE;
    itmp=mesh.size_y;  SAVE;

    // Write tile data
    y = 0;
    while(y < mesh.size_y)
    {
      x = 0;
      while(x < mesh.size_x)
      {
        fan = mesh_convert_fan(x, y);
        itmp = (Mesh_Fan[fan].type<<24)+(Mesh_Fan[fan].fx<<16)+Mesh_Fan[fan].tile;  SAVE;
        x++;
      }
      y++;
    }

    // Write twist data
    y = 0;
    while(y < mesh.size_y)
    {
      x = 0;
      while(x < mesh.size_x)
      {
        fan = mesh_convert_fan(x, y);
        ctmp = Mesh_Fan[fan].twist;  numwritten+=fwrite(&ctmp, 1, 1, filewrite);
        numattempt++;
        x++;
      }
      y++;
    }

    // Write x vertices
    y = 0;
    while(y < mesh.size_y)
    {
      x = 0;
      while(x < mesh.size_x)
      {
        fan = mesh_convert_fan(x, y);
        num = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
        vert = Mesh_Fan[fan].vrt_start;
        cnt = 0;
        while(cnt < num)
        {
          ftmp = Mesh_Mem.vrt_x[vert]*FIXNUM;  SAVEF;
          vert = Mesh_Mem.vrt_next[vert];
          cnt++;
        }
        x++;
      }
      y++;
    }

    // Write y vertices
    y = 0;
    while(y < mesh.size_y)
    {
      x = 0;
      while(x < mesh.size_x)
      {
        fan = mesh_convert_fan(x, y);
        num = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
        vert = Mesh_Fan[fan].vrt_start;
        cnt = 0;
        while(cnt < num)
        {
          ftmp = Mesh_Mem.vrt_y[vert]*FIXNUM;  SAVEF;
          vert = Mesh_Mem.vrt_next[vert];
          cnt++;
        }
        x++;
      }
      y++;
    }

    // Write z vertices
    y = 0;
    while(y < mesh.size_y)
    {
      x = 0;
      while(x < mesh.size_x)
      {
        fan = mesh_convert_fan(x, y);
        num = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
        vert = Mesh_Fan[fan].vrt_start;
        cnt = 0;
        while(cnt < num)
        {
          ftmp = Mesh_Mem.vrt_z[vert]*FIXNUM;  SAVEF;
          vert = Mesh_Mem.vrt_next[vert];
          cnt++;
        }
        x++;
      }
      y++;
    }


    // Write a vertices
    y = 0;
    while(y < mesh.size_y)
    {
      x = 0;
      while(x < mesh.size_x)
      {
        fan = mesh_convert_fan(x, y);
        num = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
        vert = Mesh_Fan[fan].vrt_start;
        cnt = 0;
        while(cnt < num)
        {
          ctmp = Mesh_Mem.vrt_a[vert];  numwritten+=fwrite(&ctmp, 1, 1, filewrite);
          numattempt++;
          vert = Mesh_Mem.vrt_next[vert];
          cnt++;
        }
        x++;
      }
      y++;
    }
  }

  return;
}
//------------------------------------------------------------------------------
//int load_mesh(char *modname)
//{
//  FILE* fileread;
//  char newloadname[256];
//  int itmp, num, cnt;
//  float ftmp;
//  Uint32 fan;
//  Uint32 numvert, numfan;
//  Uint32 vert;
//  int x, y;
//
//
//  make_newloadname(modname, "\\gamedat\\level.mpd", newloadname);
//  fileread = fopen(newloadname, "rb");
//  if(fileread)
//  {
//    free_vertices();
//    fread(&itmp, 4, 1, fileread);  if(itmp != MAPID) return bfalse;
//    fread(&itmp, 4, 1, fileread);  numvert = itmp;
//    fread(&itmp, 4, 1, fileread);  mesh.size_x = itmp;
//    fread(&itmp, 4, 1, fileread);  mesh.size_y = itmp;
//    numfan = mesh.size_x*mesh.size_y;
//    mesh.edge_x = (mesh.size_x*SMALLX)-1;
//    mesh.edge_y = (mesh.size_y*SMALLY)-1;
//    meshedgez = 180<<4;
//    numfreevertices = MAXTOTALMESHVERTICES-numvert;
//
//
//    // Load fan data
//    fan = 0;
//    while(fan < numfan)
//    {
//      fread(&itmp, 4, 1, fileread);
//      Mesh_Fan[fan].type = itmp>>24;
//      Mesh_Fan[fan].fx = itmp>>16;
//      Mesh_Fan[fan].tile = itmp;
//      fan++;
//    }
//    // Load normal data
//    fan = 0;
//    while(fan < numfan)
//    {
//      fread(&itmp, 1, 1, fileread);
//      Mesh_Fan[fan].twist = itmp;
//      fan++;
//    }
//
//
//
//
//
//
//    // Load vertex x data
//    cnt = 0;
//    while(cnt < numvert)
//    {
//      fread(&ftmp, 4, 1, fileread);
//      Mesh_Mem.vrt_x[cnt] = ftmp/FIXNUM;
//      cnt++;
//    }
//    // Load vertex y data
//    cnt = 0;
//    while(cnt < numvert)
//    {
//      fread(&ftmp, 4, 1, fileread);
//      Mesh_Mem.vrt_y[cnt] = ftmp/FIXNUM;
//      cnt++;
//    }
//    // Load vertex z data
//    cnt = 0;
//    while(cnt < numvert)
//    {
//      fread(&ftmp, 4, 1, fileread);
//      Mesh_Mem.vrt_z[cnt] = ftmp/FIXNUM;
////      Mesh_Mem.vrt_z[cnt] = 0;
//      cnt++;
//    }
//    // Load vertex a data
//    cnt = 0;
//    while(cnt < numvert)
//    {
////      fread(&itmp, 1, 1, fileread);
//      Mesh_Mem.vrt_a[cnt] = 60;  // !!!BAD!!!
//      cnt++;
//    }
//
//
//    make_fanstart();
//    vert = 0;
//    y = 0;
//    while(y < mesh.size_y)
//    {
//      x = 0;
//      while(x < mesh.size_x)
//      {
//        fan = mesh_convert_fan(x, y);
//        num = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
//        Mesh_Fan[fan].vrt_start = vert;
//        cnt = 0;
//        while(cnt < num)
//        {
//          Mesh_Mem.vrt_next[vert] = vert+1;
//          vert++;
//          cnt++;
//        }
//        Mesh_Mem.vrt_next[vert-1] = CHAINEND;
//        x++;
//      }
//      y++;
//    }
//    return btrue;
//  }
//  return bfalse;
//}

//------------------------------------------------------------------------------
void move_select(int x, int y, int z)
{
  int vert, cnt, newx, newy, newz;


  cnt = 0;
  while(cnt < numselect)
  {
    vert = select[cnt];
    newx = Mesh_Mem.vrt_x[vert]+x;
    newy = Mesh_Mem.vrt_y[vert]+y;
    newz = Mesh_Mem.vrt_z[vert]+z;
    if(newx<0)  x=0-Mesh_Mem.vrt_x[vert];
    if(newx>mesh.edge_x) x=mesh.edge_x-Mesh_Mem.vrt_x[vert];
    if(newy<0)  y=0-Mesh_Mem.vrt_y[vert];
    if(newy>mesh.edge_y) y=mesh.edge_y-Mesh_Mem.vrt_y[vert];
    if(newz<0)  z=0-Mesh_Mem.vrt_z[vert];
    if(newz>meshedgez) z=meshedgez-Mesh_Mem.vrt_z[vert];
    cnt++;
  }



  cnt = 0;
  while(cnt < numselect)
  {
    vert = select[cnt];
    newx = Mesh_Mem.vrt_x[vert]+x;
    newy = Mesh_Mem.vrt_y[vert]+y;
    newz = Mesh_Mem.vrt_z[vert]+z;


    if(newx<0)  newx=0;
    if(newx>mesh.edge_x)  newx=mesh.edge_x;
    if(newy<0)  newy=0;
    if(newy>mesh.edge_y)  newy=mesh.edge_y;
    if(newz<0)  newz=0;
    if(newz>meshedgez)  newz=meshedgez;


    Mesh_Mem.vrt_x[vert]=newx;
    Mesh_Mem.vrt_y[vert]=newy;
    Mesh_Mem.vrt_z[vert]=newz;
    cnt++;
  }
  return;
}

//------------------------------------------------------------------------------
void set_select_no_bound_z(int z)
{
  int vert, cnt;


  cnt = 0;
  while(cnt < numselect)
  {
    vert = select[cnt];
    Mesh_Mem.vrt_z[vert]=z;
    cnt++;
  }
  return;
}

//------------------------------------------------------------------------------
void move_mesh_z(int z, Uint16 tiletype, Uint16 tileand)
{
  int vert, cnt, newz, x, y, totalvert;
  Uint32 fan;


  tiletype = tiletype & tileand;
  y = 0;
  while(y < mesh.size_y)
  {
    x = 0;
    while(x < mesh.size_x)
    {
      fan = mesh_convert_fan(x, y);
      if((Mesh_Fan[fan].tile&tileand) == tiletype)
      {
        vert = Mesh_Fan[fan].vrt_start;
        totalvert = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
        cnt = 0;
        while(cnt < totalvert)
        {
          newz = Mesh_Mem.vrt_z[vert]+z;
          if(newz<0)  newz=0;
          if(newz>meshedgez) newz=meshedgez;
          Mesh_Mem.vrt_z[vert] = newz;
          vert = Mesh_Mem.vrt_next[vert];
          cnt++;
        }
      }
      x++;
    }
    y++;
  }
  return;
}

//------------------------------------------------------------------------------
void move_vert(int vert, int x, int y, int z)
{
  int newx, newy, newz;


  newx = Mesh_Mem.vrt_x[vert]+x;
  newy = Mesh_Mem.vrt_y[vert]+y;
  newz = Mesh_Mem.vrt_z[vert]+z;


  if(newx<0)  newx=0;
  if(newx>mesh.edge_x)  newx=mesh.edge_x;
  if(newy<0)  newy=0;
  if(newy>mesh.edge_y)  newy=mesh.edge_y;
  if(newz<0)  newz=0;
  if(newz>meshedgez)  newz=meshedgez;


  Mesh_Mem.vrt_x[vert]=newx;
  Mesh_Mem.vrt_y[vert]=newy;
  Mesh_Mem.vrt_z[vert]=newz;


  return;
}

//------------------------------------------------------------------------------
void raise_mesh(int x, int y, int amount, int size)
{
  int disx, disy, dis, cnt, newamount;
  Uint32 vert;


  cnt = 0;
  while(cnt < numpointsonscreen)
  {
    vert = pointsonscreen[cnt];
    disx = Mesh_Mem.vrt_x[vert]-(x/FOURNUM);
    disy = Mesh_Mem.vrt_y[vert]-(y/FOURNUM);
    dis = sqrt(disx*disx+disy*disy);


    newamount = abs(amount)-((dis<<1)>>size);
    if(newamount < 0) newamount = 0;
    if(amount < 0)  newamount = -newamount;
    move_vert(vert, 0, 0, newamount);


    cnt++;
  }


  return;
}

//------------------------------------------------------------------------------
void cart_load_module(char *modname)
{
  char newloadname[256];


  make_newloadname("..\\modules\\", modname, newloadname);
  //  show_name(newloadname);
  cart_load_basic_textures(newloadname);
  if(!load_mesh(newloadname))
  {
    setup_mesh();
  }
  numlight = 0;
  addinglight = 0;
  return;
}

//------------------------------------------------------------------------------
void render_tile_window(int window)
{
  GLtexture *bmptile;
  int x, y, xstt, ystt, cntx, cnty, numx, numy, mapx, mapy, mapxstt, mapystt;
  int cnt;


  mapxstt = (cart_pos_x-(window_rect[window].w>>1))/31;
  mapystt = (cart_pos_y-(window_rect[window].h>>1))/31;
  numx = (window_rect[window].w/SMALLX)+3;
  numy = (window_rect[window].h/SMALLY)+3;
  xstt = -((cart_pos_x-(window_rect[window].w>>1))%31)-(SMALLX);
  ystt = -((cart_pos_y-(window_rect[window].h>>1))%31)-(SMALLY);


  y = ystt;
  mapy = mapystt;
  cnty = 0;
  set_window_viewport( window );
  while(cnty < numy)
  {
    x = xstt;
    mapx = mapxstt;
    cntx = 0;
    while(cntx < numx)
    {
      FRect dst = {x, y, x + SMALLX, y + SMALLY};

      bmptile = tile_at(mapx, mapy);

      draw_texture_box(bmptile, NULL, &dst);

      mapx++;
      cntx++;
      x+=31;
    }
    mapy++;
    cnty++;
    y+=31;
  }


  cnt = 0;
  while(cnt < numlight)
  {
    draw_light(window, cnt);
    cnt++;
  }


  return;
}

//------------------------------------------------------------------------------
void render_fx_window(int window)
{
  GLtexture * bmptile;
  int x, y, xstt, ystt, cntx, cnty, numx, numy, mapx, mapy, mapxstt, mapystt;
  Uint32 fan;


  mapxstt = (cart_pos_x-(window_rect[window].w>>1))/31;
  mapystt = (cart_pos_y-(window_rect[window].h>>1))/31;
  numx = (window_rect[window].w/SMALLX)+3;
  numy = (window_rect[window].h/SMALLY)+3;
  xstt = -((cart_pos_x-(window_rect[window].w>>1))%31)-(SMALLX);
  ystt = -((cart_pos_y-(window_rect[window].h>>1))%31)-(SMALLY);

  set_window_viewport( window );

  y = ystt;
  mapy = mapystt;
  cnty = 0;
  while(cnty < numy)
  {
    x = xstt;
    mapx = mapxstt;
    cntx = 0;
    while(cntx < numx)
    {
      FRect dst = {x, y, SMALLX-1, SMALLY-1};
      bmptile = tile_at(mapx, mapy);
      draw_texture_box(bmptile, NULL, &dst);

      fan = fan_at(mapx, mapy);
      if(fan!=-1)
      {
        if( mesh_has_no_bits( fan, MESHFX_NOREFLECT ) )
          draw_blit_sprite(window, &imgref, x, y);
        if( mesh_has_some_bits( fan, MESHFX_SHINY ) )
          draw_blit_sprite(window, &imgdrawref, x+16, y);
        if( mesh_has_some_bits( fan, MESHFX_ANIM ) )
          draw_blit_sprite(window, &imganim, x, y+16);
        if( mesh_has_some_bits( fan, MESHFX_WALL ) )
          draw_blit_sprite(window, &imgwall, x+15, y+15);
        if( mesh_has_some_bits( fan, MESHFX_IMPASS ) )
          draw_blit_sprite(window, &imgimpass, x+15+8, y+15);
        if( mesh_has_some_bits( fan, MESHFX_DAMAGE ) )
          draw_blit_sprite(window, &imgdamage, x+15, y+15+8);
        if( mesh_has_some_bits( fan, MESHFX_SLIPPY ) )
          draw_blit_sprite(window, &imgslippy, x+15+8, y+15+8);
        if( mesh_has_some_bits( fan, MESHFX_WATER ) )
          draw_blit_sprite(window, &imgwater, x, y);
      }
      mapx++;
      cntx++;
      x+=31;
    }
    mapy++;
    cnty++;
    y+=31;
  }


  return;
}



//------------------------------------------------------------------------------
void render_vertex_window(int window)
{
  int x, y, cntx, cnty, numx, numy, mapx, mapy, mapxstt, mapystt;
  Uint32 fan;


  //  numpointsonscreen = 0;
  mapxstt = (cart_pos_x-(window_rect[window].w>>1))/31;
  mapystt = (cart_pos_y-(window_rect[window].h>>1))/31;
  numx = (window_rect[window].w/SMALLX)+3;
  numy = (window_rect[window].h/SMALLY)+3;
  x = -cart_pos_x+(window_rect[window].h>>1)-SMALLX;
  y = -cart_pos_y+(window_rect[window].h>>1)-SMALLY;


  mapy = mapystt;
  cnty = 0;
  while(cnty < numy)
  {
    if(mapy>=0 && mapy<mesh.size_y)
    {
      mapx = mapxstt;
      cntx = 0;
      while(cntx < numx)
      {
        if(mapx>=0 && mapx<mesh.size_x)
        {
          fan = mesh_convert_fan(mapx, mapy);
          draw_top_fan(window, fan, x, y);
        }
        mapx++;
        cntx++;
      }
    }
    mapy++;
    cnty++;
  }


  if(mouseinwinrect && mouseinwinmode==WINVERTEX)
  {
    draw_rect(window, make_color(16+(wldframe&15), 16+(wldframe&15), 0),
      (mouseinwinrectx/FOURNUM)+x, (mouseinwinrecty/FOURNUM)+y,
      (mouseinwinx/FOURNUM)+x, (mouseinwiny/FOURNUM)+y );
  }


  if((SDLKEYDOWN(SDLK_p) || ((mous.latch.b&2) && numselect == 0)) && mouseinwinmode==WINVERTEX)
  {
    raise_mesh(mouseinwinx, mouseinwiny, brushamount, brushsize);
  }



  return;
}



//------------------------------------------------------------------------------
void render_side_window(int window)
{
  int x, y, cntx, cnty, numx, numy, mapx, mapy, mapxstt, mapystt;
  Uint32 fan;

  set_window_viewport( window );

  mapxstt = (cart_pos_x-(window_rect[window].w>>1))/31;
  mapystt = (cart_pos_y-(window_rect[window].h>>1))/31;
  numx = (window_rect[window].w/SMALLX)+3;
  numy = (window_rect[window].h/SMALLY)+3;
  x = -cart_pos_x+(window_rect[window].w>>1)-SMALLX;
  y = window_rect[window].h-10;


  mapy = mapystt;
  cnty = 0;
  while(cnty < numy)
  {
    if(mapy>=0 && mapy<mesh.size_y)
    {
      mapx = mapxstt;
      cntx = 0;
      while(cntx < numx)
      {
        if(mapx>=0 && mapx<mesh.size_x)
        {
          fan = mesh_convert_fan(mapx, mapy);
          draw_side_fan(window, fan, x, y);
        }
        mapx++;
        cntx++;
      }
    }
    mapy++;
    cnty++;
  }

  if(mouseinwinrect && mouseinwinmode==WINSIDE)
  {
    draw_rect(window, make_color(16+(wldframe&15), 16+(wldframe&15), 0),
      (mouseinwinrectx/FOURNUM)+x, (mouseinwiny/FOURNUM), (mouseinwinx/FOURNUM)+x, (mouseinwinrecty/FOURNUM) );
  }


  return;
}

//------------------------------------------------------------------------------
void render_window(int window)
{

  make_onscreen();
  if(windowon[window])
  {

    if(windowmode[window]&WINTILE)
    {
      render_tile_window(window);
    }
    else
    {
      // Untiled bitmaps clear
      clear_to_color(window, make_color(0, 0, 0));
    }

    if(windowmode[window]&WINFX)
    {
      render_fx_window(window);
    }

    if(windowmode[window]&WINVERTEX)
    {
      render_vertex_window(window);
    }

    if(windowmode[window]&WINSIDE)
    {
      render_side_window(window);
    }

    draw_cursor_in_window(window);
  }

  return;
}

//------------------------------------------------------------------------------
void load_window(int window, char *loadname, int x, int y, int bx, int by,
                 int sx, int sy, Uint16 mode)
{
  GLTexture_Load(GL_TEXTURE_2D, &window_tx[window], loadname, -1);
  windowborderx[window] = bx;
  windowbordery[window] = by;
  window_rect[window].x = x;
  window_rect[window].y = y;
  window_rect[window].w = sx;
  window_rect[window].h = sy;
  windowon[window] = btrue;
  windowmode[window] = mode;
  return;
}

//------------------------------------------------------------------------------
void render_all_windows(void)
{
  int cnt;
  cnt = 0;
  while(cnt < MAXWIN)
  {
    render_window(cnt);
    cnt++;
  }
  return;
}

//------------------------------------------------------------------------------
void load_all_windows(void)
{
  int cnt;
  cnt = 0;
  while(cnt < MAXWIN)
  {
    windowon[cnt] = bfalse;
    cnt++;
  }

  load_window(0, "window.pcx", 180, 16,  7, 9, 200, 200, WINVERTEX);
  load_window(1, "window.pcx", 410, 16,  7, 9, 200, 200, WINTILE);
  load_window(2, "window.pcx", 180, 248, 7, 9, 200, 200, WINSIDE);
  load_window(3, "window.pcx", 410, 248, 7, 9, 200, 200, WINFX);
}

//------------------------------------------------------------------------------
void draw_window(int window)
{
  FRect dst;

  if(!windowon[window]) return;

  dst.left   = window_rect[window].x;
  dst.top    = window_rect[window].y;
  dst.right  = window_tx[window].txH + dst.left;
  dst.bottom = window_tx[window].txH + dst.top;

  draw_texture_box(&window_tx[window], NULL, &dst);
}


//------------------------------------------------------------------------------
void draw_all_windows(void)
{
  int cnt;
  cnt = 0;
  while(cnt < MAXWIN)
  {
    draw_window(cnt);
    cnt++;
  }
  return;
}

//------------------------------------------------------------------------------
void bound_camera(void)
{
  if(cart_pos_x < 0)
  {
    cart_pos_x = 0;
  }
  if(cart_pos_y < 0)
  {
    cart_pos_y = 0;
  }
  if(cart_pos_x > mesh.size_x*SMALLX)
  {
    cart_pos_x = mesh.size_x*SMALLX;
  }
  if(cart_pos_y > mesh.size_y*SMALLY)
  {
    cart_pos_y = mesh.size_y*SMALLY;
  }
  return;
}

////------------------------------------------------------------------------------
//void unbound_mouse()
//{
//  mstlx = 0;
//  mstly = 0;
//  msbrx = OUTX-1;
//  msbry = OUTY-1;
//  return;
//}

////------------------------------------------------------------------------------
//void bound_mouse()
//{
//  if(mouseinwin != -1)
//  {
//    mstlx = window_rect[mouseinwin].x+windowborderx[mouseinwin];
//    mstly = window_rect[mouseinwin].y+windowbordery[mouseinwin];
//    msbrx = mstlx+window_rect[mouseinwin].w-1;
//    msbry = mstly+window_rect[mouseinwin].h-1;
//  }
//  return;
//}

//------------------------------------------------------------------------------
void rect_select(void)
{
  // ZZ> This function checks the rectangular selection
  int cnt;
  Uint32 vert;
  int tlx, tly, brx, bry;
  int y;


  if(mouseinwinmode == WINVERTEX)
  {
    tlx = mouseinwinrectx/FOURNUM;
    brx = mouseinwinx/FOURNUM;
    tly = mouseinwinrecty/FOURNUM;
    bry = mouseinwiny/FOURNUM;


    if(tlx>brx)  { cnt = tlx;  tlx=brx;  brx=cnt; }
    if(tly>bry)  { cnt = tly;  tly=bry;  bry=cnt; }


    cnt = 0;
    while(cnt < numpointsonscreen && numselect<MAXSELECT)
    {
      vert = pointsonscreen[cnt];
      if(Mesh_Mem.vrt_x[vert]>=tlx &&
        Mesh_Mem.vrt_x[vert]<=brx &&
        Mesh_Mem.vrt_y[vert]>=tly &&
        Mesh_Mem.vrt_y[vert]<=bry)
      {
        add_select(vert);
      }
      cnt++;
    }
  }
  if(mouseinwinmode == WINSIDE)
  {
    tlx = mouseinwinrectx/FOURNUM;
    brx = mouseinwinx/FOURNUM;
    tly = mouseinwinrecty/FOURNUM;
    bry = mouseinwiny/FOURNUM;

    y = 190;//((*(window_tx[mouseinwin])).txH-10);


    if(tlx>brx)  { cnt = tlx;  tlx=brx;  brx=cnt; }
    if(tly>bry)  { cnt = tly;  tly=bry;  bry=cnt; }


    cnt = 0;
    while(cnt < numpointsonscreen && numselect<MAXSELECT)
    {
      vert = pointsonscreen[cnt];
      if(Mesh_Mem.vrt_x[vert]>=tlx &&
        Mesh_Mem.vrt_x[vert]<=brx &&
        -(Mesh_Mem.vrt_z[vert] / 16.0f)+y>=tly &&
        -(Mesh_Mem.vrt_z[vert] / 16.0f)+y<=bry)
      {
        add_select(vert);
      }
      cnt++;
    }
  }
}

//------------------------------------------------------------------------------
void rect_unselect(void)
{
  // ZZ> This function checks the rectangular selection, and removes any fans
  //     in the selection area
  int cnt;
  Uint32 vert;
  int tlx, tly, brx, bry;
  int y;


  if(mouseinwinmode == WINVERTEX)
  {
    tlx = mouseinwinrectx/FOURNUM;
    brx = mouseinwinx/FOURNUM;
    tly = mouseinwinrecty/FOURNUM;
    bry = mouseinwiny/FOURNUM;


    if(tlx>brx)  { cnt = tlx;  tlx=brx;  brx=cnt; }
    if(tly>bry)  { cnt = tly;  tly=bry;  bry=cnt; }


    cnt = 0;
    while(cnt < numpointsonscreen && numselect<MAXSELECT)
    {
      vert = pointsonscreen[cnt];
      if(Mesh_Mem.vrt_x[vert]>=tlx &&
        Mesh_Mem.vrt_x[vert]<=brx &&
        Mesh_Mem.vrt_y[vert]>=tly &&
        Mesh_Mem.vrt_y[vert]<=bry)
      {
        remove_select(vert);
      }
      cnt++;
    }
  }
  if(mouseinwinmode == WINSIDE)
  {
    tlx = mouseinwinrectx/FOURNUM;
    brx = mouseinwinx/FOURNUM;
    tly = mouseinwinrecty/FOURNUM;
    bry = mouseinwiny/FOURNUM;

    y = 190;//((*(window_tx[mouseinwin])).txH-10);


    if(tlx>brx)  { cnt = tlx;  tlx=brx;  brx=cnt; }
    if(tly>bry)  { cnt = tly;  tly=bry;  bry=cnt; }


    cnt = 0;
    while(cnt < numpointsonscreen && numselect<MAXSELECT)
    {
      vert = pointsonscreen[cnt];
      if(Mesh_Mem.vrt_x[vert]>=tlx &&
        Mesh_Mem.vrt_x[vert]<=brx &&
        -(Mesh_Mem.vrt_z[vert] / 16.0f)+y>=tly &&
        -(Mesh_Mem.vrt_z[vert] / 16.0f)+y<=bry)
      {
        remove_select(vert);
      }
      cnt++;
    }
  }
}

//------------------------------------------------------------------------------
int set_vrta(Uint32 vert)
{
  int newa, x, y, z, brx, bry, brz, deltaz, dist, cnt;
  int newlevel, distance, disx, disy;
  vect3 pos;


  // To make life easier
  x = Mesh_Mem.vrt_x[vert]*FOURNUM;
  y = Mesh_Mem.vrt_y[vert]*FOURNUM;
  z = Mesh_Mem.vrt_z[vert];

  pos.x = x;
  pos.y = y;
  pos.z = 0;


  // Directional light
  brx = x+64;
  bry = y+64;
  brz = mesh_get_level(mesh_get_fan(pos), brx, y, bfalse) +
    mesh_get_level(mesh_get_fan(pos), x, bry, bfalse) +
    mesh_get_level(mesh_get_fan(pos), x+46, y+46, bfalse);
  if(z < -128) z = -128;
  if(brz < -128) brz = -128;
  deltaz = z+z+z-brz;
  newa = (deltaz*direct>>8);


  // Point lights !!!BAD!!!
  newlevel = 0;
  cnt = 0;
  while(cnt < numlight)
  {
    disx = x-lightx[cnt];
    disy = y-lighty[cnt];
    distance = sqrt(disx*disx + disy*disy);
    if(distance < lightradius[cnt])
    {
      newlevel += ((lightlevel[cnt]*(lightradius[cnt]-distance))/lightradius[cnt]);
    }
    cnt++;
  }
  newa += newlevel;



  // Bounds
  if(newa < -ambicut) newa = -ambicut;
  newa+=ambi;
  if(newa <= 0) newa = 1;
  if(newa > 255) newa = 255;
  Mesh_Mem.vrt_a[vert]=newa;



  // Edge fade
  dist = dist_from_border(Mesh_Mem.vrt_x[vert], Mesh_Mem.vrt_y[vert]);
  if(dist <= FADEBORDER)
  {
    newa = newa*dist/FADEBORDER;
    if(newa==VERTEXUNUSED)  newa=1;
    Mesh_Mem.vrt_a[vert]=newa;
  }


  return 0;
}

//------------------------------------------------------------------------------
void calc_vrta()
{
  int x, y, fan, num, cnt;
  Uint32 vert;


  y = 0;
  while(y < mesh.size_y)
  {
    x = 0;
    while(x < mesh.size_x)
    {
      fan = mesh_convert_fan(x, y);
      vert = Mesh_Fan[fan].vrt_start;
      num = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
      cnt = 0;
      while(cnt < num)
      {
        set_vrta(vert);
        vert = Mesh_Mem.vrt_next[vert];
        cnt++;
      }
      x++;
    }
    y++;
  }
  return;
}

//------------------------------------------------------------------------------
void level_vrtz()
{
  int x, y, fan, num, cnt;
  Uint32 vert;


  y = 0;
  while(y < mesh.size_y)
  {
    x = 0;
    while(x < mesh.size_x)
    {
      fan = mesh_convert_fan(x, y);
      vert = Mesh_Fan[fan].vrt_start;
      num = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
      cnt = 0;
      while(cnt < num)
      {
        Mesh_Mem.vrt_z[vert] = 0;
        vert = Mesh_Mem.vrt_next[vert];
        cnt++;
      }
      x++;
    }
    y++;
  }
  return;
}

//------------------------------------------------------------------------------
void jitter_select()
{
  int cnt;
  Uint32 vert;


  cnt = 0;
  while(cnt < numselect)
  {
    vert = select[cnt];
    move_vert(vert, (rand()%3)-1, (rand()%3)-1, 0);
    cnt++;
  }
  return;
}

//------------------------------------------------------------------------------
void jitter_mesh()
{
  int x, y, fan, num, cnt;
  Uint32 vert;


  y = 0;
  while(y < mesh.size_y)
  {
    x = 0;
    while(x < mesh.size_x)
    {
      fan = mesh_convert_fan(x, y);
      vert = Mesh_Fan[fan].vrt_start;
      num = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
      cnt = 0;
      while(cnt < num)
      {
        clear_select();
        add_select(vert);
        //        srand(Mesh_Mem.vrt_x[vert]+Mesh_Mem.vrt_y[vert]+dunframe);
        move_select((rand()&7)-3,(rand()&7)-3,(rand()&63)-32);
        vert = Mesh_Mem.vrt_next[vert];
        cnt++;
      }
      x++;
    }
    y++;
  }
  clear_select();
  return;
}

//------------------------------------------------------------------------------
void flatten_mesh()
{
  int x, y, fan, num, cnt;
  Uint32 vert;
  int height;


  height = (780 - (mouseinwiny)) * 4;
  if(height < 0)  height = 0;
  if(height > meshedgez) height = meshedgez;
  y = 0;
  while(y < mesh.size_y)
  {
    x = 0;
    while(x < mesh.size_x)
    {
      fan = mesh_convert_fan(x, y);
      vert = Mesh_Fan[fan].vrt_start;
      num = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
      cnt = 0;
      while(cnt < num)
      {
        if(Mesh_Mem.vrt_z[vert] > height - 50)
          if(Mesh_Mem.vrt_z[vert] < height + 50)
            Mesh_Mem.vrt_z[vert] = height;
        vert = Mesh_Mem.vrt_next[vert];
        cnt++;
      }
      x++;
    }
    y++;
  }
  clear_select();
  return;
}


//------------------------------------------------------------------------------
//void move_camera()
//{
//  if(((mous.latch.b&4) || SDLKEYDOWN(SDLK_m)) && mouseinwin!=-1)
//  {
//    cart_pos_x+=mcx;
//    cart_pos_y+=mcy;
//    bound_camera();
//    ui_getMouseX()=msxold;
//    ui_getMouseY()=msyold;
//  }
//  return;
//}

//------------------------------------------------------------------------------
//void mouse_side(int cnt)
//{
//  mouseinwinx = ui_getMouseX() - window_rect[cnt].x-windowborderx[cnt]+cart_pos_x-69;
//  mouseinwiny = ui_getMouseY()-window_rect[cnt].y-windowbordery[cnt];
//  mouseinwinx = mouseinwinx*FOURNUM;
//  mouseinwiny = mouseinwiny*FOURNUM;
//  if(SDLKEYDOWN(SDLK_f))
//  {
//    flatten_mesh();
//  }
//  if(mous.latch.b&1)
//  {
//    if(mouseinwinrect==bfalse)
//    {
//      mouseinwinrect=btrue;
//      mouseinwinrectx=mouseinwinx;
//      mouseinwinrecty=mouseinwiny;
//    }
//  }
//  else
//  {
//    if(mouseinwinrect==btrue)
//    {
//      if(numselect!=0 && !SDLKEYDOWN(SDLK_LALT) &&  !SDLKEYDOWN(SDLK_RALT) &&
//                         !SDLKEYDOWN(SDLK_LCTRL) && !SDLKEYDOWN(SDLK_RCTRL))
//      {
//        clear_select();
//      }
//      if(SDLKEYDOWN(SDLK_ALT) || SDLKEYDOWN(SDLK_ALTGR))
//      {
//        rect_unselect();
//      }
//      else
//      {
//        rect_select();
//      }
//      mouseinwinrect = bfalse;
//    }
//  }
//  if(mous.latch.b&2)
//  {
//    move_select(mcx, 0, -(mcy<<4));
//    bound_mouse();
//  }
//
//  if(SDLKEYDOWN(SDLK_y))
//  {
//    move_select(0, 0, -(mcy<<4));
//    bound_mouse();
//  }
//  if(SDLKEYDOWN(SDLK_5))
//  {
//    set_select_no_bound_z(-8000<<2);
//  }
//  if(SDLKEYDOWN(SDLK_6))
//  {
//    set_select_no_bound_z(-127<<2);
//  }
//  if(SDLKEYDOWN(SDLK_7))
//  {
//    set_select_no_bound_z(127<<2);
//  }
//  if(SDLKEYDOWN(SDLK_u))
//  {
//    if(mouseinwintype >= (MAXMESHTYPE>>1))
//    {
//      move_mesh_z(-(mcy<<4), mouseinwintile, 192);
//    }
//    else
//    {
//      move_mesh_z(-(mcy<<4), mouseinwintile, 240);
//    }
//    bound_mouse();
//  }
//  if(SDLKEYDOWN(SDLK_n))
//  {
//    if(SDLKEYDOWN(SDLK_RSHIFT))
//    {
//      // Move the first 16 up and down
//      move_mesh_z(-(mcy<<4), 0, 240);
//    }
//    else
//    {
//      // Move the entire mesh up and down
//      move_mesh_z(-(mcy<<4), 0, 0);
//    }
//    bound_mouse();
//  }
//  if(SDLKEYDOWN(SDLK_q))
//  {
//    fix_walls();
//  }
//
//
//  return;
//}

//------------------------------------------------------------------------------
//void mouse_tile(int cnt)
//{
//  int x, y, keyt, vert, keyv;
//  float tl, tr, bl, br;
//
//
//  mouseinwinx = ui_getMouseX() - window_rect[cnt].x-windowborderx[cnt]+cart_pos_x-69;
//  mouseinwiny = ui_getMouseY() - window_rect[cnt].y-windowbordery[cnt]+cart_pos_y-69;
//  if(mouseinwinx < 0 ||
//     mouseinwinx >= SMALLX*mesh.size_x ||
//     mouseinwiny < 0 ||
//     mouseinwiny >= SMALLY*mesh.size_y)
//  {
//    mouseinwinx = mouseinwinx*FOURNUM;
//    mouseinwiny = mouseinwiny*FOURNUM;
//    if(mous.latch.b&2)
//    {
//       mouseinwintype = 0+0;
//       mouseinwintile = 0xffff;
//    }
//  }
//  else
//  {
//    mouseinwinx = mouseinwinx*FOURNUM;
//    mouseinwiny = mouseinwiny*FOURNUM;
//    if(mouseinwinx >= (mesh.size_x<<7))  mouseinwinx = (mesh.size_x<<7)-1;
//    if(mouseinwiny >= (mesh.size_y<<7))  mouseinwiny = (mesh.size_y<<7)-1;
//    debugx = mouseinwinx/128.0;
//    debugy = mouseinwiny/128.0;
//    x = mouseinwinx>>7;
//    y = mouseinwiny>>7;
//    mouseinwinonfan = mesh_convert_fan(x, y);
//
//
//    if(!SDLKEYDOWN(SDLK_k))
//    {
//      addinglight = bfalse;
//    }
//    if(SDLKEYDOWN(SDLK_k)&&addinglight==bfalse)
//    {
//      add_light(mouseinwinx, mouseinwiny, MINRADIUS, MAXHEIGHT);
//      addinglight = btrue;
//    }
//    if(addinglight)
//    {
//      alter_light(mouseinwinx, mouseinwiny);
//    }
//    if(mous.latch.b&1)
//    {
//      keyt = SDLKEYDOWN(SDLK_t);
//      keyv = SDLKEYDOWN(SDLK_v);
//      if(!keyt)
//      {
//        if(!keyv)
//        {
//          // Save corner heights
//          vert = Mesh_Fan[mouseinwinonfan].vrt_start;
//          tl = Mesh_Mem.vrt_z[vert];
//          vert = Mesh_Mem.vrt_next[vert];
//          tr = Mesh_Mem.vrt_z[vert];
//          vert = Mesh_Mem.vrt_next[vert];
//          br = Mesh_Mem.vrt_z[vert];
//          vert = Mesh_Mem.vrt_next[vert];
//          bl = Mesh_Mem.vrt_z[vert];
//        }
//        remove_fan(mouseinwinonfan);
//      }
//      switch(mouseinwinpresser)
//      {
//        case 0:
//          Mesh_Fan[mouseinwinonfan].tile=mouseinwintile;
//          break;
//        case 1:
//          Mesh_Fan[mouseinwinonfan].tile=(mouseinwintile&0xfffe)+(rand()&1);
//          break;
//        case 2:
//          Mesh_Fan[mouseinwinonfan].tile=(mouseinwintile&0xfffc)+(rand()&3);
//          break;
//        case 3:
//          Mesh_Fan[mouseinwinonfan].tile=(mouseinwintile&0xfff0)+(rand()&6);
//          break;
//      }
//      if(!keyt)
//      {
//        Mesh_Fan[mouseinwinonfan].type=mouseinwintype;
//        add_fan(mouseinwinonfan, (mouseinwinx>>7)*31, (mouseinwiny>>7)*31);
//        Mesh_Fan[mouseinwinonfan].fx=mouseinwinfx;
//        if(!keyv)
//        {
//          // Return corner heights
//          vert = Mesh_Fan[mouseinwinonfan].vrt_start;
//          Mesh_Mem.vrt_z[vert] = tl;
//          vert = Mesh_Mem.vrt_next[vert];
//          Mesh_Mem.vrt_z[vert] = tr;
//          vert = Mesh_Mem.vrt_next[vert];
//          Mesh_Mem.vrt_z[vert] = br;
//          vert = Mesh_Mem.vrt_next[vert];
//          Mesh_Mem.vrt_z[vert] = bl;
//        }
//      }
//    }
//    if(mous.latch.b&2)
//    {
//      mouseinwintype = Mesh_Fan[mouseinwinonfan].type;
//      mouseinwintile = Mesh_Fan[mouseinwinonfan].tile;
//    }
//  }
//
//
//  return;
//}

//------------------------------------------------------------------------------
//void mouse_fx(int cnt)
//{
//  int x, y;
//
//
//  mouseinwinx = ui_getMouseX() - window_rect[cnt].x-windowborderx[cnt]+cart_pos_x-69;
//  mouseinwiny = ui_getMouseY() - window_rect[cnt].y-windowbordery[cnt]+cart_pos_y-69;
//  if(mouseinwinx < 0 ||
//     mouseinwinx >= SMALLX*mesh.size_x ||
//     mouseinwiny < 0 ||
//     mouseinwiny >= SMALLY*mesh.size_y)
//  {
//  }
//  else
//  {
//    mouseinwinx = mouseinwinx*FOURNUM;
//    mouseinwiny = mouseinwiny*FOURNUM;
//    if(mouseinwinx >= (mesh.size_x<<7))  mouseinwinx = (mesh.size_x<<7)-1;
//    if(mouseinwiny >= (mesh.size_y<<7))  mouseinwiny = (mesh.size_y<<7)-1;
//    debugx = mouseinwinx/128.0;
//    debugy = mouseinwiny/128.0;
//    x = mouseinwinx>>7;
//    y = mouseinwiny>>7;
//    mouseinwinonfan = mesh_convert_fan(x, y);
//
//
//    if(mous.latch.b&1)
//    {
//      Mesh_Fan[mouseinwinonfan].fx = mouseinwinfx;
//    }
//    if(mous.latch.b&2)
//    {
//      mouseinwinfx = Mesh_Fan[mouseinwinonfan].fx;
//    }
//  }
//
//
//  return;
//}

//------------------------------------------------------------------------------
//void mouse_vertex(int cnt)
//{
//  mouseinwinx = ui_getMouseX() - window_rect[cnt].x-windowborderx[cnt]+cart_pos_x-69;
//  mouseinwiny = ui_getMouseY() - window_rect[cnt].y-windowbordery[cnt]+cart_pos_y-69;
//  mouseinwinx = mouseinwinx*FOURNUM;
//  mouseinwiny = mouseinwiny*FOURNUM;
//  if(SDLKEYDOWN(SDLK_f))
//  {
////    fix_corners(mouseinwinx>>7, mouseinwiny>>7);
//    fix_vertices(mouseinwinx>>7, mouseinwiny>>7);
//  }
//  if(SDLKEYDOWN(SDLK_5))
//  {
//    set_select_no_bound_z(-8000<<2);
//  }
//  if(SDLKEYDOWN(SDLK_6))
//  {
//    set_select_no_bound_z(-127<<2);
//  }
//  if(SDLKEYDOWN(SDLK_7))
//  {
//    set_select_no_bound_z(127<<2);
//  }
//  if(mous.latch.b&1)
//  {
//    if(mouseinwinrect==bfalse)
//    {
//      mouseinwinrect=btrue;
//      mouseinwinrectx=mouseinwinx;
//      mouseinwinrecty=mouseinwiny;
//    }
//  }
//  else
//  {
//    if(mouseinwinrect==btrue)
//    {
//      if(numselect!=0 && !SDLKEYDOWN(SDLK_ALT) && !SDLKEYDOWN(SDLK_ALTGR) &&
//                         !SDLKEYDOWN(SDLK_LCTRL) && !SDLKEYDOWN(SDLK_RCTRL))
//      {
//        clear_select();
//      }
//      if(SDLKEYDOWN(SDLK_ALT) || SDLKEYDOWN(SDLK_ALTGR))
//      {
//        rect_unselect();
//      }
//      else
//      {
//        rect_select();
//      }
//      mouseinwinrect = bfalse;
//    }
//  }
//  if(mous.latch.b&2)
//  {
//    move_select(mcx, mcy, 0);
//    bound_mouse();
//  }
//
//
//  return;
//}

//------------------------------------------------------------------------------
//void check_mouse(void)
//{
//  int x, y, cnt;
//
//
//  debugx = -1;
//  debugy = -1;
//
//  unbound_mouse();
//  move_camera(1);
//  mouseinwin = -1;
//  mouseinwinx = -1;
//  mouseinwiny = -1;
//  mouseinwinmode = 0;
//  cnt = 0;
//  while(cnt < MAXWIN)
//  {
//    if(windowon[cnt])
//    {
//      if(ui_getMouseX() >= window_rect[cnt].x+windowborderx[cnt] &&
//         ui_getMouseX() <  window_rect[cnt].x+windowborderx[cnt]+window_rect[cnt].w &&
//         ui_getMouseY() >= window_rect[cnt].y+windowbordery[cnt] &&
//         ui_getMouseY() <  window_rect[cnt].y+windowbordery[cnt]+window_rect[cnt].h)
//      {
//        mouseinwin = cnt;
//        mouseinwinmode = windowmode[cnt];
//        if(mouseinwinmode==WINTILE)
//        {
//          mouse_tile(cnt);
//        }
//        if(mouseinwinmode==WINVERTEX)
//        {
//          mouse_vertex(cnt);
//        }
//        if(mouseinwinmode==WINSIDE)
//        {
//          mouse_side(cnt);
//        }
//        if(mouseinwinmode==WINFX)
//        {
//          mouse_fx(cnt);
//        }
//      }
//    }
//    cnt++;
//  }
//  return;
//}

//------------------------------------------------------------------------------
void clear_mesh()
{
  int x, y;
  Uint32 fan;


  if(mouseinwintile != INVALID_FAN)
  {
    y = 0;
    while(y < mesh.size_y)
    {
      x = 0;
      while(x < mesh.size_x)
      {
        fan = mesh_convert_fan(x, y);
        remove_fan(fan);
        switch(mouseinwinpresser)
        {
        case 0:
          Mesh_Fan[fan].tile=mouseinwintile;
          break;
        case 1:
          Mesh_Fan[fan].tile=(mouseinwintile&0xfffe)+(rand()&1);
          break;
        case 2:
          if(mouseinwintype >= 32)
            Mesh_Fan[fan].tile=(mouseinwintile&0xfff8)+(rand()&6);
          else
            Mesh_Fan[fan].tile=(mouseinwintile&0xfffc)+(rand()&3);
          break;
        case 3:
          Mesh_Fan[fan].tile=(mouseinwintile&0xfff0)+(rand()&6);
          break;
        }
        Mesh_Fan[fan].type=mouseinwintype;
        if(mouseinwintype<=1) Mesh_Fan[fan].type = rand()&1;
        if(mouseinwintype == 32 || mouseinwintype == 33)
          Mesh_Fan[fan].type = 32 + (rand()&1);
        add_fan(fan, x*31, y*31);
        x++;
      }
      y++;
    }
  }
  return;
}

//------------------------------------------------------------------------------
void three_e_mesh()
{
  // ZZ> Replace all 3F tiles with 3E tiles...
  int x, y;
  Uint32 fan;


  if(mouseinwintile != INVALID_FAN)
  {
    y = 0;
    while(y < mesh.size_y)
    {
      x = 0;
      while(x < mesh.size_x)
      {
        fan = mesh_convert_fan(x, y);
        if(Mesh_Fan[fan].tile==0x3F)  Mesh_Fan[fan].tile=0x3E;
        x++;
      }
      y++;
    }
  }
  return;
}

//------------------------------------------------------------------------------
void toggle_fx(int fxmask)
{
  if(mouseinwinfx&fxmask)
  {
    mouseinwinfx-=fxmask;
  }
  else
  {
    mouseinwinfx+=fxmask;
  }
  return;
}

//------------------------------------------------------------------------------
void ease_up_mesh(int zadd)
{
  // ZZ> This function lifts the entire mesh
  int x, y, cnt;
  Uint32 fan, vert;

  y = 0;
  while(y < mesh.size_y)
  {
    x = 0;
    while(x < mesh.size_x)
    {
      fan = mesh_convert_fan(x, y);
      vert = Mesh_Fan[fan].vrt_start;
      cnt = 0;
      while(cnt < Mesh_Cmd[Mesh_Fan[fan].type].vrt_count)
      {
        move_vert(vert, 0, 0, zadd);
        vert = Mesh_Mem.vrt_next[vert];
        cnt++;
      }
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
void select_connected()
{
  int vert, cnt, tnc, x, y, totalvert;
  Uint32 fan;
  Uint8 found, selectfan;


  y = 0;
  while(y < mesh.size_y)
  {
    x = 0;
    while(x < mesh.size_x)
    {
      fan = mesh_convert_fan(x, y);
      selectfan = bfalse;
      totalvert = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
      cnt = 0;
      vert = Mesh_Fan[fan].vrt_start;
      while(cnt < totalvert)
      {

        found = bfalse;
        tnc = 0;
        while(tnc < numselect && !found)
        {
          if(select[tnc]==vert)
          {
            found=btrue;
          }
          tnc++;
        }
        if(found) selectfan = btrue;
        vert = Mesh_Mem.vrt_next[vert];
        cnt++;
      }
      if(selectfan)
      {
        cnt = 0;
        vert = Mesh_Fan[fan].vrt_start;
        while(cnt < totalvert)
        {
          add_select(vert);
          vert = Mesh_Mem.vrt_next[vert];
          cnt++;
        }
      }
      x++;
    }
    y++;
  }
}


//------------------------------------------------------------------------------
void check_keys(char *modname)
{
  char newloadname[256];

  if(keydelay <= 0)
  {
    // Hurt
    if(SDLKEYDOWN(SDLK_h))
    {
      toggle_fx(MESHFX_DAMAGE);
      keydelay=KEYDELAY;
    }
    // Impassable
    if(SDLKEYDOWN(SDLK_i))
    {
      toggle_fx(MESHFX_IMPASS);
      keydelay=KEYDELAY;
    }
    // Barrier
    if(SDLKEYDOWN(SDLK_b))
    {
      toggle_fx(MESHFX_WALL);
      keydelay=KEYDELAY;
    }
    // Overlay
    if(SDLKEYDOWN(SDLK_o))
    {
      toggle_fx(MESHFX_WATER);
      keydelay=KEYDELAY;
    }
    // Reflective
    if(SDLKEYDOWN(SDLK_r))
    {
      toggle_fx(MESHFX_NOREFLECT);
      keydelay=KEYDELAY;
    }
    // Draw reflections
    if(SDLKEYDOWN(SDLK_d))
    {
      toggle_fx(MESHFX_SHINY);
      keydelay=KEYDELAY;
    }
    // Animated
    if(SDLKEYDOWN(SDLK_a))
    {
      toggle_fx(MESHFX_ANIM);
      keydelay=KEYDELAY;
    }
    // Slippy
    if(SDLKEYDOWN(SDLK_s))
    {
      toggle_fx(MESHFX_SLIPPY);
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_g))
    {
      fix_mesh();
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_z))
    {
      set_mesh_tile(Mesh_Fan[mouseinwinonfan].tile);
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_LSHIFT))
    {
      if(Mesh_Fan[mouseinwinonfan].type >= (MAXMESHTYPE>>1))
      {
        fx_mesh_tile(Mesh_Fan[mouseinwinonfan].tile, 192, mouseinwinfx);
      }
      else
      {
        fx_mesh_tile(Mesh_Fan[mouseinwinonfan].tile, 240, mouseinwinfx);
      }
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_x))
    {
      if(Mesh_Fan[mouseinwinonfan].type >= (MAXMESHTYPE>>1))
      {
        trim_mesh_tile(Mesh_Fan[mouseinwinonfan].tile, 192);
      }
      else
      {
        trim_mesh_tile(Mesh_Fan[mouseinwinonfan].tile, 240);
      }
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_e))
    {
      ease_up_mesh(10);
    }
    if(SDLKEYDOWN(SDLK_c))
    {
      clear_mesh();
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_LEFTBRACKET) || SDLKEYDOWN(SDLK_RIGHTBRACKET))
    {
      select_connected();
    }
    if(SDLKEYDOWN(SDLK_8))
    {
      three_e_mesh();
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_j))
    {
      if(numselect == 0) { jitter_mesh(); }
      else { jitter_select(); }
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_l))
    {
      level_vrtz();
    }
    if(SDLKEYDOWN(SDLK_w))
    {
      impass_edges(2);
      calc_vrta();
      make_newloadname("..\\modules\\", modname, newloadname);
      save_mesh(newloadname);
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_SPACE))
    {
      weld_select();
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_INSERT))
    {
      mouseinwintype=(mouseinwintype-1)&(MAXMESHTYPE-1);
      while(Mesh[mouseinwintype].numline==0)
      {
        mouseinwintype=(mouseinwintype-1)&(MAXMESHTYPE-1);
      }
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_BACKSPACE))
    {
      mouseinwintype=(mouseinwintype+1)&(MAXMESHTYPE-1);
      while(Mesh[mouseinwintype].numline==0)
      {
        mouseinwintype=(mouseinwintype+1)&(MAXMESHTYPE-1);
      }
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_KP_PLUS))
    {
      mouseinwintile=(mouseinwintile+1)&255;
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_KP_MINUS))
    {
      mouseinwintile=(mouseinwintile-1)&255;
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_UP) || SDLKEYDOWN(SDLK_LEFT) || SDLKEYDOWN(SDLK_DOWN) || SDLKEYDOWN(SDLK_RIGHT))
    {
      if(SDLKEYDOWN(SDLK_UP))
      {
        cart_pos_y-=CAMRATE;
      }
      if(SDLKEYDOWN(SDLK_LEFT))
      {
        cart_pos_x-=CAMRATE;
      }
      if(SDLKEYDOWN(SDLK_DOWN))
      {
        cart_pos_y+=CAMRATE;
      }
      if(SDLKEYDOWN(SDLK_RIGHT))
      {
        cart_pos_x+=CAMRATE;
      }
    }
    if(SDLKEYDOWN(SDLK_END))
    {
      brushsize = 0;
    }
    if(SDLKEYDOWN(SDLK_PAGEDOWN))
    {
      brushsize = 1;
    }
    if(SDLKEYDOWN(SDLK_HOME))
    {
      brushsize = 2;
    }
    if(SDLKEYDOWN(SDLK_PAGEUP))
    {
      brushsize = 3;
    }
    if(SDLKEYDOWN(SDLK_1))
    {
      mouseinwinpresser = 0;
    }
    if(SDLKEYDOWN(SDLK_2))
    {
      mouseinwinpresser = 1;
    }
    if(SDLKEYDOWN(SDLK_3))
    {
      mouseinwinpresser = 2;
    }
    if(SDLKEYDOWN(SDLK_4))
    {
      mouseinwinpresser = 3;
    }
  }


  return;
}

//------------------------------------------------------------------------------
//void setup_screen(void)
//{
//  set_color_depth(colordepth);
//  set_gfx_mode(GFX_AUTODETECT, SCRX, SCRY, SCRX, SCRY);
//  set_palette(goodpal);
//  clear(screen);
//  bmpdbuff = SDL_CreateRGBSurface(SDL_SWSURFACE, OUTX, OUTY, 32, rmask, gmask, bmask, amask);
//  clear(bmpdbuff);
//  text_mode(-1);
//
//
//  return;
//}

//------------------------------------------------------------------------------
//void create_imgcursor(void)
//{
//  int x, y;
//  Uint8 col, loc;
//
//  col = make_color(31, 31, 31);      // White color
//  loc = make_color(3, 3, 3);        // Gray color
//  bmptemp = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, 8, 32, rmask, gmask, bmask, amask);
//
//  // Simple triangle
//  draw_line(-1, loc, 0, 0, 0, 7, );
//  y = 1;
//  while(y < 8)
//  {
//    _putpixel(bmptemp, 0, y, loc);
//    x = 1;
//    while(x < 8)
//    {
//      if(x < 8-y) _putpixel(bmptemp, x, y, col);
//      else _putpixel(bmptemp, x, y, 0);
//      x++;
//    }
//    y++;
//  }
//  imgcursor = get_rle_sprite(bmptemp);
//  GLTexture_Release(&bmptemp);
//
//
//  return;
//}

//------------------------------------------------------------------------------
void load_img(void)
{
  int cnt;
  SDL_Surface *bmpother, *bmptemp;


  bmptemp = IMG_Load("point.pcx");
  cnt = 0;
  while(cnt < 16)
  {
    bmpother = SDL_CreateRGBSurface(SDL_SWSURFACE, (cnt>>1)+4, ((cnt+1)>>1)+4, 32, rmask, gmask, bmask, amask);
    SDL_BlitSurface(bmptemp, &bmptemp->clip_rect, bmpother, &bmpother->clip_rect);

    GLTexture_Convert( GL_TEXTURE_2D, &imgpoint[cnt], bmpother, INVALID_KEY);

    SDL_FreeSurface( bmpother );

    cnt++;
  }
  SDL_FreeSurface(bmptemp);

  bmptemp = IMG_Load("pointon.pcx");
  cnt = 0;
  while(cnt < 16)
  {
    bmpother = SDL_CreateRGBSurface(SDL_SWSURFACE, (cnt>>1)+4, ((cnt+1)>>1)+4, 32, rmask, gmask, bmask, amask);
    SDL_BlitSurface(bmptemp, &bmptemp->clip_rect, bmpother, &bmpother->clip_rect);

    GLTexture_Convert( GL_TEXTURE_2D, &imgpointon[cnt] , bmpother, INVALID_KEY);

    SDL_FreeSurface( bmpother );

    cnt++;
  }
  SDL_FreeSurface( bmptemp );

  GLTexture_Load( GL_TEXTURE_2D, &imgref,     "ref.pcx",     INVALID_KEY);
  GLTexture_Load( GL_TEXTURE_2D, &imgdrawref, "drawref.pcx", INVALID_KEY);
  GLTexture_Load( GL_TEXTURE_2D, &imganim,    "anim.pcx",    INVALID_KEY);
  GLTexture_Load( GL_TEXTURE_2D, &imgwater,   "water.pcx",   INVALID_KEY);
  GLTexture_Load( GL_TEXTURE_2D, &imgwall,    "slit.pcx",    INVALID_KEY);
  GLTexture_Load( GL_TEXTURE_2D, &imgimpass,  "impass.pcx",  INVALID_KEY);
  GLTexture_Load( GL_TEXTURE_2D, &imgdamage,  "damage.pcx",  INVALID_KEY);
  GLTexture_Load( GL_TEXTURE_2D, &imgslippy,  "slippy.pcx",  INVALID_KEY);

  return;
}

//------------------------------------------------------------------------------
void draw_lotsa_stuff(void)
{
  int x, y, cnt, todo, tile, add;


  // Tell which tile we're in
  x = debugx * 128;
  y = debugy * 128;
  draw_string( &bmfont,  0, 226, make_color(31, 31, 31), "X = %6.2f (%d)", debugx, x);
  draw_string( &bmfont,  0, 234, make_color(31, 31, 31), "Y = %6.2f (%d)", debugy, y);


  // Tell user what keys are important
  draw_string( &bmfont,  0, OUTY-120, make_color(31, 31, 31), "O = Overlay (Water)");
  draw_string( &bmfont,  0, OUTY-112, make_color(31, 31, 31), "R = Reflective");
  draw_string( &bmfont,  0, OUTY-104, make_color(31, 31, 31), "D = Draw Reflection");
  draw_string( &bmfont,  0, OUTY- 96, make_color(31, 31, 31), "A = Animated");
  draw_string( &bmfont,  0, OUTY- 88, make_color(31, 31, 31), "B = Barrier (Slit)");
  draw_string( &bmfont,  0, OUTY- 80, make_color(31, 31, 31), "I = Impassable (Wall)");
  draw_string( &bmfont,  0, OUTY- 72, make_color(31, 31, 31), "H = Hurt");
  draw_string( &bmfont,  0, OUTY- 64, make_color(31, 31, 31), "S = Slippy");


  // Vertices left
  draw_string( &bmfont,  0, OUTY-56, make_color(31, 31, 31), "Vertices %d", numfreevertices);


  // Misc data
  draw_string( &bmfont,  0, OUTY-40, make_color(31, 31, 31), "Ambient   %d", ambi);
  draw_string( &bmfont,  0, OUTY-32, make_color(31, 31, 31), "Ambicut   %d", ambicut);
  draw_string( &bmfont,  0, OUTY-24, make_color(31, 31, 31), "Direct    %d", direct);
  draw_string( &bmfont,  0, OUTY-16, make_color(31, 31, 31), "Brush amount %d", brushamount);
  draw_string( &bmfont,  0, OUTY-8,  make_color(31, 31, 31), "Brush size   %d", brushsize);


  // Cursor
  draw_blit_sprite(-1, &imgcursor, ui_getMouseX(), ui_getMouseY());


  // Tile picks
  if(mouseinwintile<=MAXTILE)
  {
    switch(mouseinwinpresser)
    {
    case 0:
      todo = 1;
      tile = mouseinwintile;
      add = 1;
      break;
    case 1:
      todo = 2;
      tile = mouseinwintile&0xfffe;
      add = 1;
      break;
    case 2:
      todo = 4;
      tile = mouseinwintile&0xfffc;
      add = 1;
      break;
    case 3:
      todo = 4;
      tile = mouseinwintile&0xfff0;
      add = 2;
      break;
    }

    x = 0;
    cnt = 0;
    while(cnt < todo)
    {

      if(mouseinwintype&32)
      {
        FRect dst = {x, 0, x + SMALLX, SMALLY};
        draw_texture_box(&bmpbigtile[tile], NULL, &dst);
      }
      else
      {
        FRect dst = {x, 0, x + SMALLX, SMALLY};
        draw_texture_box(&bmpsmalltile[tile], NULL, &dst);
      }

      x+=SMALLX;
      tile+=add;
      cnt++;
    }
    draw_string( &bmfont,  0, 32, make_color(31, 31, 31), "Tile 0x%02x", mouseinwintile);
    draw_string( &bmfont,  0, 40, make_color(31, 31, 31), "Eats %d verts", Mesh_Cmd[mouseinwintype].vrt_count);
    if(mouseinwintype>=MAXMESHTYPE/2)
    {
      draw_string( &bmfont,  0, 56, make_color(31, 16, 16), "63x63 Tile");
    }
    else
    {
      draw_string( &bmfont,  0, 56, make_color(16, 16, 31), "31x31 Tile");
    }

    draw_schematic(-1, mouseinwintype, 0, 64);
  }

  // FX selection
  if( HAS_NO_BITS( mouseinwinfx, MESHFX_NOREFLECT ) )
    draw_blit_sprite(-1, &imgref, 0, 200);

  if( HAS_SOME_BITS( mouseinwinfx, MESHFX_SHINY ) )
    draw_blit_sprite(-1, &imgdrawref, 16, 200);

  if( HAS_SOME_BITS( mouseinwinfx, MESHFX_ANIM ) )
    draw_blit_sprite(-1, &imganim, 0, 216);

  if( HAS_SOME_BITS( mouseinwinfx, MESHFX_WALL ) )
    draw_blit_sprite(-1, &imgwall, 15, 215);

  if( HAS_SOME_BITS( mouseinwinfx, MESHFX_IMPASS ) )
    draw_blit_sprite(-1, &imgimpass, 15+8, 215);

  if( HAS_SOME_BITS( mouseinwinfx, MESHFX_DAMAGE ) )
    draw_blit_sprite(-1, &imgdamage, 15, 215+8);

  if( HAS_SOME_BITS( mouseinwinfx, MESHFX_SLIPPY ) )
    draw_blit_sprite(-1, &imgslippy, 15+8, 215+8);

  if( HAS_SOME_BITS( mouseinwinfx, MESHFX_WATER ) )
    draw_blit_sprite(-1, &imgwater, 0, 200);


  if(numattempt > 0)
  {
    draw_string( &bmfont,  0, 0, make_color(31, 31, 31), "numwritten %d/%d", numwritten, numattempt);
  }


  // Write double buffer to screen
  SDL_GL_SwapBuffers();


  return;
}


//------------------------------------------------------------------------------
void draw_slider(int tlx, int tly, int brx, int bry, int* pvalue,
                 int minvalue, int maxvalue)
{
  int cnt;
  int value;


  // Pick a new value
  value = *pvalue;
  if(ui_getMouseX() >= tlx && ui_getMouseX() <= brx && ui_getMouseY() >= tly && ui_getMouseY() <= bry && mous.latch.b)
  {
    value = (((ui_getMouseY() - tly)*(maxvalue-minvalue))/(bry - tly)) + minvalue;
  }
  if(value < minvalue) value = minvalue;
  if(value > maxvalue) value = maxvalue;
  *pvalue = value;


  // Draw it
  if(maxvalue != 0)
  {
    cnt = ((value-minvalue)*20/(maxvalue-minvalue))+11;

    draw_line(-1, make_color(cnt, cnt, cnt), tlx, (((value-minvalue)*(bry-tly)/(maxvalue-minvalue)))+tly, brx, (((value-minvalue)*(bry-tly)/(maxvalue-minvalue)))+tly);
  }

  draw_rect(-1, make_color(31, 31, 31), tlx, tly, brx, bry );
}

//------------------------------------------------------------------------------
void cart_draw_main(void)
{
  do_clear();

  draw_all_windows();
  draw_slider( 0, 250, 19, 350, &ambi,          0, 200);
  draw_slider(20, 250, 39, 350, &ambicut,       0, ambi);
  draw_slider(40, 250, 59, 350, &direct,        0, 100);
  draw_slider(60, 250, 79, 350, &brushamount, -50,  50);
  draw_lotsa_stuff();

  return;
}

//------------------------------------------------------------------------------
void show_info(void)
{
  log_info("%s - Version %01d.%02d\n", CARTMAN_NAME, CARTMAN_VERSION/100, CARTMAN_VERSION%100);
  return;
}

//------------------------------------------------------------------------------
int cartman(char * modulename)
{
  show_info();            // Text title

  load_all_windows();          // Load windows
  load_img();            // Load other images
  load_mesh_fans();          // Get fan data
  cart_load_module(modulename);        // Load the module
  render_all_windows();          // Create basic windows

  while(!SDLKEYDOWN(SDLK_ESCAPE) && !SDLKEYDOWN(SDLK_F1))      // Main loop
  {

    render_all_windows();
    cart_draw_main();
  }


  show_info();            // Ending statistics

  return 0;
}

//------------------------------------------------------------------------------
