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

/* Egoboo - graphic_fan.c
 * World mesh drawing.
 */

#include "graphic.h"
#include "game.h"

#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void render_fan( ego_mpd_t * pmesh, Uint32 fan )
{
    // ZZ> This function draws a mesh fan

    Uint16 commands;
    Uint16 vertices;
    Uint16 basetile;
    Uint16 texture;
    Uint16 cnt, tnc, entry, vertex;
    Uint16 tile;
    Uint8  fx;
    Uint8 type;

    mesh_mem_t  * pmem;
    tile_info_t * ptile;

    pmem  = &(pmesh->mmem);

    if ( !VALID_TILE(pmesh, fan) ) return;
    ptile = pmem->tile_list + fan;

    // vertex is a value from 0-15, for the meshcommandref/u/v variables
    // badvertex is a value that references the actual vertex number

    tile = ptile->img;               // Tile
    fx   = ptile->fx;                 // Fx bits
    type = ptile->type;               // Command type ( index to points in fan )

    if ( 0 != (tile & 0xFF00) ) return;
    type &= 0x3F;

    // Animate the tiles
    if ( fx & MPDFX_ANIM )
    {
        if ( type >= ( MAXMESHTYPE >> 1 ) )
        {
            // Big tiles
            basetile = tile & animtile[1].base_and;       // Animation set
            tile += animtile_data.frame_add << 1;         // Animated tile
            tile = ( tile & animtile[1].frame_and ) + basetile;
        }
        else
        {
            // Small tiles
            basetile = tile & animtile[0].base_and;  // Animation set
            tile += animtile_data.frame_add;         // Animated tile
            tile = ( tile & animtile[0].frame_and ) + basetile;
        }
    }

    texture = ( tile >> 6 ) + TX_TILE_0;       // 64 tiles in each 256x256 texture
    vertices = tile_dict[type].numvertices;    // Number of vertices
    commands = tile_dict[type].command_count;  // Number of commands

    // Change texture if need be
    if ( meshlasttexture != texture )
    {
        if ( meshnotexture )
        {
            oglx_texture_Bind( NULL );
            meshlasttexture = (Uint16)(~0);
        }
        else
        {
            oglx_texture_Bind( TxTexture + texture );
            meshlasttexture = texture;
        }
    }

    GL_DEBUG(glPushClientAttrib)(GL_CLIENT_VERTEX_ARRAY_BIT);
    {
        GL_DEBUG(glShadeModel)( GL_SMOOTH );

        //[claforte] Put this in an initialization function.
        GL_DEBUG(glEnableClientState)( GL_VERTEX_ARRAY );
        GL_DEBUG(glVertexPointer)(3, GL_FLOAT, 0, pmem->plst + ptile->vrtstart );

        GL_DEBUG(glEnableClientState)( GL_TEXTURE_COORD_ARRAY );
        GL_DEBUG(glTexCoordPointer)(2, GL_FLOAT, 0, pmem->tlst + ptile->vrtstart );

        GL_DEBUG(glEnableClientState)( GL_COLOR_ARRAY );
        GL_DEBUG(glColorPointer)(3, GL_FLOAT, 0, pmem->clst + ptile->vrtstart );

        // Render each command
        entry = 0;
        for ( cnt = 0; cnt < commands; cnt++ )
        {
            GL_DEBUG(glBegin)(GL_TRIANGLE_FAN );
            {
                for ( tnc = 0; tnc < tile_dict[type].command_entries[cnt]; tnc++ )
                {
                    vertex = tile_dict[type].command_verts[entry];

                    glArrayElement(vertex);

                    entry++;
                }

            }
            GL_DEBUG_END();
        }
    }
    GL_DEBUG(glPopClientAttrib)();

#if defined(DEBUG_MESH_NORMALS)
    GL_DEBUG(glDisable)( GL_TEXTURE_2D );
    GL_DEBUG(glColor4f)( 1, 1, 1, 1 );
    entry = ptile->vrtstart;
    for ( cnt = 0; cnt < 4; cnt++, entry++ )
    {
        GL_DEBUG(glBegin)( GL_LINES );
        {
            GL_DEBUG(glVertex3fv)(pmem->plst[entry]);
            GL_DEBUG(glVertex3f)(
                pmem->plst[entry][XX] + 128*pmem->ncache[fan][cnt][XX],
                pmem->plst[entry][YY] + 128*pmem->ncache[fan][cnt][YY],
                pmem->plst[entry][ZZ] + 128*pmem->ncache[fan][cnt][ZZ] );

        }
        GL_DEBUG_END();
    }
    GL_DEBUG(glEnable)( GL_TEXTURE_2D );
#endif

}

