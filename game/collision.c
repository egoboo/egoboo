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

/// @file collision.c
/// @brief The code that handles collisions between in-game objects
/// @details

#include "collision.h"
#include "obj_BSP.h"

#include "log.h"
#include "hash.h"
#include "game.h"
#include "SDL_extensions.h"

#include "char.inl"
#include "particle.inl"
#include "enchant.inl"
#include "profile.inl"
#include "physics.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAKE_HASH(AA,BB)         CLIP_TO_08BITS( ((AA) * 0x0111 + 0x006E) + ((BB) * 0x0111 + 0x006E) )

#define CHR_MAX_COLLISIONS       (MAX_CHR*8 + MAX_PRT)
#define COLLISION_HASH_NODES     (CHR_MAX_COLLISIONS*2)
#define COLLISION_LIST_SIZE      256

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// data block used to communicate between the different "modules" governing the character-particle collision
struct s_chr_prt_collsion_data
{
    // object parameters
    CHR_REF ichr;
    chr_t * pchr;
    cap_t * pcap;

    PRT_REF iprt;
    prt_t * pprt;
    pip_t * ppip;

    //---- collision parameters

    // true collisions
    bool_t    int_min;
    float     depth_min;

    // hit-box collisions
    bool_t    int_max;
    float     depth_max;

    // platform interactions
    //bool_t    int_plat;
    //float     plat_lerp;

    bool_t    is_impact;
    bool_t    is_pressure;
    bool_t    is_collision;
    float     dot;
    fvec3_t   nrm;

    // collision modifications
    bool_t   mana_paid;
    int      max_damage, actual_damage;
    fvec3_t  vdiff, vdiff_para, vdiff_perp;
    float    block_factor;

    // collision reaction
    fvec3_t vimpulse;                      ///< the velocity impulse
    fvec3_t pimpulse;                      ///< the position impulse
    bool_t  terminate_particle;
    bool_t  prt_bumps_chr;
    bool_t  prt_damages_chr;
};

typedef struct s_chr_prt_collsion_data chr_prt_collsion_data_t;

//--------------------------------------------------------------------------------------------

/// one element of the data for partitioning character and particle positions
struct s_bumplist
{
    size_t   chrnum;                  // Number on the block
    CHR_REF  chr;                     // For character collisions

    size_t   prtnum;                  // Number on the block
    CHR_REF  prt;                     // For particle collisions
};
typedef struct s_bumplist bumplist_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool_t add_chr_chr_interaction( CHashList_t * pclst, const CHR_REF ichr_a, const CHR_REF ichr_b, CoNode_ary_t * pcn_lst, HashNode_ary_t * phn_lst );
static bool_t add_chr_prt_interaction( CHashList_t * pclst, const CHR_REF ichr_a, const PRT_REF iprt_b, CoNode_ary_t * pcn_lst, HashNode_ary_t * phn_lst );

static bool_t detect_chr_chr_interaction_valid( const CHR_REF ichr_a, const CHR_REF ichr_b );
static bool_t detect_chr_prt_interaction_valid( const CHR_REF ichr_a, const PRT_REF iprt_b );

//static bool_t detect_chr_chr_interaction( const CHR_REF ichr_a, const CHR_REF ichr_b );
//static bool_t detect_chr_prt_interaction( const CHR_REF ichr_a, const PRT_REF iprt_b );

static bool_t do_chr_platform_detection( const CHR_REF ichr_a, const CHR_REF ichr_b );
static bool_t do_prt_platform_detection( const CHR_REF ichr_a, const PRT_REF iprt_b );

static bool_t fill_interaction_list( CHashList_t * pclst, CoNode_ary_t * pcn_lst, HashNode_ary_t * phn_lst );
static bool_t fill_bumplists();

static bool_t bump_all_platforms( CoNode_ary_t * pcn_ary );
static bool_t bump_all_mounts( CoNode_ary_t * pcn_ary );
static bool_t bump_all_collisions( CoNode_ary_t * pcn_ary );

static bool_t bump_one_mount( const CHR_REF ichr_a, const CHR_REF ichr_b );
static bool_t do_chr_platform_physics( chr_t * pitem, chr_t * pplat );
static float  estimate_chr_prt_normal( chr_t * pchr, prt_t * pprt, fvec3_base_t nrm, fvec3_base_t vdiff );
static bool_t do_chr_chr_collision( CoNode_t * d );

