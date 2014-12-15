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
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file game/player.c
/// @brief Implementation of player functions
/// @details

#include "game/player.h"

#include "game/ChrList.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INSTANTIATE_STACK( ACCESS_TYPE_NONE, player_t, PlaStack, MAX_PLAYER );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

IMPLEMENT_STACK( player_t, PlaStack, MAX_PLAYER );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void PlaStack_reset_all( void )
{
    PLA_REF cnt;

    // Reset the initial player data and latches
    for ( cnt = 0; cnt < MAX_PLAYER; cnt++ )
    {
        pla_reinit( PlaStack_get_ptr( cnt ) );
    }
    PlaStack.count        = 0;

    nexttimestamp = (( Uint32 )~0 );
    numplatimes   = 0;
}

//--------------------------------------------------------------------------------------------

CHR_REF PlaStack_get_ichr( const PLA_REF iplayer )
{
    player_t * pplayer;

    if ( iplayer >= MAX_PLAYER || !PlaStack.lst[iplayer].valid ) return INVALID_CHR_REF;
    pplayer = PlaStack_get_ptr( iplayer );

    if ( !INGAME_CHR( pplayer->index ) ) return INVALID_CHR_REF;

    return pplayer->index;
}

//--------------------------------------------------------------------------------------------
chr_t  * PlaStack_get_pchr( const PLA_REF iplayer )
{
    player_t * pplayer;

    if ( iplayer >= MAX_PLAYER || !PlaStack.lst[iplayer].valid ) return NULL;
    pplayer = PlaStack_get_ptr( iplayer );

    if ( !INGAME_CHR( pplayer->index ) ) return NULL;

    return ChrList_get_ptr( pplayer->index );
}

//--------------------------------------------------------------------------------------------
void PlaStack_add_tlatch( const PLA_REF iplayer, Uint32 time, latch_t net_latch )
{
    player_t * ppla;

    if ( !VALID_PLA( iplayer ) ) return;
    ppla = PlaStack_get_ptr( iplayer );

    if ( ppla->tlatch_count >= MAXLAG ) return;

    ppla->tlatch[ ppla->tlatch_count ].button = net_latch.b;
    ppla->tlatch[ ppla->tlatch_count ].x      = net_latch.x;
    ppla->tlatch[ ppla->tlatch_count ].y      = net_latch.y;
    ppla->tlatch[ ppla->tlatch_count ].time   = time;

    ppla->tlatch_count++;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void pla_reinit( player_t * ppla )
{
    if ( NULL == ppla ) return;

    //clear input pointer
    ppla->pdevice = NULL;

    //reset data
    BLANK_STRUCT_PTR( ppla )
    ppla->index  = INVALID_CHR_REF;

    // initialize the latches
    latch_init( &( ppla->local_latch ) );
    latch_init( &( ppla->net_latch ) );

    // initialize the tlatch array
    tlatch_ary_init( ppla->tlatch, MAXLAG );
}

//--------------------------------------------------------------------------------------------
/*void player_init( player_t * ppla )
{
    if ( NULL == ppla ) return;

    BLANK_STRUCT_PTR( ppla )

    ppla->index       = INVALID_CHR_REF;

    // initialize the device
    input_device_init( &( ppla->device ) );

    // initialize the latches
    latch_init( &( ppla->local_latch ) );
    latch_init( &( ppla->net_latch ) );

    // initialize the tlatch array
    tlatch_ary_init( ppla->tlatch, MAXLAG );
}*/