//--------------------------------------------------------------------------------------------
void render_hmap_fan( ego_mpd_t * pmesh, Uint32 fan )
{
    // ZZ> This function draws a mesh fan
    GLvertex v[4];
    Uint16 cnt, vertex;
    Uint32 badvertex;
    Uint16 tile;
    Uint8  fx;
    Uint8 type, twist;
    int ix, iy, ix_off[4] = {0, 1, 1, 0}, iy_off[4] = {0, 0, 1, 1};

    mesh_mem_t  * pmem;
    ego_mpd_info_t * pinfo;
    tile_info_t * ptile;

    pmem  = &(pmesh->mmem);
    pinfo = &(pmesh->info);

    if ( !VALID_TILE(pmesh, fan) ) return;
    ptile = pmem->tile_list + fan;

    // BB > the water info is for TILES, not fot vertices, so ignore all vertex info and just draw the water
    //      tile where it's supposed to go

    ix = fan % pmesh->info.tiles_x;
    iy = fan / pmesh->info.tiles_x;

    // vertex is a value from 0-15, for the meshcommandref/u/v variables
    // badvertex is a value that references the actual vertex number

    tile = ptile->img;               // Tile
    fx   = ptile->fx;                 // Fx bits
    type = ptile->type;               // Command type ( index to points in fan )
    twist = ptile->twist;

    if ( 0xFF00 == (tile & 0xFF00) ) return;
    type &= 0x3F;

    // Original points
    badvertex = ptile->vrtstart;          // Get big reference value
    for ( cnt = 0; cnt < 4; cnt++ )
    {
        float tmp;
        v[cnt].pos[XX] = (ix + ix_off[cnt]) * TILE_SIZE;
        v[cnt].pos[YY] = (iy + iy_off[cnt]) * TILE_SIZE;
        v[cnt].pos[ZZ] = pmem->plst[badvertex][ZZ];

        tmp = map_twist_nrm[twist].z;
        tmp *= tmp;

        v[cnt].col[RR] = tmp * ( tmp + (1.0f - tmp) * map_twist_nrm[twist].x * map_twist_nrm[twist].x);
        v[cnt].col[GG] = tmp * ( tmp + (1.0f - tmp) * map_twist_nrm[twist].y * map_twist_nrm[twist].y);
        v[cnt].col[BB] = tmp;

        badvertex++;
    }

    oglx_texture_Bind( NULL );

    // Render each command
    GL_DEBUG(glBegin)(GL_TRIANGLE_FAN );
    {
        for ( vertex = 0; vertex < 4; vertex++ )
        {
            GL_DEBUG(glColor3fv)(v[vertex].col );
            GL_DEBUG(glVertex3fv)(v[vertex].pos );
        }
    }
    GL_DEBUG_END();

}