static bool_t do_chr_prt_collision_init( CHR_REF ichr, PRT_REF iprt, chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_deflect( chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_recoil( chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_damage( chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_impulse( chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_bump( chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_handle_bump( chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision( CoNode_t * d );

IMPLEMENT_DYNAMIC_ARY( CoNode_ary,   CoNode_t );
IMPLEMENT_DYNAMIC_ARY( HashNode_ary, hash_node_t );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//static bumplist_t bumplist[MAXMESHFAN/16];

static CHashList_t   * _CHashList_ptr = NULL;
static HashNode_ary_t  _hn_ary;                 ///< the available hash_node_t collision nodes for the CHashList_t
static CoNode_ary_t    _co_ary;                 ///< the available CoNode_t    data pointed to by the hash_node_t nodes
static BSP_leaf_pary_t _coll_leaf_lst;
static CoNode_ary_t    _coll_node_lst;

static bool_t _collision_hash_initialized = bfalse;
static bool_t _collision_system_initialized = bfalse;

int CHashList_inserted = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t collision_system_begin()
{
    if ( !_collision_system_initialized )
    {
        if ( NULL == CoNode_ary_ctor( &_co_ary, CHR_MAX_COLLISIONS ) ) goto collision_system_begin_fail;

        if ( NULL == HashNode_ary_ctor( &_hn_ary, COLLISION_HASH_NODES ) ) goto collision_system_begin_fail;

        if ( NULL == BSP_leaf_pary_ctor( &_coll_leaf_lst, COLLISION_LIST_SIZE ) ) goto collision_system_begin_fail;

        if ( NULL == CoNode_ary_ctor( &_coll_node_lst, COLLISION_LIST_SIZE ) ) goto collision_system_begin_fail;

        _collision_system_initialized = btrue;
    }

    return btrue;

collision_system_begin_fail:

    CoNode_ary_dtor( &_co_ary );
    HashNode_ary_dtor( &_hn_ary );
    BSP_leaf_pary_dtor( &_coll_leaf_lst );
    CoNode_ary_dtor( &_coll_node_lst );

    _collision_system_initialized = bfalse;

    log_error( "Cannot initialize the collision system" );

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void collision_system_end()
{
    if ( _collision_hash_initialized )
    {
        hash_list_destroy( &_CHashList_ptr );
        _collision_hash_initialized = bfalse;
    }
    _CHashList_ptr = NULL;

    if ( _collision_system_initialized )
    {
        CoNode_ary_dtor( &_co_ary );
        HashNode_ary_dtor( &_hn_ary );
        BSP_leaf_pary_dtor( &_coll_leaf_lst );
        CoNode_ary_dtor( &_coll_node_lst );

        _collision_system_initialized = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CoNode_t * CoNode_ctor( CoNode_t * n )
{
    if ( NULL == n ) return n;

    // clear all data
    memset( n, 0, sizeof( *n ) );

    // the "colliding" objects
    n->chra = ( CHR_REF )MAX_CHR;
    n->prta = MAX_PRT;

    // the "collided with" objects
    n->chrb  = ( CHR_REF )MAX_CHR;
    n->prtb  = MAX_PRT;
    n->tileb = FANOFF;

    // intialize the time
    n->tmin = n->tmax = -1.0f;

    return n;
}

//--------------------------------------------------------------------------------------------
Uint8 CoNode_generate_hash( CoNode_t * coll )
{
    REF_T AA, BB;

    AA = ( Uint32 )( ~0 );
    if ( VALID_CHR_RANGE( coll->chra ) )
    {
        AA = REF_TO_INT( coll->chra );
    }
    else if ( VALID_PRT_RANGE( coll->prta ) )
    {
        AA = REF_TO_INT( coll->prta );
    }

    BB = ( Uint32 )( ~0 );
    if ( VALID_CHR_RANGE( coll->chrb ) )
    {
        BB = REF_TO_INT( coll->chrb );
    }
    else if ( VALID_PRT_RANGE( coll->prtb ) )
    {
        BB = REF_TO_INT( coll->prtb );
    }
    else if ( FANOFF != coll->tileb )
    {
        BB = coll->tileb;
    }

    return MAKE_HASH( AA, BB );
}

//--------------------------------------------------------------------------------------------
int CoNode_cmp( const void * vleft, const void * vright )
{
    int   itmp;
    float ftmp;

    CoNode_t * pleft  = ( CoNode_t * )vleft;
    CoNode_t * pright = ( CoNode_t * )vright;

    // sort by initial time first
    ftmp = pleft->tmin - pright->tmin;
    if ( ftmp <= 0.0f ) return -1;
    else if ( ftmp >= 0.0f ) return 1;

    // sort by final time second
    ftmp = pleft->tmax - pright->tmax;
    if ( ftmp <= 0.0f ) return -1;
    else if ( ftmp >= 0.0f ) return 1;

    itmp = ( signed )REF_TO_INT( pleft->chra ) - ( signed )REF_TO_INT( pright->chra );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )REF_TO_INT( pleft->prta ) - ( signed )REF_TO_INT( pright->prta );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )REF_TO_INT( pleft->chrb ) - ( signed )REF_TO_INT( pright->chrb );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )REF_TO_INT( pleft->prtb ) - ( signed )REF_TO_INT( pright->prtb );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )pleft->tileb - ( signed )pright->tileb;
    if ( 0 != itmp ) return itmp;

    return 0;
}

//--------------------------------------------------------------------------------------------
int CoNode_cmp_unique( const void * vleft, const void * vright )
{
    int   itmp;

    CoNode_t * pleft  = ( CoNode_t * )vleft;
    CoNode_t * pright = ( CoNode_t * )vright;

    // don't compare the times

    itmp = ( signed )REF_TO_INT( pleft->chra ) - ( signed )REF_TO_INT( pright->chra );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )REF_TO_INT( pleft->prta ) - ( signed )REF_TO_INT( pright->prta );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )REF_TO_INT( pleft->chrb ) - ( signed )REF_TO_INT( pright->chrb );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )REF_TO_INT( pleft->prtb ) - ( signed )REF_TO_INT( pright->prtb );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )pleft->tileb - ( signed )pright->tileb;
    if ( 0 != itmp ) return itmp;

    return 0;
}

//--------------------------------------------------------------------------------------------
int CoNode_matches( CoNode_t * pleft, CoNode_t * pright )
{
    CoNode_t right_rev;

    if ( 0 == CoNode_cmp_unique( pleft, pright ) ) return btrue;

    // make a reversed version of pright
    right_rev.tmin = pright->tmin;
    right_rev.tmax = pright->tmax;
    right_rev.chra = pright->chrb;
    right_rev.prta = pright->prtb;
    right_rev.chrb = pright->chra;
    right_rev.prtb = pright->prta;
    right_rev.tileb  = pright->tileb;

    if ( 0 == CoNode_cmp_unique( pleft, &right_rev ) ) return btrue;

    return bfalse;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CHashList_t * CHashList_ctor( CHashList_t * pchlst, int size )
{
    return hash_list_ctor( pchlst, size );
}

//--------------------------------------------------------------------------------------------
CHashList_t * CHashList_dtor( CHashList_t * pchlst )
{
    return hash_list_dtor( pchlst );
}

//--------------------------------------------------------------------------------------------
CHashList_t * CHashList_get_Instance( int size )
{
    /// @details BB@> allows access to a "private" CHashList singleton object. This will automatically
    ///               initialze the _Colist_singleton and (almost) prevent anything from messing up
    ///               the initialization.

    // make sure that the collsion system was started
    collision_system_begin();

    // if the _CHashList_ptr doesn't exist, create it (and initialize it)
    if ( NULL == _CHashList_ptr )
    {
        _CHashList_ptr              = hash_list_create( size );
        _collision_hash_initialized = ( NULL != _CHashList_ptr );
    }

    // it the pointer exists, but it (somehow) not initialized, do the initialization
    if ( NULL != _CHashList_ptr && !_collision_hash_initialized )
    {
        _CHashList_ptr              = CHashList_ctor( _CHashList_ptr, size );
        _collision_hash_initialized = ( NULL != _CHashList_ptr );
    }

    return _collision_hash_initialized ? _CHashList_ptr : NULL;
}

//--------------------------------------------------------------------------------------------
bool_t CHashList_insert_unique( CHashList_t * pchlst, CoNode_t * pdata, CoNode_ary_t * free_cdata, HashNode_ary_t * free_hnodes )
{
    Uint32 hashval = 0;
    CoNode_t * d;

    hash_node_t * hn;
    bool_t found;
    size_t count;

    if ( NULL == pchlst || NULL == pdata ) return bfalse;

    // find the hash value for this interaction
    hashval = CoNode_generate_hash( pdata );

    found = bfalse;
    count = hash_list_get_count( pchlst, hashval );
    if ( count > 0 )
    {
        int k;

        // this hash already exists. check to see if the binary collision exists, too
        hn = hash_list_get_node( pchlst, hashval );
        for ( k = 0; k < count; k++ )
        {
            if ( !CoNode_matches(( CoNode_t * )( hn->data ), pdata ) )
            {
                found = btrue;
                break;
            }
        }
    }

    // insert this collision
    if ( !found )
    {
        size_t old_count;
        hash_node_t * old_head, * new_head, * hn;

        // pick a free collision data
        d = CoNode_ary_pop_back( free_cdata );

        // fill it in
        *d = *pdata;

        // generate a new hash node
        hn = HashNode_ary_pop_back( free_hnodes );

        // link the hash node to the free CoNode
        hn->data = d;

        // insert the node at the front of the collision list for this hash
        old_head = hash_list_get_node( pchlst, hashval );
        new_head = hash_node_insert_before( old_head, hn );
        hash_list_set_node( pchlst, hashval, new_head );

        // add 1 to the count at this hash
        old_count = hash_list_get_count( pchlst, hashval );
        hash_list_set_count( pchlst, hashval, old_count + 1 );
    }

    return !found;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t get_chr_mass( chr_t * pchr, float * wt )
{
    /// @details BB@> calculate a "mass" for an object, taking into account possible infinite masses.

    float loc_wta;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

    // handle oprtional parameters
    if ( NULL == wt ) wt = &loc_wta;

    if ( CHR_INFINITE_WEIGHT == pchr->phys.weight )
    {
        *wt = -( float )CHR_INFINITE_WEIGHT;
    }
    else if ( 0.0f == pchr->phys.bumpdampen )
    {
        *wt = -( float )CHR_INFINITE_WEIGHT;
    }
    else
    {
        *wt = pchr->phys.weight / pchr->phys.bumpdampen;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t get_prt_mass( prt_t * pprt, chr_t * pchr, float * wt )
{
    /// @details BB@> calculate a "mass" for each object, taking into account possible infinite masses.

    float loc_wprt;

    if ( NULL == pprt || NULL == pchr ) return bfalse;

    if ( NULL == wt ) wt = &loc_wprt;

    // determine an approximate mass for the particle
    if ( 0.0f == pprt->phys.bumpdampen )
    {
        *wt = -( float )CHR_INFINITE_WEIGHT;
    }
    else if ( DEFINED_CHR( pprt->attachedto_ref ) )
    {
        if ( CHR_INFINITE_WEIGHT == pprt->phys.weight )
        {
            *wt = -( float )CHR_INFINITE_WEIGHT;
        }
        else
        {
            *wt = pprt->phys.weight / pprt->phys.bumpdampen;
        }
    }
    else
    {
        float max_damage = ABS( pprt->damage.base ) + ABS( pprt->damage.rand );

        *wt = 1.0f;

        if ( 0 == max_damage )
        {
            // this is a particle like the wind particles in the whirlwind
            // make the particle have some kind of predictable constant effect
            // relative to any character;
            *wt = pchr->phys.weight / 10.0f;
        }
        else
        {
            // determine an "effective mass" for the particle, based on it's max damage
            // and velocity

            float prt_vel2;
            float prt_ke;

            fvec3_t vdiff = fvec3_sub( pprt->vel.v, pchr->vel.v );

            // the damage is basically like the kinetic energy of the particle
            prt_vel2 = fvec3_dot_product( vdiff.v, vdiff.v );

            // It can happen that a damage particle can hit something
            // at almost zero velocity, which would make for a huge "effective mass".
            // by making a reasonable "minimum velocity", we limit the maximum mass to
            // something reasonable
            prt_vel2 = MAX( 100.0f, prt_vel2 );

            // get the "kinetic energy" from the damage
            prt_ke = 3.0f * max_damage;

            // the faster the particle is going, the smaller the "mass" it
            // needs to do the damage
            *wt = prt_ke / ( 0.5f * prt_vel2 );
        }

        *wt /= pprt->phys.bumpdampen;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void get_recoil_factors( float wta, float wtb, float * recoil_a, float * recoil_b )
{
    float loc_recoil_a, loc_recoil_b;

    if ( NULL == recoil_a ) recoil_a = &loc_recoil_a;
    if ( NULL == recoil_b ) recoil_b = &loc_recoil_b;

    if ( wta >= ( float )CHR_INFINITE_WEIGHT ) wta = -( float )CHR_INFINITE_WEIGHT;
    if ( wtb >= ( float )CHR_INFINITE_WEIGHT ) wtb = -( float )CHR_INFINITE_WEIGHT;

    if ( wta < 0.0f && wtb < 0.0f )
    {
        *recoil_a = 0.5f;
        *recoil_b = 0.5f;
    }
    else if ( wta == wtb )
    {
        *recoil_a = 0.5f;
        *recoil_b = 0.5f;
    }
    else if ( wta < 0.0f || 0.0f == wtb )
    {
        *recoil_a = 0.0f;
        *recoil_b = 1.0f;
    }
    else if ( wtb < 0.0f || 0.0f == wta )
    {
        *recoil_a = 1.0f;
        *recoil_b = 0.0f;
    }
    else
    {
        *recoil_a = wtb / ( wta + wtb );
        *recoil_b = wta / ( wta + wtb );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t detect_chr_chr_interaction_valid( const CHR_REF ichr_a, const CHR_REF ichr_b )
{
    chr_t *pchr_a, *pchr_b;

    // Don't interact with self
    if ( ichr_a == ichr_b ) return bfalse;

    // Ignore invalid characters
    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    // Ignore invalid characters
    if ( !INGAME_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    // "non-interacting" objects interact with platforms
    if (( 0 == pchr_a->bump.size && !pchr_b->platform ) ||
        ( 0 == pchr_b->bump.size && !pchr_a->platform ) )
    {
        return bfalse;
    }

    // reject characters that are hidden
    if ( pchr_a->is_hidden || pchr_b->is_hidden ) return bfalse;

    // don't interact with your mount, or your held items
    if ( ichr_a == pchr_b->attachedto || ichr_b == pchr_a->attachedto ) return bfalse;

    // handle the dismount exception
    if ( pchr_a->dismount_timer > 0 && pchr_a->dismount_object == ichr_b ) return bfalse;
    if ( pchr_b->dismount_timer > 0 && pchr_b->dismount_object == ichr_a ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t detect_chr_prt_interaction_valid( const CHR_REF ichr_a, const PRT_REF iprt_b )
{
    chr_t * pchr_a;
    prt_t * pprt_b;

    // Ignore invalid characters
    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    // Ignore invalid characters
    if ( !INGAME_PRT( iprt_b ) ) return bfalse;
    pprt_b = PrtList.lst + iprt_b;

    // reject characters that are hidden
    if ( pchr_a->is_hidden || pprt_b->is_hidden ) return bfalse;

    // particles don't "collide" with anything they are attached to.
    // that only happes through doing bump particle damamge
    if ( ichr_a == pprt_b->attachedto_ref ) return bfalse;

    // don't interact if there is no interaction...
    // the particles and characters should not have been added to the list unless they
    // are valid for collision

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t add_chr_chr_interaction( CHashList_t * pchlst, const CHR_REF ichr_a, const CHR_REF ichr_b, CoNode_ary_t * pcn_lst, HashNode_ary_t * phn_lst )
{
    Uint32 hashval = 0;
    int count;
    bool_t found;

    hash_node_t * n;
    CoNode_t    * d;

    if ( NULL == pchlst || NULL == pcn_lst || NULL == phn_lst ) return bfalse;

    // there is no situation in the game where we allow characters to interact with themselves
    if ( ichr_a == ichr_b ) return bfalse;

    // create a hash that is order-independent
    hashval = MAKE_HASH( REF_TO_INT( ichr_a ), REF_TO_INT( ichr_b ) );

    found = bfalse;
    count = pchlst->subcount[hashval];
    if ( count > 0 )
    {
        int i;

        // this hash already exists. check to see if the binary collision exists, too
        n = pchlst->sublist[hashval];
        for ( i = 0; i < count; i++ )
        {
            d = ( CoNode_t * )( n->data );

            // make sure to test both orders
            if (( d->chra == ichr_a && d->chrb == ichr_b ) || ( d->chra == ichr_b && d->chrb == ichr_a ) )
            {
                found = btrue;
                break;
            }
        }
    }

    // insert this collision
    if ( !found )
    {
        // pick a free collision data
        EGOBOO_ASSERT( CoNode_ary_get_top( pcn_lst ) < CHR_MAX_COLLISIONS );
        d = CoNode_ary_pop_back( pcn_lst );

        // fill it in
        CoNode_ctor( d );
        d->chra = ichr_a;
        d->chrb = ichr_b;

        // generate a new hash node
        EGOBOO_ASSERT( HashNode_ary_get_top( phn_lst ) < COLLISION_HASH_NODES );
        n = HashNode_ary_pop_back( phn_lst );

        hash_node_ctor( n, ( void* )d );

        // insert the node
        pchlst->subcount[hashval]++;
        pchlst->sublist[hashval] = hash_node_insert_before( pchlst->sublist[hashval], n );
    }

    return !found;
}

//--------------------------------------------------------------------------------------------
bool_t add_chr_prt_interaction( CHashList_t * pchlst, const CHR_REF ichr_a, const PRT_REF iprt_b, CoNode_ary_t * pcn_lst, HashNode_ary_t * phn_lst )
{
    bool_t found;
    int    count;
    Uint32 hashval = 0;

    hash_node_t * n;
    CoNode_t    * d;

    if ( NULL == pchlst ) return bfalse;

    // create a hash that is order-independent
    hashval = MAKE_HASH( REF_TO_INT( ichr_a ), REF_TO_INT( iprt_b ) );

    found = bfalse;
    count = pchlst->subcount[hashval];
    if ( count > 0 )
    {
        int i ;

        // this hash already exists. check to see if the binary collision exists, too
        n = pchlst->sublist[hashval];
        for ( i = 0; i < count; i++ )
        {
            d = ( CoNode_t * )( n->data );
            if ( d->chra == ichr_a && d->prtb == iprt_b )
            {
                found = btrue;
                break;
            }
        }
    }

    // insert this collision
    if ( !found )
    {
        // pick a free collision data
        EGOBOO_ASSERT( CoNode_ary_get_top( pcn_lst ) < CHR_MAX_COLLISIONS );
        d = CoNode_ary_pop_back( pcn_lst );

        // fill it in
        CoNode_ctor( d );
        d->chra = ichr_a;
        d->prtb = iprt_b;

        // generate a new hash node
        EGOBOO_ASSERT( HashNode_ary_get_top( phn_lst ) < COLLISION_HASH_NODES );
        n = HashNode_ary_pop_back( phn_lst );

        hash_node_ctor( n, ( void* )d );

        // insert the node
        pchlst->subcount[hashval]++;
        pchlst->sublist[hashval] = hash_node_insert_before( pchlst->sublist[hashval], n );
    }

    return !found;
}

//--------------------------------------------------------------------------------------------
bool_t fill_interaction_list( CHashList_t * pchlst, CoNode_ary_t * cn_lst, HashNode_ary_t * hn_lst )
{
    int              cnt;
    int              reaffirmation_count;
    int              reaffirmation_list[DAMAGE_COUNT];
    ego_mpd_info_t * mi;
    BSP_aabb_t       tmp_aabb;

    if ( NULL == pchlst || NULL == cn_lst || NULL == hn_lst ) return bfalse;

    mi = &( PMesh->info );

    // allocate a BSP_aabb_t once, to be shared for all collision tests
    BSP_aabb_ctor( &tmp_aabb, 2 );

    // renew the CoNode_t hash table.
    hash_list_renew( pchlst );

    // initialize the reaffirmation counters
    reaffirmation_count = 0;
    for ( cnt = 0; cnt < DAMAGE_COUNT; cnt++ )
    {
        reaffirmation_list[cnt] = 0;
    }

    //---- find the character/particle interactions

    // Find the character-character interactions. Use the ChrList.used_ref, for a change
    CHashList_inserted = 0;
    CHR_BEGIN_LOOP_ACTIVE( ichr_a, pchr_a )
    {
        oct_bb_t   tmp_oct;

        // ignore in-accessible objects
        if ( pchr_a->pack.is_packed || pchr_a->is_hidden ) continue;

        // keep track of how many objects use reaffirmation, and what kinds of reaffirmation
        if ( pchr_a->reaffirm_damagetype < DAMAGE_COUNT )
        {
            cap_t * pcap = pro_get_pcap( pchr_a->profile_ref );
            if ( NULL != pcap && pcap->attachedprt_amount > 0 )
            {
                // we COULD use number_of_attached_particles() to determin if the
                // character is full of particles, BUT since it scans through the
                // entire particle list I don't think it's worth it

                reaffirmation_count++;
                reaffirmation_list[pchr_a->reaffirm_damagetype]++;
            }
        }

        // use the object velocity to figure out where the volume that the object will occupy during this
        // update
        phys_expand_chr_bb( pchr_a, 0.0f, 1.0f, &tmp_oct );

        // convert the oct_bb_t to a correct BSP_aabb_t
        BSP_aabb_from_oct_bb( &tmp_aabb, &tmp_oct );

        // find all collisions with other characters and particles
        _coll_leaf_lst.top = 0;
        obj_BSP_collide( &( chr_BSP_root ), &tmp_aabb, &_coll_leaf_lst );

        // transfer valid _coll_leaf_lst entries to pchlst entries
        // and sort them by their initial times
        if ( _coll_leaf_lst.top > 0 )
        {
            int j;

            for ( j = 0; j < _coll_leaf_lst.top; j++ )
            {
                BSP_leaf_t * pleaf;
                CoNode_t    tmp_codata;
                bool_t      do_insert;
                BIT_FIELD   test_platform;

                pleaf = _coll_leaf_lst.ary[j];
                if ( NULL == pleaf ) continue;

                do_insert = bfalse;

                if ( BSP_LEAF_CHR == pleaf->data_type )
                {
                    // collided with a character
                    CHR_REF ichr_b = ( CHR_REF )( pleaf->index );

                    // do some logic on this to determine whether the collision is valid
                    if ( detect_chr_chr_interaction_valid( ichr_a, ichr_b ) )
                    {
                        chr_t * pchr_b = ChrList.lst + ichr_b;

                        CoNode_ctor( &tmp_codata );

                        // do a simple test, since I do not want to resolve the cap_t for these objects here
                        test_platform = EMPTY_BIT_FIELD;
                        if ( pchr_a->platform && pchr_b->canuseplatforms ) SET_BIT( test_platform, PHYS_PLATFORM_OBJ1 );
                        if ( pchr_b->platform && pchr_a->canuseplatforms ) SET_BIT( test_platform, PHYS_PLATFORM_OBJ2 );

                        // detect a when the possible collision occurred
                        if ( phys_intersect_oct_bb( &( pchr_a->chr_min_cv ), chr_get_pos_v( pchr_a ), pchr_a->vel.v, &( pchr_b->chr_min_cv ), chr_get_pos_v( pchr_b ), pchr_b->vel.v, test_platform, &( tmp_codata.cv ), &( tmp_codata.tmin ), &( tmp_codata.tmax ) ) )
                        {
                            tmp_codata.chra = ichr_a;
                            tmp_codata.chrb = ichr_b;

                            do_insert = btrue;
                        }
                    }
                }
                else
                {
                    // how did we get here?
                    log_warning( "fill_interaction_list() - found non-character in the character BSP\n" );
                }

                if ( do_insert )
                {
                    if ( CHashList_insert_unique( pchlst, &tmp_codata, cn_lst, hn_lst ) )
                    {
                        CHashList_inserted++;
                    }
                }
            }
        }

        _coll_leaf_lst.top = 0;
        obj_BSP_collide( &( prt_BSP_root ), &tmp_aabb, &_coll_leaf_lst );
        if ( _coll_leaf_lst.top > 0 )
        {
            int j;

            for ( j = 0; j < _coll_leaf_lst.top; j++ )
            {
                BSP_leaf_t * pleaf;
                CoNode_t    tmp_codata;
                bool_t      do_insert;
                BIT_FIELD   test_platform;

                pleaf = _coll_leaf_lst.ary[j];
                if ( NULL == pleaf ) continue;

                do_insert = bfalse;

                if ( BSP_LEAF_PRT == pleaf->data_type )
                {
                    // collided with a particle
                    PRT_REF iprt_b = ( PRT_REF )( pleaf->index );

                    // do some logic on this to determine whether the collision is valid
                    if ( detect_chr_prt_interaction_valid( ichr_a, iprt_b ) )
                    {
                        prt_t * pprt_b = PrtList.lst + iprt_b;

                        CoNode_ctor( &tmp_codata );

                        // do a simple test, since I do not want to resolve the cap_t for these objects here
                        test_platform = pchr_a->platform ? PHYS_PLATFORM_OBJ1 : 0;

                        // detect a when the possible collision occurred
                        if ( phys_intersect_oct_bb( &( pchr_a->chr_max_cv ), chr_get_pos_v( pchr_a ), pchr_a->vel.v, &( pprt_b->prt_max_cv ), prt_get_pos_v( pprt_b ), pprt_b->vel.v, test_platform, &( tmp_codata.cv ), &( tmp_codata.tmin ), &( tmp_codata.tmax ) ) )
                        {
                            tmp_codata.chra = ichr_a;
                            tmp_codata.prtb = iprt_b;

                            do_insert = btrue;
                        }
                    }
                }
                else
                {
                    // how did we get here?
                    log_warning( "fill_interaction_list() - found non-particle in the particle BSP\n" );
                }

                if ( do_insert )
                {
                    if ( CHashList_insert_unique( pchlst, &tmp_codata, cn_lst, hn_lst ) )
                    {
                        CHashList_inserted++;
                    }
                }
            }
        }
    }
    CHR_END_LOOP();

    //---- find some specialized character-particle interactions
    //     namely particles that end-bump or particles that reaffirm characters

    PRT_BEGIN_LOOP_ACTIVE( iprt, bdl )
    {
        oct_bb_t   tmp_oct;
        bool_t     can_reaffirm, needs_bump;

        // if the particle is in the BSP, then it has already had it's chance to collide
        if ( bdl.prt_ptr->bsp_leaf.inserted ) continue;

        // does the particle potentially reaffirm a character?
        can_reaffirm = ( bdl.prt_ptr->damagetype < DAMAGE_COUNT ) && ( 0 != reaffirmation_list[bdl.prt_ptr->damagetype] );

        // does the particle end_bump or end_ground?
        needs_bump = bdl.pip_ptr->end_bump || bdl.pip_ptr->end_ground;

        if ( !can_reaffirm && !needs_bump ) continue;

        // use the object velocity to figure out where the volume that the object will occupy during this
        // update
        phys_expand_prt_bb( bdl.prt_ptr, 0.0f, 1.0f, &tmp_oct );

        // convert the oct_bb_t to a correct BSP_aabb_t
        BSP_aabb_from_oct_bb( &tmp_aabb, &tmp_oct );

        // find all collisions with characters
        _coll_leaf_lst.top = 0;
        obj_BSP_collide( &( chr_BSP_root ), &tmp_aabb, &_coll_leaf_lst );

        // transfer valid _coll_leaf_lst entries to pchlst entries
        // and sort them by their initial times
        if ( _coll_leaf_lst.top > 0 )
        {
            int          j;
            CoNode_t     tmp_codata;
            BIT_FIELD    test_platform;
            CHR_REF      ichr_a = MAX_CHR;
            BSP_leaf_t * pleaf = NULL;
            bool_t       do_insert = bfalse;

            for ( j = 0; j < _coll_leaf_lst.top; j++ )
            {
                pleaf = _coll_leaf_lst.ary[j];
                if ( NULL == pleaf ) continue;

                ichr_a = ( CHR_REF )( pleaf->index );

                do_insert = bfalse;

                if ( BSP_LEAF_CHR == pleaf->data_type && VALID_CHR_RANGE( ichr_a ) )
                {
                    // collided with a character
                    bool_t loc_reaffirms     = can_reaffirm;
                    bool_t loc_needs_bump    = needs_bump;
                    bool_t interaction_valid = bfalse;

                    chr_t * pchr_a = ChrList.lst + ichr_a;

                    // can this particle affect the character through reaffirmation
                    if ( loc_reaffirms )
                    {
                        // does this interaction support affirmation?
                        if ( bdl.prt_ptr->damagetype != pchr_a->reaffirm_damagetype )
                        {
                            loc_reaffirms = bfalse;
                        }

                        // if it is already attached to this character, no more reaffirmation
                        if ( bdl.prt_ptr->attachedto_ref == ichr_a )
                        {
                            loc_reaffirms = bfalse;
                        }
                    }

                    // you can't be bumped by items that you are attached to
                    if ( loc_needs_bump && bdl.prt_ptr->attachedto_ref == ichr_a )
                    {
                        loc_needs_bump = bfalse;
                    }

                    // can this character affect this particle through bumping?
                    if ( loc_needs_bump )
                    {
                        // the valid bump interactions
                        bool_t end_money  = ( bdl.pip_ptr->bump_money > 0 ) && pchr_a->cangrabmoney;
                        bool_t end_bump   = ( bdl.pip_ptr->end_bump ) && ( 0 != pchr_a->bump_stt.size );
                        bool_t end_ground = ( bdl.pip_ptr->end_ground ) && (( 0 != pchr_a->bump_stt.size ) || pchr_a->platform );

                        if ( !end_money && !end_bump && !end_ground )
                        {
                            loc_needs_bump = bfalse;
                        }
                    }

                    // do a little more logic on this to determine whether the collision is valid
                    interaction_valid = bfalse;
                    if ( loc_reaffirms || loc_needs_bump )
                    {
                        if ( detect_chr_prt_interaction_valid( ichr_a, bdl.prt_ref ) )
                        {
                            interaction_valid = btrue;
                        }
                        else
                        {
                            interaction_valid = bfalse;
                        }
                    }

                    // only do the more expensive calculation if the
                    // particle can interact with the object
                    if ( interaction_valid )
                    {
                        CoNode_ctor( &tmp_codata );

                        // do a simple test, since I do not want to resolve the cap_t for these objects here
                        test_platform = EMPTY_BIT_FIELD;
                        if ( pchr_a->platform && ( SPRITE_SOLID == bdl.prt_ptr->type ) ) SET_BIT( test_platform, PHYS_PLATFORM_OBJ1 );

                        // detect a when the possible collision occurred
                        if ( phys_intersect_oct_bb( &( pchr_a->chr_min_cv ), chr_get_pos_v( pchr_a ), pchr_a->vel.v, &( bdl.prt_ptr->prt_max_cv ), prt_get_pos_v( bdl.prt_ptr ), bdl.prt_ptr->vel.v, test_platform, &( tmp_codata.cv ), &( tmp_codata.tmin ), &( tmp_codata.tmax ) ) )
                        {

                            tmp_codata.chra = ichr_a;
                            tmp_codata.prtb = bdl.prt_ref;

                            do_insert = btrue;
                        }
                    }
                }
                else if ( BSP_LEAF_PRT == pleaf->data_type )
                {
                    // this should never happen
                }

                if ( do_insert )
                {
                    if ( CHashList_insert_unique( pchlst, &tmp_codata, cn_lst, hn_lst ) )
                    {
                        CHashList_inserted++;
                    }
                }
            }
        }
    }
    PRT_END_LOOP();

    // do this manually in C
    BSP_aabb_dtor( &tmp_aabb );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t fill_bumplists()
{
    /// @details BB@> Fill in the obj_BSP_t for this frame
    ///
    /// @note do not use obj_BSP_clear every frame, because the number of pre-allocated nodes can be quite large.
    /// Instead, just remove the nodes from the tree, fill the tree, and then prune any empty branches/leaves

    // empty out the BSP node lists
    chr_BSP_clear();
    prt_BSP_clear();

    // fill up the BSP list based on the current locations
    chr_BSP_fill();
    prt_BSP_fill();

    // remove empty branches from the tree
    if ( 63 == ( game_frame_all & 63 ) )
    {
        BSP_tree_prune( &( chr_BSP_root.tree ) );
        BSP_tree_prune( &( prt_BSP_root.tree ) );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_platform_detection( const CHR_REF ichr_a, const CHR_REF ichr_b )
{
    chr_t * pchr_a, * pchr_b;

    bool_t platform_a, platform_b;

    oct_vec_t odepth;
    bool_t collide_x  = bfalse;
    bool_t collide_y  = bfalse;
    bool_t collide_xy = bfalse;
    bool_t collide_yx = bfalse;
    bool_t collide_z  = bfalse;
    bool_t chara_on_top;

    // make sure that A is valid
    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    // make sure that B is valid
    if ( !INGAME_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    // if you are mounted, only your mount is affected by platforms
    if ( INGAME_CHR( pchr_a->attachedto ) || INGAME_CHR( pchr_b->attachedto ) ) return bfalse;

    // only check possible object-platform interactions
    platform_a = pchr_b->canuseplatforms && pchr_a->platform;
    platform_b = pchr_a->canuseplatforms && pchr_b->platform;
    if ( !platform_a && !platform_b ) return bfalse;

    //---- since we are doing bump_all_mounts() before bump_all_platforms()
    // mount detection is done before platform attachment, and these lines of code
    // aren't needed
    //
    //// If we can mount this platform, skip it
    //mount_a = chr_can_mount( ichr_b, ichr_a );
    //if ( mount_a && pchr_a->enviro.level < pchr_b->pos.z + pchr_b->bump.height + PLATTOLERANCE )
    //    return bfalse;
    //
    //// If we can mount this platform, skip it
    //mount_b = chr_can_mount( ichr_a, ichr_b );
    //if ( mount_b && pchr_b->enviro.level < pchr_a->pos.z + pchr_a->bump.height + PLATTOLERANCE )
    //    return bfalse;

    odepth[OCT_Z]  = MIN( pchr_b->chr_min_cv.maxs[OCT_Z] + pchr_b->pos.z, pchr_a->chr_min_cv.maxs[OCT_Z] + pchr_a->pos.z ) -
                     MAX( pchr_b->chr_min_cv.mins[OCT_Z] + pchr_b->pos.z, pchr_a->chr_min_cv.mins[OCT_Z] + pchr_a->pos.z );

    collide_z  = ( odepth[OCT_Z] > -PLATTOLERANCE && odepth[OCT_Z] < PLATTOLERANCE );

    if ( !collide_z ) return bfalse;

    // initialize the overlap depths
    odepth[OCT_X] = odepth[OCT_Y] = odepth[OCT_XY] = odepth[OCT_YX] = 0.0f;

    // determine how the characters can be attached
    chara_on_top = btrue;
    odepth[OCT_Z] = 2 * PLATTOLERANCE;
    if ( platform_a && platform_b )
    {
        float depth_a, depth_b;

        depth_a = ( pchr_b->pos.z + pchr_b->chr_min_cv.maxs[OCT_Z] ) - ( pchr_a->pos.z + pchr_a->chr_min_cv.mins[OCT_Z] );
        depth_b = ( pchr_a->pos.z + pchr_a->chr_min_cv.maxs[OCT_Z] ) - ( pchr_b->pos.z + pchr_b->chr_min_cv.mins[OCT_Z] );

        odepth[OCT_Z] = MIN( pchr_b->pos.z + pchr_b->chr_min_cv.maxs[OCT_Z], pchr_a->pos.z + pchr_a->chr_min_cv.maxs[OCT_Z] ) -
                        MAX( pchr_b->pos.z + pchr_b->chr_min_cv.mins[OCT_Z], pchr_a->pos.z + pchr_a->chr_min_cv.mins[OCT_Z] );

        chara_on_top = ABS( odepth[OCT_Z] - depth_a ) < ABS( odepth[OCT_Z] - depth_b );

        // the collision is determined by the platform size
        if ( chara_on_top )
        {
            // size of a doesn't matter
            odepth[OCT_X]  = MIN(( pchr_b->chr_min_cv.maxs[OCT_X] + pchr_b->pos.x ) - pchr_a->pos.x,
                                 pchr_a->pos.x - ( pchr_b->chr_min_cv.mins[OCT_X] + pchr_b->pos.x ) );

            odepth[OCT_Y]  = MIN(( pchr_b->chr_min_cv.maxs[OCT_Y] + pchr_b->pos.y ) -  pchr_a->pos.y,
                                 pchr_a->pos.y - ( pchr_b->chr_min_cv.mins[OCT_Y] + pchr_b->pos.y ) );

            odepth[OCT_XY] = MIN(( pchr_b->chr_min_cv.maxs[OCT_XY] + ( pchr_b->pos.x + pchr_b->pos.y ) ) - ( pchr_a->pos.x + pchr_a->pos.y ),
                                 ( pchr_a->pos.x + pchr_a->pos.y ) - ( pchr_b->chr_min_cv.mins[OCT_XY] + ( pchr_b->pos.x + pchr_b->pos.y ) ) );

            odepth[OCT_YX] = MIN(( pchr_b->chr_min_cv.maxs[OCT_YX] + ( -pchr_b->pos.x + pchr_b->pos.y ) ) - ( -pchr_a->pos.x + pchr_a->pos.y ),
                                 ( -pchr_a->pos.x + pchr_a->pos.y ) - ( pchr_b->chr_min_cv.mins[OCT_YX] + ( -pchr_b->pos.x + pchr_b->pos.y ) ) );
        }
        else
        {
            // size of b doesn't matter

            odepth[OCT_X]  = MIN(( pchr_a->chr_min_cv.maxs[OCT_X] + pchr_a->pos.x ) - pchr_b->pos.x,
                                 pchr_b->pos.x - ( pchr_a->chr_min_cv.mins[OCT_X] + pchr_a->pos.x ) );

            odepth[OCT_Y]  = MIN(( pchr_a->chr_min_cv.maxs[OCT_Y] + pchr_a->pos.y ) -  pchr_b->pos.y,
                                 pchr_b->pos.y - ( pchr_a->chr_min_cv.mins[OCT_Y] + pchr_a->pos.y ) );

            odepth[OCT_XY] = MIN(( pchr_a->chr_min_cv.maxs[OCT_XY] + ( pchr_a->pos.x + pchr_a->pos.y ) ) - ( pchr_b->pos.x + pchr_b->pos.y ),
                                 ( pchr_b->pos.x + pchr_b->pos.y ) - ( pchr_a->chr_min_cv.mins[OCT_XY] + ( pchr_a->pos.x + pchr_a->pos.y ) ) );

            odepth[OCT_YX] = MIN(( pchr_a->chr_min_cv.maxs[OCT_YX] + ( -pchr_a->pos.x + pchr_a->pos.y ) ) - ( -pchr_b->pos.x + pchr_b->pos.y ),
                                 ( -pchr_b->pos.x + pchr_b->pos.y ) - ( pchr_a->chr_min_cv.mins[OCT_YX] + ( -pchr_a->pos.x + pchr_a->pos.y ) ) );
        }
    }
    else if ( platform_a )
    {
        chara_on_top = bfalse;
        odepth[OCT_Z] = ( pchr_a->pos.z + pchr_a->chr_min_cv.maxs[OCT_Z] ) - ( pchr_b->pos.z + pchr_b->chr_min_cv.mins[OCT_Z] );

        // size of b doesn't matter

        odepth[OCT_X]  = MIN(( pchr_a->chr_min_cv.maxs[OCT_X] + pchr_a->pos.x ) - pchr_b->pos.x,
                             pchr_b->pos.x - ( pchr_a->chr_min_cv.mins[OCT_X] + pchr_a->pos.x ) );

        odepth[OCT_Y]  = MIN(( pchr_a->chr_min_cv.maxs[OCT_Y] + pchr_a->pos.y ) -  pchr_b->pos.y,
                             pchr_b->pos.y - ( pchr_a->chr_min_cv.mins[OCT_Y] + pchr_a->pos.y ) );

        odepth[OCT_XY] = MIN(( pchr_a->chr_min_cv.maxs[OCT_XY] + ( pchr_a->pos.x + pchr_a->pos.y ) ) - ( pchr_b->pos.x + pchr_b->pos.y ),
                             ( pchr_b->pos.x + pchr_b->pos.y ) - ( pchr_a->chr_min_cv.mins[OCT_XY] + ( pchr_a->pos.x + pchr_a->pos.y ) ) );

        odepth[OCT_YX] = MIN(( pchr_a->chr_min_cv.maxs[OCT_YX] + ( -pchr_a->pos.x + pchr_a->pos.y ) ) - ( -pchr_b->pos.x + pchr_b->pos.y ),
                             ( -pchr_b->pos.x + pchr_b->pos.y ) - ( pchr_a->chr_min_cv.mins[OCT_YX] + ( -pchr_a->pos.x + pchr_a->pos.y ) ) );
    }
    else if ( platform_b )
    {
        chara_on_top = btrue;
        odepth[OCT_Z] = ( pchr_b->pos.z + pchr_b->chr_min_cv.maxs[OCT_Z] ) - ( pchr_a->pos.z + pchr_a->chr_min_cv.mins[OCT_Z] );

        // size of a doesn't matter
        odepth[OCT_X]  = MIN(( pchr_b->chr_min_cv.maxs[OCT_X] + pchr_b->pos.x ) - pchr_a->pos.x,
                             pchr_a->pos.x - ( pchr_b->chr_min_cv.mins[OCT_X] + pchr_b->pos.x ) );

        odepth[OCT_Y]  = MIN( pchr_b->chr_min_cv.maxs[OCT_Y] + ( pchr_b->pos.y -  pchr_a->pos.y ),
                              ( pchr_a->pos.y - pchr_b->chr_min_cv.mins[OCT_Y] ) + pchr_b->pos.y );

        odepth[OCT_XY] = MIN(( pchr_b->chr_min_cv.maxs[OCT_XY] + ( pchr_b->pos.x + pchr_b->pos.y ) ) - ( pchr_a->pos.x + pchr_a->pos.y ),
                             ( pchr_a->pos.x + pchr_a->pos.y ) - ( pchr_b->chr_min_cv.mins[OCT_XY] + ( pchr_b->pos.x + pchr_b->pos.y ) ) );

        odepth[OCT_YX] = MIN(( pchr_b->chr_min_cv.maxs[OCT_YX] + ( -pchr_b->pos.x + pchr_b->pos.y ) ) - ( -pchr_a->pos.x + pchr_a->pos.y ),
                             ( -pchr_a->pos.x + pchr_a->pos.y ) - ( pchr_b->chr_min_cv.mins[OCT_YX] + ( -pchr_b->pos.x + pchr_b->pos.y ) ) );

    }

    collide_x  = odepth[OCT_X]  > 0.0f;
    collide_y  = odepth[OCT_Y]  > 0.0f;
    collide_xy = odepth[OCT_XY] > 0.0f;
    collide_yx = odepth[OCT_YX] > 0.0f;
    collide_z  = ( odepth[OCT_Z] > -PLATTOLERANCE && odepth[OCT_Z] < PLATTOLERANCE );

    if ( collide_x && collide_y && collide_xy && collide_yx && collide_z )
    {
        // check for the best possible attachment
        if ( chara_on_top )
        {
            if ( pchr_b->pos.z + pchr_b->chr_min_cv.maxs[OCT_Z] > pchr_a->targetplatform_level )
            {
                // set, but do not attach the platforms yet
                pchr_a->targetplatform_level = pchr_b->pos.z + pchr_b->chr_min_cv.maxs[OCT_Z];
                pchr_a->targetplatform_ref   = ichr_b;
            }
        }
        else
        {
            if ( pchr_a->pos.z + pchr_a->chr_min_cv.maxs[OCT_Z] > pchr_b->targetplatform_level )
            {
                // set, but do not attach the platforms yet
                pchr_b->targetplatform_level = pchr_a->pos.z + pchr_a->chr_min_cv.maxs[OCT_Z];
                pchr_b->targetplatform_ref   = ichr_a;
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_prt_platform_detection( const CHR_REF ichr_a, const PRT_REF iprt_b )
{
    chr_t * pchr_a;
    prt_t * pprt_b;

    bool_t platform_a;

    oct_vec_t odepth;
    bool_t collide_x  = bfalse;
    bool_t collide_y  = bfalse;
    bool_t collide_xy = bfalse;
    bool_t collide_yx = bfalse;
    bool_t collide_z  = bfalse;

    // make sure that A is valid
    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    // make sure that B is valid
    if ( !INGAME_PRT( iprt_b ) ) return bfalse;
    pprt_b = PrtList.lst + iprt_b;

    // if you are mounted, only your mount is affected by platforms
    if ( INGAME_CHR( pchr_a->attachedto ) || INGAME_CHR( pprt_b->attachedto_ref ) ) return bfalse;

    // only check possible object-platform interactions
    platform_a = /* pprt_b->canuseplatforms && */ pchr_a->platform;
    if ( !platform_a ) return bfalse;

    odepth[OCT_Z]  = MIN( pprt_b->prt_max_cv.maxs[OCT_Z] + pprt_b->pos.z, pchr_a->chr_min_cv.maxs[OCT_Z] + pchr_a->pos.z ) -
                     MAX( pprt_b->prt_max_cv.mins[OCT_Z] + pprt_b->pos.z, pchr_a->chr_min_cv.mins[OCT_Z] + pchr_a->pos.z );

    collide_z  = ( odepth[OCT_Z] > -PLATTOLERANCE && odepth[OCT_Z] < PLATTOLERANCE );

    if ( !collide_z ) return bfalse;

    // initialize the overlap depths
    odepth[OCT_X] = odepth[OCT_Y] = odepth[OCT_XY] = odepth[OCT_YX] = 0.0f;

    // determine how the characters can be attached
    odepth[OCT_Z] = ( pchr_a->pos.z + pchr_a->chr_min_cv.maxs[OCT_Z] ) - ( pprt_b->pos.z + pprt_b->prt_max_cv.mins[OCT_Z] );

    // size of b doesn't matter

    odepth[OCT_X]  = MIN(( pchr_a->chr_min_cv.maxs[OCT_X] + pchr_a->pos.x ) - pprt_b->pos.x,
                         pprt_b->pos.x - ( pchr_a->chr_min_cv.mins[OCT_X] + pchr_a->pos.x ) );

    odepth[OCT_Y]  = MIN(( pchr_a->chr_min_cv.maxs[OCT_Y] + pchr_a->pos.y ) -  pprt_b->pos.y,
                         pprt_b->pos.y - ( pchr_a->chr_min_cv.mins[OCT_Y] + pchr_a->pos.y ) );

    odepth[OCT_XY] = MIN(( pchr_a->chr_min_cv.maxs[OCT_XY] + ( pchr_a->pos.x + pchr_a->pos.y ) ) - ( pprt_b->pos.x + pprt_b->pos.y ),
                         ( pprt_b->pos.x + pprt_b->pos.y ) - ( pchr_a->chr_min_cv.mins[OCT_XY] + ( pchr_a->pos.x + pchr_a->pos.y ) ) );

    odepth[OCT_YX] = MIN(( pchr_a->chr_min_cv.maxs[OCT_YX] + ( -pchr_a->pos.x + pchr_a->pos.y ) ) - ( -pprt_b->pos.x + pprt_b->pos.y ),
                         ( -pprt_b->pos.x + pprt_b->pos.y ) - ( pchr_a->chr_min_cv.mins[OCT_YX] + ( -pchr_a->pos.x + pchr_a->pos.y ) ) );

    collide_x  = odepth[OCT_X]  > 0.0f;
    collide_y  = odepth[OCT_Y]  > 0.0f;
    collide_xy = odepth[OCT_XY] > 0.0f;
    collide_yx = odepth[OCT_YX] > 0.0f;
    collide_z  = ( odepth[OCT_Z] > -PLATTOLERANCE && odepth[OCT_Z] < PLATTOLERANCE );

    if ( collide_x && collide_y && collide_xy && collide_yx && collide_z )
    {
        // check for the best possible attachment
        if ( pchr_a->pos.z + pchr_a->chr_min_cv.maxs[OCT_Z] > pprt_b->targetplatform_level )
        {
            // set, but do not attach the platforms yet
            pprt_b->targetplatform_level = pchr_a->pos.z + pchr_a->chr_min_cv.maxs[OCT_Z];
            pprt_b->targetplatform_ref   = ichr_a;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
void bump_all_objects()
{
    /// @details ZZ@> This function sets handles characters hitting other characters or particles

    CHashList_t * pchlst;
    size_t        co_node_count;

    // create a collision hash table that can keep track of 512
    // binary collisions per frame
    pchlst = CHashList_get_Instance( -1 );
    if ( NULL == pchlst )
    {
        log_error( "bump_all_objects() - cannot access the CHashList_t singleton" );
    }

    // set up the collision node array
    _co_ary.top = _co_ary.alloc;

    // set up the hash node array
    _hn_ary.top = _hn_ary.alloc;

    // fill up the BSP structures
    fill_bumplists();

    // use the BSP structures to detect possible binary interactions
    fill_interaction_list( pchlst, &_co_ary, &_hn_ary );

    // convert the CHashList_t into a CoNode_ary_t and sort
    co_node_count = hash_list_count_nodes( pchlst );

    if ( co_node_count > 0 )
    {
        hash_list_iterator_t it;

        _coll_node_lst.top = 0;

        hash_list_iterator_ctor( &it );
        hash_list_iterator_set_begin( &it, pchlst );
        for ( /* nothing */; !hash_list_iterator_done( &it, pchlst ); hash_list_iterator_next( &it, pchlst ) )
        {
            CoNode_t * ptr = ( CoNode_t * )hash_list_iterator_ptr( &it );
            if ( NULL == ptr ) break;

            CoNode_ary_push_back( &_coll_node_lst, *ptr );
        }

        if ( _coll_node_lst.top > 1 )
        {
            // arrange the actual nodes by time order
            qsort( _coll_node_lst.ary, _coll_node_lst.top, sizeof( CoNode_t ), CoNode_cmp );
        }

        // handle interaction with mounts
        // put this before platforms, otherwise pointing is just too hard
        bump_all_mounts( &_coll_node_lst );

        // handle interaction with platforms
        bump_all_platforms( &_coll_node_lst );

        // handle all the collisions
        bump_all_collisions( &_coll_node_lst );
    }

    // The following functions need to be called any time you actually change a charcter's position
    keep_weapons_with_holders();
    attach_all_particles();
    update_all_character_matrices();
}

//--------------------------------------------------------------------------------------------
bool_t bump_all_platforms( CoNode_ary_t * pcn_ary )
{
    /// @details BB@> Detect all character and particle interactions with platforms, then attach them.
    ///
    /// @note it is important to only attach the character to a platform once, so its
    ///  weight does not get applied to multiple platforms
    ///
    /// @note the function move_one_character_get_environment() has already been called from within the
    ///  move_one_character() function, so the environment has already been determined this round

    int        cnt;
    CoNode_t * d;

    if ( NULL == pcn_ary ) return bfalse;

    //---- Detect all platform attachments
    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
    {
        d = pcn_ary->ary + cnt;

        // only look at character-platform or particle-platform interactions interactions
        if ( MAX_PRT != d->prta && MAX_PRT != d->prtb ) continue;

        if ( MAX_CHR != d->chra && MAX_CHR != d->chrb )
        {
            do_chr_platform_detection( d->chra, d->chrb );
        }
        else if ( MAX_CHR != d->chra && MAX_PRT != d->prtb )
        {
            do_prt_platform_detection( d->chra, d->prtb );
        }
        if ( MAX_PRT != d->prta && MAX_CHR != d->chrb )
        {
            do_prt_platform_detection( d->chrb, d->prta );
        }
    }

    //---- Do the actual platform attachments.

    // Doing the attachments after detecting the best platform
    // prevents an object from attaching it to multiple platforms as it
    // is still trying to find the best one
    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
    {
        d = pcn_ary->ary + cnt;

        // only look at character-character interactions
        //if ( MAX_PRT != d->prta && MAX_PRT != d->prtb ) continue;

        if ( MAX_CHR != d->chra && MAX_CHR != d->chrb )
        {
            if ( INGAME_CHR( d->chra ) && INGAME_CHR( d->chrb ) )
            {
                if ( ChrList.lst[d->chra].targetplatform_ref == d->chrb )
                {
                    attach_chr_to_platform( ChrList.lst + d->chra, ChrList.lst + d->chrb );
                }
                else if ( ChrList.lst[d->chrb].targetplatform_ref == d->chra )
                {
                    attach_chr_to_platform( ChrList.lst + d->chrb, ChrList.lst + d->chra );
                }

            }
        }
        else if ( MAX_CHR != d->chra && MAX_PRT != d->prtb )
        {
            if ( INGAME_CHR( d->chra ) && INGAME_PRT( d->prtb ) )
            {
                if ( PrtList.lst[d->prtb].targetplatform_ref == d->chra )
                {
                    attach_prt_to_platform( PrtList.lst + d->prtb, ChrList.lst + d->chra );
                }
            }
        }
        else if ( MAX_CHR != d->chrb && MAX_PRT != d->prta )
        {
            if ( INGAME_CHR( d->chrb ) && INGAME_PRT( d->prta ) )
            {
                if ( PrtList.lst[d->prta].targetplatform_ref == d->chrb )
                {
                    attach_prt_to_platform( PrtList.lst + d->prta, ChrList.lst + d->chrb );
                }
            }
        }
    }

    //---- remove any bad platforms

    // attach_prt_to_platform() erases targetplatform_ref, so any character with
    // (MAX_CHR != targetplatform_ref) must not be connected to a platform at all
    CHR_BEGIN_LOOP_ACTIVE( ichr, pchr )
    {
        if ( MAX_CHR != pchr->onwhichplatform_ref && pchr->onwhichplatform_update < update_wld )
        {
            detach_character_from_platform( pchr );
        }
    }
    CHR_END_LOOP();

    // attach_prt_to_platform() erases targetplatform_ref, so any particle with
    // (MAX_CHR != targetplatform_ref) must not be connected to a platform at all
    PRT_BEGIN_LOOP_DISPLAY( iprt, bdl_prt )
    {
        if ( MAX_CHR != bdl_prt.prt_ptr->onwhichplatform_ref && bdl_prt.prt_ptr->onwhichplatform_update < update_wld )
        {
            detach_particle_from_platform( bdl_prt.prt_ptr );
        }
    }
    PRT_END_LOOP();

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t bump_all_mounts( CoNode_ary_t * pcn_ary )
{
    /// @details BB@> Detect all character interactions with mounts, then attach them.

    int        cnt;
    CoNode_t * d;

    if ( NULL == pcn_ary ) return bfalse;

    // Do mounts
    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
    {
        d = pcn_ary->ary + cnt;

        // only look at character-character interactions
        if ( MAX_CHR == d->chra || MAX_CHR == d->chrb ) continue;

        bump_one_mount( d->chra, d->chrb );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t bump_all_collisions( CoNode_ary_t * pcn_ary )
{
    /// @details BB@> Detect all character-character and character-particle collsions (with exclusions
    ///               for the mounts and platforms found in the previous steps)

    int        cnt;

    // blank the accumulators
    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        fvec3_self_clear( pchr->phys.apos_plat.v );
        fvec3_self_clear( pchr->phys.apos_coll.v );
        fvec3_self_clear( pchr->phys.avel.v );
    }
    CHR_END_LOOP();

    PRT_BEGIN_LOOP_ACTIVE( cnt, prt_bdl )
    {
        fvec3_self_clear( prt_bdl.prt_ptr->phys.apos_plat.v );
        fvec3_self_clear( prt_bdl.prt_ptr->phys.apos_coll.v );
        fvec3_self_clear( prt_bdl.prt_ptr->phys.avel.v );
    }
    PRT_END_LOOP();

    // do all interactions
    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
    {
        bool_t handled = bfalse;

        // use this form of the function call so that we could add more modules or
        // rearrange them without needing to change anything
        if ( !handled )
        {
            handled = do_chr_chr_collision( pcn_ary->ary + cnt );
        }

        if ( !handled )
        {
            handled = do_chr_prt_collision( pcn_ary->ary + cnt );
        }
    }

    // accumulate the accumulators
    CHR_BEGIN_LOOP_ACTIVE( ichr, pchr )
    {
        float tmpx, tmpy, tmpz;
        float bump_str;
        bool_t position_updated = bfalse;
        fvec3_t max_apos;

        fvec3_t tmp_pos = chr_get_pos( pchr );

        bump_str = 1.0f;
        if ( INGAME_CHR( pchr->attachedto ) )
        {
            bump_str = 0.0f;
        }

        // do the "integration" of the accumulated accelerations
        fvec3_self_sum( pchr->vel.v, pchr->phys.avel.v );

        position_updated = bfalse;

        max_apos = fvec3_add( pchr->phys.apos_plat.v, pchr->phys.apos_coll.v );
        max_apos.x = CLIP( max_apos.x, -GRID_FSIZE, GRID_FSIZE );
        max_apos.y = CLIP( max_apos.y, -GRID_FSIZE, GRID_FSIZE );
        max_apos.z = CLIP( max_apos.z, -GRID_FSIZE, GRID_FSIZE );

        // do the "integration" on the position
        if ( ABS( max_apos.x ) > 0.0f )
        {
            tmpx = tmp_pos.x;
            tmp_pos.x += max_apos.x;
            if ( EMPTY_BIT_FIELD != chr_test_wall( pchr, tmp_pos.v, NULL ) )
            {
                // restore the old values
                tmp_pos.x = tmpx;
            }
            else
            {
                //pchr->vel.x += pchr->phys.apos_coll.x * bump_str;
                position_updated = btrue;
            }
        }

        if ( ABS( max_apos.y ) > 0.0f )
        {
            tmpy = tmp_pos.y;
            tmp_pos.y += max_apos.y;
            if ( EMPTY_BIT_FIELD != chr_test_wall( pchr, tmp_pos.v, NULL ) )
            {
                // restore the old values
                tmp_pos.y = tmpy;
            }
            else
            {
                //pchr->vel.y += pchr->phys.apos_coll.y * bump_str;
                position_updated = btrue;
            }
        }

        if ( ABS( max_apos.z ) > 0.0f )
        {
            tmpz = tmp_pos.z;
            tmp_pos.z += max_apos.z;
            if ( tmp_pos.z < pchr->enviro.floor_level )
            {
                // restore the old values
                tmp_pos.z = pchr->enviro.floor_level;
                if ( pchr->vel.z < 0 )
                {
                    cap_t * pcap = chr_get_pcap( ichr );
                    if ( NULL != pcap )
                    {
                        pchr->vel.z += -( 1.0f + pcap->dampen ) * pchr->vel.z;
                    }
                }
                position_updated = btrue;
            }
            else
            {
                //pchr->vel.z += pchr->phys.apos_coll.z * bump_str;
                position_updated = btrue;
            }
        }

        if ( position_updated )
        {
            chr_set_pos( pchr, tmp_pos.v );
        }
    }
    CHR_END_LOOP();

    // accumulate the accumulators
    PRT_BEGIN_LOOP_ACTIVE( iprt, bdl )
    {
        float tmpx, tmpy, tmpz;
        float bump_str;
        bool_t position_updated = bfalse;
        fvec3_t max_apos;

        fvec3_t tmp_pos = prt_get_pos( bdl.prt_ptr );

        bump_str = 1.0f;
        if ( INGAME_CHR( bdl.prt_ptr->attachedto_ref ) )
        {
            bump_str = 0.0f;
        }

        // do the "integration" of the accumulated accelerations
        fvec3_self_sum( bdl.prt_ptr->vel.v, bdl.prt_ptr->phys.avel.v );

        position_updated = bfalse;

        max_apos = fvec3_add( bdl.prt_ptr->phys.apos_plat.v, bdl.prt_ptr->phys.apos_coll.v );
        max_apos.x = CLIP( max_apos.x, -GRID_FSIZE, GRID_FSIZE );
        max_apos.y = CLIP( max_apos.y, -GRID_FSIZE, GRID_FSIZE );
        max_apos.z = CLIP( max_apos.z, -GRID_FSIZE, GRID_FSIZE );

        // do the "integration" on the position
        if ( ABS( max_apos.x ) > 0.0f )
        {
            tmpx = tmp_pos.x;
            tmp_pos.x += max_apos.x;
            if ( EMPTY_BIT_FIELD != prt_test_wall( bdl.prt_ptr, tmp_pos.v, NULL ) )
            {
                // restore the old values
                tmp_pos.x = tmpx;
            }
            else
            {
                //bdl.prt_ptr->vel.x += bdl.prt_ptr->phys.apos_coll.x * bump_str;
                position_updated = btrue;
            }
        }

        if ( ABS( max_apos.y ) > 0.0f )
        {
            tmpy = tmp_pos.y;
            tmp_pos.y += max_apos.y;
            if ( EMPTY_BIT_FIELD != prt_test_wall( bdl.prt_ptr, tmp_pos.v, NULL ) )
            {
                // restore the old values
                tmp_pos.y = tmpy;
            }
            else
            {
                //bdl.prt_ptr->vel.y += bdl.prt_ptr->phys.apos_coll.y * bump_str;
                position_updated = btrue;
            }
        }

        if ( ABS( max_apos.z ) > 0.0f )
        {
            tmpz = tmp_pos.z;
            tmp_pos.z += max_apos.z;
            if ( tmp_pos.z < bdl.prt_ptr->enviro.floor_level )
            {
                // restore the old values
                tmp_pos.z = bdl.prt_ptr->enviro.floor_level;
                if ( bdl.prt_ptr->vel.z < 0 )
                {
                    if ( LOADED_PIP( bdl.prt_ptr->pip_ref ) )
                    {
                        pip_t * ppip = PipStack.lst + bdl.prt_ptr->pip_ref;
                        bdl.prt_ptr->vel.z += -( 1.0f + ppip->dampen ) * bdl.prt_ptr->vel.z;
                    }
                    else
                    {
                        bdl.prt_ptr->vel.z += -( 1.0f + 0.5f ) * bdl.prt_ptr->vel.z;
                    }
                }
                position_updated = btrue;
            }
            else
            {
                //bdl.prt_ptr->vel.z += bdl.prt_ptr->phys.apos_coll.z * bump_str;
                position_updated = btrue;
            }
        }

        // Change the direction of the particle
        if ( bdl.pip_ptr->rotatetoface )
        {
            // Turn to face new direction
            bdl.prt_ptr->facing = vec_to_facing( bdl.prt_ptr->vel.x , bdl.prt_ptr->vel.y );
        }

        if ( position_updated )
        {
            prt_set_pos( bdl.prt_ptr, tmp_pos.v );
        }
    }
    PRT_END_LOOP();

    // blank the accumulators
    CHR_BEGIN_LOOP_ACTIVE( cnt, pchr )
    {
        fvec3_self_clear( pchr->phys.apos_plat.v );
        fvec3_self_clear( pchr->phys.apos_coll.v );
        fvec3_self_clear( pchr->phys.avel.v );
    }
    CHR_END_LOOP();

    PRT_BEGIN_LOOP_ACTIVE( cnt, prt_bdl )
    {
        fvec3_self_clear( prt_bdl.prt_ptr->phys.apos_plat.v );
        fvec3_self_clear( prt_bdl.prt_ptr->phys.apos_coll.v );
        fvec3_self_clear( prt_bdl.prt_ptr->phys.avel.v );
    }
    PRT_END_LOOP();

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t bump_one_mount( const CHR_REF ichr_a, const CHR_REF ichr_b )
{
    fvec3_t vdiff;

    oct_vec_t apos, bpos;

    chr_t * pchr_a, * pchr_b;

    bool_t mount_a, mount_b;

    bool_t mounted = bfalse;
    bool_t handled = bfalse;

    // make sure that A is valid
    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    // make sure that B is valid
    if ( !INGAME_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    // find the difference in velocities
    vdiff = fvec3_sub( pchr_b->vel.v, pchr_a->vel.v );

    // can either of these objects mount the other?
    mount_a = chr_can_mount( ichr_b, ichr_a );
    mount_b = chr_can_mount( ichr_a, ichr_b );

    if ( !mount_a && !mount_b ) return bfalse;

    //Ready for position calulations
    oct_vec_ctor( apos, chr_get_pos_v( pchr_a ) );
    oct_vec_ctor( bpos, chr_get_pos_v( pchr_b ) );

    // assume the worst
    mounted = bfalse;

    // mount a on b ?
    if ( !mounted && mount_b )
    {
        oct_bb_t  tmp_cv, saddle_cv;

        //---- find out whether the object is overlapping with the saddle

        // the position of the saddle over the frame
        oct_bb_add_fvec3( pchr_b->slot_cv + SLOT_LEFT, chr_get_pos_v( pchr_b ), &tmp_cv );
        phys_expand_oct_bb( &tmp_cv, pchr_b->vel.v, 0.0f, 1.0f, &saddle_cv );

        if ( oct_bb_point_inside( &saddle_cv, apos ) )
        {
            oct_vec_t saddle_pos;
            fvec3_t   pdiff;

            oct_bb_get_mids( &saddle_cv, saddle_pos );
            pdiff.x = saddle_pos[OCT_X] - apos[OCT_X];
            pdiff.y = saddle_pos[OCT_Y] - apos[OCT_Y];
            pdiff.z = saddle_pos[OCT_Z] - apos[OCT_Z];

            if ( fvec3_dot_product( pdiff.v, vdiff.v ) >= 0.0f )
            {
                // the rider is in a mountable position, don't do any more collisions
                // even if the object is doesn't actually mount
                handled = btrue;

                if ( rv_success == attach_character_to_mount( ichr_a, ichr_b, GRIP_ONLY ) )
                {
                    mounted = INGAME_CHR( pchr_a->attachedto );
                }
            }
        }
    }

    // mount b on a ?
    if ( !mounted && mount_a )
    {
        oct_bb_t  tmp_cv, saddle_cv;

        //---- find out whether the object is overlapping with the saddle

        // the position of the saddle over the frame
        oct_bb_add_fvec3( pchr_a->slot_cv + SLOT_LEFT, chr_get_pos_v( pchr_a ), &tmp_cv );
        phys_expand_oct_bb( &tmp_cv, pchr_a->vel.v, 0.0f, 1.0f, &saddle_cv );

        if ( oct_bb_point_inside( &saddle_cv, bpos ) )
        {
            oct_vec_t saddle_pos;
            fvec3_t   pdiff;

            oct_bb_get_mids( &saddle_cv, saddle_pos );

            // vdiff is computed as b - a. keep the pdiff in the same sense
            pdiff.x = bpos[OCT_X] - saddle_pos[OCT_X];
            pdiff.y = bpos[OCT_Y] - saddle_pos[OCT_Y];
            pdiff.z = bpos[OCT_Z] - saddle_pos[OCT_Z];

            if ( fvec3_dot_product( pdiff.v, vdiff.v ) >= 0.0f )
            {
                // the rider is in a mountable position, don't do any more collisions
                // even if the object is doesn't actually mount
                handled = btrue;

                if ( rv_success == attach_character_to_mount( ichr_b, ichr_a, GRIP_ONLY ) )
                {
                    mounted = INGAME_CHR( pchr_b->attachedto );
                }
            }
        }
    }

    return handled;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_platform_physics( chr_t * pitem, chr_t * pplat )
{
    // we know that ichr_a is a platform and ichr_b is on it
    Sint16 rot_a, rot_b;
    float lerp_z, vlerp_z;

    if ( !ACTIVE_PCHR( pitem ) ) return bfalse;
    if ( !ACTIVE_PCHR( pplat ) ) return bfalse;

    if ( pitem->onwhichplatform_ref != GET_REF_PCHR( pplat ) ) return bfalse;

    // grab the pre-computed zlerp value, and map it to our needs
    lerp_z = 1.0f - pitem->enviro.zlerp;

    // if your velocity is going up much faster then the
    // platform, there is no need to suck you to the level of the platform
    // this was one of the things preventing you from jumping from platforms
    // properly
    vlerp_z = ABS( pitem->vel.z - pplat->vel.z ) / 5;
    vlerp_z  = 1.0f - CLIP( vlerp_z, 0.0f, 1.0f );

    // determine the rotation rates
    rot_b = pitem->ori.facing_z - pitem->ori_old.facing_z;
    rot_a = pplat->ori.facing_z - pplat->ori_old.facing_z;

    if ( lerp_z == 1.0f )
    {
        pitem->phys.apos_plat.z += ( pitem->enviro.level - pitem->pos.z ) * 0.125f;
        pitem->phys.avel.z += ( pplat->vel.z  - pitem->vel.z ) * 0.25f;
        pitem->ori.facing_z      += ( rot_a         - rot_b ) * platstick;
    }
    else
    {
        pitem->phys.apos_plat.z += ( pitem->enviro.level - pitem->pos.z ) * 0.125f * lerp_z * vlerp_z;
        pitem->phys.avel.z += ( pplat->vel.z  - pitem->vel.z ) * 0.25f * lerp_z * vlerp_z;
        pitem->ori.facing_z      += ( rot_a         - rot_b ) * platstick * lerp_z * vlerp_z;
    };

    return btrue;
}

//--------------------------------------------------------------------------------------------
float estimate_chr_prt_normal( chr_t * pchr, prt_t * pprt, fvec3_base_t nrm, fvec3_base_t vdiff )
{
    fvec3_t collision_size;
    float dot;

    collision_size.x = MAX( pchr->chr_max_cv.maxs[OCT_X] - pchr->chr_max_cv.mins[OCT_X], 2.0f * pprt->bump_padded.size );
    collision_size.y = MAX( pchr->chr_max_cv.maxs[OCT_Y] - pchr->chr_max_cv.mins[OCT_Y], 2.0f * pprt->bump_padded.size );
    collision_size.z = MAX( pchr->chr_max_cv.maxs[OCT_Z] - pchr->chr_max_cv.mins[OCT_Z], 2.0f * pprt->bump_padded.height );

    // estimate the "normal" for the collision, using the center-of-mass difference
    nrm[kX] = pprt->pos.x - pchr->pos.x;
    nrm[kY] = pprt->pos.y - pchr->pos.y;
    nrm[kZ] = pprt->pos.z - ( pchr->pos.z + 0.5f * ( pchr->chr_max_cv.maxs[OCT_Z] + pchr->chr_max_cv.mins[OCT_Z] ) );

    // scale the collision box
    nrm[kX] /= collision_size.x;
    nrm[kY] /= collision_size.y;
    nrm[kZ] /= collision_size.z;

    // scale the normals so that the collision volume will act somewhat like a cylinder
    if ( pchr->platform )
    {
        nrm[kX] *= nrm[kX] * nrm[kX] + nrm[kY] * nrm[kY];
        nrm[kY] *= nrm[kX] * nrm[kX] + nrm[kY] * nrm[kY];

        nrm[kZ] *= nrm[kZ] * nrm[kZ];
    }

    // reject the reflection request if the particle is moving in the wrong direction
    vdiff[kX] = pchr->vel.x - pprt->vel.x;
    vdiff[kY] = pchr->vel.y - pprt->vel.y;
    vdiff[kZ] = pchr->vel.z - pprt->vel.z;
    dot       = fvec3_dot_product( vdiff, nrm );

    // we really never should have the condition that dot > 0, unless the particle is "fast"
    if ( dot >= 0.0f )
    {
        fvec3_t vtmp;

        // If the particle is "fast" relative to the object size, it can happen that the particle
        // can be more than halfway through the character before it is detected.

        vtmp.x = vdiff[kX] / collision_size.x;
        vtmp.y = vdiff[kY] / collision_size.y;
        vtmp.z = vdiff[kZ] / collision_size.z;

        // If it is fast, re-evaluate the normal in a different way
        if ( vtmp.x*vtmp.x + vtmp.y*vtmp.y + vtmp.z*vtmp.z > 0.5f*0.5f )
        {
            // use the old position, which SHOULD be before the collision
            // to determine the normal
            nrm[kX] = pprt->pos_old.x - pchr->pos_old.x;
            nrm[kY] = pprt->pos_old.y - pchr->pos_old.y;
            nrm[kZ] = pprt->pos_old.z - ( pchr->pos_old.z + 0.5f * ( pchr->chr_max_cv.maxs[OCT_Z] + pchr->chr_max_cv.mins[OCT_Z] ) );

            // scale the collision box
            nrm[kX] /= collision_size.x;
            nrm[kY] /= collision_size.y;
            nrm[kZ] /= collision_size.z;

            // scale the z-normals so that the collision volume will act somewhat like a cylinder
            nrm[kZ] *= nrm[kZ] * nrm[kZ];
        }
    }

    // assume the function fails
    dot = 0.0f;

    // does the normal exist?
    if ( ABS( nrm[kX] ) + ABS( nrm[kY] ) + ABS( nrm[kZ] ) > 0.0f )
    {
        // Make the normal a unit normal
        fvec3_t ntmp = fvec3_normalize( nrm );
        memcpy( nrm, ntmp.v, sizeof( fvec3_base_t ) );

        // determine the actual dot product
        dot = fvec3_dot_product( vdiff, nrm );
    }

    return dot;
}
//--------------------------------------------------------------------------------------------
bool_t do_chr_chr_collision_pressure_normal( chr_t * pchr_a, chr_t * pchr_b, float exponent, oct_vec_t * podepth, fvec3_base_t nrm, float * tmin )
{
    oct_bb_t otmp_a, otmp_b;

    oct_bb_add_fvec3( &( pchr_a->chr_min_cv ), chr_get_pos_v( pchr_a ), &otmp_a );
    oct_bb_add_fvec3( &( pchr_b->chr_min_cv ), chr_get_pos_v( pchr_b ), &otmp_b );

    return phys_estimate_pressure_normal( &otmp_a, &otmp_b, exponent, podepth, nrm, tmin );
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_chr_collision( CoNode_t * d )
{
    CHR_REF ichr_a, ichr_b;
    chr_t * pchr_a, * pchr_b;
    cap_t * pcap_a, * pcap_b;

    float depth_min;
    float interaction_strength = 1.0f;

    float wta, wtb;
    float recoil_a, recoil_b;

    // object bounding boxes shifted so that they are in the correct place on the map
    oct_bb_t map_bb_a, map_bb_b;

    fvec3_t   nrm;
    int exponent = 1;

    oct_vec_t odepth;
    bool_t    collision = bfalse, bump = bfalse, valid_normal = bfalse;

    if ( NULL == d || MAX_PRT != d->prtb ) return bfalse;
    ichr_a = d->chra;
    ichr_b = d->chrb;

    // make sure that it is on
    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    pcap_a = chr_get_pcap( ichr_a );
    if ( NULL == pcap_a ) return bfalse;

    // make sure that it is on
    if ( !INGAME_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    pcap_b = chr_get_pcap( ichr_b );
    if ( NULL == pcap_b ) return bfalse;

    //skip objects that are inside inventories
    if ( pchr_a->pack.is_packed || pchr_b->pack.is_packed ) return bfalse;

    // skip all objects that are mounted or attached to something
    if ( INGAME_CHR( pchr_a->attachedto ) || INGAME_CHR( pchr_b->attachedto ) ) return bfalse;

    // platform interaction. if the onwhichplatform_ref is set, then
    // all collision tests have been met
    if ( ichr_a == pchr_b->onwhichplatform_ref )
    {
        if ( do_chr_platform_physics( pchr_b, pchr_a ) )
        {
            // this is handled
            return btrue;
        }
    }

    // platform interaction. if the onwhichplatform_ref is set, then
    // all collision tests have been met
    if ( ichr_b == pchr_a->onwhichplatform_ref )
    {
        if ( do_chr_platform_physics( pchr_a, pchr_b ) )
        {
            // this is handled
            return btrue;
        }
    }

    // items can interact with platforms but not with other characters/objects
    if ( pchr_a->isitem || pchr_b->isitem ) return bfalse;

    // don't interact with your mount, or your held items
    if ( ichr_a == pchr_b->attachedto || ichr_b == pchr_a->attachedto ) return bfalse;

    // don't do anything if there is no interaction strength
    if ( 0.0f == pchr_a->bump_stt.size || 0.0f == pchr_b->bump_stt.size ) return bfalse;

    interaction_strength = 1.0f;
    interaction_strength *= pchr_a->inst.alpha * INV_FF;
    interaction_strength *= pchr_b->inst.alpha * INV_FF;

    // reduce your interaction strength if you have just detached from an object
    if ( pchr_a->dismount_object == ichr_b )
    {
        float dismount_lerp = ( float )pchr_a->dismount_timer / ( float )PHYS_DISMOUNT_TIME;
        dismount_lerp = CLIP( dismount_lerp, 0.0f, 1.0f );

        interaction_strength *= dismount_lerp;
    }

    if ( pchr_b->dismount_object == ichr_a )
    {
        float dismount_lerp = ( float )pchr_b->dismount_timer / ( float )PHYS_DISMOUNT_TIME;
        dismount_lerp = CLIP( dismount_lerp, 0.0f, 1.0f );

        interaction_strength *= dismount_lerp;
    }

    // seriously reduce the interaction_strength with mounts
    // this thould allow characters to mount certain mounts a lot easier
    if (( pchr_a->ismount && MAX_CHR == pchr_a->holdingwhich[SLOT_LEFT] && !pchr_b->ismount ) ||
        ( pchr_b->ismount && MAX_CHR == pchr_b->holdingwhich[SLOT_LEFT] && !pchr_a->ismount ) )
    {
        interaction_strength *= 0.25;
    }

    // reduce the interaction strength with platforms
    // that are overlapping with the platform you are actually on
    if ( pchr_b->canuseplatforms && pchr_a->platform && MAX_CHR != pchr_b->onwhichplatform_ref && ichr_a != pchr_b->onwhichplatform_ref )
    {
        float lerp_z = ( pchr_b->pos.z - ( pchr_a->pos.z + pchr_a->chr_min_cv.maxs[OCT_Z] ) ) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1.0f, 1.0f );

        if ( lerp_z >= 0.0f )
        {
            interaction_strength = 0.0f;
        }
        else
        {
            interaction_strength *= -lerp_z;
        }
    }

    if ( pchr_a->canuseplatforms && pchr_b->platform && MAX_CHR != pchr_a->onwhichplatform_ref && ichr_b != pchr_a->onwhichplatform_ref )
    {
        float lerp_z = ( pchr_a->pos.z - ( pchr_b->pos.z + pchr_b->chr_min_cv.maxs[OCT_Z] ) ) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1, 1 );

        if ( lerp_z >= 0.0f )
        {
            interaction_strength = 0.0f;
        }
        else
        {
            interaction_strength *= -lerp_z;
        }
    }

    // shift the character bounding boxes to be centered on their positions
    oct_bb_add_fvec3( &( pchr_a->chr_min_cv ), chr_get_pos_v( pchr_a ), &map_bb_a );
    oct_bb_add_fvec3( &( pchr_b->chr_min_cv ), chr_get_pos_v( pchr_b ), &map_bb_b );

    // make the object more like a table if there is a platform-like interaction
    exponent = 1.0f;
    if ( pchr_a->canuseplatforms && pchr_b->platform ) exponent += 2;
    if ( pchr_b->canuseplatforms && pchr_a->platform ) exponent += 2;

    // use the info from the collision volume to determine whether the objects are colliding
    collision = ( d->tmin > 0.0f );

    // estimate the collision normal at the point of contact
    valid_normal = bfalse;
    depth_min    = 0.0f;
    if ( collision )
    {
        // find the collision volumes at 10% overlap
        oct_bb_t exp1, exp2;

        float tmp_min, tmp_max;

        tmp_min = d->tmin;
        tmp_max = d->tmin + ( d->tmax - d->tmin ) * 0.1f;

        // determine the expanded collision volumes for both objects
        phys_expand_oct_bb( &map_bb_a, pchr_a->vel.v, tmp_min, tmp_max, &exp1 );
        phys_expand_oct_bb( &map_bb_b, pchr_b->vel.v, tmp_min, tmp_max, &exp2 );

        valid_normal = phys_estimate_collision_normal( &exp1, &exp2, exponent, &odepth, nrm.v, &depth_min );
    }

    if ( !collision || depth_min <= 0.0f )
    {
        valid_normal = phys_estimate_pressure_normal( &map_bb_a, &map_bb_b, exponent, &odepth, nrm.v, &depth_min );
    }

    if ( depth_min <= 0.0f )
        return bfalse;

    // if we can't obtain a valid collision normal, we fail
    if ( !valid_normal ) return bfalse;

    //------------------
    // do character-character interactions

    // calculate a "mass" for each object, taking into account possible infinite masses
    get_chr_mass( pchr_a, &wta );
    get_chr_mass( pchr_b, &wtb );

    // make a special exception for interaction between "Mario platforms"
    if (( wta < 0.0f && pchr_a->platform ) && ( wtb < 0.0f && pchr_a->platform ) )
    {
        return bfalse;
    }

    // make a special exception for immovable scenery objects
    // they can collide, but cannot push each other apart... that might mess up the scenery ;)
    if ( !collision && ( wta < 0.0f && 0.0f == pchr_a->maxaccel ) && ( wtb < 0.0f && 0.0f == pchr_b->maxaccel ) )
    {
        return bfalse;
    }

    // determine the relative effect of impulses, given the known weights
    get_recoil_factors( wta, wtb, &recoil_a, &recoil_b );

    //---- calculate the character-character interactions
    {
        const float max_pressure_strength = 0.125f;
        const float pressure_strength     = max_pressure_strength * interaction_strength;

        fvec3_t   pdiff_a;

        bool_t need_displacement = bfalse;
        bool_t need_velocity = bfalse;

        fvec3_t   vdiff_a;

        if ( depth_min <= 0.0f || collision )
        {
            need_displacement = bfalse;
            fvec3_self_clear( pdiff_a.v );
        }
        else
        {
            // add a small amount to the pressure difference so that
            // the function will actually separate the objects in a finite number
            // of iterations
            need_displacement = ( recoil_a > 0.0f ) || ( recoil_b > 0.0f );
            pdiff_a = fvec3_scale( nrm.v, depth_min + 1.0f );
        }

        // find the relative velocity
        vdiff_a = fvec3_sub( pchr_b->vel.v, pchr_a->vel.v );

        need_velocity = bfalse;
        if ( fvec3_length_abs( vdiff_a.v ) > 1e-6 )
        {
            need_velocity = ( recoil_a > 0.0f ) || ( recoil_b > 0.0f );
        }

        //---- handle the relative velocity
        if ( need_velocity )
        {

            // what type of interaction is this? (collision or pressure)
            if ( collision )
            {
                // !!!! COLLISION !!!!

                // an actual bump, use impulse to make the objects bounce appart

                fvec3_t vdiff_para_a, vdiff_perp_a;

                // generic coefficient of restitution.
                float cr = pchr_a->phys.dampen * pchr_b->phys.dampen;

                // decompose this relative to the collision normal
                fvec3_decompose( vdiff_a.v, nrm.v, vdiff_perp_a.v, vdiff_para_a.v );

                if ( recoil_a > 0.0f )
                {
                    fvec3_t vimp_a;

                    vimp_a = fvec3_scale( vdiff_perp_a.v, recoil_a * ( 1.0f + cr ) * interaction_strength );

                    fvec3_self_sum( pchr_a->phys.avel.v, vimp_a.v );
                }

                if ( recoil_b > 0.0f )
                {
                    fvec3_t vimp_b;

                    vimp_b = fvec3_scale( vdiff_perp_a.v, -recoil_b * ( 1.0f + cr ) * interaction_strength );

                    fvec3_self_sum( pchr_b->phys.avel.v, vimp_b.v );
                }

                // this was definitely a bump
                bump = btrue;
            }
            // ignore the case of both objects having infinite mass
            // this is normally due to two scenery objects being too close to each other
            else
            {
                // !!!! PRESSURE !!!!

                // not a bump at all. two objects are rubbing against one another
                // and continually overlapping.
                //
                // reduce the relative velocity if the objects are moving towards each other,
                // but ignore it if they are moving away.

                // use pressure to push them appart. reduce their relative velocities.

                float     vdot;

                // are the objects moving towards each other, or appart?
                vdot = fvec3_dot_product( vdiff_a.v, nrm.v );

                if ( vdot < 0.0f )
                {
                    if ( recoil_a > 0.0f )
                    {
                        fvec3_t vimp_a;

                        vimp_a = fvec3_scale( vdiff_a.v, recoil_a * pressure_strength );

                        fvec3_self_sum( pchr_a->phys.avel.v,      vimp_a.v );
                    }

                    if ( recoil_b > 0.0f )
                    {
                        fvec3_t vimp_b;

                        vimp_b = fvec3_scale( vdiff_a.v, -recoil_b * pressure_strength );

                        fvec3_self_sum( pchr_b->phys.avel.v,      vimp_b.v );
                    }
                }

                // you could "bump" something if you changed your velocity, even if you were still touching
                bump = ( fvec3_dot_product( pchr_a->vel.v, nrm.v ) * fvec3_dot_product( pchr_a->vel_old.v, nrm.v ) < 0 ) ||
                       ( fvec3_dot_product( pchr_b->vel.v, nrm.v ) * fvec3_dot_product( pchr_b->vel_old.v, nrm.v ) < 0 );
            }

        }

        //---- fix the displacement regardless of what kind of interaction
        if ( need_displacement )
        {
            if ( recoil_a > 0.0f )
            {
                fvec3_t pimp_a;

                pimp_a = fvec3_scale( pdiff_a.v, recoil_a * pressure_strength );

                fvec3_self_sum( pchr_a->phys.apos_coll.v, pimp_a.v );
            }

            if ( recoil_b > 0.0f )
            {
                fvec3_t pimp_b;

                pimp_b = fvec3_scale( pdiff_a.v,  -recoil_b * pressure_strength );

                fvec3_self_sum( pchr_b->phys.apos_coll.v, pimp_b.v );
            }
        }

        //// add in the friction due to the "collision"
        //// assume coeff of friction of 0.5
        //if ( ABS( vimp_a.x ) + ABS( vimp_a.y ) + ABS( vimp_a.z ) > 0.0f &&
        //     ABS( vpara_a.x ) + ABS( vpara_a.y ) + ABS( vpara_a.z ) > 0.0f &&
        //     pchr_a->dismount_timer <= 0 )
        //{
        //    float imp, vel, factor;

        //    imp = 0.5f * SQRT( vimp_a.x * vimp_a.x + vimp_a.y * vimp_a.y + vimp_a.z * vimp_a.z );
        //    vel = SQRT( vpara_a.x * vpara_a.x + vpara_a.y * vpara_a.y + vpara_a.z * vpara_a.z );

        //    factor = imp / vel;
        //    factor = CLIP( factor, 0.0f, 1.0f );

        //    pchr_a->phys.avel.x -= factor * vpara_a.x * interaction_strength;
        //    pchr_a->phys.avel.y -= factor * vpara_a.y * interaction_strength;
        //    pchr_a->phys.avel.z -= factor * vpara_a.z * interaction_strength;
        //    LOG_NAN( pchr_a->phys.avel.z );
        //}

        //if ( ABS( vimp_b.x ) + ABS( vimp_b.y ) + ABS( vimp_b.z ) > 0.0f &&
        //     ABS( vpara_b.x ) + ABS( vpara_b.y ) + ABS( vpara_b.z ) > 0.0f &&
        //     pchr_b->dismount_timer <= 0 )
        //{
        //    float imp, vel, factor;

        //    imp = 0.5f * SQRT( vimp_b.x * vimp_b.x + vimp_b.y * vimp_b.y + vimp_b.z * vimp_b.z );
        //    vel = SQRT( vpara_b.x * vpara_b.x + vpara_b.y * vpara_b.y + vpara_b.z * vpara_b.z );

        //    factor = imp / vel;
        //    factor = CLIP( factor, 0.0f, 1.0f );

        //    pchr_b->phys.avel.x -= factor * vpara_b.x * interaction_strength;
        //    pchr_b->phys.avel.y -= factor * vpara_b.y * interaction_strength;
        //    pchr_b->phys.avel.z -= factor * vpara_b.z * interaction_strength;
        //    LOG_NAN( pchr_b->phys.avel.z );
        //}
    }

    if ( bump )
    {
        ai_state_set_bumplast( &( pchr_a->ai ), ichr_b );
        ai_state_set_bumplast( &( pchr_b->ai ), ichr_a );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//bool_t do_chr_prt_collision_get_depth_base( CoNode_t * d, oct_bb_t * pcv_a, fvec3_base_t vel_a, oct_bb_t * pcv_b, fvec3_base_t vel_b, float exponent, fvec3_base_t nrm, float *depth )
//{
//    oct_vec_t odepth;
//
//    if ( NULL == nrm || NULL == depth ) return bfalse;
//
//    if ( NULL == pcv_a || NULL == pcv_b ) return bfalse;
//
//    if ( d->tmin <= 0.0f || ABS( d->tmin ) > 1e6 || ABS( d->tmax ) > 1e6 )
//    {
//        // the objects are in contact for an extrodinary amount of time
//        // just use the "shortest way out" to find the interaction normal
//
//        phys_estimate_pressure_normal( pcv_a, pcv_b, exponent, &odepth, nrm, depth );
//    }
//    else
//    {
//        oct_bb_t exp1, exp2;
//
//        float tmp_min, tmp_max;
//
//        tmp_min = d->tmin;
//        tmp_max = d->tmin + ( d->tmax - d->tmin ) * 0.1f;
//
//        // determine the expanded collision volumes for both objects
//        phys_expand_oct_bb( pcv_a, vel_a, tmp_min, tmp_max, &exp1 );
//        phys_expand_oct_bb( pcv_b, vel_b, tmp_min, tmp_max, &exp2 );
//
//        phys_estimate_collision_normal( &exp1, &exp2, exponent, &odepth, nrm, depth );
//
//    }
//
//    return *depth > 0.0f;
//}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_get_details( CoNode_t * d, chr_prt_collsion_data_t * pdata )
{
    // Get details about the character-particle interaction
    //
    // We already know that the largest particle cv intersects with the a
    // character cv sometime this frame. We need more details to know
    // how to handle the collision.

    bool_t handled;

    float exponent;
    oct_bb_t cv_chr, cv_prt_max, cv_prt_min;
    oct_vec_t odepth;

    if ( NULL == d || NULL == pdata ) return bfalse;

    // make the object more like a table if there is a platform-like interaction
    exponent = 1;
    if ( SPRITE_SOLID == pdata->pprt->type && pdata->pchr->platform ) exponent += 2;

    // assume the simplest interaction normal
    pdata->nrm.x = pdata->nrm.y = 0.0f;
    pdata->nrm.z = 1.0f;

    // no valid interactions, yet
    handled = bfalse;

    // shift the source bounding boxes to be centered on the given positions
    oct_bb_add_fvec3( &( pdata->pchr->chr_min_cv ), chr_get_pos_v( pdata->pchr ), &cv_chr );

    // the smallest particle collision volume
    oct_bb_add_fvec3( &( pdata->pprt->prt_min_cv ), prt_get_pos_v( pdata->pprt ), &cv_prt_min );

    // the largest particle collision volume (the hit-box)
    oct_bb_add_fvec3( &( pdata->pprt->prt_max_cv ), prt_get_pos_v( pdata->pprt ), &cv_prt_max );

    if ( d->tmin <= 0.0f || ABS( d->tmin ) > 1e6 || ABS( d->tmax ) > 1e6 )
    {
        // use "pressure" to determine the normal and overlap
        phys_estimate_pressure_normal( &cv_prt_min, &cv_chr, exponent, &odepth, pdata->nrm.v, &( pdata->depth_min ) );

        handled = btrue;
        if ( d->tmin <= 0.0f )
        {
            handled = pdata->depth_min > 0.0f;
        }

        // tag the type of interaction
        pdata->int_min = handled;
        pdata->is_pressure = handled;
    }
    else
    {
        // find the collision volumes at 10% overlap
        oct_bb_t exp1, exp2;

        float tmp_min, tmp_max;

        tmp_min = d->tmin;
        tmp_max = d->tmin + ( d->tmax - d->tmin ) * 0.1f;

        // determine the expanded collision volumes for both objects
        phys_expand_oct_bb( &cv_prt_min, pdata->pprt->vel.v, tmp_min, tmp_max, &exp1 );
        phys_expand_oct_bb( &cv_chr,     pdata->pchr->vel.v, tmp_min, tmp_max, &exp2 );

        // use "collision" to determine the normal and overlap
        handled = phys_estimate_collision_normal( &exp1, &exp2, exponent, &odepth, pdata->nrm.v, &( pdata->depth_min ) );

        // tag the type of interaction
        pdata->int_min      = handled;
        pdata->is_collision = handled;
    }

    if ( !handled )
    {
        if ( d->tmin <= 0.0f || ABS( d->tmin ) > 1e6 || ABS( d->tmax ) > 1e6 )
        {
            // use "pressure" to determine the normal and overlap
            phys_estimate_pressure_normal( &cv_prt_max, &cv_chr, exponent, &odepth, pdata->nrm.v, &( pdata->depth_max ) );

            handled = btrue;
            if ( d->tmin <= 0.0f )
            {
                handled = pdata->depth_max > 0.0f;
            }

            // tag the type of interaction
            pdata->int_max     = handled;
            pdata->is_pressure = handled;
        }
        else
        {
            // find the collision volumes at 10% overlap
            oct_bb_t exp1, exp2;

            float tmp_min, tmp_max;

            tmp_min = d->tmin;
            tmp_max = d->tmin + ( d->tmax - d->tmin ) * 0.1f;

            // determine the expanded collision volumes for both objects
            phys_expand_oct_bb( &cv_prt_max, pdata->pprt->vel.v, tmp_min, tmp_max, &exp1 );
            phys_expand_oct_bb( &cv_chr,     pdata->pchr->vel.v, tmp_min, tmp_max, &exp2 );

            // use "collision" to determine the normal and overlap
            handled = phys_estimate_collision_normal( &exp1, &exp2, exponent, &odepth, pdata->nrm.v, &( pdata->depth_max ) );

            // tag the type of interaction
            pdata->int_max      = handled;
            pdata->is_collision = handled;
        }
    }

    // check for possible platform interactions
    //if( pdata->pchr->platform && !INGAME_CHR( pdata->pprt->attachedto_ref ) )
    //{
    //    // find the interaction strength
    //    pdata->plat_lerp = (cv_prt_min.mins[OCT_Z] - cv_chr.maxs[OCT_Z]) / PLATTOLERANCE;
    //    pdata->plat_lerp = CLIP( pdata->plat_lerp, -1.0f, 1.0f );

    //    if( pdata->plat_lerp < 1.0f )
    //    {
    //        bool_t   plat_retval;
    //        oct_bb_t plat_cv;

    //        // construct a special collision volume for the platform
    //        oct_bb_copy( &plat_cv, &cv_chr );
    //        plat_cv.maxs[OCT_Z] += PLATTOLERANCE;

    //        // is there any overlap?
    //        plat_retval = get_depth_close_2( &cv_prt_min, &plat_cv, btrue, odepth );

    //        // tag it as a platform interaction
    //        pdata->int_plat = plat_retval;

    //        if( !handled && plat_retval )
    //        {
    //            handled = pdata->int_plat;

    //            // if there is overlap, calculate a normal
    //            chr_getMatUp( pdata->pchr, pdata->nrm.v );
    //            fvec3_self_normalize( pdata->nrm.v );
    //        }
    //    }
    //}

    return handled;
}

//--------------------------------------------------------------------------------------------
bool_t do_prt_platform_physics( chr_prt_collsion_data_t * pdata )
{
    /// @details BB@> handle the particle interaction with a platform it is not attached "on".
    ///               @note gravity is not handled here

    bool_t plat_collision = bfalse;
    bool_t z_collide, was_z_collide;

    if ( NULL == pdata ) return bfalse;

    // is the platform a platform?
    if ( !pdata->pchr->platform ) return bfalse;

    // can the particle interact with it?
    if ( INGAME_CHR( pdata->pprt->attachedto_ref ) ) return bfalse;

    // this is handled elsewhere
    if ( GET_REF_PCHR( pdata->pchr ) == pdata->pprt->onwhichplatform_ref ) return bfalse;

    // Test to see whether the particle is in the right position to interact with the platform.
    // You have to be closer to a platform to interact with it then for a general object,
    // but the vertical distance is looser.
    plat_collision = test_interaction_close_1( &( pdata->pchr->chr_max_cv ), chr_get_pos_v( pdata->pchr ), pdata->pprt->bump_padded, prt_get_pos_v( pdata->pprt ), btrue );

    if ( !plat_collision ) return bfalse;

    // the only way to get to this point is if the two objects don't collide
    // but they are within the PLATTOLERANCE of each other in the z direction
    // it is a valid platform. now figure out the physics

    // are they colliding for the first time?
    z_collide     = ( pdata->pprt->pos.z < pdata->pchr->pos.z + pdata->pchr->chr_max_cv.maxs[OCT_Z] ) && ( pdata->pprt->pos.z > pdata->pchr->pos.z + pdata->pchr->chr_max_cv.mins[OCT_Z] );
    was_z_collide = ( pdata->pprt->pos.z - pdata->pprt->vel.z < pdata->pchr->pos.z + pdata->pchr->chr_max_cv.maxs[OCT_Z] - pdata->pchr->vel.z ) && ( pdata->pprt->pos.z - pdata->pprt->vel.z  > pdata->pchr->pos.z + pdata->pchr->chr_max_cv.mins[OCT_Z] );

    if ( z_collide && !was_z_collide )
    {
        // Particle is falling onto the platform
        pdata->pprt->phys.apos_plat.z += pdata->pchr->pos.z + pdata->pchr->chr_max_cv.maxs[OCT_Z] - pdata->pprt->pos.z;
        pdata->pprt->phys.avel.z      += ( pdata->pchr->pos.z - pdata->pprt->vel.z ) * ( 1.0f + pdata->ppip->dampen );

        // This should prevent raindrops from stacking up on the top of trees and other
        // objects
        if ( pdata->ppip->end_ground && pdata->pchr->platform )
        {
            pdata->terminate_particle = btrue;
        }

        plat_collision = btrue;
    }
    else if ( z_collide && was_z_collide )
    {
        // colliding this time and last time. particle is *embedded* in the platform
        pdata->pprt->phys.apos_plat.z += pdata->pchr->pos.z + pdata->pchr->chr_max_cv.maxs[OCT_Z] - pdata->pprt->pos.z;

        if ( pdata->pprt->vel.z - pdata->pchr->vel.z < 0 )
        {
            pdata->pprt->phys.avel.z += pdata->pchr->vel.z * pdata->ppip->dampen + platstick * pdata->pchr->vel.z - pdata->pprt->vel.z;
        }
        else
        {
            pdata->pprt->phys.avel.z += pdata->pprt->vel.z * ( 1.0f - platstick ) + pdata->pchr->vel.z * platstick - pdata->pprt->vel.z;
        }
        pdata->pprt->phys.avel.x += pdata->pprt->vel.x * ( 1.0f - platstick ) + pdata->pchr->vel.x * platstick - pdata->pprt->vel.x;
        pdata->pprt->phys.avel.y += pdata->pprt->vel.y * ( 1.0f - platstick ) + pdata->pchr->vel.y * platstick - pdata->pprt->vel.y;

        plat_collision = btrue;
    }
    else
    {
        // not colliding this time or last time. particle is just near the platform
        float lerp_z = ( pdata->pprt->pos.z - ( pdata->pchr->pos.z + pdata->pchr->chr_max_cv.maxs[OCT_Z] ) ) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1, 1 );

        if ( lerp_z > 0.0f )
        {
            pdata->pprt->phys.avel.x += ( pdata->pchr->vel.x - pdata->pprt->vel.x ) * platstick * lerp_z;
            pdata->pprt->phys.avel.y += ( pdata->pchr->vel.y - pdata->pprt->vel.y ) * platstick * lerp_z;
            pdata->pprt->phys.avel.z += ( pdata->pchr->vel.z - pdata->pprt->vel.z ) * platstick * lerp_z;

            plat_collision = btrue;
        }
    }

    return plat_collision;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_deflect( chr_prt_collsion_data_t * pdata )
{
    bool_t prt_deflected = bfalse;

    bool_t chr_is_invictus, chr_can_deflect;
    bool_t prt_wants_deflection;
    FACING_T direction;

    if ( NULL == pdata ) return bfalse;

    /// @note ZF@> Simply ignore characters with invictus for now, it causes some strange effects
    if ( pdata->pchr->invictus ) return btrue;

    // find the "attack direction" of the particle
    direction = vec_to_facing( pdata->pchr->pos.x - pdata->pprt->pos.x, pdata->pchr->pos.y - pdata->pprt->pos.y );
    direction = pdata->pchr->ori.facing_z - direction + ATK_BEHIND;

    // shield block?
    chr_is_invictus = is_invictus_direction( direction, GET_REF_PCHR( pdata->pchr ), pdata->ppip->damfx );

    // determine whether the character is magically protected from missile attacks
    prt_wants_deflection  = ( MISSILE_NORMAL != pdata->pchr->missiletreatment ) &&
                            ( pdata->pprt->owner_ref != GET_REF_PCHR( pdata->pchr ) ) && !pdata->ppip->bump_money;

    chr_can_deflect = ( 0 != pdata->pchr->damage_timer ) && ( pdata->max_damage > 0 );

    // try to deflect the particle
    prt_deflected = bfalse;
    pdata->mana_paid = bfalse;
    if ( chr_is_invictus || ( prt_wants_deflection && chr_can_deflect ) )
    {
        //Initialize for the billboard
        const float lifetime = 3;
        SDL_Color text_color = {0xFF, 0xFF, 0xFF, 0xFF};
        GLXvector4f tint  = { 0.0f, 0.75f, 1.00f, 1.00f };

        // magically deflect the particle or make a ricochet if the character is invictus
        int treatment;

        treatment     = MISSILE_DEFLECT;
        prt_deflected = btrue;
        if ( prt_wants_deflection )
        {
            treatment = pdata->pchr->missiletreatment;
            pdata->mana_paid = cost_mana( pdata->pchr->missilehandler, pdata->pchr->missilecost << 8, pdata->pprt->owner_ref );
            prt_deflected = pdata->mana_paid;
        }

        if ( prt_deflected )
        {
            // Treat the missile
            if ( treatment == MISSILE_DEFLECT )
            {
                // Deflect the incoming ray off the normal
                pdata->vimpulse.x -= 2.0f * pdata->vdiff_para.x;
                pdata->vimpulse.y -= 2.0f * pdata->vdiff_para.y;
                pdata->vimpulse.z -= 2.0f * pdata->vdiff_para.z;

                // the ricochet is not guided
                pdata->ppip->homing     = bfalse;
            }
            else if ( treatment == MISSILE_REFLECT )
            {
                // Reflect it back in the direction it came
                pdata->vimpulse.x -= 2.0f * pdata->vdiff.x;
                pdata->vimpulse.y -= 2.0f * pdata->vdiff.y;
                pdata->vimpulse.z -= 2.0f * pdata->vdiff.z;

                // Change the owner of the missile
                pdata->pprt->team       = pdata->pchr->team;
                pdata->pprt->owner_ref  = GET_REF_PCHR( pdata->pchr );
            }

            //Blocked!
            spawn_defense_ping( pdata->pchr, pdata->pprt->owner_ref );
            chr_make_text_billboard( GET_REF_PCHR( pdata->pchr ), "Blocked!", text_color, tint, lifetime, bb_opt_all );

            //If the attack was blocked by a shield, then check if the block caused a knockback
            if ( chr_is_invictus && ACTION_IS_TYPE( pdata->pchr->inst.action_which, P ) )
            {
                bool_t using_shield;
                CHR_REF item;

                // Figure out if we are really using a shield or if it is just a invictus frame
                using_shield = bfalse;
                item         = MAX_CHR;

                // Check right hand for a shield
                if ( !using_shield )
                {
                    item = pdata->pchr->holdingwhich[SLOT_RIGHT];
                    if ( INGAME_CHR( item ) && pdata->pchr->ai.lastitemused == item )
                    {
                        using_shield = btrue;
                    }
                }

                // Check left hand for a shield
                if ( !using_shield )
                {
                    item = pdata->pchr->holdingwhich[SLOT_LEFT];
                    if ( INGAME_CHR( item ) && pdata->pchr->ai.lastitemused == item )
                    {
                        using_shield = btrue;
                    }
                }

                // Now we have the block rating and know the enemy
                if ( INGAME_CHR( pdata->pprt->owner_ref ) && using_shield )
                {
                    int   total_block_rating;
                    IPair rand_pair;

                    chr_t *pshield   = ChrList.lst + item;
                    chr_t *pattacker = ChrList.lst + pdata->pprt->owner_ref;

                    //use the character block skill plus the base block rating of the shield and adjust for strength
                    total_block_rating = chr_get_skill( pdata->pchr, MAKE_IDSZ( 'B', 'L', 'O', 'C' ) );
                    total_block_rating += chr_get_skill( pshield, MAKE_IDSZ( 'B', 'L', 'O', 'C' ) );

                    // -4% per attacker strength
                    total_block_rating -= 4 * SFP8_TO_SINT( pattacker->strength );

                    // +2% per defender strength
                    total_block_rating += 2 * SFP8_TO_SINT( pdata->pchr->strength );

                    //Now determine the result of the block
                    rand_pair.base = 0;
                    rand_pair.rand = 100;
                    if ( generate_irand_pair( rand_pair ) <= total_block_rating )
                    {
                        // Defender won, the block holds
                        // Add a small stun to the attacker = 40/50 (0.8 seconds)
                        pattacker->reload_timer += 40;
                    }
                    else
                    {
                        // Attacker broke the block and batters away the shield
                        // Time to raise shield again = 40/50 (0.8 seconds)
                        pdata->pchr->reload_timer += 40;
                        sound_play_chunk( pdata->pchr->pos, g_wavelist[GSND_SHIELDBLOCK] );
                    }
                }
            }

        }
    }

    return prt_deflected;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_recoil( chr_prt_collsion_data_t * pdata )
{
    /// @details BB@> make the character and particle recoil from the collision
    float chr_mass, prt_mass;
    float chr_recoil, prt_recoil;

    float attack_factor;

    if ( NULL == pdata ) return bfalse;

    if ( 0.0f == fvec3_length_abs( pdata->vimpulse.v ) &&
         0.0f == fvec3_length_abs( pdata->pimpulse.v ) )
    {
        return btrue;
    }

    if ( !pdata->ppip->allowpush ) return bfalse;

    // do the reaction force of the particle on the character

    // determine how much the attack is "felt"
    attack_factor = 1.0f;
    if ( DAMAGE_CRUSH == pdata->pprt->damagetype )
    {
        // very blunt type of attack, the maximum effect
        attack_factor = 1.0f;
    }
    else if ( DAMAGE_POKE == pdata->pprt->damagetype )
    {
        // very focussed type of attack, the minimum effect
        attack_factor = 0.5f;
    }
    else
    {
        // all other damage types are in the middle
        attack_factor = INV_SQRT_TWO;
    }

    // get some type of mass info for the particle
    get_chr_mass( pdata->pchr, &chr_mass );
    get_prt_mass( pdata->pprt, pdata->pchr, &prt_mass );

    // get recoil factors for the masses
    get_recoil_factors( chr_mass, prt_mass, &chr_recoil, &prt_recoil );

    // now, we have the particle's impulse and mass
    // Do the impulse to the object that was hit
    // If the particle was magically deflected, there is no rebound on the target
    if ( !pdata->mana_paid )
    {
        fvec3_t tmp_impulse;

        // calculate the "impulse" to the character
        tmp_impulse = fvec3_scale( pdata->vimpulse.v, -chr_recoil * attack_factor * pdata->block_factor );
        fvec3_self_sum( pdata->pchr->phys.avel.v, tmp_impulse.v );

        tmp_impulse = fvec3_scale( pdata->pimpulse.v, -chr_recoil * attack_factor * pdata->block_factor );
        fvec3_self_sum( pdata->pchr->phys.apos_coll.v, tmp_impulse.v );
    }

    // if the particle is attached to a weapon, the particle can force the
    // weapon (actually, the weapon's holder) to rebound.
    if ( INGAME_CHR( pdata->pprt->attachedto_ref ) )
    {
        chr_t * pholder;
        chr_t * pattached;
        CHR_REF iholder;

        // get the attached mass
        pattached = ChrList.lst + pdata->pprt->attachedto_ref;

        // assume the worst
        pholder = NULL;

        // who is holding the weapon?
        iholder = chr_get_lowest_attachment( pdata->pprt->attachedto_ref, bfalse );
        if ( INGAME_CHR( iholder ) )
        {
            pholder = ChrList.lst + iholder;
        }
        else
        {
            iholder = chr_get_lowest_attachment( pdata->pprt->owner_ref, bfalse );
            if ( INGAME_CHR( iholder ) )
            {
                pholder = ChrList.lst + iholder;
            }
        }

        {
            fvec3_t tmp_impulse;

            float holder_mass, total_mass;
            float attached_mass;
            float tmp_holder_recoil, tmp_prt_recoil, holder_recoil;

            holder_mass = 0.0f;
            if (( NULL != pholder ) && ( iholder != pdata->pprt->attachedto_ref ) )
            {
                get_chr_mass( pholder, &holder_mass );
            }

            get_chr_mass( pattached, &attached_mass );

            total_mass = ABS( holder_mass ) + ABS( attached_mass );
            if ( holder_mass < 0.0f ||  attached_mass < 0.0f )
            {
                total_mass = -total_mass;
            }

            get_recoil_factors( total_mass, prt_mass, &tmp_holder_recoil, &tmp_prt_recoil );

            // get the actual holder recoil
            holder_recoil = tmp_holder_recoil * attack_factor;

            // in the SAME direction as the particle
            tmp_impulse = fvec3_scale( pdata->vimpulse.v, holder_recoil );
            fvec3_self_sum( pholder->phys.avel.v, tmp_impulse.v );

            tmp_impulse = fvec3_scale( pdata->pimpulse.v, holder_recoil );
            fvec3_self_sum( pholder->phys.apos_coll.v, tmp_impulse.v );
        }
    }

    // apply the impulse to the particle velocity
    if ( MAX_CHR == pdata->pprt->attachedto_ref )
    {
        fvec3_t tmp_impulse;

        tmp_impulse = fvec3_scale( pdata->vimpulse.v, prt_recoil );
        fvec3_self_sum( pdata->pprt->phys.avel.v, tmp_impulse.v );

        tmp_impulse = fvec3_scale( pdata->pimpulse.v, prt_recoil );
        fvec3_self_sum( pdata->pprt->phys.apos_coll.v, tmp_impulse.v );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_damage( chr_prt_collsion_data_t * pdata )
{
    ENC_REF ienc_now, ienc_nxt;
    size_t  ienc_count;

    bool_t prt_needs_impact;

    chr_t * powner = NULL;
    cap_t * powner_cap = NULL;

    if ( NULL == pdata ) return bfalse;

    if ( INGAME_CHR( pdata->pprt->owner_ref ) )
    {
        powner = ChrList.lst + pdata->pprt->owner_ref;
        powner_cap = pro_get_pcap( powner->profile_ref );
    }

    // clean up the enchant list before doing anything
    cleanup_character_enchants( pdata->pchr );

    // Check all enchants to see if they are removed
    ienc_now = pdata->pchr->firstenchant;
    ienc_count = 0;
    while (( MAX_ENC != ienc_now ) && ( ienc_count < MAX_ENC ) )
    {
        ienc_nxt = EncList.lst[ienc_now].nextenchant_ref;

        if ( enc_is_removed( ienc_now, pdata->pprt->profile_ref ) )
        {
            remove_enchant( ienc_now, NULL );
        }

        ienc_now = ienc_nxt;
        ienc_count++;
    }
    if ( ienc_count >= MAX_ENC ) log_error( "%s - bad enchant loop\n", __FUNCTION__ );

    // Steal some life
    if ( pdata->pprt->lifedrain > 0 )
    {
        int val_stt, val_end;
        int drain;

        val_stt = pdata->pchr->life;
        val_end = MAX( 1, val_stt - pdata->pprt->lifedrain );
        drain   = val_stt - val_end;

        // remove the drain from the character that was hit
        pdata->pchr->life -= drain;

        // add it to the "caster"
        if ( NULL != powner )
        {
            powner->life = MIN( powner->life + drain, powner->lifemax );
        }
    }

    // Steal some mana
    if ( pdata->pprt->manadrain > 0 )
    {
        int val_stt, val_end;
        int drain;

        val_stt = pdata->pchr->mana;
        val_end = MAX( 1, val_stt - pdata->pprt->manadrain );
        drain   = val_stt - val_end;

        // remove the drain from the character that was hit
        pdata->pchr->mana -= drain;

        // add it to the "caster"
        if ( NULL != powner )
        {
            powner->mana = MIN( powner->mana + drain, powner->manamax );
        }
    }

    // Do grog
    if ( pdata->ppip->grog_time > 0 && pdata->pcap->canbegrogged )
    {
        SET_BIT( pdata->pchr->ai.alert, ALERTIF_CONFUSED );
        pdata->pchr->grog_timer = MAX( pdata->pchr->grog_timer, pdata->ppip->grog_time );
    }

    // Do daze
    if ( pdata->ppip->daze_time > 0 && pdata->pcap->canbedazed )
    {
        SET_BIT( pdata->pchr->ai.alert, ALERTIF_CONFUSED );
        pdata->pchr->daze_timer = MAX( pdata->pchr->daze_timer, pdata->ppip->daze_time );
    }

    //---- Damage the character, if necessary
    if ( 0 != ABS( pdata->pprt->damage.base ) + ABS( pdata->pprt->damage.rand ) )
    {
        prt_needs_impact = pdata->ppip->rotatetoface || INGAME_CHR( pdata->pprt->attachedto_ref );
        if ( NULL != powner_cap && powner_cap->isranged ) prt_needs_impact = btrue;

        // DAMFX_ARRO means that it only does damage to the one it's attached to
        if ( HAS_NO_BITS( pdata->ppip->damfx, DAMFX_ARRO ) && ( !prt_needs_impact || pdata->is_impact ) )
        {
            FACING_T direction;
            IPair loc_damage = pdata->pprt->damage;

            direction = vec_to_facing( pdata->pprt->vel.x , pdata->pprt->vel.y );
            direction = pdata->pchr->ori.facing_z - direction + ATK_BEHIND;

            //These things only apply if the particle has an owner
            if ( NULL != powner )
            {
                CHR_REF item;

                // Apply intelligence/wisdom bonus damage for particles with the [IDAM] and [WDAM] expansions (Low ability gives penality)
                // +2% bonus for every point of intelligence and/or wisdom above 14. Below 14 gives -2% instead!
                if ( pdata->ppip->intdamagebonus )
                {
                    float percent;
                    percent = (( FP8_TO_INT( powner->intelligence ) ) - 14 ) * 2;
                    percent /= 100;
                    loc_damage.base *= 1.00f + percent;
                    loc_damage.rand *= 1.00f + percent;
                }

                if ( pdata->ppip->wisdamagebonus )
                {
                    float percent;
                    percent = ( FP8_TO_INT( powner->wisdom ) - 14 ) * 2;
                    percent /= 100;
                    loc_damage.base *= 1.00f + percent;
                    loc_damage.rand *= 1.00f + percent;
                }

                // Notify the attacker of a scored hit
                SET_BIT( powner->ai.alert, ALERTIF_SCOREDAHIT );
                powner->ai.hitlast = GET_REF_PCHR( pdata->pchr );

                //Tell the weapons who the attacker hit last
                item = powner->holdingwhich[SLOT_LEFT];
                if ( INGAME_CHR( item ) )
                {
                    ChrList.lst[item].ai.hitlast = GET_REF_PCHR( pdata->pchr );
                    if ( powner->ai.lastitemused == item ) SET_BIT( ChrList.lst[item].ai.alert, ALERTIF_SCOREDAHIT );
                }

                item = powner->holdingwhich[SLOT_RIGHT];
                if ( INGAME_CHR( item ) )
                {
                    ChrList.lst[item].ai.hitlast = GET_REF_PCHR( pdata->pchr );
                    if ( powner->ai.lastitemused == item ) SET_BIT( ChrList.lst[item].ai.alert, ALERTIF_SCOREDAHIT );
                }
            }

            // handle vulnerabilities, double the damage
            if ( chr_has_vulnie( GET_REF_PCHR( pdata->pchr ), pdata->pprt->profile_ref ) )
            {
                //Double the damage
                loc_damage.base = ( loc_damage.base << 1 );
                loc_damage.rand = ( loc_damage.rand << 1 ) | 1;

                SET_BIT( pdata->pchr->ai.alert, ALERTIF_HITVULNERABLE );
            }

            // Damage the character
            pdata->actual_damage = damage_character( GET_REF_PCHR( pdata->pchr ), direction, loc_damage, pdata->pprt->damagetype, pdata->pprt->team, pdata->pprt->owner_ref, pdata->ppip->damfx, bfalse );
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_impulse( chr_prt_collsion_data_t * pdata )
{
    // estimate the impulse on the particle

    bool_t did_something = bfalse;

    if ( NULL == pdata ) return bfalse;

    if ( !pdata->ppip->allowpush ) return bfalse;

    // the impulse due to particle damage
    if ( pdata->is_impact && pdata->prt_damages_chr )
    {
        int left_over_damage;

        did_something = btrue;

        left_over_damage = 0;
        if ( ABS( pdata->actual_damage ) < ABS( pdata->max_damage ) )
        {
            left_over_damage = ABS( pdata->max_damage ) - ABS( pdata->actual_damage );
        }

        if ( 0 == pdata->max_damage )
        {
            pdata->block_factor = 0.0f;
        }
        else
        {
            pdata->block_factor = ( float )left_over_damage / ( float )ABS( pdata->max_damage );
            pdata->block_factor = pdata->block_factor / ( 1.0f + pdata->block_factor );
        }

        if ( 0.0f == pdata->block_factor )
        {
            // the simple case (particle comes to a stop)
            pdata->vimpulse.x -= pdata->pprt->vel.x;
            pdata->vimpulse.y -= pdata->pprt->vel.y;
            pdata->vimpulse.z -= pdata->pprt->vel.z;
        }
        else if ( 0.0f != pdata->dot )
        {
            float sgn = SGN( pdata->dot );

            pdata->vimpulse.x += -sgn * ( 1.0f + pdata->block_factor ) * pdata->vdiff_perp.x;
            pdata->vimpulse.y += -sgn * ( 1.0f + pdata->block_factor ) * pdata->vdiff_perp.y;
            pdata->vimpulse.z += -sgn * ( 1.0f + pdata->block_factor ) * pdata->vdiff_perp.z;
        }
    }

    // the "pressure" impulse due to overlap
    if ( pdata->int_min && pdata->depth_min > 0.0f && pdata->ichr != pdata->pprt->owner_ref )
    {
        fvec3_t tmp_imp;

        // is the normal reversed?
        tmp_imp = fvec3_scale( pdata->nrm.v, pdata->depth_min );

        fvec3_self_sum( pdata->pimpulse.v, tmp_imp.v );

        did_something = btrue;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_bump( chr_prt_collsion_data_t * pdata )
{
    bool_t prt_belongs_to_chr;
    bool_t prt_hates_chr, prt_attacks_chr, prt_hateonly;
    bool_t valid_onlydamagefriendly;
    bool_t valid_friendlyfire;
    bool_t valid_onlydamagehate;

    if ( NULL == pdata ) return bfalse;

    // always allow valid reaffirmation
    if (( pdata->pchr->reaffirm_damagetype < DAMAGE_COUNT ) &&
        ( pdata->pprt->damagetype < DAMAGE_COUNT ) &&
        ( pdata->pchr->reaffirm_damagetype < pdata->pprt->damagetype ) )
    {
        return btrue;
    }

    // if the particle was deflected, then it can't bump the character
    if ( pdata->pchr->invictus || pdata->pprt->attachedto_ref == GET_REF_PCHR( pdata->pchr ) ) return bfalse;

    prt_belongs_to_chr = ( GET_REF_PCHR( pdata->pchr ) == pdata->pprt->owner_ref );

    if ( !prt_belongs_to_chr )
    {
        // no simple owner relationship. Check for something deeper.
        CHR_REF prt_owner = prt_get_iowner( GET_REF_PPRT( pdata->pprt ), 0 );
        if ( INGAME_CHR( prt_owner ) )
        {
            CHR_REF chr_wielder = chr_get_lowest_attachment( GET_REF_PCHR( pdata->pchr ), btrue );
            CHR_REF prt_wielder = chr_get_lowest_attachment( prt_owner, btrue );

            if ( !INGAME_CHR( chr_wielder ) ) chr_wielder = GET_REF_PCHR( pdata->pchr );
            if ( !INGAME_CHR( prt_wielder ) ) prt_wielder = prt_owner;

            prt_belongs_to_chr = ( chr_wielder == prt_wielder );
        }
    }

    // does the particle team hate the character's team
    prt_hates_chr = team_hates_team( pdata->pprt->team, pdata->pchr->team );

    // Only bump into hated characters?
    prt_hateonly = PipStack.lst[pdata->pprt->pip_ref].hateonly;
    valid_onlydamagehate = prt_hates_chr && PipStack.lst[pdata->pprt->pip_ref].hateonly;

    // allow neutral particles to attack anything
    prt_attacks_chr = prt_hates_chr || (( TEAM_NULL != pdata->pchr->team ) && ( TEAM_NULL == pdata->pprt->team ) );

    // this is the onlydamagefriendly condition from the particle search code
    valid_onlydamagefriendly = ( pdata->ppip->onlydamagefriendly && pdata->pprt->team == pdata->pchr->team ) ||
                               ( !pdata->ppip->onlydamagefriendly && prt_attacks_chr );

    // I guess "friendly fire" does not mean "self fire", which is a bit unfortunate.
    valid_friendlyfire = ( pdata->ppip->friendlyfire && !prt_hates_chr && !prt_belongs_to_chr ) ||
                         ( !pdata->ppip->friendlyfire && prt_attacks_chr );

    pdata->prt_bumps_chr =  valid_friendlyfire || valid_onlydamagefriendly || valid_onlydamagehate;

    return pdata->prt_bumps_chr;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_handle_bump( chr_prt_collsion_data_t * pdata )
{
    if ( NULL == pdata || !pdata->prt_bumps_chr ) return bfalse;

    if ( !pdata->prt_bumps_chr ) return bfalse;

    // Catch on fire
    spawn_bump_particles( GET_REF_PCHR( pdata->pchr ), GET_REF_PPRT( pdata->pprt ) );

    //handle some special particle interactions
    if ( pdata->ppip->end_bump )
    {
        if ( pdata->ppip->bump_money )
        {
            chr_t * pcollector = pdata->pchr;

            // Let mounts collect money for their riders
            if ( pdata->pchr->ismount && INGAME_CHR( pdata->pchr->holdingwhich[SLOT_LEFT] ) )
            {
                pcollector = ChrList.lst + pdata->pchr->holdingwhich[SLOT_LEFT];

                // if the mount's rider can't get money, the mount gets to keep the money!
                if ( !pcollector->cangrabmoney )
                {
                    pcollector = pdata->pchr;
                }
            }

            if ( pcollector->cangrabmoney && pcollector->alive && 0 == pcollector->damage_timer && pcollector->money < MAXMONEY )
            {
                pcollector->money += pdata->ppip->bump_money;
                pcollector->money = CLIP( pcollector->money, 0, MAXMONEY );

                // the coin disappears when you pick it up
                pdata->terminate_particle = btrue;
            }
        }
        else
        {
            // Only hit one character, not several
            pdata->terminate_particle = btrue;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_init( CHR_REF ichr, PRT_REF iprt, chr_prt_collsion_data_t * pdata )
{
    if ( NULL == pdata ) return bfalse;

    memset( pdata, 0, sizeof( *pdata ) );

    if ( !INGAME_PRT( iprt ) ) return bfalse;
    pdata->iprt = iprt;
    pdata->pprt = PrtList.lst + iprt;

    // make sure that it is on
    if ( !INGAME_CHR( ichr ) ) return bfalse;
    pdata->ichr = ichr;
    pdata->pchr = ChrList.lst + ichr;

    // initialize the collision data
    pdata->pcap = pro_get_pcap( pdata->pchr->profile_ref );
    if ( NULL == pdata->pcap ) return bfalse;

    if ( !LOADED_PIP( pdata->pprt->pip_ref ) ) return bfalse;
    pdata->ppip = PipStack.lst + pdata->pprt->pip_ref;

    // estimate the maximum possible "damage" from this particle
    // other effects can magnify this number, like vulnerabilities
    // or DAMFX_* bits
    pdata->max_damage = ABS( pdata->pprt->damage.base ) + ABS( pdata->pprt->damage.rand );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision( CoNode_t * d )
{
    /// @details BB@> this funciton goes through all of the steps to handle character-particle
    ///               interactions. A basic interaction has been detected. This needs to be refined
    ///               and then handled. The function returns bfalse if the basic interaction was wrong
    ///               or if the interaction had no effect.
    ///
    /// @note This function is a little more complicated than the character-character case because
    ///       of the friend-foe logic as well as the damage and other special effects that particles can do.

    bool_t retval = bfalse;

    bool_t prt_deflected;
    bool_t prt_can_hit_chr;

    chr_prt_collsion_data_t cn_data;
    bool_t intialized;

    // valid node?
    if ( NULL == d ) return bfalse;

    if ( MAX_CHR != d->chra && MAX_PRT != d->prtb )
    {
        // character was first
        intialized = do_chr_prt_collision_init( d->chra, d->prtb, &cn_data );
    }
    else if ( MAX_CHR != d->chrb && MAX_PRT != d->prta )
    {
        // particle was first
        intialized = do_chr_prt_collision_init( d->chrb, d->prta, &cn_data );
    }
    else
    {
        // not a valid interaction
        intialized = bfalse;

        // in here to keep the compiler from complaining
        memset( &cn_data, 0, sizeof( cn_data ) );
    }

    if ( !intialized ) return bfalse;

    // ignore dead characters
    if ( !cn_data.pchr->alive ) return bfalse;

    // skip objects that are inside inventories
    if ( cn_data.pchr->pack.is_packed ) return bfalse;

    // if the particle is attached to this character, ignore a "collision"
    if ( MAX_CHR != cn_data.pprt->attachedto_ref && cn_data.ichr == cn_data.pprt->attachedto_ref )
    {
        return bfalse;
    }

    // is there any collision at all?
    if ( !do_chr_prt_collision_get_details( d, &cn_data ) )
    {
        return bfalse;
    }
    else
    {
        // help classify impacts

        if ( cn_data.is_pressure )
        {
            // on the odd chance that we want to use the pressure
            // algorithm for an obvious collision....
            if ( d->tmin > 0.0f ) cn_data.is_impact = btrue;

            // if, say, a melee attack particle is and already intersects its target
            if ( 0 == cn_data.pprt->obj_base.update_count ) cn_data.is_impact = btrue;
        }

        if ( cn_data.is_collision )
        {
            cn_data.is_impact = btrue;
        }
    }

    // if there is no collision, no point in going farther
    if ( !cn_data.int_min && !cn_data.int_max /* && !cn_data.int_plat */ ) return bfalse;

    // if the particle is not actually hitting the object, then limit the
    // interaction to 2d
    if ( cn_data.int_max && !cn_data.int_min )
    {
        // do not re-normalize this vector
        cn_data.nrm.z = 0.0f;
    }

    // find the relative velocity
    cn_data.vdiff = fvec3_sub( cn_data.pchr->vel.v, cn_data.pprt->vel.v );

    // decompose the relative velocity parallel and perpendicular to the surface normal
    cn_data.dot = fvec3_decompose( cn_data.vdiff.v, cn_data.nrm.v, cn_data.vdiff_perp.v, cn_data.vdiff_para.v );

    // handle particle deflection
    prt_deflected = bfalse;
    if ( cn_data.int_min || cn_data.int_max )
    {
        // determine whether the particle is deflected by the character
        prt_deflected = do_chr_prt_collision_deflect( &cn_data );
        if ( prt_deflected )
        {
            retval = btrue;
        }
    }

    // refine the logic for a particle to hit a character
    prt_can_hit_chr = do_chr_prt_collision_bump( &cn_data );

    // Torches and such are marked as invulnerable, so the particle is always deflected.
    // make a special case for reaffirmation
    if (( cn_data.int_min || cn_data.int_max ) && 0 == cn_data.pchr->damage_timer )
    {
        // Check reaffirmation of particles
        if ( cn_data.pchr->reaffirm_damagetype == cn_data.pprt->damagetype )
        {
            // This prevents items in shops from being burned
            if ( !cn_data.pchr->isshopitem )
            {
                if ( 0 != reaffirm_attached_particles( cn_data.ichr ) )
                {
                    retval = btrue;
                }
            }
        }
    }

    // do "damage" to the character
    if (( cn_data.int_min || cn_data.int_max ) && !prt_deflected && 0 == cn_data.pchr->damage_timer && prt_can_hit_chr )
    {
        // we can't even get to this point if the character is completely invulnerable (invictus)
        // or can't be damaged this round
        cn_data.prt_damages_chr = do_chr_prt_collision_damage( &cn_data );
        if ( cn_data.prt_damages_chr )
        {
            retval = btrue;
        }
    }

    // calculate the impulse.
    if (( cn_data.int_min || cn_data.int_max ) && cn_data.ppip->allowpush )
    {
        do_chr_prt_collision_impulse( &cn_data );
    }

    // make the character and particle recoil from the collision
    if ( fvec3_length_abs( cn_data.vimpulse.v ) > 0.0f ||
         fvec3_length_abs( cn_data.pimpulse.v ) > 0.0f )
    {
        if ( do_chr_prt_collision_recoil( &cn_data ) )
        {
            retval = btrue;
        }
    }

    // handle a couple of special cases
    if ( cn_data.prt_bumps_chr )
    {
        if ( do_chr_prt_collision_handle_bump( &cn_data ) )
        {
            retval = btrue;
        }
    }

    // platform interaction. do this last, and only if there is no other interaction
    //if ( cn_data.int_plat && !cn_data.int_max && !cn_data.int_min )
    //{
    //    cn_data.int_plat = do_prt_platform_physics( &cn_data );
    //}

    // terminate the particle if needed
    if ( cn_data.terminate_particle )
    {
        end_one_particle_in_game( cn_data.iprt );
        retval = btrue;
    }

    return retval;
}
