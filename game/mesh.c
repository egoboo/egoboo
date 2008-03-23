#include "egoboo.h"
#include "mesh.h"

Uint32  numfanblock  = 0;                                   // Number of collision areas
Uint16  bumplistchr[MAXMESHFAN/16];                     // For character collisions
Uint16  bumplistchrnum[MAXMESHFAN/16];                  // Number on the block
Uint16  bumplistprt[MAXMESHFAN/16];                     // For particle collisions
Uint16  bumplistprtnum[MAXMESHFAN/16];                  // Number on the block
bool_t  meshexploremode  = bfalse;                          // Explore mode?
int     meshsizex;                                          // Size in fansquares
int     meshsizey;                                          //
float   meshedgex;                                          // Limits !!!BAD!!!
float   meshedgey;                                          //
float   meshedgez;                                          //
Uint16  meshlasttexture;                                    // Last texture used
Uint8   meshtype[MAXMESHFAN];                               // Command type
Uint8   meshfx[MAXMESHFAN];                                 // Special effects flags
Uint8   meshtwist[MAXMESHFAN];                              //
bool_t  meshinrenderlist[MAXMESHFAN];                       //
Uint16  meshtile[MAXMESHFAN];                               // Get texture from this
Uint32  meshvrtstart[MAXMESHFAN];                           // Which vertex to start at
vect3   meshvrtmins[MAXMESHFAN];                            // what is the minimum extent of the fan
vect3   meshvrtmaxs[MAXMESHFAN];                            // what is the maximum extent of the tile

Uint32  meshblockstart[( MAXMESHSIZEY/4 ) +1];
Uint32  meshfanstart[MAXMESHSIZEY];                         // Which fan to start a row with
float*  floatmemory   = NULL;                               // For malloc
float*  meshvrtx      = NULL;                               // Vertex position
float*  meshvrty      = NULL;                               //
float*  meshvrtz      = NULL;                               // Vertex elevation
Uint8*  meshvrtar_fp8 = NULL;                               // Vertex base light
Uint8*  meshvrtag_fp8 = NULL;                               // Vertex base light
Uint8*  meshvrtab_fp8 = NULL;                               // Vertex base light
Uint8*  meshvrtlr_fp8 = NULL;                               // Vertex light
Uint8*  meshvrtlg_fp8 = NULL;                               // Vertex light
Uint8*  meshvrtlb_fp8 = NULL;                               // Vertex light
Uint8   meshcommands[MAXMESHTYPE];                          // Number of commands
Uint8   meshcommandsize[MAXMESHTYPE][MAXMESHCOMMAND];       // Entries in each command
Uint16  meshcommandvrt[MAXMESHTYPE][MAXMESHCOMMANDENTRIES]; // Fansquare vertex list
Uint8   meshcommandnumvertices[MAXMESHTYPE];                // Number of vertices
Uint8   meshcommandref[MAXMESHTYPE][MAXMESHVERTICES];       // Lighting references
float   meshcommandu[MAXMESHTYPE][MAXMESHVERTICES];         // Vertex texture posi
float   meshcommandv[MAXMESHTYPE][MAXMESHVERTICES];         //
float   meshtileoffu[MAXTILETYPE];                          // Tile texture offset
float   meshtileoffv[MAXTILETYPE];                          //


