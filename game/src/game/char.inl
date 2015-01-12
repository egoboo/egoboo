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

/// @file game/char.inl
/// @note You will routinely include "char.inl" in all *.inl files or *.c/*.cpp files, instead of "char.h"

#pragma once

#include "game/char.h"
/// @note Include "game/profile.inl" here.
///       Do not include "char.inl" in "profile.inl", otherwise there is a bootstrapping problem.
#include "game/profile.inl"
#include "game/enchant.inl"
#include "game/particle.inl"

//--------------------------------------------------------------------------------------------
// FORWARD DECLARARIONS
//--------------------------------------------------------------------------------------------
// cap_t accessor functions
static INLINE bool cap_is_type_idsz( const CAP_REF icap, IDSZ test_idsz );
static INLINE bool cap_has_idsz( const CAP_REF icap, IDSZ idsz );

//--------------------------------------------------------------------------------------------
// team_t accessor functions
static INLINE CHR_REF team_get_ileader( const TEAM_REF iteam );
static INLINE chr_t  *team_get_pleader( const TEAM_REF iteam );

static INLINE bool team_hates_team( const TEAM_REF ipredator_team, const TEAM_REF iprey_team );

//--------------------------------------------------------------------------------------------
// chr_t accessor functions
static INLINE PRO_REF  chr_get_ipro( const CHR_REF ichr );
static INLINE CAP_REF  chr_get_icap( const CHR_REF ichr );
static INLINE EVE_REF  chr_get_ieve( const CHR_REF ichr );
static INLINE PIP_REF  chr_get_ipip( const CHR_REF ichr, int ipip );
static INLINE TEAM_REF chr_get_iteam( const CHR_REF ichr );
static INLINE TEAM_REF chr_get_iteam_base( const CHR_REF ichr );

static INLINE pro_t *chr_get_ppro( const CHR_REF ichr );
static INLINE cap_t *chr_get_pcap( const CHR_REF ichr );
static INLINE eve_t *chr_get_peve( const CHR_REF ichr );
static INLINE pip_t *chr_get_ppip( const CHR_REF ichr, int ipip );

static INLINE Mix_Chunk      *chr_get_chunk_ptr( chr_t * pchr, int index );
static INLINE Mix_Chunk      *chr_get_chunk( const CHR_REF ichr, int index );
static INLINE team_t         *chr_get_pteam( const CHR_REF ichr );
static INLINE team_t         *chr_get_pteam_base( const CHR_REF ichr );
static INLINE ai_state_t     *chr_get_pai( const CHR_REF ichr );
static INLINE chr_instance_t *chr_get_pinstance( const CHR_REF ichr );

static INLINE IDSZ chr_get_idsz( const CHR_REF ichr, int type );

static INLINE void chr_update_size( chr_t * pchr );
static INLINE void chr_init_size( chr_t * pchr, cap_t * pcap );


static INLINE bool chr_has_idsz( const CHR_REF ichr, IDSZ idsz );
static INLINE bool chr_is_type_idsz( const CHR_REF ichr, IDSZ idsz );
static INLINE bool chr_has_vulnie( const CHR_REF item, const PRO_REF weapon_profile );

static INLINE const float *chr_get_pos_v_const( const chr_t * pchr );
static INLINE float       *chr_get_pos_v( chr_t * pchr );
static INLINE bool         chr_get_pos( const chr_t * pchr, fvec3_base_t pos );

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------------------------------
static INLINE bool cap_is_type_idsz( const CAP_REF icap, IDSZ test_idsz )
{
    /// @author BB
    /// @details check IDSZ_PARENT and IDSZ_TYPE to see if the test_idsz matches. If we are not
    ///     picky (i.e. IDSZ_NONE == test_idsz), then it matches any valid item.

    cap_t * pcap;

    if ( !LOADED_CAP( icap ) ) return false;
    pcap = CapStack_get_ptr( icap );

    if ( IDSZ_NONE == test_idsz ) return true;
    if ( test_idsz == pcap->idsz[IDSZ_TYPE  ] ) return true;
    if ( test_idsz == pcap->idsz[IDSZ_PARENT] ) return true;

    return false;
}

