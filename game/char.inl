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

/// @file char.inl
/// @note You will routinely include "char.inl" in all *.inl files or *.c/*.cpp files, instead of "char.h"

#include "char.h"

#include "enchant.inl"
#include "particle.inl"

/// @note include "profile.inl" here.
///  Do not include "char.inl" in "profile.inl", otherwise there is a bootstrapping problem.
#include "profile.inl"

#include "egoboo_math.inl"

//---------------------------------------------------------------------------------------------
// FORWARD DECLARARIONS
//---------------------------------------------------------------------------------------------
// cap_t accessor functions
INLINE bool_t cap_is_type_idsz( CAP_REF icap, IDSZ test_idsz );
INLINE bool_t cap_has_idsz( CAP_REF icap, IDSZ idsz );
//---------------------------------------------------------------------------------------------
// team_t accessor functions
INLINE CHR_REF  team_get_ileader( TEAM_REF iteam );
INLINE chr_t  * team_get_pleader( TEAM_REF iteam );

INLINE bool_t team_hates_team( TEAM_REF ipredator_team, TEAM_REF iprey_team );

//---------------------------------------------------------------------------------------------
// chr_t accessor functions
INLINE PRO_REF  chr_get_ipro( CHR_REF ichr );
INLINE CAP_REF  chr_get_icap( CHR_REF ichr );
INLINE EVE_REF  chr_get_ieve( CHR_REF ichr );
INLINE PIP_REF  chr_get_ipip( CHR_REF ichr, int ipip );
INLINE TEAM_REF chr_get_iteam( CHR_REF ichr );
INLINE TEAM_REF chr_get_iteam_base( CHR_REF ichr );

INLINE pro_t * chr_get_ppro( CHR_REF ichr );
INLINE cap_t * chr_get_pcap( CHR_REF ichr );
INLINE eve_t * chr_get_peve( CHR_REF ichr );
INLINE pip_t * chr_get_ppip( CHR_REF ichr, int ipip );

INLINE Mix_Chunk      * chr_get_chunk_ptr( chr_t * pchr, int index );
INLINE Mix_Chunk      * chr_get_chunk( CHR_REF ichr, int index );
INLINE team_t         * chr_get_pteam( CHR_REF ichr );
INLINE team_t         * chr_get_pteam_base( CHR_REF ichr );
INLINE ai_state_t     * chr_get_pai( CHR_REF ichr );
INLINE chr_instance_t * chr_get_pinstance( CHR_REF ichr );

INLINE IDSZ chr_get_idsz( CHR_REF ichr, int type );

INLINE void chr_update_size( chr_t * pchr );
INLINE void chr_init_size( chr_t * pchr, cap_t * pcap );
INLINE void chr_set_size( chr_t * pchr, float size );
INLINE void chr_set_width( chr_t * pchr, float width );
INLINE void chr_set_shadow( chr_t * pchr, float width );
INLINE void chr_set_height( chr_t * pchr, float height );
INLINE void chr_set_fat( chr_t * pchr, float fat );

INLINE bool_t chr_has_idsz( CHR_REF ichr, IDSZ idsz );
INLINE bool_t chr_is_type_idsz( CHR_REF ichr, IDSZ idsz );
INLINE bool_t chr_has_vulnie( CHR_REF item, PRO_REF weapon_profile );

INLINE bool_t chr_getMatUp( chr_t *pchr, fvec3_t   * pvec );
INLINE bool_t chr_getMatRight( chr_t *pchr, fvec3_t   * pvec );
INLINE bool_t chr_getMatForward( chr_t *pchr, fvec3_t   * pvec );
INLINE bool_t chr_getMatTranslate( chr_t *pchr, fvec3_t   * pvec );