//--------------------------------------------------------------------------------------------
bool_t load_mesh( char *modname )
{
  // ZZ> This function loads the level.mpd file
  FILE* fileread;
  STRING newloadname;
  int itmp, cnt;
  float ftmp;
  int fan, fancount;
  int numvert, numfan;
  int vert, vrt;

  snprintf( newloadname, sizeof( newloadname ), "%s%s/%s", modname, CData.gamedat_dir, CData.mesh_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, newloadname, "rb" );
  if ( NULL == fileread ) return bfalse;


#if SDL_BYTEORDER == SDL_LIL_ENDIAN
  fread( &itmp, 4, 1, fileread );  if ( itmp != MAPID ) return bfalse;
  fread( &itmp, 4, 1, fileread );  numvert = itmp;
  fread( &itmp, 4, 1, fileread );  meshsizex = itmp;
  fread( &itmp, 4, 1, fileread );  meshsizey = itmp;
#else
  fread( &itmp, 4, 1, fileread );  if (( int ) SDL_Swap32( itmp ) != MAPID ) return bfalse;
  fread( &itmp, 4, 1, fileread );  numvert = ( int ) SDL_Swap32( itmp );
  fread( &itmp, 4, 1, fileread );  meshsizex = ( int ) SDL_Swap32( itmp );
  fread( &itmp, 4, 1, fileread );  meshsizey = ( int ) SDL_Swap32( itmp );
#endif

  numfan = meshsizex * meshsizey;
  meshedgex = meshsizex * 128;
  meshedgey = meshsizey * 128;
  numfanblock = (( meshsizex >> 2 ) ) * (( meshsizey >> 2 ) );    // MESHSIZEX MUST BE MULTIPLE OF 4
  watershift = 3;
  if ( meshsizex > 16 )  watershift++;
  if ( meshsizex > 32 )  watershift++;
  if ( meshsizex > 64 )  watershift++;
  if ( meshsizex > 128 )  watershift++;
  if ( meshsizex > 256 )  watershift++;


  // Load fan data
  fan = 0;
  while ( fan < numfan )
  {
    fread( &itmp, 4, 1, fileread );

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
    itmp = SDL_Swap32( itmp );
#endif

    meshtype[fan] = itmp >> 24;
    meshfx[fan] = itmp >> 16;
    meshtile[fan] = itmp;

    fan++;
  }
  // Load fan data
  fan = 0;
  while ( fan < numfan )
  {
    fread( &itmp, 1, 1, fileread );
    meshtwist[fan] = itmp;
    fan++;
  }


  // Load vertex fan_x data
  cnt = 0;
  while ( cnt < numvert )
  {
    fread( &ftmp, 4, 1, fileread );

    meshvrtx[cnt] = ftmp;

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
    meshvrtx[cnt] = LoadFloatByteswapped( &meshvrtx[cnt] );
#endif
    cnt++;
  }


  // Load vertex fan_y data
  cnt = 0;
  while ( cnt < numvert )
  {
    fread( &ftmp, 4, 1, fileread );

    meshvrty[cnt] = ftmp;

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
    meshvrty[cnt] = LoadFloatByteswapped( &meshvrty[cnt] );
#endif

    cnt++;
  }


  // Load vertex z data
  cnt = 0;
  while ( cnt < numvert )
  {
    fread( &ftmp, 4, 1, fileread );

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    meshvrtz[cnt] = ftmp / 16.0;  // Cartman uses 4 bit fixed point for Z
#else
    meshvrtz[cnt] = ( LoadFloatByteswapped( &ftmp ) ) / 16.0;   // Cartman uses 4 bit fixed point for Z
#endif

    cnt++;
  }

  // Load vertex a data
  cnt = 0;
  while ( cnt < numvert )
  {
    fread( &itmp, 1, 1, fileread );

    meshvrtar_fp8[cnt] = 0; // itmp;
    meshvrtag_fp8[cnt] = 0; // itmp;
    meshvrtab_fp8[cnt] = 0; // itmp;

    meshvrtlr_fp8[cnt] = 0;
    meshvrtlg_fp8[cnt] = 0;
    meshvrtlb_fp8[cnt] = 0;

    cnt++;
  }

  fs_fileClose( fileread );

  make_fanstart();

  vert = 0;
  fancount = meshsizey * meshsizex;
  for ( fan = 0; fan < fancount; fan++ )
  {
    int vrtcount = meshcommandnumvertices[meshtype[fan]];
    int vrtstart = vert;

    meshvrtstart[fan] = vrtstart;

    meshvrtmins[fan].x = meshvrtmaxs[fan].x = meshvrtx[vrtstart];
    meshvrtmins[fan].y = meshvrtmaxs[fan].y = meshvrty[vrtstart];
    meshvrtmins[fan].z = meshvrtmaxs[fan].z = meshvrtz[vrtstart];

    for ( vrt = vrtstart + 1; vrt < vrtstart + vrtcount; vrt++ )
    {
      meshvrtmins[fan].x = MIN( meshvrtmins[fan].x, meshvrtx[vrt] );
      meshvrtmins[fan].y = MIN( meshvrtmins[fan].y, meshvrty[vrt] );
      meshvrtmins[fan].z = MIN( meshvrtmins[fan].z, meshvrtz[vrt] );

      meshvrtmaxs[fan].x = MAX( meshvrtmaxs[fan].x, meshvrtx[vrt] );
      meshvrtmaxs[fan].y = MAX( meshvrtmaxs[fan].y, meshvrty[vrt] );
      meshvrtmaxs[fan].z = MAX( meshvrtmaxs[fan].z, meshvrtz[vrt] );
    }

    vert += vrtcount;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
void load_mesh_fans()
{
  // ZZ> This function loads fan types for the terrain
  int cnt, entry;
  int numfantype, fantype, bigfantype, vertices;
  int numcommand, command, commandsize;
  int itmp;
  FILE* fileread;
  float offx, offy;


  // Initialize all mesh types to 0
  entry = 0;
  while ( entry < MAXMESHTYPE )
  {
    meshcommandnumvertices[entry] = 0;
    meshcommands[entry] = 0;
    entry++;
  }


  // Open the file and go to it
  log_info("Loading fan types of the terrain... ");
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s/%s", CData.basicdat_dir, CData.fans_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, CStringTmp1, "r" );
  if ( NULL == fileread )
  {
    log_message("Failed!\n");
    return bfalse;
  }

  log_message("Succeeded!\n");
  numfantype = fget_next_int( fileread );
  fantype = 0;
  bigfantype = MAXMESHTYPE / 2; // Duplicate for 64x64 tiles
  while ( fantype < numfantype )
  {
    vertices = fget_next_int( fileread );
    meshcommandnumvertices[fantype] = vertices;
    meshcommandnumvertices[bigfantype] = vertices;  // Dupe
    cnt = 0;
    while ( cnt < vertices )
    {
      meshcommandref[fantype][cnt] =
      meshcommandref[bigfantype][cnt] = fget_next_int( fileread );

      meshcommandu[fantype][cnt] =
      meshcommandu[bigfantype][cnt] = fget_next_float( fileread );   // Dupe

      meshcommandv[fantype][cnt] =
      meshcommandv[bigfantype][cnt] = fget_next_float( fileread );   // Dupe
      cnt++;
    }


    numcommand = fget_next_int( fileread );
    meshcommands[fantype] = numcommand;
    meshcommands[bigfantype] = numcommand;  // Dupe
    entry = 0;
    command = 0;
    while ( command < numcommand )
    {
      commandsize = fget_next_int( fileread );
      meshcommandsize[fantype][command] = commandsize;
      meshcommandsize[bigfantype][command] = commandsize;  // Dupe
      cnt = 0;
      while ( cnt < commandsize )
      {
        itmp = fget_next_int( fileread );
        meshcommandvrt[fantype][entry] = itmp;
        meshcommandvrt[bigfantype][entry] = itmp;  // Dupe
        entry++;
        cnt++;
      }
      command++;
    }
    fantype++;
    bigfantype++;  // Dupe
  }
  fs_fileClose( fileread );



  // Correct all of them silly texture positions for seamless tiling
  entry = 0;
  while ( entry < MAXMESHTYPE / 2 )
  {
    cnt = 0;
    while ( cnt < meshcommandnumvertices[entry] )
    {
      meshcommandu[entry][cnt] = (( .6 / 32 ) + ( meshcommandu[entry][cnt] * 30.8 / 32 ) ) / 8;
      meshcommandv[entry][cnt] = (( .6 / 32 ) + ( meshcommandv[entry][cnt] * 30.8 / 32 ) ) / 8;
      cnt++;
    }
    entry++;
  }
  // Do for big tiles too
  while ( entry < MAXMESHTYPE )
  {
    cnt = 0;
    while ( cnt < meshcommandnumvertices[entry] )
    {
      meshcommandu[entry][cnt] = (( .6 / 64 ) + ( meshcommandu[entry][cnt] * 62.8 / 64 ) ) / 4;
      meshcommandv[entry][cnt] = (( .6 / 64 ) + ( meshcommandv[entry][cnt] * 62.8 / 64 ) ) / 4;
      cnt++;
    }
    entry++;
  }


  // Make tile texture offsets
  entry = 0;
  while ( entry < MAXTILETYPE )
  {
    offx = ( entry & 7 ) / 8.0;
    offy = ( entry >> 3 ) / 8.0;
    meshtileoffu[entry] = offx;
    meshtileoffv[entry] = offy;
    entry++;
  }
}

//--------------------------------------------------------------------------------------------
void make_fanstart()
{
  // ZZ> This function builds a look up table to ease calculating the
  //     fan number given an x,y pair
  int cnt;


  cnt = 0;
  while ( cnt < meshsizey )
  {
    meshfanstart[cnt] = meshsizex * cnt;
    cnt++;
  }

  cnt = 0;
  while ( cnt < ( meshsizey >> 2 ) )
  {
    meshblockstart[cnt] = ( meshsizex >> 2 ) * cnt;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void make_twist()
{
  // ZZ> This function precomputes surface normals and steep hill acceleration for
  //     the mesh
  int cnt;
  int x, y;
  float fx, fy, fz, ftmp;
  float xslide, yslide;


  for ( cnt = 0; cnt < 256; cnt++ )
  {
    y = cnt >> 4;
    x = cnt & 15;

    fy = y - 7;  // -7 to 8
    fx = x - 7;  // -7 to 8
    maptwist_ud[cnt] = 32768 + fy * SLOPE;
    maptwist_lr[cnt] = 32768 + fx * SLOPE;

    ftmp = fx * fx + fy * fy;
    if ( ftmp > 121.0f )
    {
      fz = 0.0f;
      ftmp = sqrt( ftmp );
      fx /= ftmp;
      fy /= ftmp;
    }
    else
    {
      fz = sqrt( 1.0f - ftmp / 121.0f );
      fx /= 11.0f;
      fy /= 11.0f;
    }

    xslide = fx;
    yslide = fy;

    mapnrm[cnt].x = fx;
    mapnrm[cnt].y = -fy;
    mapnrm[cnt].z = fz;

    maptwistflat[cnt] = fz > ( 1.0 - SLIDEFIX );
  }
}

//--------------------------------------------------------------------------------------------
bool_t mesh_calc_normal_fan( int fan, vect3 * pnrm, vect3 * ppos )
{
  bool_t retval = bfalse;
  Uint32 cnt;
  vect3 normal, position;

  if ( INVALID_TILE == fan )
  {
    normal.x = 0.0f;
    normal.y = 0.0f;
    normal.z = MESH_FAN_TO_FLOAT( 1 );

    position.x =
      position.y =
        position.z = 0.0f;
  }
  else
  {
    float dzdx, dzdy, dx, dy;
    float z0, z1, z2, z3;
    int vrtstart = meshvrtstart[fan];

    position.x = 0;
    position.y = 0;
    position.z = 0;
    for ( cnt = 0; cnt < 4; cnt++ )
    {
      position.x += meshvrtx[vrtstart + cnt];
      position.y += meshvrty[vrtstart + cnt];
      position.z += meshvrtz[vrtstart + cnt];
    };
    position.x /= 4.0f;
    position.y /= 4.0f;
    position.z /= 4.0f;

    dx = 1;
    if ( meshvrtx[vrtstart + 0] > meshvrtx[vrtstart + 1] ) dx *= -1;

    dy = 1;
    if ( meshvrtx[vrtstart + 0] > meshvrtx[vrtstart + 3] ) dy *= -1;

    z0 = meshvrtz[vrtstart + 0];
    z1 = meshvrtz[vrtstart + 1];
    z2 = meshvrtz[vrtstart + 2];
    z3 = meshvrtz[vrtstart + 3];

    // find the derivatives of the height function used to find level
    dzdx = 0.5 * ( z1 - z0 + z2 - z3 );
    dzdy = 0.5 * ( z3 - z0 + z2 - z1 );

    // use these to compute the normal
    normal.x = -dy * dzdx;
    normal.y = -dx * dzdy;
    normal.z =  dx * dy * MESH_FAN_TO_FLOAT( 1 );

    // orient the normal in the proper direction
    if ( normal.z*gravity > 0.0f )
    {
      normal.x *= -1.0f;
      normal.y *= -1.0f;
      normal.z *= -1.0f;
    };
  };


  // make sure that the normal is not trivial
  retval = ( ABS( normal.x ) + ABS( normal.y ) + ABS( normal.z ) ) > 0.0f;

  if ( NULL != pnrm )
  {
    *pnrm = normal;
  };

  if ( NULL != ppos )
  {
    *ppos = position;
  };

  return retval;
};


//--------------------------------------------------------------------------------------------
bool_t mesh_calc_normal_pos( int fan, vect3 pos, vect3 * pnrm )
{
  bool_t retval = bfalse;
  vect3 normal;

  if ( INVALID_TILE == fan )
  {
    normal.x = 0.0f;
    normal.y = 0.0f;
    normal.z = 1.0f;
  }
  else
  {
    float dzdx, dzdy, dx, dy;
    float z0, z1, z2, z3;
    float fx, fy;
    int vrtstart = meshvrtstart[fan];

    dx = 1;
    if ( meshvrtx[vrtstart + 0] > meshvrtx[vrtstart + 1] ) dx = -1;

    dy = 1;
    if ( meshvrty[vrtstart + 0] > meshvrty[vrtstart + 3] ) dy = -1;

    z0 = meshvrtz[vrtstart + 0];
    z1 = meshvrtz[vrtstart + 1];
    z2 = meshvrtz[vrtstart + 2];
    z3 = meshvrtz[vrtstart + 3];

    pos.x = MESH_FLOAT_TO_FAN( pos.x );
    pos.y = MESH_FLOAT_TO_FAN( pos.y );

    fx = pos.x - (( int ) pos.x );
    fy = pos.y - (( int ) pos.y );

    // find the derivatives of the height function used to find level

    dzdx = (( 1.0f - fy ) * ( z1 - z0 ) + fy * ( z2 - z3 ) );
    dzdy = (( 1.0f - fx ) * ( z3 - z0 ) + fx * ( z2 - z1 ) );

    // use these to compute the normal
    normal.x = -dy * dzdx;
    normal.y = -dx * dzdy;
    normal.z =  dx * dy * MESH_FAN_TO_FLOAT( 1 );

    // orient the normal in the proper direction
    if ( normal.z*gravity > 0.0f )
    {
      normal.x *= -1.0f;
      normal.y *= -1.0f;
      normal.z *= -1.0f;
    };

    normal = Normalize( normal );
  };


  // make sure that the normal is not trivial
  retval = ABS( normal.x ) + ABS( normal.y ) + ABS( normal.z ) > 0.0f;

  if ( NULL != pnrm )
  {
    *pnrm = normal;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t mesh_calc_normal( vect3 pos, vect3 * pnrm )
{
  bool_t retval = bfalse;
  Uint32 fan;
  vect3 normal;

  fan = mesh_get_fan( pos );
  if ( INVALID_TILE == fan )
  {
    normal.x = 0.0f;
    normal.y = 0.0f;
    normal.z = 1.0f;
  }
  else
  {
    float dzdx, dzdy, dx, dy;
    float z0, z1, z2, z3;
    float fx, fy;
    int vrtstart = meshvrtstart[fan];

    dx = 1;
    if ( meshvrtx[vrtstart + 0] > meshvrtx[vrtstart + 1] ) dx = -1;

    dy = 1;
    if ( meshvrty[vrtstart + 0] > meshvrty[vrtstart + 3] ) dy = -1;

    z0 = meshvrtz[vrtstart + 0];
    z1 = meshvrtz[vrtstart + 1];
    z2 = meshvrtz[vrtstart + 2];
    z3 = meshvrtz[vrtstart + 3];

    pos.x = MESH_FLOAT_TO_FAN( pos.x );
    pos.y = MESH_FLOAT_TO_FAN( pos.y );

    fx = pos.x - (( int ) pos.x );
    fy = pos.y - (( int ) pos.y );

    // find the derivatives of the height function used to find level

    dzdx = (( 1.0f - fy ) * ( z1 - z0 ) + fy * ( z2 - z3 ) );
    dzdy = (( 1.0f - fx ) * ( z3 - z0 ) + fx * ( z2 - z1 ) );

    // use these to compute the normal
    normal.x = -dy * dzdx;
    normal.y = -dx * dzdy;
    normal.z =  dx * dy * MESH_FAN_TO_FLOAT( 1 );

    // orient the normal in the proper direction
    if ( normal.z*gravity > 0.0f )
    {
      normal.x *= -1.0f;
      normal.y *= -1.0f;
      normal.z *= -1.0f;
    };

    normal = Normalize( normal );
  };


  // make sure that the normal is not trivial
  retval = ABS( normal.x ) + ABS( normal.y ) + ABS( normal.z ) > 0.0f;

  if ( NULL != pnrm )
  {
    *pnrm = normal;
  };

  return retval;
};


//--------------------------------------------------------------------------------------------
Uint32 mesh_get_fan( vect3 pos )
{
  // BB > find the tile under <pos.x,pos.y>, but MAKE SURE we have the right tile.

  Uint32 ivert, testfan = INVALID_FAN;
  float minx, maxx, miny, maxy;
  int i, ix, iy;
  bool_t bfound = bfalse;

  if ( !mesh_check( pos.x, pos.y ) )
    return testfan;

  ix = MESH_FLOAT_TO_FAN( pos.x );
  iy = MESH_FLOAT_TO_FAN( pos.y );

  testfan = ix + meshfanstart[iy];

  ivert = meshvrtstart[testfan];
  minx = maxx = meshvrtx[ivert];
  miny = maxy = meshvrty[ivert];
  for ( i = 1;i < 4;i++, ivert++ )
  {
    minx = MIN( minx, meshvrtx[ivert] );
    maxx = MAX( maxx, meshvrtx[ivert] );

    miny = MIN( miny, meshvrty[ivert] );
    maxy = MAX( maxy, meshvrty[ivert] );
  };

  if ( pos.x < minx ) { ix--; bfound = btrue; }
  else if ( pos.x > maxx ) { ix++; bfound = btrue; }
  if ( pos.y < miny ) { iy--; bfound = btrue; }
  else if ( pos.y > maxy ) { iy++; bfound = btrue; }

  if ( ix < 0 || iy < 0 || ix > meshsizex || iy > meshsizey )
    testfan = INVALID_FAN;
  else if ( bfound )
    testfan = ix + meshfanstart[iy];

  return testfan;
};

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_block( vect3 pos )
{
  // BB > find the block under <x,y>

  return mesh_convert_block( MESH_FLOAT_TO_BLOCK( pos.x ), MESH_FLOAT_TO_BLOCK( pos.y ) );
};



//---------------------------------------------------------------------------------------------
float mesh_get_level( Uint32 fan, float x, float y, bool_t waterwalk )
{
  // ZZ> This function returns the height of a point within a mesh fan, precise
  //     If waterwalk is nonzero and the fan is watery, then the level returned is the
  //     level of the water.
  float z0, z1, z2, z3;         // Height of each fan corner
  float zleft, zright, zdone;   // Weighted height of each side
  float fx, fy;

  if ( INVALID_FAN == fan ) return 0.0f;

  x = MESH_FLOAT_TO_FAN( x );
  y = MESH_FLOAT_TO_FAN( y );

  fx = x - (( int ) x );
  fy = y - (( int ) y );


  z0 = meshvrtz[meshvrtstart[fan] + 0];
  z1 = meshvrtz[meshvrtstart[fan] + 1];
  z2 = meshvrtz[meshvrtstart[fan] + 2];
  z3 = meshvrtz[meshvrtstart[fan] + 3];

  zleft = ( z0 * ( 1.0f - fy ) + z3 * fy );
  zright = ( z1 * ( 1.0f - fy ) + z2 * fy );
  zdone = ( zleft * ( 1.0f - fx ) + zright * fx );

  if ( waterwalk )
  {
    if ( watersurfacelevel > zdone && mesh_has_some_bits( fan, MESHFX_WATER ) && wateriswater )
    {
      return watersurfacelevel;
    }
  }

  return zdone;
}

//--------------------------------------------------------------------------------------------
bool_t get_mesh_memory()
{
  // ZZ> This function gets a load of memory for the terrain mesh
  floatmemory = ( float * ) malloc( CData.maxtotalmeshvertices * BYTESFOREACHVERTEX );
  if ( floatmemory == NULL )
    return bfalse;

  meshvrtx = &floatmemory[0];
  meshvrty = &meshvrtx[CData.maxtotalmeshvertices];
  meshvrtz = &meshvrty[CData.maxtotalmeshvertices];
  meshvrtar_fp8 = ( Uint8 * ) & meshvrtz[CData.maxtotalmeshvertices];
  meshvrtag_fp8 = &meshvrtar_fp8[CData.maxtotalmeshvertices];
  meshvrtab_fp8 = &meshvrtag_fp8[CData.maxtotalmeshvertices];
  meshvrtlr_fp8 = &meshvrtab_fp8[CData.maxtotalmeshvertices];
  meshvrtlg_fp8 = &meshvrtlr_fp8[CData.maxtotalmeshvertices];
  meshvrtlb_fp8 = &meshvrtlg_fp8[CData.maxtotalmeshvertices];
  return btrue;
}

//--------------------------------------------------------------------------------------------
void free_mesh_memory()
{
  if ( floatmemory != NULL )
  {
    free( floatmemory );
    floatmemory = NULL;
  }
}

//--------------------------------------------------------------------------------------------
void mesh_set_colora( int fan_x, int fan_y, int color )
{
  Uint32 cnt, fan, vert, numvert;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return;

  vert = meshvrtstart[fan];
  cnt = 0;
  numvert = meshcommandnumvertices[meshtype[fan]];
  while ( cnt < numvert )
  {
    meshvrtar_fp8[vert] = color;
    meshvrtag_fp8[vert] = color;
    meshvrtab_fp8[vert] = color;
    vert++;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
void set_fan_colorl( int fan_x, int fan_y, int color )
{
  Uint32 cnt, fan, vert, numvert;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return;

  vert = meshvrtstart[fan];
  cnt = 0;
  numvert = meshcommandnumvertices[meshtype[fan]];
  while ( cnt < numvert )
  {
    meshvrtlr_fp8[vert] = color;
    meshvrtlg_fp8[vert] = color;
    meshvrtlb_fp8[vert] = color;
    vert++;
    cnt++;
  }
}


//--------------------------------------------------------------------------------------------
bool_t mesh_clear_fan_bits( int fan_x, int fan_y, Uint32 bits )
{
  bool_t retval = bfalse;
  Uint32 fan;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return retval;

  retval = mesh_has_some_bits( fan, bits );

  meshfx[fan] &= ~bits;

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t mesh_add_fan_bits( int fan_x, int fan_y, Uint32 bits )
{
  bool_t retval = bfalse;
  Uint32 fan;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return retval;

  retval = MISSING_BITS( meshfx[fan], bits );

  meshfx[fan] |= bits;

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t mesh_set_fan_bits( int fan_x, int fan_y, Uint32 bits )
{
  bool_t retval = bfalse;
  Uint32 fan;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return retval;

  retval = ( meshfx[fan] != bits );

  meshfx[fan] = bits;

  return retval;
};

//--------------------------------------------------------------------------------------------
int mesh_bump_tile( int fan_x, int fan_y )
{
  Uint32 fan;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return 0;

  meshtile[fan]++;

  return meshtile[fan];
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_tile( int fan_x, int fan_y )
{
  Uint32 fan;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return INVALID_TILE;

  return meshtile[fan];
}

//--------------------------------------------------------------------------------------------
bool_t mesh_set_tile( int fan_x, int fan_y, Uint32 become )
{
  bool_t retval = bfalse;
  Uint32 fan;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return retval;

  if ( become != 0 )
  {
    meshtile[fan] = become;
    retval = btrue;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_convert_fan( int fan_x, int fan_y )
{
  // BB > convert <fan_x,fan_y> to a fanblock

  if ( fan_x < 0 || fan_x > meshsizex || fan_y < 0 || fan_y > meshsizey ) return INVALID_FAN;

  return fan_x + meshfanstart[fan_y];
};

//--------------------------------------------------------------------------------------------
Uint32 mesh_convert_block( int block_x, int block_y )
{
  // BB > convert <block_x,block_y> to a fanblock

  if ( block_x < 0 || block_x > ( meshsizex >> 2 ) || block_y < 0 || block_y > ( meshsizey >> 2 ) ) return INVALID_FAN;

  return block_x + meshblockstart[block_y];
};

//--------------------------------------------------------------------------------------------
float mesh_clip_x( float x )
{
  if ( x <      0.0f )  x = 0.0f;
  if ( x > meshedgex )  x = meshedgex;

  return x;
}

//--------------------------------------------------------------------------------------------
float mesh_clip_y( float y )
{
  if ( y <      0.0f )  y = 0.0f;
  if ( y > meshedgey )  y = meshedgey;

  return y;
}

//--------------------------------------------------------------------------------------------
int mesh_clip_fan_x( int ix )
{
  if ( ix < 0 )  ix = 0;
  if ( ix > meshsizex - 1 )  ix = meshsizex - 1;

  return ix;
};

//--------------------------------------------------------------------------------------------
int mesh_clip_fan_y( int iy )
{
  if ( iy < 0 )  iy = 0;
  if ( iy > meshsizey - 1 )  iy = meshsizey - 1;

  return iy;
};

//--------------------------------------------------------------------------------------------
int mesh_clip_block_x( int ix )
{
  if ( ix < 0 )  ix = 0;
  if ( ix > ( meshsizex >> 2 ) - 1 )  ix = ( meshsizex >> 2 ) - 1;

  return ix;
};

//--------------------------------------------------------------------------------------------
int mesh_clip_block_y( int iy )
{
  if ( iy < 0 )  iy = 0;
  if ( iy > ( meshsizey >> 2 ) - 1 )  iy = ( meshsizey >> 2 ) - 1;

  return iy;
};

//--------------------------------------------------------------------------------------------
bool_t mesh_check( float x, float y )
{
  if ( x < 0 || x > meshedgex ) return bfalse;
  if ( y < 0 || y > meshedgex ) return bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_hitawall( vect3 pos, float size_x, float size_y, Uint32 collision_bits )
{
  // ZZ> This function returns nonzero if <pos.x,pos.y> is in an invalid tile

  int fan_x, fan_x_min, fan_x_max, fan_y, fan_y_min, fan_y_max;
  Uint32 fan, pass = 0;

  fan_x_min = ( pos.x - size_x < 0 ) ? 0 : MESH_FLOAT_TO_FAN( pos.x - size_x );
  fan_x_max = ( pos.x + size_x < 0 ) ? 0 : MESH_FLOAT_TO_FAN( pos.x + size_x );

  fan_y_min = ( pos.y - size_y < 0 ) ? 0 : MESH_FLOAT_TO_FAN( pos.y - size_y );
  fan_y_max = ( pos.y + size_y < 0 ) ? 0 : MESH_FLOAT_TO_FAN( pos.y + size_y );

  for ( fan_x = fan_x_min; fan_x <= fan_x_max; fan_x++ )
  {
    for ( fan_y = fan_y_min; fan_y <= fan_y_max; fan_y++ )
    {
      pos.x = MESH_FAN_TO_INT( fan_x ) + ( 1 << 6 );
      pos.y = MESH_FAN_TO_INT( fan_y ) + ( 1 << 6 );

      fan = mesh_get_fan( pos );
      if ( INVALID_FAN != fan ) pass |= meshfx[ fan ];
    };
  };

  return pass & collision_bits;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_bits( int fan, Uint32 bits )
{
  if ( INVALID_FAN == fan ) return 0;

  return meshfx[fan] & bits;
};

//--------------------------------------------------------------------------------------------
bool_t mesh_has_some_bits( int fan, Uint32 bits )
{
  if ( INVALID_FAN == fan ) return 0;

  return HAS_SOME_BITS( meshfx[fan], bits );
};

//--------------------------------------------------------------------------------------------
bool_t mesh_has_no_bits( int fan, Uint32 bits )
{
  if ( INVALID_FAN == fan ) return 0;

  return HAS_NO_BITS( meshfx[fan], bits );
};

//--------------------------------------------------------------------------------------------
bool_t mesh_has_all_bits( int fan, Uint32 bits )
{
  if ( INVALID_FAN == fan ) return 0;

  return HAS_ALL_BITS( meshfx[fan], bits );
};

//--------------------------------------------------------------------------------------------
float mesh_fraction_x( float x )
{
  return x / meshedgex;
};

//--------------------------------------------------------------------------------------------
float mesh_fraction_y( float y )
{
  return y / meshedgey;
};

//--------------------------------------------------------------------------------------------
bool_t mesh_in_renderlist( int fan )
{
  if ( INVALID_FAN == fan ) return bfalse;

  return meshinrenderlist[fan];
};

//--------------------------------------------------------------------------------------------
void mesh_remove_renderlist( int fan )
{
  meshinrenderlist[fan] = bfalse;
};

//--------------------------------------------------------------------------------------------
void mesh_add_renderlist( int fan )
{
  meshinrenderlist[fan] = btrue;
};

//--------------------------------------------------------------------------------------------
Uint8 mesh_get_twist( int fan )
{
  Uint8 retval = 0x77;

  if ( INVALID_FAN != fan )
  {
    retval = meshtwist[fan];
  }

  return retval;
};