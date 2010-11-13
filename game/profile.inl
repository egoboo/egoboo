#pragma once

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

#include "profile.h"

#include "char.h"
#include "particle.h"
#include "enchant.h"
#include "mad.h"

//--------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//--------------------------------------------------------------------------------------------

static INLINE CAP_REF pro_get_icap( const PRO_REF iobj );
static INLINE MAD_REF pro_get_imad( const PRO_REF iobj );
static INLINE EVE_REF pro_get_ieve( const PRO_REF iobj );
static INLINE PIP_REF pro_get_ipip( const PRO_REF iobj, int ipip );
static INLINE IDSZ    pro_get_idsz( const PRO_REF iobj, int type );

static INLINE cap_t *     pro_get_pcap( const PRO_REF iobj );
static INLINE mad_t *     pro_get_pmad( const PRO_REF iobj );
static INLINE eve_t *     pro_get_peve( const PRO_REF iobj );
static INLINE pip_t *     pro_get_ppip( const PRO_REF iobj, int pip_index );
static INLINE Mix_Chunk * pro_get_chunk( const PRO_REF iobj, int index );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE CAP_REF pro_get_icap( const PRO_REF iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return ( CAP_REF )MAX_CAP;
    pobj = ProList.lst + iobj;

    return LOADED_CAP( pobj->icap ) ? pobj->icap : ( CAP_REF )MAX_CAP;
}

//--------------------------------------------------------------------------------------------
static INLINE MAD_REF pro_get_imad( const PRO_REF iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return ( MAD_REF )MAX_MAD;
    pobj = ProList.lst + iobj;

    return LOADED_MAD( pobj->imad ) ? pobj->imad : ( MAD_REF )MAX_MAD;
}

//--------------------------------------------------------------------------------------------
static INLINE EVE_REF pro_get_ieve( const PRO_REF iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return ( EVE_REF )MAX_EVE;
    pobj = ProList.lst + iobj;

    return LOADED_EVE( pobj->ieve ) ? pobj->ieve : ( EVE_REF )MAX_EVE;
}

//--------------------------------------------------------------------------------------------
static INLINE PIP_REF pro_get_ipip( const PRO_REF iobj, int pip_index )
{
    pro_t * pobj;
    PIP_REF found_pip, global_pip;

    found_pip = ( PIP_REF )MAX_PIP;

    if ( !LOADED_PRO( iobj ) )
    {
        // check for a global pip
        global_pip = pip_index;
        if ( LOADED_PIP( global_pip ) )
        {
            found_pip = global_pip;
        }
    }
    else
    {
        // this pip is relative to a certain object
        pobj = ProList.lst + iobj;

        // find the local pip if it exists
        if ( pip_index < MAX_PIP_PER_PROFILE )
        {
            found_pip = pobj->prtpip[pip_index];
        }
    }

    return found_pip;
}

//--------------------------------------------------------------------------------------------
static INLINE IDSZ pro_get_idsz( const PRO_REF iobj, int type )
{
    cap_t * pcap;

    if ( type >= IDSZ_COUNT ) return IDSZ_NONE;

    pcap = pro_get_pcap( iobj );
    if ( NULL == pcap ) return IDSZ_NONE;

    return pcap->idsz[type];
}

//--------------------------------------------------------------------------------------------
static INLINE cap_t * pro_get_pcap( const PRO_REF iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return NULL;
    pobj = ProList.lst + iobj;

    if ( !LOADED_CAP( pobj->icap ) ) return NULL;

    return CapStack.lst + pobj->icap;
}

//--------------------------------------------------------------------------------------------
static INLINE mad_t * pro_get_pmad( const PRO_REF iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return NULL;
    pobj = ProList.lst + iobj;

    if ( !LOADED_MAD( pobj->imad ) ) return NULL;

    return MadStack.lst + pobj->imad;
}

//--------------------------------------------------------------------------------------------
static INLINE eve_t * pro_get_peve( const PRO_REF iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return NULL;
    pobj = ProList.lst + iobj;

    if ( !LOADED_EVE( pobj->ieve ) ) return NULL;

    return EveStack.lst + pobj->ieve;
}

//--------------------------------------------------------------------------------------------
static INLINE pip_t * pro_get_ppip( const PRO_REF iobj, int pip_index )
{
    pro_t * pobj;
    PIP_REF global_pip, local_pip;

    if ( !LOADED_PRO( iobj ) )
    {
        // check for a global pip
        global_pip = pip_index;
        if ( LOADED_PIP( global_pip ) )
        {
            return PipStack.lst + global_pip;
        }
        else
        {
            return NULL;
        }
    }

    // this pip is relative to a certain object
    pobj = ProList.lst + iobj;

    // find the local pip if it exists
    local_pip = ( PIP_REF )MAX_PIP;
    if ( pip_index < MAX_PIP_PER_PROFILE )
    {
        local_pip = pobj->prtpip[pip_index];
    }

    return LOADED_PIP( local_pip ) ? PipStack.lst + local_pip : NULL;
}

//--------------------------------------------------------------------------------------------
static INLINE Mix_Chunk * pro_get_chunk( const PRO_REF iobj, int index )
{
    pro_t * pobj;

    if ( !VALID_SND( index ) ) return NULL;

    if ( !LOADED_PRO( iobj ) ) return NULL;
    pobj = ProList.lst + iobj;

    return pobj->wavelist[index];
}