//--------------------------------------------------------------------------------------------
static INLINE bool cap_has_idsz( const CAP_REF icap, IDSZ idsz )
{
    /// @author BB
    /// @details does idsz match any of the stored values in pcap->idsz[]?
    ///               Matches anything if not picky (idsz == IDSZ_NONE)

    int     cnt;
    cap_t * pcap;
    bool  retval;

    if ( !LOADED_CAP( icap ) ) return false;
    pcap = CapStack_get_ptr( icap );

    if ( IDSZ_NONE == idsz ) return true;

    retval = false;
    for ( cnt = 0; cnt < IDSZ_COUNT; cnt++ )
    {
        if ( pcap->idsz[cnt] == idsz )
        {
            retval = true;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE CHR_REF team_get_ileader( const TEAM_REF iteam )
{
    CHR_REF ichr;

    if ( iteam >= TEAM_MAX ) return INVALID_CHR_REF;

    ichr = TeamStack.lst[iteam].leader;
    if ( !DEFINED_CHR( ichr ) ) return INVALID_CHR_REF;

    return ichr;
}

//--------------------------------------------------------------------------------------------
static INLINE chr_t  * team_get_pleader( const TEAM_REF iteam )
{
    CHR_REF ichr;

    if ( iteam >= TEAM_MAX ) return NULL;

    ichr = TeamStack.lst[iteam].leader;
    if ( !DEFINED_CHR( ichr ) ) return NULL;

    return ChrList_get_ptr( ichr );
}

//--------------------------------------------------------------------------------------------
static INLINE bool team_hates_team( const TEAM_REF ipredator_team, const TEAM_REF iprey_team )
{
    /// @author BB
    /// @details a wrapper function for access to the hatesteam data

    if ( ipredator_team >= TEAM_MAX || iprey_team >= TEAM_MAX ) return false;

    return TeamStack.lst[ipredator_team].hatesteam[ REF_TO_INT( iprey_team )];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE PRO_REF chr_get_ipro( const CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return INVALID_PRO_REF;
    pchr = ChrList_get_ptr( ichr );

    if ( !LOADED_PRO( pchr->profile_ref ) ) return INVALID_PRO_REF;

    return pchr->profile_ref;
}

//--------------------------------------------------------------------------------------------
static INLINE CAP_REF chr_get_icap( const CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return INVALID_CAP_REF;
    pchr = ChrList_get_ptr( ichr );

    return pro_get_icap( pchr->profile_ref );
}

//--------------------------------------------------------------------------------------------
static INLINE EVE_REF chr_get_ieve( const CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return INVALID_EVE_REF;
    pchr = ChrList_get_ptr( ichr );

    return pro_get_ieve( pchr->profile_ref );
}

//--------------------------------------------------------------------------------------------
static INLINE PIP_REF chr_get_ipip( const CHR_REF ichr, int ipip )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return INVALID_PIP_REF;
    pchr = ChrList_get_ptr( ichr );

    return pro_get_ipip( pchr->profile_ref, ipip );
}

//--------------------------------------------------------------------------------------------
static INLINE TEAM_REF chr_get_iteam( const CHR_REF ichr )
{
    chr_t * pchr;
    int iteam;

    if ( !DEFINED_CHR( ichr ) ) return ( TEAM_REF )TEAM_DAMAGE;
    pchr = ChrList_get_ptr( ichr );

    iteam = REF_TO_INT( pchr->team );
    iteam = CLIP( iteam, 0, (int)TEAM_MAX );

    return ( TEAM_REF )iteam;
}

//--------------------------------------------------------------------------------------------
static INLINE TEAM_REF chr_get_iteam_base( const CHR_REF ichr )
{
    chr_t * pchr;
    int iteam;

    if ( !DEFINED_CHR( ichr ) ) return ( TEAM_REF )TEAM_MAX;
    pchr = ChrList_get_ptr( ichr );

    iteam = REF_TO_INT( pchr->team_base );
    iteam = CLIP( iteam, 0, (int)TEAM_MAX );

    return ( TEAM_REF )iteam;
}

//--------------------------------------------------------------------------------------------
static INLINE pro_t * chr_get_ppro( const CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList_get_ptr( ichr );

    if ( !LOADED_PRO( pchr->profile_ref ) ) return NULL;

    return ProList_get_ptr( pchr->profile_ref );
}

//--------------------------------------------------------------------------------------------
static INLINE cap_t * chr_get_pcap( const CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList_get_ptr( ichr );

    return pro_get_pcap( pchr->profile_ref );
}

//--------------------------------------------------------------------------------------------
static INLINE eve_t * chr_get_peve( const CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList_get_ptr( ichr );

    return pro_get_peve( pchr->profile_ref );
}

//--------------------------------------------------------------------------------------------
static INLINE pip_t * chr_get_ppip( const CHR_REF ichr, int ipip )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList_get_ptr( ichr );

    return pro_get_ppip( pchr->profile_ref, ipip );
}

//--------------------------------------------------------------------------------------------
static INLINE Mix_Chunk * chr_get_chunk( const CHR_REF ichr, int index )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList_get_ptr( ichr );

    return pro_get_chunk( pchr->profile_ref, index );
}

//--------------------------------------------------------------------------------------------
static INLINE Mix_Chunk * chr_get_chunk_ptr( chr_t * pchr, int index )
{
    if ( !DEFINED_PCHR( pchr ) ) return NULL;

    return pro_get_chunk( pchr->profile_ref, index );
}

//--------------------------------------------------------------------------------------------
static INLINE team_t * chr_get_pteam( const CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList_get_ptr( ichr );

    return TeamStack_get_ptr( pchr->team );
}

//--------------------------------------------------------------------------------------------
static INLINE team_t * chr_get_pteam_base( const CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList_get_ptr( ichr );

    return TeamStack_get_ptr( pchr->team_base );
}

//--------------------------------------------------------------------------------------------
static INLINE ai_state_t * chr_get_pai( const CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList_get_ptr( ichr );

    return &( pchr->ai );
}

//--------------------------------------------------------------------------------------------
static INLINE chr_instance_t * chr_get_pinstance( const CHR_REF ichr )
{
    chr_t * pchr;

    if ( !DEFINED_CHR( ichr ) ) return NULL;
    pchr = ChrList_get_ptr( ichr );

    return &( pchr->inst );
}

//--------------------------------------------------------------------------------------------
static INLINE IDSZ chr_get_idsz( const CHR_REF ichr, int type )
{
    cap_t * pcap;

    if ( type >= IDSZ_COUNT ) return IDSZ_NONE;

    pcap = chr_get_pcap( ichr );
    if ( NULL == pcap ) return IDSZ_NONE;

    return pcap->idsz[type];
}

//--------------------------------------------------------------------------------------------
static INLINE bool chr_has_idsz( const CHR_REF ichr, IDSZ idsz )
{
    /// @author BB
    /// @details a wrapper for cap_has_idsz

    CAP_REF icap = chr_get_icap( ichr );

    return cap_has_idsz( icap, idsz );
}

//--------------------------------------------------------------------------------------------
static INLINE bool chr_is_type_idsz( const CHR_REF item, IDSZ test_idsz )
{
    /// @author BB
    /// @details check IDSZ_PARENT and IDSZ_TYPE to see if the test_idsz matches. If we are not
    ///     picky (i.e. IDSZ_NONE == test_idsz), then it matches any valid item.

    CAP_REF icap;

    icap = chr_get_icap( item );

    return cap_is_type_idsz( icap, test_idsz );
}

//--------------------------------------------------------------------------------------------
static INLINE bool chr_has_vulnie( const CHR_REF item, const PRO_REF test_profile )
{
    /// @author BB
    /// @details is item vulnerable to the type in profile test_profile?

    IDSZ vulnie;

    if ( !INGAME_CHR( item ) ) return false;
    vulnie = chr_get_idsz( item, IDSZ_VULNERABILITY );

    // not vulnerable if there is no specific weakness
    if ( IDSZ_NONE == vulnie ) return false;

    // check vs. every IDSZ that could have something to do with attacking
    if ( vulnie == pro_get_idsz( test_profile, IDSZ_TYPE ) ) return true;
    if ( vulnie == pro_get_idsz( test_profile, IDSZ_PARENT ) ) return true;

    return false;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE void chr_update_size( chr_t * pchr )
{
    /// @author BB
    /// @details Convert the base size values to the size values that are used in the game

    if ( !ALLOCATED_PCHR( pchr ) ) return;

    pchr->shadow_size   = pchr->shadow_size_save   * pchr->fat;
    pchr->bump.size     = pchr->bump_save.size     * pchr->fat;
    pchr->bump.size_big = pchr->bump_save.size_big * pchr->fat;
    pchr->bump.height   = pchr->bump_save.height   * pchr->fat;

    chr_update_collision_size( pchr, true );
}

//--------------------------------------------------------------------------------------------
static INLINE void chr_init_size( chr_t * pchr, cap_t * pcap )
{
    /// @author BB
    /// @details initalize the character size info

    if ( !ALLOCATED_PCHR( pchr ) || !LOADED_PCAP( pcap ) ) return;

    pchr->fat_stt           = pcap->size;
    pchr->shadow_size_stt   = pcap->shadow_size;
    pchr->bump_stt.size     = pcap->bump_size;
    pchr->bump_stt.size_big = pcap->bump_sizebig;
    pchr->bump_stt.height   = pcap->bump_height;

    pchr->fat                = pchr->fat_stt;
    pchr->shadow_size_save   = pchr->shadow_size_stt;
    pchr->bump_save.size     = pchr->bump_stt.size;
    pchr->bump_save.size_big = pchr->bump_stt.size_big;
    pchr->bump_save.height   = pchr->bump_stt.height;

    chr_update_size( pchr );
}

//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
static INLINE const float * chr_get_pos_v_const( const chr_t * pchr )
{
    static fvec3_t vtmp = fvec3_t::zero;

    if ( !ALLOCATED_PCHR( pchr ) ) return vtmp.v;

    return pchr->pos.v;
}

//--------------------------------------------------------------------------------------------
static INLINE float * chr_get_pos_v( chr_t * pchr )
{
    static fvec3_t vtmp = fvec3_t::zero;

    if ( !ALLOCATED_PCHR( pchr ) ) return vtmp.v;

    return pchr->pos.v;
}

//--------------------------------------------------------------------------------------------
static INLINE bool chr_get_pos( const chr_t * pchr, fvec3_base_t pos )
{
    float * copy_retval;

    if ( !ALLOCATED_PCHR( pchr ) ) return false;

    copy_retval = fvec3_base_copy( pos, pchr->pos.v );

    return NULL != copy_retval;
}