//---------------------------------------------------------------------------------------------
// IMPLEMENTATION
//---------------------------------------------------------------------------------------------
INLINE bool_t cap_is_type_idsz( CAP_REF icap, IDSZ test_idsz )
{
    /// @details BB@> check IDSZ_PARENT and IDSZ_TYPE to see if the test_idsz matches. If we are not
    ///     picky (i.e. IDSZ_NONE == test_idsz), then it matches any valid item.

    cap_t * pcap;

    if ( !LOADED_CAP( icap ) ) return bfalse;
    pcap = CapList + icap;

    if ( IDSZ_NONE == test_idsz ) return btrue;
    if ( test_idsz == pcap->idsz[IDSZ_TYPE  ] ) return btrue;
    if ( test_idsz == pcap->idsz[IDSZ_PARENT] ) return btrue;

    return bfalse;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t cap_has_idsz( CAP_REF icap, IDSZ idsz )
{
    /// @detalis BB@> does idsz match any of the stored values in pcap->idsz[]?
    ///               Matches anything if not picky (idsz == IDSZ_NONE)

    int     cnt;
    cap_t * pcap;
    bool_t  retval;

    if ( !LOADED_CAP( icap ) ) return bfalse;
    pcap = CapList + icap;

    if ( IDSZ_NONE == idsz ) return btrue;

    retval = bfalse;
    for ( cnt = 0; cnt < IDSZ_COUNT; cnt++ )
    {
        if ( pcap->idsz[cnt] == idsz )
        {
            retval = btrue;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE CHR_REF team_get_ileader( TEAM_REF iteam )
{
    int ichr;

    if ( iteam >= TEAM_MAX ) return MAX_CHR;

    ichr = TeamList[iteam].leader;
    if ( !DEFINED_CHR( ichr ) ) return MAX_CHR;

    return ichr;
}

//--------------------------------------------------------------------------------------------
INLINE chr_t  * team_get_pleader( TEAM_REF iteam )
{
    int ichr;

    if ( iteam >= TEAM_MAX ) return NULL;

    ichr = TeamList[iteam].leader;
    if ( !DEFINED_CHR( ichr ) ) return NULL;

    return ChrList.lst + ichr;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t team_hates_team( TEAM_REF ipredator_team, TEAM_REF iprey_team )
{
    /// @details BB@> a wrapper function for access to the hatesteam data

    if ( ipredator_team >= TEAM_MAX || iprey_team >= TEAM_MAX ) return bfalse;

    return TeamList[ipredator_team].hatesteam[iprey_team];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE PRO_REF chr_get_ipro( CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return MAX_PROFILE;
    pchr = ChrList.lst + ichr;

    if ( !LOADED_PRO( pchr->iprofile ) ) return MAX_PROFILE;

    return pchr->iprofile;
}

//--------------------------------------------------------------------------------------------
INLINE CAP_REF chr_get_icap( CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return MAX_CAP;
    pchr = ChrList.lst + ichr;

    return pro_get_icap( pchr->iprofile );
}

//--------------------------------------------------------------------------------------------
INLINE EVE_REF chr_get_ieve( CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return MAX_EVE;
    pchr = ChrList.lst + ichr;

    return pro_get_ieve( pchr->iprofile );
}

//--------------------------------------------------------------------------------------------
INLINE PIP_REF chr_get_ipip( CHR_REF ichr, int ipip )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return MAX_PIP;
    pchr = ChrList.lst + ichr;

    return pro_get_ipip( pchr->iprofile, ipip );
}

//--------------------------------------------------------------------------------------------
INLINE TEAM_REF chr_get_iteam( CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return TEAM_DAMAGE;
    pchr = ChrList.lst + ichr;

    return CLIP( pchr->team, 0, TEAM_MAX );
}

//--------------------------------------------------------------------------------------------
INLINE TEAM_REF chr_get_iteam_base( CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return TEAM_MAX;
    pchr = ChrList.lst + ichr;

    return CLIP( pchr->baseteam, 0, TEAM_MAX );
}

//--------------------------------------------------------------------------------------------
INLINE pro_t * chr_get_ppro( CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.lst + ichr;

    if ( !LOADED_PRO( pchr->iprofile ) ) return NULL;

    return ProList.lst + pchr->iprofile;
}

//--------------------------------------------------------------------------------------------
INLINE cap_t * chr_get_pcap( CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.lst + ichr;

    return pro_get_pcap( pchr->iprofile );
}

//--------------------------------------------------------------------------------------------
INLINE eve_t * chr_get_peve( CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.lst + ichr;

    return pro_get_peve( pchr->iprofile );
}

//--------------------------------------------------------------------------------------------
INLINE pip_t * chr_get_ppip( CHR_REF ichr, int ipip )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.lst + ichr;

    return pro_get_ppip( pchr->iprofile, ipip );
}

//--------------------------------------------------------------------------------------------
INLINE Mix_Chunk * chr_get_chunk( CHR_REF ichr, int index )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.lst + ichr;

    return pro_get_chunk( pchr->iprofile, index );
}

//--------------------------------------------------------------------------------------------
INLINE Mix_Chunk * chr_get_chunk_ptr( chr_t * pchr, int index )
{
    if ( !DEFINED_PCHR( pchr ) ) return NULL;

    return pro_get_chunk( pchr->iprofile, index );
}

//--------------------------------------------------------------------------------------------
INLINE team_t * chr_get_pteam( CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.lst + ichr;

    if ( pchr->team < 0 && pchr->team >= TEAM_MAX ) return NULL;

    return TeamList + pchr->team;
}

//--------------------------------------------------------------------------------------------
INLINE team_t * chr_get_pteam_base( CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.lst + ichr;

    if ( pchr->baseteam < 0 || pchr->baseteam >= TEAM_MAX ) return NULL;

    return TeamList + pchr->baseteam;
}

//--------------------------------------------------------------------------------------------
INLINE ai_state_t * chr_get_pai( CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.lst + ichr;

    return &( pchr->ai );
}

//--------------------------------------------------------------------------------------------
INLINE chr_instance_t * chr_get_pinstance( CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList.lst + ichr;

    return &( pchr->inst );
}

//--------------------------------------------------------------------------------------------
INLINE IDSZ chr_get_idsz( CHR_REF ichr, int type )
{
    cap_t * pcap;

    if ( type >= IDSZ_COUNT ) return IDSZ_NONE;

    pcap = chr_get_pcap( ichr );
    if ( NULL == pcap ) return IDSZ_NONE;

    return pcap->idsz[type];
}

//--------------------------------------------------------------------------------------------
INLINE bool_t chr_has_idsz( CHR_REF ichr, IDSZ idsz )
{
    /// @detalis BB@> a wrapper for cap_has_idsz

    CAP_REF icap = chr_get_icap( ichr );

    return cap_has_idsz( icap, idsz );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t chr_has_vulnie( CHR_REF item, PRO_REF test_profile )
{
    /// @detalis BB@> is item vulnerable to the type in profile test_profile?

    IDSZ vulnie;

    if ( !ACTIVE_CHR( item ) ) return bfalse;
    vulnie = chr_get_idsz( item, IDSZ_VULNERABILITY );

    // not vulnerable if there is no specific weakness
    if ( IDSZ_NONE == vulnie ) return bfalse;

    // check vs. every IDSZ that could have something to do with attacking
    if ( vulnie == pro_get_idsz( test_profile, IDSZ_TYPE ) ) return btrue;
    if ( vulnie == pro_get_idsz( test_profile, IDSZ_PARENT ) ) return btrue;

    return bfalse;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t chr_getMatUp( chr_t *pchr, fvec3_t   *pvec )
{
    /// @details BB@> MAKE SURE the value it calculated relative to a valid matrix

    if ( !ALLOCATED_PCHR( pchr ) ) return bfalse;

    if ( NULL == pvec ) return bfalse;

    if ( !chr_matrix_valid( pchr ) )
    {
        chr_update_matrix( pchr, btrue );
    }

    if ( chr_matrix_valid( pchr ) )
    {
        ( *pvec ) = mat_getChrUp( pchr->inst.matrix );
    }
    else
    {
        ( *pvec ).x = ( *pvec ).y = 0.0f;
        ( *pvec ).z = 1.0f;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t chr_getMatRight( chr_t *pchr, fvec3_t   *pvec )
{
    /// @details BB@> MAKE SURE the value it calculated relative to a valid matrix

    if ( !ALLOCATED_PCHR( pchr ) ) return bfalse;

    if ( NULL == pvec ) return bfalse;

    if ( !chr_matrix_valid( pchr ) )
    {
        chr_update_matrix( pchr, btrue );
    }

    if ( chr_matrix_valid( pchr ) )
    {
        ( *pvec ) = mat_getChrRight( pchr->inst.matrix );
    }
    else
    {
        // assume default Right is +y
        ( *pvec ).y = 1.0f;
        ( *pvec ).x = ( *pvec ).z = 0.0f;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t chr_getMatForward( chr_t *pchr, fvec3_t   *pvec )
{
    /// @details BB@> MAKE SURE the value it calculated relative to a valid matrix

    if ( !ALLOCATED_PCHR( pchr ) ) return bfalse;

    if ( NULL == pvec ) return bfalse;

    if ( !chr_matrix_valid( pchr ) )
    {
        chr_update_matrix( pchr, btrue );
    }

    if ( chr_matrix_valid( pchr ) )
    {
        ( *pvec ) = mat_getChrForward( pchr->inst.matrix );
    }
    else
    {
        // assume default Forward is +x
        ( *pvec ).x = 1.0f;
        ( *pvec ).y = ( *pvec ).z = 0.0f;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t chr_getMatTranslate( chr_t *pchr, fvec3_t   *pvec )
{
    /// @details BB@> MAKE SURE the value it calculated relative to a valid matrix

    if ( !ALLOCATED_PCHR( pchr ) ) return bfalse;

    if ( NULL == pvec ) return bfalse;

    if ( !chr_matrix_valid( pchr ) )
    {
        chr_update_matrix( pchr, btrue );
    }

    if ( chr_matrix_valid( pchr ) )
    {
        ( *pvec ) = mat_getTranslate( pchr->inst.matrix );
    }
    else
    {
        ( *pvec ) = pchr->pos;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE void chr_set_size( chr_t * pchr, float size )
{
    /// @details BB@> scale the entire character so that the size matches the given value

    float ratio;

    if ( !DEFINED_PCHR( pchr ) ) return;

    ratio = size / pchr->bump.size;

    pchr->shadow_size_save  *= ratio;
    pchr->bump_save.size    *= ratio;
    pchr->bump_save.sizebig *= ratio;
    pchr->bump_save.height  *= ratio;

    chr_update_size( pchr );
}

//--------------------------------------------------------------------------------------------
INLINE void chr_set_width( chr_t * pchr, float width )
{
    /// @details BB@> update the base character "width". This also modifies the shadow size

    float ratio;

    if ( !DEFINED_PCHR( pchr ) ) return;

    ratio = width / pchr->bump.size;

    pchr->shadow_size_save    *= ratio;
    pchr->bump_save.size    *= ratio;
    pchr->bump_save.sizebig *= ratio;

    chr_update_size( pchr );
}

//--------------------------------------------------------------------------------------------
INLINE void chr_set_shadow( chr_t * pchr, float width )
{
    /// @details BB@> update the base shadow size

    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->shadow_size_save = width;

    chr_update_size( pchr );
}

//--------------------------------------------------------------------------------------------
INLINE void chr_set_fat( chr_t * pchr, float fat )
{
    /// @details BB@> update all the character size info by specifying the fat value

    if ( !DEFINED_PCHR( pchr ) ) return;

    pchr->fat = fat;

    chr_update_size( pchr );
}

//--------------------------------------------------------------------------------------------
INLINE void chr_set_height( chr_t * pchr, float height )
{
    /// @details BB@> update the base character height

    if ( !DEFINED_PCHR( pchr ) ) return;

    if ( height < 0 ) height = 0;

    pchr->bump_save.height = height;

    chr_update_size( pchr );
}