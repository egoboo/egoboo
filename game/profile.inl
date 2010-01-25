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
#include "mad.h"
#include "enchant.h"

//--------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//--------------------------------------------------------------------------------------------

INLINE CAP_REF pro_get_icap( PRO_REF iobj );
INLINE MAD_REF pro_get_imad( PRO_REF iobj );
INLINE EVE_REF pro_get_ieve( PRO_REF iobj );
INLINE PIP_REF pro_get_ipip( PRO_REF iobj, int ipip );
INLINE IDSZ    pro_get_idsz( PRO_REF iobj, int type );

INLINE cap_t *     pro_get_pcap( PRO_REF iobj );
INLINE mad_t *     pro_get_pmad( PRO_REF iobj );
INLINE eve_t *     pro_get_peve( PRO_REF iobj );
INLINE pip_t *     pro_get_ppip( PRO_REF iobj, int ipip );
INLINE Mix_Chunk * pro_get_chunk( PRO_REF iobj, int index );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE CAP_REF pro_get_icap( PRO_REF iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return MAX_CAP;
    pobj = ProList.lst + iobj;

    return LOADED_CAP( pobj->icap ) ? pobj->icap : MAX_CAP;
}

//--------------------------------------------------------------------------------------------
INLINE MAD_REF pro_get_imad( PRO_REF iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return MAX_MAD;
    pobj = ProList.lst + iobj;

    return LOADED_MAD( pobj->imad ) ? pobj->imad : MAX_MAD;
}

//--------------------------------------------------------------------------------------------
INLINE EVE_REF pro_get_ieve( PRO_REF iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return MAX_EVE;
    pobj = ProList.lst + iobj;

    return LOADED_EVE( pobj->ieve ) ? pobj->ieve : MAX_EVE;
}

//--------------------------------------------------------------------------------------------
INLINE PIP_REF pro_get_ipip( PRO_REF iobj, int ipip )
{
    pro_t * pobj;
    PIP_REF pip_found;

    if ( !LOADED_PRO( iobj ) ) return MAX_PIP;
    pobj = ProList.lst + iobj;

    // find the local pip if it exists
    pip_found = MAX_PIP;
    if ( ipip >= 0 && ipip < MAX_PIP_PER_PROFILE )
    {
        pip_found = pobj->prtpip[ipip];
    }

    return LOADED_PIP( pip_found ) ? pip_found : MAX_PIP;
}

//--------------------------------------------------------------------------------------------
INLINE IDSZ pro_get_idsz( PRO_REF iobj, int type )
{
    cap_t * pcap;

    if ( type >= IDSZ_COUNT ) return IDSZ_NONE;

    pcap = pro_get_pcap( iobj );
    if ( NULL == pcap ) return IDSZ_NONE;

    return pcap->idsz[type];
}

//--------------------------------------------------------------------------------------------
INLINE cap_t * pro_get_pcap( PRO_REF iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return NULL;
    pobj = ProList.lst + iobj;

    if ( !LOADED_CAP( pobj->icap ) ) return NULL;

    return CapList + pobj->icap;
}

//--------------------------------------------------------------------------------------------
INLINE mad_t * pro_get_pmad( PRO_REF iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return NULL;
    pobj = ProList.lst + iobj;

    if ( !LOADED_MAD( pobj->imad ) ) return NULL;

    return MadList + pobj->imad;
}

//--------------------------------------------------------------------------------------------
INLINE eve_t * pro_get_peve( PRO_REF iobj )
{
    pro_t * pobj;

    if ( !LOADED_PRO( iobj ) ) return NULL;
    pobj = ProList.lst + iobj;

    if ( !LOADED_EVE( pobj->ieve ) ) return NULL;

    return EveStack.lst + pobj->ieve;
}

//--------------------------------------------------------------------------------------------
INLINE pip_t * pro_get_ppip( PRO_REF iobj, int ipip )
{
    pro_t * pobj;
    PIP_REF found_pip;

    if ( !LOADED_PRO( iobj ) ) return NULL;
    pobj = ProList.lst + iobj;

    // find the local pip if it exists
    found_pip = MAX_PIP;
    if ( ipip < MAX_PIP_PER_PROFILE )
    {
        found_pip = pobj->prtpip[ipip];
    }

    if ( !LOADED_PIP( found_pip ) ) return NULL;

    return PipStack.lst + found_pip;
}

//--------------------------------------------------------------------------------------------
INLINE Mix_Chunk * pro_get_chunk( PRO_REF iobj, int index )
{
    pro_t * pobj;

    if ( !VALID_SND( index ) ) return NULL;

    if ( !LOADED_PRO( iobj ) ) return NULL;
    pobj = ProList.lst + iobj;

    return pobj->wavelist[index];
}