//--------------------------------------------------------------------------------------------
void render_water_fan( ego_mpd_t * pmesh, Uint32 fan, Uint8 layer )
{
    // ZZ> This function draws a water fan
    GLvertex v[MAXMESHVERTICES];
    Uint16 type;
    Uint16 commands;
    Uint16 vertices;
    Uint16 texture, frame;
    Uint16 cnt, tnc;
    Uint32  badvertex;
    float offu, offv;
    Uint32  ambi;
    Uint8 mode;
    int ix, iy;
    int ix_off[4] = {1, 1, 0, 0}, iy_off[4] = {0, 1, 1, 0};
    int  imap[4];
    float x1, y1, fx_off[4], fy_off[4];

    if ( !VALID_TILE(pmesh, fan) ) return;

    // BB > the water info is for TILES, not for vertices, so ignore all vertex info and just draw the water
    //      tile where it's supposed to go

    ix = fan % pmesh->info.tiles_x;
    iy = fan / pmesh->info.tiles_x;

    // just do the mode this way instead of requiring all meshes to be multiples of 4
    mode = (iy & 1) | ((ix & 1) << 1);

    // To make life easier
    type = 0;                           // Command type ( index to points in fan )
    offu = water.layer[layer].tx.x;          // Texture offsets
    offv = water.layer[layer].tx.y;
    frame = water.layer[layer].frame;     // Frame

    texture = layer + TX_WATER_TOP;         // Water starts at texture 5
    vertices = tile_dict[type].numvertices;// Number of vertices
    commands = tile_dict[type].command_count;          // Number of commands

    x1 = ( float ) oglx_texture_GetTextureWidth( TxTexture + texture ) / ( float ) oglx_texture_GetImageWidth( TxTexture + texture );
    y1 = ( float ) oglx_texture_GetTextureHeight( TxTexture + texture ) / ( float ) oglx_texture_GetImageHeight( TxTexture + texture );

    fx_off[0] = x1;
    fy_off[0] = 0;

    fx_off[1] = x1;
    fy_off[1] = y1;

    fx_off[2] = 0;
    fy_off[2] = y1;

    fx_off[3] = 0;
    fy_off[3] = 0;

    for ( cnt = 0; cnt < 4; cnt++ )
    {
        imap[cnt] = cnt;
    }

    // flip the coordinates around based on the "mode" of the tile
    if ( 0 == (ix & 1) )
    {
        SWAP(int, imap[0], imap[3]);
        SWAP(int, imap[1], imap[2]);
    }

    if ( 0 == (iy & 1) )
    {
        SWAP(int, imap[0], imap[1]);
        SWAP(int, imap[2], imap[3]);
    }

    // Original points
    GL_DEBUG(glDisable)(GL_CULL_FACE );
    badvertex = pmesh->mmem.tile_list[fan].vrtstart;          // Get big reference value
    {
        float glob_amb = gfx.usefaredge ? light_a : 0;

        for ( cnt = 0; cnt < 4; cnt++ )
        {
            float ftmp;
            tnc = imap[cnt];

            v[cnt].pos[XX] = (ix + ix_off[cnt]) * TILE_SIZE;
            v[cnt].pos[YY] = (iy + iy_off[cnt]) * TILE_SIZE;
            v[cnt].pos[ZZ] = water.layer_z_add[layer][frame][tnc] + water.layer[layer].z;

            v[cnt].tex[SS] = fx_off[cnt] + offu;
            v[cnt].tex[TT] = fy_off[cnt] + offv;

            ambi = water.layer->light_add;
            ftmp = FF_TO_FLOAT( ambi ) + glob_amb;
            ftmp = CLIP(ftmp, 0, 1);

            v[cnt].col[RR] = pmesh->mmem.clst[badvertex][RR] + ftmp;
            v[cnt].col[GG] = pmesh->mmem.clst[badvertex][GG] + ftmp;
            v[cnt].col[BB] = pmesh->mmem.clst[badvertex][BB] + ftmp;
            v[cnt].col[AA] = FF_TO_FLOAT( water_data.layer[layer].alpha );

            badvertex++;
        }
    }

    // Change texture if need be
    if ( meshlasttexture != texture )
    {
        oglx_texture_Bind( TxTexture + texture );
        meshlasttexture = texture;
    }

    // Render each command
    for ( cnt = 0; cnt < commands; cnt++ )
    {
        GL_DEBUG(glBegin)(GL_TRIANGLE_FAN );
        {
            for ( tnc = 0; tnc < 4; tnc++ )
            {
                GL_DEBUG(glColor4fv)(v[tnc].col );
                GL_DEBUG(glTexCoord2fv)(v[tnc].tex );
                GL_DEBUG(glVertex3fv)(v[tnc].pos );
            }
        }
        GL_DEBUG_END();
    }
}
