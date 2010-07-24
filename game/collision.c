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

#include "char.inl"
#include "particle.inl"
#include "enchant.inl"
#include "profile.inl"
#include "physics.inl"

#include "log.h"
#include "hash.h"
#include "game.h"
#include "SDL_extensions.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAKE_HASH(AA,BB)         CLIP_TO_08BITS( ((AA) * 0x0111 + 0x006E) + ((BB) * 0x0111 + 0x006E) )

#define CHR_MAX_COLLISIONS       512*16
#define COLLISION_HASH_NODES     (CHR_MAX_COLLISIONS*2)
#define COLLISION_LIST_SIZE      256

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/// data block used to communicate between the different "modules" governing the character-particle collision
struct s_chr_prt_collsion_data
{
    // object parameters
    cap_t * pcap;
    pip_t * ppip;

    // collision parameters
    oct_vec_t odepth;
    float     dot;
    fvec3_t   nrm;

    // collision modifications
    bool_t   mana_paid;
    int      max_damage, actual_damage;
    fvec3_t  vdiff;

    // collision reaction
    fvec3_t impulse;
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
static bool_t add_chr_chr_interaction( CHashList_t * pclst, const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b, CoNode_ary_t * pcn_lst, HashNode_ary_t * phn_lst );
static bool_t add_chr_prt_interaction( CHashList_t * pclst, const CHR_REF by_reference ichr_a, const PRT_REF by_reference iprt_b, CoNode_ary_t * pcn_lst, HashNode_ary_t * phn_lst );

static bool_t detect_chr_chr_interaction_valid( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b );
static bool_t detect_chr_prt_interaction_valid( const CHR_REF by_reference ichr_a, const PRT_REF by_reference iprt_b );

//static bool_t detect_chr_chr_interaction( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b );
//static bool_t detect_chr_prt_interaction( const CHR_REF by_reference ichr_a, const PRT_REF by_reference iprt_b );

static bool_t do_chr_platform_detection( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b );
static bool_t do_prt_platform_detection( const CHR_REF by_reference ichr_a, const PRT_REF by_reference iprt_b );

static bool_t attach_chr_to_platform( chr_t * pchr, chr_t * pplat );
static bool_t attach_prt_to_platform( prt_t * pprt, chr_t * pplat );

static bool_t fill_interaction_list( CHashList_t * pclst, CoNode_ary_t * pcn_lst, HashNode_ary_t * phn_lst );
static bool_t fill_bumplists( obj_BSP_t * pbsp );

static bool_t bump_all_platforms( CoNode_ary_t * pcn_ary );
static bool_t bump_all_mounts( CoNode_ary_t * pcn_ary );
static bool_t bump_all_collisions( CoNode_ary_t * pcn_ary );

static bool_t do_mounts( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b );
static bool_t do_chr_platform_physics( chr_t * pitem, chr_t * pplat );
static float  estimate_chr_prt_normal( chr_t * pchr, prt_t * pprt, fvec3_base_t nrm, fvec3_base_t vdiff );
static bool_t do_chr_chr_collision( CoNode_t * d );

static bool_t do_chr_prt_collision_deflect( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_recoil( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_damage( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_bump( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_handle_bump( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_init( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata );
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
    n->prta = TOTAL_MAX_PRT;

    // the "collided with" objects
    n->chrb  = ( CHR_REF )MAX_CHR;
    n->prtb  = TOTAL_MAX_PRT;
    n->tileb = FANOFF;

    // intialize the time
    n->tmin = n->tmax = -1.0f;

    return n;
}

//--------------------------------------------------------------------------------------------
Uint8 CoNode_generate_hash( CoNode_t * coll )
{
    REF_T AA, BB;

    AA = ( Uint32 )( ~((Uint32)0) );
    if ( INGAME_CHR( coll->chra ) )
    {
        AA = REF_TO_INT( coll->chra );
    }
    else if ( INGAME_PRT( coll->prta ) )
    {
        AA = REF_TO_INT( coll->prta );
    }

    BB = ( Uint32 )( ~((Uint32)0) );
    if ( INGAME_CHR( coll->chrb ) )
    {
        BB = REF_TO_INT( coll->chra );
    }
    else if ( INGAME_PRT( coll->prtb ) )
    {
        BB = REF_TO_INT( coll->prta );
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

    // fort by final time second
    ftmp = pleft->tmax - pright->tmax;
    if ( ftmp <= 0.0f ) return -1;
    else if ( ftmp >= 0.0f ) return 1;

    itmp = ( signed )REF_TO_INT( pleft->chra ) - ( signed )REF_TO_INT( pright->chra );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )REF_TO_INT( pleft->prta ) - ( signed )REF_TO_INT( pright->prta );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )REF_TO_INT( pleft->chra ) - ( signed )REF_TO_INT( pright->chra );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )REF_TO_INT( pleft->prtb ) - ( signed )REF_TO_INT( pright->prtb );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )REF_TO_INT( pleft->chrb ) - ( signed )REF_TO_INT( pright->chrb );
    if ( 0 != itmp ) return itmp;

    itmp = ( signed )pleft->tileb - ( signed )pright->tileb;
    if ( 0 != itmp ) return itmp;

    return 0;
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
            if ( 0 == CoNode_cmp( hn->data, pdata ) )
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
bool_t detect_chr_chr_interaction_valid( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b )
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

    // don't interact if there is no interaction
    if ( 0 == pchr_a->bump.size || 0 == pchr_b->bump.size ) return bfalse;

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
bool_t detect_chr_prt_interaction_valid( const CHR_REF by_reference ichr_a, const PRT_REF by_reference iprt_b )
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
bool_t add_chr_chr_interaction( CHashList_t * pchlst, const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b, CoNode_ary_t * pcn_lst, HashNode_ary_t * phn_lst )
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
bool_t add_chr_prt_interaction( CHashList_t * pchlst, const CHR_REF by_reference ichr_a, const PRT_REF by_reference iprt_b, CoNode_ary_t * pcn_lst, HashNode_ary_t * phn_lst )
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
    ego_mpd_info_t * mi;
    BSP_aabb_t       tmp_aabb;

    if ( NULL == pchlst || NULL == cn_lst || NULL == hn_lst ) return bfalse;

    mi = &( PMesh->info );

    // allocate a BSP_aabb_t once, to be shared for all collision tests
    BSP_aabb_ctor( &tmp_aabb, obj_BSP_root.tree.dimensions );

    // renew the CoNode_t hash table.
    hash_list_renew( pchlst );

    //---- find the character/particle interactions

    // Find the character-character interactions. Use the ChrList.used_ref, for a change
    CHashList_inserted = 0;
    CHR_BEGIN_LOOP_ACTIVE( ichr_a, pchr_a )
    {
        oct_bb_t   tmp_oct;

        // use the object velocity to figure out where the volume that the object will occupy during this
        // update
        phys_expand_chr_bb( pchr_a, 0.0f, 1.0f, &tmp_oct );

        // convert the oct_bb_t to a correct BSP_aabb_t
        BSP_aabb_from_oct_bb( &tmp_aabb, &tmp_oct );

        // find all collisions with other characters and particles
        _coll_leaf_lst.top = 0;
        obj_BSP_collide( &( obj_BSP_root ), &tmp_aabb, &_coll_leaf_lst );

        // transfer valid _coll_leaf_lst entries to pchlst entries
        // and sort them by their initial times
        if ( _coll_leaf_lst.top > 0 )
        {
            int j;

            for ( j = 0; j < _coll_leaf_lst.top; j++ )
            {
                BSP_leaf_t * pleaf;
                size_t      coll_ref;
                CoNode_t    tmp_codata;
                bool_t      do_insert;
                int         test_platform;

                pleaf = _coll_leaf_lst.ary[j];
                if ( NULL == pleaf ) continue;

                do_insert = bfalse;

                coll_ref = pleaf->index;
                if ( 1 == pleaf->data_type )
                {
                    // collided with a character
                    CHR_REF ichr_b = ( CHR_REF )coll_ref;

                    // do some logic on this to determine whether the collision is valid
                    if ( detect_chr_chr_interaction_valid( ichr_a, ichr_b ) )
                    {
                        chr_t * pchr_b = ChrList.lst + ichr_b;

                        CoNode_ctor( &tmp_codata );

                        // do a simple test, since I do not want to resolve the cap_t for these objects here
                        test_platform = 0;
                        if ( pchr_a->platform && pchr_b->canuseplatforms ) test_platform |= PHYS_PLATFORM_OBJ1;
                        if ( pchr_b->platform && pchr_a->canuseplatforms ) test_platform |= PHYS_PLATFORM_OBJ2;

                        // detect a when the possible collision occurred
                        if ( phys_intersect_oct_bb( pchr_a->chr_chr_cv, pchr_a->pos, pchr_a->vel, pchr_b->chr_chr_cv, pchr_b->pos, pchr_b->vel, test_platform, &( tmp_codata.cv ), &( tmp_codata.tmin ), &( tmp_codata.tmax ) ) )
                        {
                            tmp_codata.chra = ichr_a;
                            tmp_codata.chrb = ichr_b;

                            do_insert = btrue;
                        }
                    }
                }
                else if ( 2 == pleaf->data_type )
                {
                    // collided with a particle
                    PRT_REF iprt_b = ( PRT_REF )coll_ref;

                    // do some logic on this to determine whether the collision is valid
                    if ( detect_chr_prt_interaction_valid( ichr_a, iprt_b ) )
                    {
                        prt_t * pprt_b = PrtList.lst + iprt_b;

                        CoNode_ctor( &tmp_codata );

                        // do a simple test, since I do not want to resolve the cap_t for these objects here
                        test_platform = pchr_a->platform ? PHYS_PLATFORM_OBJ1 : 0;

                        // detect a when the possible collision occurred
                        if ( phys_intersect_oct_bb( pchr_a->chr_prt_cv, pchr_a->pos, pchr_a->vel, pprt_b->chr_prt_cv, prt_get_pos(pprt_b), pprt_b->vel, test_platform, &( tmp_codata.cv ), &( tmp_codata.tmin ), &( tmp_codata.tmax ) ) )
                        {
                            tmp_codata.chra = ichr_a;
                            tmp_codata.prtb = iprt_b;

                            do_insert = btrue;
                        }
                    }
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

    //---- find the character and particle interactions with the mesh (not implemented)

    //// search through all characters. Use the ChrList.used_ref, for a change
    //_coll_leaf_lst.top = 0;
    //for ( i = 0; i < ChrList.used_count; i++ )
    //{
    //    CHR_REF ichra = ChrList.used_ref[i];
    //    if ( !INGAME_CHR( ichra ) ) continue;

    //    // find all character collisions with mesh tiles
    //    mesh_BSP_collide( &mesh_BSP_root, &( ChrList.lst[ichra].chr_prt_cv ), &_coll_leaf_lst );
    //    if ( _coll_leaf_lst.top > 0 )
    //    {
    //        int j;

    //        for ( j = 0; j < _coll_leaf_lst.top; j++ )
    //        {
    //            int coll_ref;
    //            CoNode_t tmp_codata;

    //            coll_ref = _coll_leaf_lst.ary[j];

    //            CoNode_ctor( &tmp_codata );
    //            tmp_codata.chra  = ichra;
    //            tmp_codata.tileb = coll_ref;

    //            CHashList_insert_unique( pchlst, &tmp_codata, cn_lst, hn_lst );
    //        }
    //    }
    //}

    //// search through all particles. Use the PrtList.used_ref, for a change
    //_coll_leaf_lst.top = 0;
    //for ( i = 0; i < PrtList.used_count; i++ )
    //{
    //    PRT_REF iprta = PrtList.used_ref[i];
    //    if ( !INGAME_PRT( iprta ) ) continue;

    //    // find all particle collisions with mesh tiles
    //    mesh_BSP_collide( &mesh_BSP_root, &( PrtList.lst[iprta].chr_prt_cv ), &_coll_leaf_lst );
    //    if ( _coll_leaf_lst.top > 0 )
    //    {
    //        int j;

    //        for ( j = 0; j < _coll_leaf_lst.top; j++ )
    //        {
    //            int coll_ref;
    //            CoNode_t tmp_codata;

    //            coll_ref = _coll_leaf_lst.ary[j];

    //            CoNode_ctor( &tmp_codata );
    //            tmp_codata.prta  = iprta;
    //            tmp_codata.tileb = coll_ref;

    //            CHashList_insert_unique( pchlst, &tmp_codata, cn_lst, hn_lst );
    //        }
    //    }
    //}

    // do this manually in C
    BSP_aabb_dtor( &tmp_aabb );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t fill_bumplists( obj_BSP_t * pbsp )
{
    /// @details BB@> Fill in the obj_BSP_t for this frame
    ///
    /// @note do not use obj_BSP_empty every frame, because the number of pre-allocated nodes can be quite large.
    /// Instead, just remove the nodes from the tree, fill the tree, and then prune any empty leaves

    if ( NULL == pbsp ) return bfalse;

    // Remove any unused branches from the tree.
    // If you do this after BSP_tree_free_nodes() it will remove all branches
    if ( 7 == ( frame_all & 7 ) )
    {
        BSP_tree_prune( &( pbsp->tree ) );
    }

    // empty out the BSP node lists
    obj_BSP_empty( pbsp );

    // fill up the BSP list based on the current locations
    obj_BSP_fill( pbsp );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_platform_detection( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b )
{
    chr_t * pchr_a, * pchr_b;

    bool_t platform_a, platform_b;
    bool_t mount_a, mount_b;

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

    // If we can mount this platform, skip it
    mount_a = chr_can_mount( ichr_b, ichr_a );
    if ( mount_a && pchr_a->enviro.level < pchr_b->pos.z + pchr_b->bump.height + PLATTOLERANCE )
        return bfalse;

    // If we can mount this platform, skip it
    mount_b = chr_can_mount( ichr_a, ichr_b );
    if ( mount_b && pchr_b->enviro.level < pchr_a->pos.z + pchr_a->bump.height + PLATTOLERANCE )
        return bfalse;

    odepth[OCT_Z]  = MIN( pchr_b->chr_chr_cv.maxs[OCT_Z] + pchr_b->pos.z, pchr_a->chr_chr_cv.maxs[OCT_Z] + pchr_a->pos.z ) -
                     MAX( pchr_b->chr_chr_cv.mins[OCT_Z] + pchr_b->pos.z, pchr_a->chr_chr_cv.mins[OCT_Z] + pchr_a->pos.z );

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

        depth_a = ( pchr_b->pos.z + pchr_b->chr_chr_cv.maxs[OCT_Z] ) - ( pchr_a->pos.z + pchr_a->chr_chr_cv.mins[OCT_Z] );
        depth_b = ( pchr_a->pos.z + pchr_a->chr_chr_cv.maxs[OCT_Z] ) - ( pchr_b->pos.z + pchr_b->chr_chr_cv.mins[OCT_Z] );

        odepth[OCT_Z] = MIN( pchr_b->pos.z + pchr_b->chr_chr_cv.maxs[OCT_Z], pchr_a->pos.z + pchr_a->chr_chr_cv.maxs[OCT_Z] ) -
                        MAX( pchr_b->pos.z + pchr_b->chr_chr_cv.mins[OCT_Z], pchr_a->pos.z + pchr_a->chr_chr_cv.mins[OCT_Z] );

        chara_on_top = ABS( odepth[OCT_Z] - depth_a ) < ABS( odepth[OCT_Z] - depth_b );

        // the collision is determined by the platform size
        if ( chara_on_top )
        {
            // size of a doesn't matter
            odepth[OCT_X]  = MIN(( pchr_b->chr_chr_cv.maxs[OCT_X] + pchr_b->pos.x ) - pchr_a->pos.x,
                                 pchr_a->pos.x - ( pchr_b->chr_chr_cv.mins[OCT_X] + pchr_b->pos.x ) );

            odepth[OCT_Y]  = MIN(( pchr_b->chr_chr_cv.maxs[OCT_Y] + pchr_b->pos.y ) -  pchr_a->pos.y,
                                 pchr_a->pos.y - ( pchr_b->chr_chr_cv.mins[OCT_Y] + pchr_b->pos.y ) );

            odepth[OCT_XY] = MIN(( pchr_b->chr_chr_cv.maxs[OCT_XY] + ( pchr_b->pos.x + pchr_b->pos.y ) ) - ( pchr_a->pos.x + pchr_a->pos.y ),
                                 ( pchr_a->pos.x + pchr_a->pos.y ) - ( pchr_b->chr_chr_cv.mins[OCT_XY] + ( pchr_b->pos.x + pchr_b->pos.y ) ) );

            odepth[OCT_YX] = MIN(( pchr_b->chr_chr_cv.maxs[OCT_YX] + ( -pchr_b->pos.x + pchr_b->pos.y ) ) - ( -pchr_a->pos.x + pchr_a->pos.y ),
                                 ( -pchr_a->pos.x + pchr_a->pos.y ) - ( pchr_b->chr_chr_cv.mins[OCT_YX] + ( -pchr_b->pos.x + pchr_b->pos.y ) ) );
        }
        else
        {
            // size of b doesn't matter

            odepth[OCT_X]  = MIN(( pchr_a->chr_chr_cv.maxs[OCT_X] + pchr_a->pos.x ) - pchr_b->pos.x,
                                 pchr_b->pos.x - ( pchr_a->chr_chr_cv.mins[OCT_X] + pchr_a->pos.x ) );

            odepth[OCT_Y]  = MIN(( pchr_a->chr_chr_cv.maxs[OCT_Y] + pchr_a->pos.y ) -  pchr_b->pos.y,
                                 pchr_b->pos.y - ( pchr_a->chr_chr_cv.mins[OCT_Y] + pchr_a->pos.y ) );

            odepth[OCT_XY] = MIN(( pchr_a->chr_chr_cv.maxs[OCT_XY] + ( pchr_a->pos.x + pchr_a->pos.y ) ) - ( pchr_b->pos.x + pchr_b->pos.y ),
                                 ( pchr_b->pos.x + pchr_b->pos.y ) - ( pchr_a->chr_chr_cv.mins[OCT_XY] + ( pchr_a->pos.x + pchr_a->pos.y ) ) );

            odepth[OCT_YX] = MIN(( pchr_a->chr_chr_cv.maxs[OCT_YX] + ( -pchr_a->pos.x + pchr_a->pos.y ) ) - ( -pchr_b->pos.x + pchr_b->pos.y ),
                                 ( -pchr_b->pos.x + pchr_b->pos.y ) - ( pchr_a->chr_chr_cv.mins[OCT_YX] + ( -pchr_a->pos.x + pchr_a->pos.y ) ) );
        }
    }
    else if ( platform_a )
    {
        chara_on_top = bfalse;
        odepth[OCT_Z] = ( pchr_a->pos.z + pchr_a->chr_chr_cv.maxs[OCT_Z] ) - ( pchr_b->pos.z + pchr_b->chr_chr_cv.mins[OCT_Z] );

        // size of b doesn't matter

        odepth[OCT_X]  = MIN(( pchr_a->chr_chr_cv.maxs[OCT_X] + pchr_a->pos.x ) - pchr_b->pos.x,
                             pchr_b->pos.x - ( pchr_a->chr_chr_cv.mins[OCT_X] + pchr_a->pos.x ) );

        odepth[OCT_Y]  = MIN(( pchr_a->chr_chr_cv.maxs[OCT_Y] + pchr_a->pos.y ) -  pchr_b->pos.y,
                             pchr_b->pos.y - ( pchr_a->chr_chr_cv.mins[OCT_Y] + pchr_a->pos.y ) );

        odepth[OCT_XY] = MIN(( pchr_a->chr_chr_cv.maxs[OCT_XY] + ( pchr_a->pos.x + pchr_a->pos.y ) ) - ( pchr_b->pos.x + pchr_b->pos.y ),
                             ( pchr_b->pos.x + pchr_b->pos.y ) - ( pchr_a->chr_chr_cv.mins[OCT_XY] + ( pchr_a->pos.x + pchr_a->pos.y ) ) );

        odepth[OCT_YX] = MIN(( pchr_a->chr_chr_cv.maxs[OCT_YX] + ( -pchr_a->pos.x + pchr_a->pos.y ) ) - ( -pchr_b->pos.x + pchr_b->pos.y ),
                             ( -pchr_b->pos.x + pchr_b->pos.y ) - ( pchr_a->chr_chr_cv.mins[OCT_YX] + ( -pchr_a->pos.x + pchr_a->pos.y ) ) );
    }
    else if ( platform_b )
    {
        chara_on_top = btrue;
        odepth[OCT_Z] = ( pchr_b->pos.z + pchr_b->chr_chr_cv.maxs[OCT_Z] ) - ( pchr_a->pos.z + pchr_a->chr_chr_cv.mins[OCT_Z] );

        // size of a doesn't matter
        odepth[OCT_X]  = MIN(( pchr_b->chr_chr_cv.maxs[OCT_X] + pchr_b->pos.x ) - pchr_a->pos.x,
                             pchr_a->pos.x - ( pchr_b->chr_chr_cv.mins[OCT_X] + pchr_b->pos.x ) );

        odepth[OCT_Y]  = MIN( pchr_b->chr_chr_cv.maxs[OCT_Y] + ( pchr_b->pos.y -  pchr_a->pos.y ),
                              ( pchr_a->pos.y - pchr_b->chr_chr_cv.mins[OCT_Y] ) + pchr_b->pos.y );

        odepth[OCT_XY] = MIN(( pchr_b->chr_chr_cv.maxs[OCT_XY] + ( pchr_b->pos.x + pchr_b->pos.y ) ) - ( pchr_a->pos.x + pchr_a->pos.y ),
                             ( pchr_a->pos.x + pchr_a->pos.y ) - ( pchr_b->chr_chr_cv.mins[OCT_XY] + ( pchr_b->pos.x + pchr_b->pos.y ) ) );

        odepth[OCT_YX] = MIN(( pchr_b->chr_chr_cv.maxs[OCT_YX] + ( -pchr_b->pos.x + pchr_b->pos.y ) ) - ( -pchr_a->pos.x + pchr_a->pos.y ),
                             ( -pchr_a->pos.x + pchr_a->pos.y ) - ( pchr_b->chr_chr_cv.mins[OCT_YX] + ( -pchr_b->pos.x + pchr_b->pos.y ) ) );

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
            if ( pchr_b->pos.z + pchr_b->chr_chr_cv.maxs[OCT_Z] > pchr_a->enviro.level )
            {
                // set, but do not attach the platforms yet
                pchr_a->enviro.level    = pchr_b->pos.z + pchr_b->chr_chr_cv.maxs[OCT_Z];
                pchr_a->onwhichplatform_ref = ichr_b;
            }
        }
        else
        {
            if ( pchr_a->pos.z + pchr_a->chr_chr_cv.maxs[OCT_Z] > pchr_b->enviro.level )
            {
                // set, but do not attach the platforms yet
                pchr_b->enviro.level    = pchr_a->pos.z + pchr_a->chr_chr_cv.maxs[OCT_Z];
                pchr_b->onwhichplatform_ref = ichr_a;
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_prt_platform_detection( const CHR_REF by_reference ichr_a, const PRT_REF by_reference iprt_b )
{
    chr_t * pchr_a;
    prt_t * pprt_b;

    oct_vec_t odepth;

    // make sure that B is valid
    if ( !INGAME_PRT( iprt_b ) ) return bfalse;
    pprt_b = PrtList.lst + iprt_b;

    // if the particle is attached to something, it can't be affected by a platform
    if ( INGAME_CHR( pprt_b->attachedto_ref ) ) return bfalse;

    // make sure that A is valid
    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    // only check possible particle-platform interactions
    if ( !pchr_a->platform ) return bfalse;

    // is this calculation going to matter, even if it succeeds?
    if ( pchr_a->pos.z + pchr_a->chr_chr_cv.maxs[OCT_Z] > pprt_b->enviro.level ) return bfalse;

    //---- determine the interaction depth for each dimension
    odepth[OCT_Z] = ( pchr_a->pos.z + pchr_a->chr_chr_cv.maxs[OCT_Z] ) - ( pprt_b->pos.z - pprt_b->bump_padded.height );
    if ( odepth[OCT_Z] < -PLATTOLERANCE || odepth[OCT_Z] > PLATTOLERANCE ) return bfalse;

    odepth[OCT_X]  = MIN(( pchr_a->chr_chr_cv.maxs[OCT_X] + pchr_a->pos.x ) - pprt_b->pos.x,
                         pprt_b->pos.x - ( pchr_a->chr_chr_cv.mins[OCT_X] + pchr_a->pos.x ) );
    if ( odepth[OCT_X] <= 0 ) return bfalse;

    odepth[OCT_Y]  = MIN(( pchr_a->chr_chr_cv.maxs[OCT_Y] + pchr_a->pos.y ) -  pprt_b->pos.y,
                         pprt_b->pos.y - ( pchr_a->chr_chr_cv.mins[OCT_Y] + pchr_a->pos.y ) );
    if ( odepth[OCT_Y] <= 0 ) return bfalse;

    odepth[OCT_XY] = MIN(( pchr_a->chr_chr_cv.maxs[OCT_XY] + ( pchr_a->pos.x + pchr_a->pos.y ) ) - ( pprt_b->pos.x + pprt_b->pos.y ),
                         ( pprt_b->pos.x + pprt_b->pos.y ) - ( pchr_a->chr_chr_cv.mins[OCT_XY] + ( pchr_a->pos.x + pchr_a->pos.y ) ) );
    if ( odepth[OCT_XY] <= 0 ) return bfalse;

    odepth[OCT_YX] = MIN(( pchr_a->chr_chr_cv.maxs[OCT_YX] + ( -pchr_a->pos.x + pchr_a->pos.y ) ) - ( -pprt_b->pos.x + pprt_b->pos.y ),
                         ( -pprt_b->pos.x + pprt_b->pos.y ) - ( pchr_a->chr_chr_cv.mins[OCT_YX] + ( -pchr_a->pos.x + pchr_a->pos.y ) ) );
    if ( odepth[OCT_YX] <= 0 ) return bfalse;

    //---- this is the best possible attachment
    attach_prt_to_platform( pprt_b, pchr_a );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t attach_chr_to_platform( chr_t * pchr, chr_t * pplat )
{
    /// @details BB@> attach a character to a platform

    cap_t * pchr_cap;
    fvec3_t   platform_up;

    // verify that we do not have two dud pointers
    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;
    if ( !ACTIVE_PCHR( pplat ) ) return bfalse;

    pchr_cap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pchr_cap ) return bfalse;

    // check if they can be connected
    if ( !pchr_cap->canuseplatforms || ( 0 != pchr->flyheight ) ) return bfalse;
    if ( !pplat->platform ) return bfalse;

    // do the attachment
    pchr->onwhichplatform_ref = GET_REF_PCHR( pplat );

    // update the character's relationship to the ground
    pchr->enviro.level     = MAX( pchr->enviro.floor_level, pplat->pos.z + pplat->chr_chr_cv.maxs[OCT_Z] );
    pchr->enviro.zlerp     = ( pchr->pos.z - pchr->enviro.level ) / PLATTOLERANCE;
    pchr->enviro.zlerp     = CLIP( pchr->enviro.zlerp, 0, 1 );
    pchr->enviro.grounded  = ( 0 == pchr->flyheight ) && ( pchr->enviro.zlerp < 0.25f );

    pchr->enviro.fly_level = MAX( pchr->enviro.fly_level, pchr->enviro.level );
    if ( 0 != pchr->flyheight )
    {
        if ( pchr->enviro.fly_level < 0 ) pchr->enviro.fly_level = 0;  // fly above pits...
    }

    // add the weight to the platform based on the new zlerp
    pplat->holdingweight += pchr->phys.weight * ( 1.0f - pchr->enviro.zlerp );

    // update the character jupming
    pchr->jumpready = pchr->enviro.grounded;
    if ( pchr->jumpready )
    {
        pchr->jumpnumber = pchr->jumpnumberreset;
    }

    // what to do about traction if the platform is tilted... hmmm?
    chr_getMatUp( pplat, &platform_up );
    platform_up = fvec3_normalize( platform_up.v );

    pchr->enviro.traction = ABS( platform_up.z ) * ( 1.0f - pchr->enviro.zlerp ) + 0.25 * pchr->enviro.zlerp;

    // tell the platform that we bumped into it
    // this is necessary for key buttons to work properly, for instance
    ai_state_set_bumplast( &( pplat->ai ), GET_REF_PCHR( pchr ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t attach_prt_to_platform( prt_t * pprt, chr_t * pplat )
{
    /// @details BB@> attach a particle to a platform

    pip_t   * pprt_pip;

    // verify that we do not have two dud pointers
    if ( !ACTIVE_PPRT( pprt ) ) return bfalse;
    if ( !ACTIVE_PCHR( pplat ) ) return bfalse;

    pprt_pip = prt_get_ppip( GET_REF_PPRT( pprt ) );
    if ( NULL == pprt_pip ) return bfalse;

    // check if they can be connected
    if ( !pplat->platform ) return bfalse;

    // do the attachment
    pprt->onwhichplatform_ref = GET_REF_PCHR( pplat );

    // update the character's relationship to the ground
    prt_set_level( pprt, MAX( pprt->enviro.level, pplat->pos.z + pplat->chr_chr_cv.maxs[OCT_Z] ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//bool_t do_pre_bumping( CHashList_t * pclst, CoNode_ary_t * cn_lst, HashNode_ary_t * hn_lst )
//{
//    int i;
//    int cnt;
//
//    ego_mpd_info_t * mi;
//    int_ary_t        coll_lst;
//
//    if( NULL == pchlst ) return bfalse;
//
//    int_ary_ctor( &coll_lst, COLLISION_LIST_SIZE );
//
//    mi    = &( PMesh->info );
//
//    // fill in the entries in the bumplist
//    fill_bumplists();
//
//    // renew the CoNode_t hash table. Since we are filling this list with pre-allocated CoNode_t's,
//    // there is no need to delete any of the existing pclst->sublist elements
//    for ( cnt = 0; cnt < 256; cnt++ )
//    {
//        hash_list_set_count( pclst, cnt, 0 );
//        hash_list_set_node( pclst, cnt, NULL );
//    }
//
//    //---- find the character/particle interactions
//
//    // search through all characters. Use the ChrList.used_ref, for a change
//    coll_lst.top = 0;
//    for ( i = 0; i < ChrList.used_count; i++ )
//    {
//        CHR_REF ichra = ChrList.used_ref[i];
//        if ( !INGAME_CHR( ichra ) ) continue;
//
//        // find all collisions with other characters and particles
//        obj_BSP_collide( &( obj_BSP_root ), &( ChrList.lst[ichra].chr_prt_cv ), &coll_lst );
//
//        if ( coll_lst.top > 0 )
//        {
//            int j;
//
//            for ( j = 0; j < coll_lst.top; j++ )
//            {
//                int coll_ref;
//                CoNode_t tmp_codata;
//                bool_t do_insert;
//
//                do_insert = bfalse;
//
//                CoNode_ctor( &tmp_codata );
//                tmp_codata.chra  = ichra;
//
//                coll_ref = coll_lst.ary[j];
//                if ( coll_ref > 0 )
//                {
//                    // collided with a character
//                    tmp_codata.chrb = coll_lst.ary[j];
//
//                    if ( detect_chr_chr_interaction( ichra, tmp_codata.chrb ) )
//                    {
//                        do_insert = btrue;
//                    }
//                }
//                else
//                {
//                    // collided with a particle
//                    tmp_codata.prtb = -coll_lst.ary[j];
//
//                    if ( detect_chr_prt_interaction( ichra, tmp_codata.prtb ) )
//                    {
//                        do_insert = btrue;
//                    }
//                }
//
//                if( do_insert )
//                {
//                    CHashList_insert_unique( pclst, &tmp_codata, cn_lst, hn_lst );
//                }
//            }
//        }
//    }
//
//
//    //---- find the character and particle interactions with the mesh (not implemented)
//
//    //// search through all characters. Use the ChrList.used_ref, for a change
//    //coll_lst.top = 0;
//    //for ( i = 0; i < ChrList.used_count; i++ )
//    //{
//    //    CHR_REF ichra = ChrList.used_ref[i];
//    //    if ( !INGAME_CHR( ichra ) ) continue;
//
//    //    // find all character collisions with mesh tiles
//    //    mesh_BSP_collide( &mesh_BSP_root, &( ChrList.lst[ichra].chr_prt_cv ), &coll_lst );
//    //    if ( coll_lst.top > 0 )
//    //    {
//    //        int j;
//
//    //        for ( j = 0; j < coll_lst.top; j++ )
//    //        {
//    //            int coll_ref;
//    //            CoNode_t tmp_codata;
//
//    //            coll_ref = coll_lst.ary[j];
//
//    //            CoNode_ctor( &tmp_codata );
//    //            tmp_codata.chra  = ichra;
//    //            tmp_codata.tileb = coll_ref;
//
//    //            CHashList_insert_unique( pclst, &tmp_codata, cn_lst, hn_lst );
//    //        }
//    //    }
//    //}
//
//
//    //// search through all particles. Use the PrtList.used_ref, for a change
//    //coll_lst.top = 0;
//    //for ( i = 0; i < PrtList.used_count; i++ )
//    //{
//    //    PRT_REF iprta = PrtList.used_ref[i];
//    //    if ( !INGAME_PRT( iprta ) ) continue;
//
//    //    // find all particle collisions with mesh tiles
//    //    mesh_BSP_collide( &mesh_BSP_root, &( PrtList.lst[iprta].chr_prt_cv ), &coll_lst );
//    //    if ( coll_lst.top > 0 )
//    //    {
//    //        int j;
//
//    //        for ( j = 0; j < coll_lst.top; j++ )
//    //        {
//    //            int coll_ref;
//    //            CoNode_t tmp_codata;
//
//    //            coll_ref = coll_lst.ary[j];
//
//    //            CoNode_ctor( &tmp_codata );
//    //            tmp_codata.prta  = iprta;
//    //            tmp_codata.tileb = coll_ref;
//
//    //            CHashList_insert_unique( pclst, &tmp_codata, cn_lst, hn_lst );
//    //        }
//    //    }
//    //}
//
//
//
//    // do this manually in C
//    int_ary_dtor( &coll_lst );
//}

//--------------------------------------------------------------------------------------------
void bump_all_objects( obj_BSP_t * pbsp )
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
    fill_bumplists( pbsp );

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

        // handle interaction with platforms
        bump_all_platforms( &_coll_node_lst );

        // handle interaction with mounts
        bump_all_mounts( &_coll_node_lst );

        // handle all the collisions
        bump_all_collisions( &_coll_node_lst );
    }

    // The following functions need to be called any time you actually change a charcter's position
    keep_weapons_with_holders();
    attach_all_particles();
    make_all_character_matrices( update_wld != 0 );
}

//--------------------------------------------------------------------------------------------
bool_t bump_all_platforms( CoNode_ary_t * pcn_ary )
{
    /// @details BB@> Detect all character and particle interactions with platforms, then attach them.
    ///             @note it is important to only attach the character to a platform once, so its
    ///              weight does not get applied to multiple platforms

    int        cnt;
    CoNode_t * d;

    if ( NULL == pcn_ary ) return bfalse;

    // Find the best platforms
    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
    {
        d = pcn_ary->ary + cnt;

        // only look at character-platform or particle-platform interactions interactions
        if ( TOTAL_MAX_PRT != d->prta && TOTAL_MAX_PRT != d->prtb ) continue;

        if ( MAX_CHR != d->chra && MAX_CHR != d->chrb )
        {
            do_chr_platform_detection( d->chra, d->chrb );
        }
        else if ( MAX_CHR != d->chra && TOTAL_MAX_PRT != d->prtb )
        {
            do_prt_platform_detection( d->chra, d->prtb );
        }
        if ( TOTAL_MAX_PRT != d->prta && MAX_CHR != d->chrb )
        {
            do_prt_platform_detection( d->chrb, d->prta );
        }
    }

    // Do the actual platform attachments.
    // Doing the attachments after detecting the best platform
    // prevents an object from attaching it to multiple platforms as it
    // is still trying to find the best one
    for ( cnt = 0; cnt < pcn_ary->top; cnt++ )
    {
        d = pcn_ary->ary + cnt;

        // only look at character-character interactions
        if ( TOTAL_MAX_PRT != d->prta && TOTAL_MAX_PRT != d->prtb ) continue;

        if ( MAX_CHR != d->chra && MAX_CHR != d->chrb )
        {
            if ( INGAME_CHR( d->chra ) && INGAME_CHR( d->chrb ) )
            {
                if ( ChrList.lst[d->chra].onwhichplatform_ref == d->chrb )
                {
                    attach_chr_to_platform( ChrList.lst + d->chra, ChrList.lst + d->chrb );
                }
                else if ( ChrList.lst[d->chrb].onwhichplatform_ref == d->chra )
                {
                    attach_chr_to_platform( ChrList.lst + d->chrb, ChrList.lst + d->chra );
                }
            }
        }
        else if ( MAX_CHR != d->chra && TOTAL_MAX_PRT != d->prtb )
        {
            if ( INGAME_CHR( d->chra ) && INGAME_PRT( d->prtb ) )
            {
                if ( PrtList.lst[d->prtb].onwhichplatform_ref == d->chra )
                {
                    attach_prt_to_platform( PrtList.lst + d->prtb, ChrList.lst + d->chra );
                }
            }
        }
        else if ( MAX_CHR != d->chrb && TOTAL_MAX_PRT != d->prta )
        {
            if ( INGAME_CHR( d->chrb ) && INGAME_PRT( d->prta ) )
            {
                if ( PrtList.lst[d->prta].onwhichplatform_ref == d->chrb )
                {
                    attach_prt_to_platform( PrtList.lst + d->prta, ChrList.lst + d->chrb );
                }
            }
        }
    }

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
        if ( TOTAL_MAX_PRT != d->prtb ) continue;

        do_mounts( d->chra, d->chrb );
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
        pchr->phys.apos_0.x = 0.0f;
        pchr->phys.apos_0.y = 0.0f;
        pchr->phys.apos_0.z = 0.0f;

        pchr->phys.apos_1.x = 0.0f;
        pchr->phys.apos_1.y = 0.0f;
        pchr->phys.apos_1.z = 0.0f;

        pchr->phys.avel.x = 0.0f;
        pchr->phys.avel.y = 0.0f;
        pchr->phys.avel.z = 0.0f;
    }
    CHR_END_LOOP();

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

        fvec3_t tmp_pos = chr_get_pos( pchr );

        bump_str = 1.0f;
        if ( INGAME_CHR( pchr->attachedto ) )
        {
            bump_str = 0;
        }
        else if ( pchr->dismount_timer > 0 )
        {
            bump_str = 1.0f - ( float )pchr->dismount_timer / PHYS_DISMOUNT_TIME;
            bump_str = bump_str * bump_str * 0.5f;
        }

        // decrement the dismount timer
        if ( pchr->dismount_timer > 0 )
        {
            pchr->dismount_timer--;

            if ( 0 == pchr->dismount_timer )
            {
                pchr->dismount_object = ( CHR_REF )MAX_CHR;
            }
        }

        // do the "integration" of the accumulated accelerations
        pchr->vel.x += pchr->phys.avel.x;
        pchr->vel.y += pchr->phys.avel.y;
        pchr->vel.z += pchr->phys.avel.z;

        position_updated = bfalse;

        // do the "integration" on the position
        if ( ABS( pchr->phys.apos_0.x + pchr->phys.apos_1.x ) > 0 )
        {
            tmpx = tmp_pos.x;
            tmp_pos.x += pchr->phys.apos_0.x + pchr->phys.apos_1.x;
            if ( chr_test_wall( pchr, tmp_pos.v ) )
            {
                // restore the old values
                tmp_pos.x = tmpx;
            }
            else
            {
                pchr->vel.x += pchr->phys.apos_1.x * bump_str;
                position_updated = btrue;
            }
        }

        if ( ABS( pchr->phys.apos_0.y + pchr->phys.apos_1.y ) > 0 )
        {
            tmpy = tmp_pos.y;
            tmp_pos.y += pchr->phys.apos_0.y + pchr->phys.apos_1.y;
            if ( chr_test_wall( pchr, tmp_pos.v ) )
            {
                // restore the old values
                tmp_pos.y = tmpy;
            }
            else
            {
                pchr->vel.y += pchr->phys.apos_1.y * bump_str;
                position_updated = btrue;
            }
        }

        if ( ABS( pchr->phys.apos_0.z + pchr->phys.apos_1.z ) > 0 )
        {
            tmpz = tmp_pos.z;
            tmp_pos.z += pchr->phys.apos_0.z + pchr->phys.apos_1.z;
            if ( tmp_pos.z < pchr->enviro.floor_level )
            {
                // restore the old values
                tmp_pos.z = pchr->enviro.floor_level;
                if( pchr->vel.z < 0 )
                {
                    cap_t * pcap = chr_get_pcap( ichr );
                    if( NULL != pcap )
                    {
                        pchr->vel.z += -(1.0f + pcap->dampen) * pchr->vel.z;
                    }
                }
                position_updated = btrue;
            }
            else
            {
                pchr->vel.z += pchr->phys.apos_1.z * bump_str;
                position_updated = btrue;
            }
        }

        if( position_updated )
        {
            chr_set_pos( pchr, tmp_pos.v );
        }
    }
    CHR_END_LOOP();

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_mounts( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b )
{
    float xa, ya, za;
    float xb, yb, zb;

    chr_t * pchr_a, * pchr_b;
    cap_t * pcap_a, * pcap_b;

    bool_t mount_a, mount_b;
    float  dx, dy, dist;
    float  depth_z;

    bool_t collide_x  = bfalse;
    bool_t collide_y  = bfalse;
    bool_t collide_xy = bfalse;

    bool_t mounted;

    // make sure that A is valid
    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    pcap_a = chr_get_pcap( ichr_a );
    if ( NULL == pcap_a ) return bfalse;

    // make sure that B is valid
    if ( !INGAME_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    pcap_b = chr_get_pcap( ichr_b );
    if ( NULL == pcap_b ) return bfalse;

    // can either of these objects mount the other?
    mount_a = chr_can_mount( ichr_b, ichr_a );
    mount_b = chr_can_mount( ichr_a, ichr_b );

    if ( !mount_a && !mount_b ) return bfalse;

	//Ready for position calulations
	xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    xb = pchr_b->pos.x;
    yb = pchr_b->pos.y;
    zb = pchr_b->pos.z;

	mounted = bfalse;
    if ( !mounted && mount_b && ( pchr_a->vel.z - pchr_b->vel.z ) < 0 )
    {
        // A falling on B?
        fvec4_t   point[1], nupoint[1];

        // determine the actual location of the mount point
        {
            int vertex;
            chr_instance_t * pinst = &( pchr_b->inst );

            vertex = (( int )pinst->vrt_count ) - GRIP_LEFT;

            // do the automatic update
            chr_instance_update_vertices( pinst, vertex, vertex, bfalse );

            // Calculate grip point locations with linear interpolation and other silly things
            point[0].x = pinst->vrt_lst[vertex].pos[XX];
            point[0].y = pinst->vrt_lst[vertex].pos[YY];
            point[0].z = pinst->vrt_lst[vertex].pos[ZZ];
            point[0].w = 1.0f;

            // Do the transform
            TransformVertices( &( pinst->matrix ), point, nupoint, 1 );
        }

        dx = ABS( xa - nupoint[0].x );
        dy = ABS( ya - nupoint[0].y );
        dist = dx + dy;
        depth_z = za - nupoint[0].z;

        if ( depth_z >= -MOUNTTOLERANCE && depth_z <= MOUNTTOLERANCE )
        {
            // estimate the collisions this frame
            collide_x  = ( dx <= pchr_a->bump.size * 2 );
            collide_y  = ( dy <= pchr_a->bump.size * 2 );
            collide_xy = ( dist <= pchr_a->bump.size_big * 2 );

            if ( collide_x && collide_y && collide_xy )
            {
                attach_character_to_mount( ichr_a, ichr_b, GRIP_ONLY );
                mounted = INGAME_CHR( pchr_a->attachedto );
            }
        }
    }

    if ( !mounted && mount_a && ( pchr_b->vel.z - pchr_a->vel.z ) < 0 )
    {
        // B falling on A?

        fvec4_t   point[1], nupoint[1];

        // determine the actual location of the mount point
        {
            int vertex;
            chr_instance_t * pinst = &( pchr_a->inst );

            vertex = (( int )pinst->vrt_count ) - GRIP_LEFT;

            // do the automatic update
            chr_instance_update_vertices( pinst, vertex, vertex, bfalse );

            // Calculate grip point locations with linear interpolation and other silly things
            point[0].x = pinst->vrt_lst[vertex].pos[XX];
            point[0].y = pinst->vrt_lst[vertex].pos[YY];
            point[0].z = pinst->vrt_lst[vertex].pos[ZZ];
            point[0].w = 1.0f;

            // Do the transform
            TransformVertices( &( pinst->matrix ), point, nupoint, 1 );
        }

        dx = ABS( xb - nupoint[0].x );
        dy = ABS( yb - nupoint[0].y );
        dist = dx + dy;
        depth_z = zb - nupoint[0].z;

        if ( depth_z >= -MOUNTTOLERANCE && depth_z <= MOUNTTOLERANCE )
        {
            // estimate the collisions this frame
            collide_x  = ( dx <= pchr_b->bump.size * 2 );
            collide_y  = ( dy <= pchr_b->bump.size * 2 );
            collide_xy = ( dist <= pchr_b->bump.size_big * 2 );

            if ( collide_x && collide_y && collide_xy )
            {
                attach_character_to_mount( ichr_b, ichr_a, GRIP_ONLY );
                mounted = INGAME_CHR( pchr_a->attachedto );
            }
        }
    }

    return mounted;
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
        pitem->phys.apos_0.z += ( pitem->enviro.level - pitem->pos.z ) * 0.125f;
        pitem->phys.avel.z += ( pplat->vel.z  - pitem->vel.z ) * 0.25f;
        pitem->ori.facing_z      += ( rot_a         - rot_b ) * platstick;
    }
    else
    {
        pitem->phys.apos_0.z += ( pitem->enviro.level - pitem->pos.z ) * 0.125f * lerp_z * vlerp_z;
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

    collision_size.x = MAX( pchr->chr_prt_cv.maxs[OCT_X] - pchr->chr_prt_cv.mins[OCT_X], 2.0f * pprt->bump_padded.size );
    collision_size.y = MAX( pchr->chr_prt_cv.maxs[OCT_Y] - pchr->chr_prt_cv.mins[OCT_Y], 2.0f * pprt->bump_padded.size );
    collision_size.z = MAX( pchr->chr_prt_cv.maxs[OCT_Z] - pchr->chr_prt_cv.mins[OCT_Z], 2.0f * pprt->bump_padded.height );

    // estimate the "normal" for the collision, using the center-of-mass difference
    nrm[kX] = pprt->pos.x - pchr->pos.x;
    nrm[kY] = pprt->pos.y - pchr->pos.y;
    nrm[kZ] = pprt->pos.z - ( pchr->pos.z + 0.5f * ( pchr->chr_prt_cv.maxs[OCT_Z] + pchr->chr_prt_cv.mins[OCT_Z] ) );

    // scale the collision box
    nrm[kX] /= collision_size.x;
    nrm[kY] /= collision_size.y;
    nrm[kZ] /= collision_size.z;

    // scale the z-normals so that the collision volume will act somewhat like a cylinder
    nrm[kZ] *= nrm[kZ] * nrm[kZ];

    // reject the reflection request if the particle is moving in the wrong direction
    vdiff[kX] = pprt->vel.x - pchr->vel.x;
    vdiff[kY] = pprt->vel.y - pchr->vel.y;
    vdiff[kZ] = pprt->vel.z - pchr->vel.z;
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
            nrm[kZ] = pprt->pos_old.z - ( pchr->pos_old.z + 0.5f * ( pchr->chr_prt_cv.maxs[OCT_Z] + pchr->chr_prt_cv.mins[OCT_Z] ) );

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
bool_t do_chr_chr_collision( CoNode_t * d )
{
    CHR_REF ichr_a, ichr_b;
    chr_t * pchr_a, * pchr_b;
    cap_t * pcap_a, * pcap_b;

    float interaction_strength = 1.0f;
    float wta, wtb;

    fvec3_t   nrm;
    int exponent = 1;

    oct_vec_t opos_a, opos_b, odepth;
    //oct_vec_t odepth_old;
    bool_t    collision = bfalse, bump = bfalse;

    if ( NULL == d || TOTAL_MAX_PRT != d->prtb ) return bfalse;
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
    if ( 0 == pchr_a->bump.size || 0 == pchr_b->bump.size ) return bfalse;

    interaction_strength = 1.0f;
    interaction_strength *= pchr_a->inst.alpha * INV_FF;
    interaction_strength *= pchr_b->inst.alpha * INV_FF;

    // reduce the interaction strength with platforms
    // that are overlapping with the platform you are actually on
    if ( pchr_b->canuseplatforms && pchr_a->platform )
    {
        float lerp_z = ( pchr_b->pos.z - ( pchr_a->pos.z + pchr_a->chr_chr_cv.maxs[OCT_Z] ) ) / PLATTOLERANCE;
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

    if ( pchr_a->canuseplatforms && pchr_b->platform )
    {
        float lerp_z = ( pchr_a->pos.z - ( pchr_b->pos.z + pchr_b->chr_chr_cv.maxs[OCT_Z] ) ) / PLATTOLERANCE;
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

    // measure the collision depth
    //if ( !get_depth_2( pchr_a->chr_chr_cv, pchr_a->pos, pchr_b->chr_chr_cv, pchr_b->pos, btrue, odepth ) )
    //{
    //    // return if there was no collision
    //    return bfalse;
    //}

    // estimate the collision volume and depth from a 10% overlap
    {
        int cnt;

        oct_bb_t src1, src2;

        oct_bb_t exp1, exp2, intersect;

        //oct_vec_t opos1, opos2;
        //oct_vec_t ovel1, ovel2;

        float tmp_min, tmp_max;

        tmp_min = d->tmin;
        tmp_max = d->tmin + (d->tmax - d->tmin) * 0.1f;

        // shift the source bounding boxes to be centered on the given positions
        oct_bb_add_vector( pchr_a->chr_chr_cv, pchr_a->pos, &src1 );
        oct_bb_add_vector( pchr_b->chr_chr_cv, pchr_b->pos, &src2 );

        // determine the expanded collision volumes for both objects
        phys_expand_oct_bb( src1, pchr_a->vel, tmp_min, tmp_max, &exp1 );
        phys_expand_oct_bb( src2, pchr_b->vel, tmp_min, tmp_max, &exp2 );

        // determine the intersection of these two volumes
        oct_bb_intersection( exp1, exp2, &intersect );

        for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
        {
            odepth[cnt]  = intersect.maxs[cnt] - intersect.mins[cnt];
        }

        // scale the diagonal components so that they are actually distances
        odepth[OCT_XY] *= INV_SQRT_TWO;
        odepth[OCT_YX] *= INV_SQRT_TWO;
    }

    // measure the collision depth in the last update
    // the objects were not touching last frame, so they must have collided this frame
    //collision = !get_depth_2( pchr_a->chr_chr_cv, pchr_a->pos_old, pchr_b->chr_chr_cv, pchr_b->pos_old, btrue, odepth_old );

    // use the info from the collision volume to determine whether the objects are colliding
    collision = (d->tmin >= 0.0f);

    //------------------
    // do character-character interactions

    // calculate a "mass" for each object, taking into account possible infinite masses
    chr_get_mass_pair( pchr_a, pchr_b, &wta, &wtb );

    // create an "octagonal position" for each object
    oct_vec_ctor( opos_a, pchr_a->pos );
    oct_vec_ctor( opos_b, pchr_b->pos );

    // adjust the center-of-mass
    opos_a[OCT_Z] += ( pchr_a->chr_chr_cv.maxs[OCT_Z] + pchr_a->chr_chr_cv.mins[OCT_Z] ) * 0.5f;
    opos_b[OCT_Z] += ( pchr_b->chr_chr_cv.maxs[OCT_Z] + pchr_b->chr_chr_cv.mins[OCT_Z] ) * 0.5f;

    // make the object more like a table if there is a platform-like interaction
    if ( pchr_a->canuseplatforms && pchr_b->platform ) exponent += 2;
    if ( pchr_b->canuseplatforms && pchr_a->platform ) exponent += 2;

    if ( phys_estimate_chr_chr_normal( opos_a, opos_b, odepth, exponent, nrm.v ) )
    {
        fvec3_t   vel_a, vel_b;
        fvec3_t   vpara_a, vperp_a;
        fvec3_t   vpara_b, vperp_b;
        fvec3_t   imp_a, imp_b;
        float     vdot;

        vel_a = pchr_a->vel;
        vel_b = pchr_b->vel;

        vdot = fvec3_dot_product( nrm.v, vel_a.v );
        vperp_a.x = nrm.x * vdot;
        vperp_a.y = nrm.y * vdot;
        vperp_a.z = nrm.z * vdot;
        vpara_a   = fvec3_sub( vel_a.v, vperp_a.v );

        vdot = fvec3_dot_product( nrm.v, vel_b.v );
        vperp_b.x = nrm.x * vdot;
        vperp_b.y = nrm.y * vdot;
        vperp_b.z = nrm.z * vdot;
        vpara_b   = fvec3_sub( vel_b.v, vperp_b.v );

        // clear the "impulses"
        imp_a.x = imp_a.y = imp_a.z = 0.0f;
        imp_b.x = imp_b.y = imp_b.z = 0.0f;

        // what type of "collision" is this? (impulse or pressure)
        if ( collision )
        {
            // an actual bump, use impulse to make the objects bounce appart

            // generic coefficient of restitution
            float cr = 0.5f;

            if (( wta < 0.0f && wtb < 0.0f ) || ( wta == wtb ) )
            {
                float factor = 0.5f * ( 1.0f - cr );

                imp_a.x = factor * ( vperp_b.x - vperp_a.x );
                imp_a.y = factor * ( vperp_b.y - vperp_a.y );
                imp_a.z = factor * ( vperp_b.z - vperp_a.z );

                imp_b.x = factor * ( vperp_a.x - vperp_b.x );
                imp_b.y = factor * ( vperp_a.y - vperp_b.y );
                imp_b.z = factor * ( vperp_a.z - vperp_b.z );
            }
            else if (( wta < 0.0f ) || ( wtb == 0.0f ) )
            {
                float factor = ( 1.0f - cr );

                imp_b.x = factor * ( vperp_a.x - vperp_b.x );
                imp_b.y = factor * ( vperp_a.y - vperp_b.y );
                imp_b.z = factor * ( vperp_a.z - vperp_b.z );
            }
            else if (( wtb < 0.0f ) || ( wta == 0.0f ) )
            {
                float factor = ( 1.0f - cr );

                imp_a.x = factor * ( vperp_b.x - vperp_a.x );
                imp_a.y = factor * ( vperp_b.y - vperp_a.y );
                imp_a.z = factor * ( vperp_b.z - vperp_a.z );
            }
            else
            {
                float factor;

                factor = ( 1.0f - cr ) * wtb / ( wta + wtb );
                imp_a.x = factor * ( vperp_b.x - vperp_a.x );
                imp_a.y = factor * ( vperp_b.y - vperp_a.y );
                imp_a.z = factor * ( vperp_b.z - vperp_a.z );

                factor = ( 1.0f - cr ) * wta / ( wta + wtb );
                imp_b.x = factor * ( vperp_a.x - vperp_b.x );
                imp_b.y = factor * ( vperp_a.y - vperp_b.y );
                imp_b.z = factor * ( vperp_a.z - vperp_b.z );
            }

            // add in the bump impulses
            pchr_a->phys.avel.x += imp_a.x * interaction_strength;
            pchr_a->phys.avel.y += imp_a.y * interaction_strength;
            pchr_a->phys.avel.z += imp_a.z * interaction_strength;
            LOG_NAN( pchr_a->phys.avel.z );

            pchr_b->phys.avel.x += imp_b.x * interaction_strength;
            pchr_b->phys.avel.y += imp_b.y * interaction_strength;
            pchr_b->phys.avel.z += imp_b.z * interaction_strength;
            LOG_NAN( pchr_b->phys.avel.z );

            bump = btrue;
        }
        else
        {
            // not a bump at all. two objects are rubbing against one another
            // and continually overlapping. use pressure to push them appart.

            float tmin;

            tmin = 1e6;
            if ( nrm.x != 0 )
            {
                tmin = MIN( tmin, odepth[OCT_X] / ABS( nrm.x ) );
            }
            if ( nrm.y != 0 )
            {
                tmin = MIN( tmin, odepth[OCT_Y] / ABS( nrm.y ) );
            }
            if ( nrm.z != 0 )
            {
                tmin = MIN( tmin, odepth[OCT_Z] / ABS( nrm.z ) );
            }

            if ( nrm.x + nrm.y != 0 )
            {
                tmin = MIN( tmin, odepth[OCT_XY] / ABS( nrm.x + nrm.y ) );
            }

            if ( -nrm.x + nrm.y != 0 )
            {
                tmin = MIN( tmin, odepth[OCT_YX] / ABS( -nrm.x + nrm.y ) );
            }

            if ( tmin < 1e6 )
            {
                const float pressure_strength = 0.125f * 0.5f;
                if ( wta >= 0.0f )
                {
                    float ratio = ( float )ABS( wtb ) / (( float )ABS( wta ) + ( float )ABS( wtb ) );

                    imp_a.x = tmin * nrm.x * ratio * pressure_strength;
                    imp_a.y = tmin * nrm.y * ratio * pressure_strength;
                    imp_a.z = tmin * nrm.z * ratio * pressure_strength;
                }

                if ( wtb >= 0.0f )
                {
                    float ratio = ( float )ABS( wta ) / (( float )ABS( wta ) + ( float )ABS( wtb ) );

                    imp_b.x = -tmin * nrm.x * ratio * pressure_strength;
                    imp_b.y = -tmin * nrm.y * ratio * pressure_strength;
                    imp_b.z = -tmin * nrm.z * ratio * pressure_strength;
                }
            }

            // add in the bump impulses
            pchr_a->phys.apos_1.x += imp_a.x * interaction_strength;
            pchr_a->phys.apos_1.y += imp_a.y * interaction_strength;
            pchr_a->phys.apos_1.z += imp_a.z * interaction_strength;

            pchr_b->phys.apos_1.x += imp_b.x * interaction_strength;
            pchr_b->phys.apos_1.y += imp_b.y * interaction_strength;
            pchr_b->phys.apos_1.z += imp_b.z * interaction_strength;

            // you could "bump" something if you changed your velocity, even if you were still touching
            bump = ( fvec3_dot_product( pchr_a->vel.v, nrm.v ) * fvec3_dot_product( pchr_a->vel_old.v, nrm.v ) < 0 ) ||
                   ( fvec3_dot_product( pchr_b->vel.v, nrm.v ) * fvec3_dot_product( pchr_b->vel_old.v, nrm.v ) < 0 );
        }

        // add in the friction due to the "collision"
        // assume coeff of friction of 0.5
        if ( ABS( imp_a.x ) + ABS( imp_a.y ) + ABS( imp_a.z ) > 0.0f &&
             ABS( vpara_a.x ) + ABS( vpara_a.y ) + ABS( vpara_a.z ) > 0.0f &&
             pchr_a->dismount_timer <= 0 )
        {
            float imp, vel, factor;

            imp = 0.5f * SQRT( imp_a.x * imp_a.x + imp_a.y * imp_a.y + imp_a.z * imp_a.z );
            vel = SQRT( vpara_a.x * vpara_a.x + vpara_a.y * vpara_a.y + vpara_a.z * vpara_a.z );

            factor = imp / vel;
            factor = CLIP( factor, 0.0f, 1.0f );

            pchr_a->phys.avel.x -= factor * vpara_a.x * interaction_strength;
            pchr_a->phys.avel.y -= factor * vpara_a.y * interaction_strength;
            pchr_a->phys.avel.z -= factor * vpara_a.z * interaction_strength;
            LOG_NAN( pchr_a->phys.avel.z );
        }

        if ( ABS( imp_b.x ) + ABS( imp_b.y ) + ABS( imp_b.z ) > 0.0f &&
             ABS( vpara_b.x ) + ABS( vpara_b.y ) + ABS( vpara_b.z ) > 0.0f &&
             pchr_b->dismount_timer <= 0 )
        {
            float imp, vel, factor;

            imp = 0.5f * SQRT( imp_b.x * imp_b.x + imp_b.y * imp_b.y + imp_b.z * imp_b.z );
            vel = SQRT( vpara_b.x * vpara_b.x + vpara_b.y * vpara_b.y + vpara_b.z * vpara_b.z );

            factor = imp / vel;
            factor = CLIP( factor, 0.0f, 1.0f );

            pchr_b->phys.avel.x -= factor * vpara_b.x * interaction_strength;
            pchr_b->phys.avel.y -= factor * vpara_b.y * interaction_strength;
            pchr_b->phys.avel.z -= factor * vpara_b.z * interaction_strength;
            LOG_NAN( pchr_b->phys.avel.z );
        }
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
bool_t do_prt_platform_physics( prt_t * pprt, chr_t * pplat, chr_prt_collsion_data_t * pdata )
{
    /// @details BB@> handle the particle interaction with a platform it is not attached "on".
    ///               @note gravity is not handled here

    bool_t plat_collision = bfalse;
    bool_t z_collide, was_z_collide;

    if ( NULL == pdata ) return bfalse;

    // is the platform a platform?
    if ( !ACTIVE_PCHR( pplat ) || !pplat->platform ) return bfalse;

    // can the particle interact with it?
    if ( !ACTIVE_PPRT( pprt ) || INGAME_CHR( pprt->attachedto_ref ) ) return bfalse;

    // this is handled elsewhere
    if ( GET_REF_PCHR( pplat ) == pprt->onwhichplatform_ref ) return bfalse;

    // Test to see whether the particle is in the right position to interact with the platform.
    // You have to be closer to a platform to interact with it then for a general object,
    // but the vertical distance is looser.
    plat_collision = test_interaction_close_1( pplat->chr_prt_cv, pplat->pos, pprt->bump_padded, prt_get_pos(pprt), btrue );

    if ( !plat_collision ) return bfalse;

    // the only way to get to this point is if the two objects don't collide
    // but they are within the PLATTOLERANCE of each other in the z direction
    // it is a valid platform. now figure out the physics

    // are they colliding for the first time?
    z_collide     = ( pprt->pos.z < pplat->pos.z + pplat->chr_prt_cv.maxs[OCT_Z] ) && ( pprt->pos.z > pplat->pos.z + pplat->chr_prt_cv.mins[OCT_Z] );
    was_z_collide = ( pprt->pos.z - pprt->vel.z < pplat->pos.z + pplat->chr_prt_cv.maxs[OCT_Z] - pplat->vel.z ) && ( pprt->pos.z - pprt->vel.z  > pplat->pos.z + pplat->chr_prt_cv.mins[OCT_Z] );

    if ( z_collide && !was_z_collide )
    {
        // Particle is falling onto the platform
        pprt->pos.z = pplat->pos.z + pplat->chr_prt_cv.maxs[OCT_Z];
        pprt->vel.z = pplat->vel.z - pprt->vel.z * pdata->ppip->dampen;

        // This should prevent raindrops from stacking up on the top of trees and other
        // objects
        if ( pdata->ppip->end_ground && pplat->platform )
        {
            pdata->terminate_particle = btrue;
        }

        plat_collision = btrue;
    }
    else if ( z_collide && was_z_collide )
    {
        // colliding this time and last time. particle is *embedded* in the platform
        pprt->pos.z = pplat->pos.z + pplat->chr_prt_cv.maxs[OCT_Z];

        if ( pprt->vel.z - pplat->vel.z < 0 )
        {
            pprt->vel.z = pplat->vel.z * pdata->ppip->dampen + platstick * pplat->vel.z;
        }
        else
        {
            pprt->vel.z = pprt->vel.z * ( 1.0f - platstick ) + pplat->vel.z * platstick;
        }
        pprt->vel.x = pprt->vel.x * ( 1.0f - platstick ) + pplat->vel.x * platstick;
        pprt->vel.y = pprt->vel.y * ( 1.0f - platstick ) + pplat->vel.y * platstick;

        plat_collision = btrue;
    }
    else
    {
        // not colliding this time or last time. particle is just near the platform
        float lerp_z = ( pprt->pos.z - ( pplat->pos.z + pplat->chr_prt_cv.maxs[OCT_Z] ) ) / PLATTOLERANCE;
        lerp_z = CLIP( lerp_z, -1, 1 );

        if ( lerp_z > 0 )
        {
            float tmp_platstick = platstick * lerp_z;
            pprt->vel.z = pprt->vel.z * ( 1.0f - tmp_platstick ) + pplat->vel.z * tmp_platstick;
            pprt->vel.x = pprt->vel.x * ( 1.0f - tmp_platstick ) + pplat->vel.x * tmp_platstick;
            pprt->vel.y = pprt->vel.y * ( 1.0f - tmp_platstick ) + pplat->vel.y * tmp_platstick;

            plat_collision = btrue;
        }
    }

    return plat_collision;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_deflect( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata )
{
    bool_t prt_deflected = bfalse;

    bool_t chr_is_invictus, chr_can_deflect;
    bool_t prt_wants_deflection;
    FACING_T direction;
    pip_t  *ppip;

    if ( NULL == pdata ) return bfalse;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

    if ( !ACTIVE_PPRT( pprt ) ) return bfalse;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return bfalse;
    ppip = PipStack.lst + pprt->pip_ref;

    /// @note ZF@> Simply ignore characters with invictus for now, it causes some strange effects
    if ( pchr->invictus ) return btrue;

    // find the "attack direction" of the particle
    direction = vec_to_facing( pchr->pos.x - pprt->pos.x, pchr->pos.y - pprt->pos.y );
    direction = pchr->ori.facing_z - direction + ATK_BEHIND;

    // shield block?
    chr_is_invictus = is_invictus_direction( direction, GET_REF_PCHR( pchr ), ppip->damfx );

    // determine whether the character is magically protected from missile attacks
    prt_wants_deflection  = ( MISSILE_NORMAL != pchr->missiletreatment ) &&
                            ( pprt->owner_ref != GET_REF_PCHR( pchr ) ) && !pdata->ppip->bump_money;

    chr_can_deflect = ( 0 != pchr->damagetime ) && ( pdata->max_damage > 0 );

    // try to deflect the particle
    prt_deflected = bfalse;
    pdata->mana_paid = bfalse;
    if ( chr_is_invictus || ( prt_wants_deflection && chr_can_deflect ) )
    {
        // magically deflect the particle or make a ricochet if the character is invictus
        int treatment;

        treatment     = MISSILE_DEFLECT;
        prt_deflected = btrue;
        if ( prt_wants_deflection )
        {
            treatment = pchr->missiletreatment;
            pdata->mana_paid = cost_mana( pchr->missilehandler, pchr->missilecost << 8, pprt->owner_ref );
            prt_deflected = pdata->mana_paid;
        }

        if ( prt_deflected )
        {
            // Treat the missile
            if ( treatment == MISSILE_DEFLECT )
            {
                // Deflect the incoming ray off the normal
                pdata->impulse.x -= 2.0f * pdata->dot * pdata->nrm.x;
                pdata->impulse.y -= 2.0f * pdata->dot * pdata->nrm.y;
                pdata->impulse.z -= 2.0f * pdata->dot * pdata->nrm.z;
            }
            else if ( treatment == MISSILE_REFLECT )
            {
                // Reflect it back in the direction it came
                pdata->impulse.x -= 2.0f * pprt->vel.x;
                pdata->impulse.y -= 2.0f * pprt->vel.y;
                pdata->impulse.z -= 2.0f * pprt->vel.z;

                // Change the owner of the missile
                pprt->team              = pchr->team;
                pprt->owner_ref         = GET_REF_PCHR( pchr );
                pdata->ppip->homing     = bfalse;
            }

            // Change the direction of the particle
            if ( pdata->ppip->rotatetoface )
            {
                // Turn to face new direction
                pprt->facing = vec_to_facing( pprt->vel.x , pprt->vel.y );
            }

            //Blocked!
            spawn_defense_ping( pchr, pprt->owner_ref );
        }
    }

    return prt_deflected;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_recoil( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata )
{
    /// @details BB@> make the character and particle recoil from the collision
    float prt_mass;
    float attack_factor;

    if ( NULL == pdata ) return 0;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;

    if ( !ACTIVE_PPRT( pprt ) ) return bfalse;

    if ( 0.0f == ABS( pdata->impulse.x ) + ABS( pdata->impulse.y ) + ABS( pdata->impulse.z ) ) return btrue;

    if ( !pdata->ppip->allowpush ) return bfalse;

    // do the reaction force of the particle on the character

    // determine how much the attack is "felt"
    attack_factor = 1.0f;
    if ( DAMAGE_CRUSH == pprt->damagetype )
    {
        // very blunt type of attack, the maximum effect
        attack_factor = 1.0f;
    }
    else if ( DAMAGE_POKE == pprt->damagetype )
    {
        // very focussed type of attack, the minimum effect
        attack_factor = 0.5f;
    }
    else
    {
        // all other damage types are in the middle
        attack_factor = INV_SQRT_TWO;
    }

    prt_mass = 1.0f;
    if ( 0 == pdata->max_damage )
    {
        // this is a particle like the wind particles in the whirlwind
        // make the particle have some kind of predictable constant effect
        // relative to any character;
        prt_mass = pchr->phys.weight / 10.0f;
    }
    else
    {
        // determine an "effective mass" for the particle, based on it's max damage
        // and velocity

        float prt_vel2;
        float prt_ke;

        // the damage is basically like the kinetic energy of the particle
        prt_vel2 = fvec3_dot_product( pdata->vdiff.v, pdata->vdiff.v );

        // It can happen that a damage particle can hit something
        // at almost zero velocity, which would make for a huge "effective mass".
        // by making a reasonable "minimum velocity", we limit the maximum mass to
        // something reasonable
        prt_vel2 = MAX( 100.0f, prt_vel2 );

        // get the "kinetic energy" from the damage
        prt_ke = 3.0f * pdata->max_damage;

        // the faster the particle is going, the smaller the "mass" it
        // needs to do the damage
        prt_mass = prt_ke / ( 0.5f * prt_vel2 );
    }

    // now, we have the particle's impulse and mass
    // Do the impulse to the object that was hit
    // If the particle was magically deflected, there is no rebound on the target
    if ( pchr->phys.weight != INFINITE_WEIGHT && !pdata->mana_paid )
    {
        float factor = attack_factor;

        if ( pchr->phys.weight > 0 )
        {
            // limit the prt_mass to be something relatice to this object
            float loc_prt_mass = CLIP( prt_mass, 1.0f, 2.0f * pchr->phys.weight );

            factor *= loc_prt_mass / pchr->phys.weight;
        }

        // modify it by the the severity of the damage
        // reduces the damage below pdata->actual_damage == pchr->lifemax
        // and it doubles it if pdata->actual_damage is really huge
        //factor *= 2.0f * ( float )pdata->actual_damage / ( float )( ABS( pdata->actual_damage ) + pchr->lifemax );

        factor = CLIP( factor, 0.0f, 3.0f );

        // calculate the "impulse"
        pchr->phys.avel.x -= pdata->impulse.x * factor;
        pchr->phys.avel.y -= pdata->impulse.y * factor;
        pchr->phys.avel.z -= pdata->impulse.z * factor;
    }

    // if the particle is attached to a weapon, the particle can force the
    // weapon (actually, the weapon's holder) to rebound.
    if ( INGAME_CHR( pprt->attachedto_ref ) )
    {
        chr_t * ptarget;
        CHR_REF iholder;

        ptarget = NULL;

        // transmit the force of the blow back to the character that is
        // holding the weapon

        iholder = chr_get_lowest_attachment( pprt->attachedto_ref, bfalse );
        if ( INGAME_CHR( iholder ) )
        {
            ptarget = ChrList.lst + iholder;
        }
        else
        {
            iholder = chr_get_lowest_attachment( pprt->owner_ref, bfalse );
            if ( INGAME_CHR( iholder ) )
            {
                ptarget = ChrList.lst + iholder;
            }
        }

        if ( ptarget->phys.weight != INFINITE_WEIGHT )
        {
            float factor = attack_factor;

            if ( ptarget->phys.weight > 0 )
            {
                // limit the prt_mass to be something relative to this object
                float loc_prt_mass = CLIP( prt_mass, 1.0f, 2.0f * ptarget->phys.weight );

                factor *= ( float ) loc_prt_mass / ( float )ptarget->phys.weight;
            }

            factor = CLIP( factor, 0.0f, 3.0f );

            // in the SAME direction as the particle
            ptarget->phys.avel.x += pdata->impulse.x * factor;
            ptarget->phys.avel.y += pdata->impulse.y * factor;
            ptarget->phys.avel.z += pdata->impulse.z * factor;
        }
    }

    // apply the impulse to the particle velocity
    pprt->vel.x += pdata->impulse.x;
    pprt->vel.y += pdata->impulse.y;
    pprt->vel.z += pdata->impulse.z;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_damage( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata )
{
    ENC_REF enchant, enc_next;
    bool_t prt_needs_impact;

    if ( NULL == pdata ) return bfalse;
    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;
    if ( !ACTIVE_PPRT( pprt ) ) return bfalse;

    // clean up the enchant list before doing anything
    cleanup_character_enchants( pchr );

    // Check all enchants to see if they are removed
    enchant = pchr->firstenchant;
    while ( enchant != MAX_ENC )
    {
        enc_next = EncList.lst[enchant].nextenchant_ref;
        if ( enc_is_removed( enchant, pprt->profile_ref ) )
        {
            remove_enchant( enchant, NULL );
        }
        enchant = enc_next;
    }

    // Do confuse effects
    if ( pdata->ppip->grogtime > 0 && pdata->pcap->canbegrogged )
    {
        pchr->ai.alert |= ALERTIF_GROGGED;
        if ( pdata->ppip->grogtime > pchr->grogtime )
        {
            pchr->grogtime = MAX( 0, pchr->grogtime + pdata->ppip->grogtime );
        }
    }
    if ( pdata->ppip->dazetime > 0 && pdata->pcap->canbedazed )
    {
        pchr->ai.alert |= ALERTIF_DAZED;
        if ( pdata->ppip->dazetime > pchr->dazetime )
        {
            pchr->dazetime = MAX( 0, pchr->dazetime + pdata->ppip->dazetime );
        }
    }

    //---- Damage the character, if necessary
	if( ( pprt->damage.base + pprt->damage.base ) != 0 )
	{
		prt_needs_impact = pdata->ppip->rotatetoface || INGAME_CHR( pprt->attachedto_ref );
		if ( INGAME_CHR( pprt->owner_ref ) )
		{
			chr_t * powner = ChrList.lst + pprt->owner_ref;
			cap_t * powner_cap = pro_get_pcap( powner->profile_ref );

			if ( powner_cap->isranged ) prt_needs_impact = btrue;
		}

		// DAMFX_ARRO means that it only does damage to the one it's attached to
		if ( HAS_NO_BITS( pdata->ppip->damfx, DAMFX_ARRO ) && !( prt_needs_impact && !( pdata->dot < 0.0f ) ) )
		{
			FACING_T direction;
			IPair loc_damage = pprt->damage;

			direction = vec_to_facing( pprt->vel.x , pprt->vel.y );
			direction = pchr->ori.facing_z - direction + ATK_BEHIND;

			//These things only apply if the particle has an owner
			if ( INGAME_CHR( pprt->owner_ref ) )
			{
				CHR_REF item;
				int drain;
				chr_t * powner = ChrList.lst + pprt->owner_ref;

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

				//Steal some life
				if ( pprt->lifedrain > 0 )
				{
					drain = pchr->life;
					pchr->life = CLIP( pchr->life, 1, pchr->life - pprt->lifedrain );
					drain -= pchr->life;
					powner->life = MIN( powner->life + drain, powner->lifemax );
				}

				//Steal some mana
				if ( pprt->manadrain > 0 )
				{
					drain = pchr->mana;
					pchr->mana = CLIP( pchr->mana, 0, pchr->mana - pprt->manadrain );
					drain -= pchr->mana;
					powner->mana = MIN( powner->mana + drain, powner->manamax );
				}
				
				// Notify the attacker of a scored hit
				powner->ai.alert |= ALERTIF_SCOREDAHIT;
				powner->ai.hitlast = GET_REF_PCHR( pchr );

				//Tell the weapons who the attacker hit last
				item = powner->holdingwhich[SLOT_LEFT];
				if ( INGAME_CHR( item ) )
				{
					ChrList.lst[item].ai.hitlast = GET_REF_PCHR( pchr );
				}

				item = powner->holdingwhich[SLOT_RIGHT];
				if ( INGAME_CHR( item ) )
				{
					ChrList.lst[item].ai.hitlast = GET_REF_PCHR( pchr );
				}
			}
			
			// handle vulnerabilities, double the damage
			if ( chr_has_vulnie( GET_REF_PCHR( pchr ), pprt->profile_ref ) )
			{
				loc_damage.base = ( loc_damage.base << 1 );
				loc_damage.rand = ( loc_damage.rand << 1 ) | 1;

				pchr->ai.alert |= ALERTIF_HITVULNERABLE;
			}

			// Damage the character
			pdata->actual_damage = damage_character( GET_REF_PCHR( pchr ), direction, loc_damage, pprt->damagetype, pprt->team, pprt->owner_ref, pdata->ppip->damfx, bfalse );
		}
	}

    //---- estimate the impulse on the particle
    if ( pdata->dot < 0.0f )
    {
        if ( 0 == ABS( pdata->max_damage ) || ( ABS( pdata->max_damage ) - ABS( pdata->actual_damage ) ) == 0 )
        {
            // the simple case
            pdata->impulse.x -= pprt->vel.x;
            pdata->impulse.y -= pprt->vel.y;
            pdata->impulse.z -= pprt->vel.z;
        }
        else
        {
            float recoil;
            fvec3_t vfinal, impulse_tmp;

            vfinal.x = pprt->vel.x - 2 * pdata->dot * pdata->nrm.x;
            vfinal.y = pprt->vel.y - 2 * pdata->dot * pdata->nrm.y;
            vfinal.z = pprt->vel.z - 2 * pdata->dot * pdata->nrm.z;

            // assume that the particle damage is like the kinetic energy,
            // then vfinal must be scaled by recoil^2
            recoil = ( float )ABS( ABS( pdata->max_damage ) - ABS( pdata->actual_damage ) ) / ( float )ABS( pdata->max_damage );

            vfinal.x *= recoil * recoil;
            vfinal.y *= recoil * recoil;
            vfinal.z *= recoil * recoil;

            impulse_tmp = fvec3_sub( vfinal.v, pprt->vel.v );

            pdata->impulse.x += impulse_tmp.x;
            pdata->impulse.y += impulse_tmp.y;
            pdata->impulse.z += impulse_tmp.z;
        }

    }

    return btrue;
}
//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_bump( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata )
{
    bool_t prt_belongs_to_chr;
    bool_t prt_hates_chr, prt_attacks_chr, prt_hateonly;
    bool_t valid_onlydamagefriendly;
    bool_t valid_friendlyfire;
    bool_t valid_onlydamagehate;

    if ( NULL == pdata ) return bfalse;
    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;
    if ( !ACTIVE_PPRT( pprt ) ) return bfalse;

    // if the particle was deflected, then it can't bump the character
    if ( pchr->invictus || pprt->attachedto_ref == GET_REF_PCHR( pchr ) ) return bfalse;

    prt_belongs_to_chr = ( GET_REF_PCHR( pchr ) == pprt->owner_ref );

    if ( !prt_belongs_to_chr )
    {
        // no simple owner relationship. Check for something deeper.
        CHR_REF prt_owner = prt_get_iowner( GET_REF_PPRT( pprt ), 0 );
        if ( INGAME_CHR( prt_owner ) )
        {
            CHR_REF chr_wielder = chr_get_lowest_attachment( GET_REF_PCHR( pchr ),   btrue );
            CHR_REF prt_wielder = chr_get_lowest_attachment( prt_owner, btrue );

            if ( !INGAME_CHR( chr_wielder ) ) chr_wielder = GET_REF_PCHR( pchr );
            if ( !INGAME_CHR( prt_wielder ) ) prt_wielder = prt_owner;

            prt_belongs_to_chr = ( chr_wielder == prt_wielder );
        }
    }

    // does the particle team hate the character's team
    prt_hates_chr = team_hates_team( pprt->team, pchr->team );

    // Only bump into hated characters?
    prt_hateonly = PipStack.lst[pprt->pip_ref].hateonly;
    valid_onlydamagehate = prt_hates_chr && PipStack.lst[pprt->pip_ref].hateonly;

    // allow neutral particles to attack anything
    prt_attacks_chr = prt_hates_chr || (( TEAM_NULL != pchr->team ) && ( TEAM_NULL == pprt->team ) );

    // this is the onlydamagefriendly condition from the particle search code
    valid_onlydamagefriendly = ( pdata->ppip->onlydamagefriendly && pprt->team == pchr->team ) ||
                               ( !pdata->ppip->onlydamagefriendly && prt_attacks_chr );

    // I guess "friendly fire" does not mean "self fire", which is a bit unfortunate.
    valid_friendlyfire = ( pdata->ppip->friendlyfire && !prt_hates_chr && !prt_belongs_to_chr ) ||
                         ( !pdata->ppip->friendlyfire && prt_attacks_chr );

    pdata->prt_bumps_chr =  valid_friendlyfire || valid_onlydamagefriendly || valid_onlydamagehate;

    return pdata->prt_bumps_chr;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_handle_bump( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata )
{
    if ( NULL == pdata || !pdata->prt_bumps_chr ) return bfalse;

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;
    if ( !ACTIVE_PPRT( pprt ) ) return bfalse;

    if ( !pdata->prt_bumps_chr ) return bfalse;

    // Catch on fire
    spawn_bump_particles( GET_REF_PCHR( pchr ), GET_REF_PPRT( pprt ) );

    //handle some special particle interactions
    if ( pdata->ppip->end_bump )
    {
        if ( pdata->ppip->bump_money )
        {
            chr_t * pcollector = pchr;

            // Let mounts collect money for their riders
            if ( pchr->ismount && INGAME_CHR( pchr->holdingwhich[SLOT_LEFT] ) )
            {
                pcollector = ChrList.lst + pchr->holdingwhich[SLOT_LEFT];

                // if the mount's rider can't get money, the mount gets to keep the money!
                if ( !pcollector->cangrabmoney )
                {
                    pcollector = pchr;
                }
            }

            if ( pcollector->cangrabmoney && pcollector->alive && 0 == pcollector->damagetime && pcollector->money < MAXMONEY )
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
bool_t do_chr_prt_collision_init( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata )
{
    bool_t full_collision;

    if ( NULL == pdata ) return bfalse;

    memset( pdata, 0, sizeof( *pdata ) );

    if ( !ACTIVE_PCHR( pchr ) ) return bfalse;
    if ( !ACTIVE_PPRT( pprt ) ) return bfalse;

    // initialize the collision date
    pdata->pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pdata->pcap ) return bfalse;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return bfalse;
    pdata->ppip = PipStack.lst + pprt->pip_ref;

    // measure the collision depth
    full_collision = get_depth_1( pchr->chr_prt_cv, pchr->pos, pprt->bump_padded, prt_get_pos(pprt), btrue, pdata->odepth );

    //// measure the collision depth in the last update
    //// the objects were not touching last frame, so they must have collided this frame
    //collision = !get_depth_close_1( pchr_a->chr_prt_cv, pchr_a->pos_old, pprt_b->bump, pprt_b->pos_old, btrue, odepth_old );

    // estimate the maximum possible "damage" from this particle
    // other effects can magnify this number, like vulnerabilities
    // or DAMFX_* bits
    pdata->max_damage = ABS( pprt->damage.base ) + ABS( pprt->damage.rand );

    return full_collision;
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

    CHR_REF ichr_a;
    PRT_REF iprt_b;

    chr_t * pchr_a;
    prt_t * pprt_b;

    bool_t prt_deflected;
    bool_t prt_can_hit_chr;

    bool_t plat_collision, full_collision;

    chr_prt_collsion_data_t cn_lst;

    if ( NULL == d || TOTAL_MAX_PRT != d->prta || MAX_CHR != d->chrb ) return bfalse;
    ichr_a = d->chra;
    iprt_b = d->prtb;

    // make sure that it is on
    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    if ( !pchr_a->alive ) return bfalse;

    if ( !INGAME_PRT( iprt_b ) ) return bfalse;
    pprt_b = PrtList.lst + iprt_b;

    if ( ichr_a == pprt_b->attachedto_ref ) return bfalse;

    // detect a full collision
    full_collision = do_chr_prt_collision_init( pchr_a, pprt_b, &cn_lst );

    // platform interaction. we can still have a platform interaction even if there
    // is not a "full_collision" since the z-distance thes
    plat_collision = bfalse;
    if ( pchr_a->platform && !INGAME_CHR( pprt_b->attachedto_ref ) )
    {
        plat_collision = do_prt_platform_physics( pprt_b, pchr_a, &cn_lst );
    }

    // if there is no collision, no point in going farther
    if ( !full_collision && !plat_collision ) return bfalse;

    // estimate the "normal" for the collision, using the center-of-mass difference
    // put this off until this point to reduce calling this "expensive" function
    cn_lst.dot = estimate_chr_prt_normal( pchr_a, pprt_b, cn_lst.nrm.v, cn_lst.vdiff.v );

    // handle particle deflection.
    // if the platform collision was already handled, there is nothing left to do
    prt_deflected = bfalse;
    if ( full_collision && !plat_collision )
    {
        // determine whether the particle is deflected by the character
        if ( cn_lst.dot < 0.0f )
        {
            prt_deflected = do_chr_prt_collision_deflect( pchr_a, pprt_b, &cn_lst );
            if ( prt_deflected )
            {
                retval = btrue;
            }
        }
    }

    // do "damage" to the character
    if ( !prt_deflected && 0 == pchr_a->damagetime )
    {
        // Check reaffirmation of particles
        if ( (pprt_b->damage.base > 0 || pprt_b->damage.rand > 0) && pchr_a->reaffirmdamagetype == pprt_b->damagetype )
        {
            retval = ( 0 != reaffirm_attached_particles( ichr_a ) );
        }

        // refine the logic for a particle to hit a character
        prt_can_hit_chr = do_chr_prt_collision_bump( pchr_a, pprt_b, &cn_lst );

        // does the particle damage/heal the character?
        if ( prt_can_hit_chr )
        {
            // we can't even get to this point if the character is completely invulnerable (invictus)
            // or can't be damaged this round
            cn_lst.prt_damages_chr = do_chr_prt_collision_damage( pchr_a, pprt_b, &cn_lst );
            if ( cn_lst.prt_damages_chr )
            {
                retval = btrue;
            }
        }
    }

    // make the character and particle recoil from the collision
    if ( ABS( cn_lst.impulse.x ) + ABS( cn_lst.impulse.y ) + ABS( cn_lst.impulse.z ) > 0.0f )
    {
        if ( do_chr_prt_collision_recoil( pchr_a, pprt_b, &cn_lst ) )
        {
            retval = btrue;
        }
    }

    // handle a couple of special cases
    if ( cn_lst.prt_bumps_chr )
    {
        if ( do_chr_prt_collision_handle_bump( pchr_a, pprt_b, &cn_lst ) )
        {
            retval = btrue;
        }
    }

    // terminate the particle if needed
    if ( cn_lst.terminate_particle )
    {
        prt_request_terminate_ref( iprt_b );
        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// OBSOLETE CODE
//--------------------------------------------------------------------------------------------

//void fill_bumplists()
//{
//    CHR_REF character; PRT_REF particle;
//    Uint32 fanblock;
//
//    // Clear the lists
//    for ( fanblock = 0; fanblock < PMesh->gmem.blocks_count; fanblock++ )
//    {
//        bumplist[fanblock].chr    = (CHR_REF)MAX_CHR;
//        bumplist[fanblock].chrnum = 0;
//
//        bumplist[fanblock].prt    = TOTAL_MAX_PRT;
//        bumplist[fanblock].prtnum = 0;
//    }
//
//    // Fill 'em back up
//    for ( character = 0; character < MAX_CHR; character++ )
//    {
//        chr_t * pchr;
//
//        if ( !INGAME_CHR( character ) ) continue;
//        pchr = ChrList.lst + character;
//
//        // reset the platform stuff each update
//        pchr->holdingweight   = 0;
//        pchr->onwhichplatform_ref = (CHR_REF)MAX_CHR;
//        pchr->enviro.level    = pchr->enviro.floor_level;
//
//        // reset the fan and block position
//        pchr->onwhichgrid   = mesh_get_tile( PMesh, pchr->pos.x, pchr->pos.y );
//        pchr->onwhichblock = mesh_get_block( PMesh, pchr->pos.x, pchr->pos.y );
//
//        // reject characters that are in packs, or are marked as non-colliding
//        if ( pchr->pack.is_packed ) continue;
//
//        // reject characters that are hidden
//        if ( pchr->is_hidden ) continue;
//
//        if ( INVALID_BLOCK != pchr->onwhichblock )
//        {
//            // Insert before any other characters on the block
//            pchr->bumplist_next = bumplist[pchr->onwhichblock].chr;
//            bumplist[pchr->onwhichblock].chr = character;
//            bumplist[pchr->onwhichblock].chrnum++;
//        }
//    }
//
//    for ( particle = 0; particle < maxparticles; particle++ )
//    {
//        prt_t * pprt;
//
//        // reject invalid particles
//        if ( !INGAME_PRT( particle ) ) continue;
//        pprt = PrtList.lst + particle;
//
//        pprt->onwhichplatform_ref = (CHR_REF)MAX_CHR;
//        prt_set_level( pprt, pprt->enviro.floor_level );
//
//        // reject characters that are hidden
//        if ( pprt->is_hidden ) continue;
//
//        // reset the fan and block position
//        pprt->onwhichgrid   = mesh_get_tile( PMesh, pprt->pos.x, pprt->pos.y );
//        pprt->onwhichblock = mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );
//
//        if ( INVALID_BLOCK != pprt->onwhichblock )
//        {
//            // Insert before any other particles on the block
//            pprt->bumplist_next = bumplist[pprt->onwhichblock].prt;
//            bumplist[pprt->onwhichblock].prt = particle;
//            bumplist[pprt->onwhichblock].prtnum++;
//        }
//    }
//}

//--------------------------------------------------------------------------------------------
//bool_t do_collisions( CHashList_t * pchlst, CoNode_ary_t * pcn_lst, HashNode_ary_t * phn_lst )
//{
//    int cnt, tnc;
//
//    if( NULL == pclst ) return bfalse;
//
//    // Do collisions
//    chr_collisions = 0;
//    if ( CoNode_ary_get_size(pcn_lst) > 0)
//    {
//        //process the saved interactions
//        for ( cnt = 0; cnt < CHashList_get_allocd(pchlst); cnt++ )
//        {
//            s_CoNode * pn;
//            int count = CHashList_get_count( pchlst, cnt);
//
//            chr_collisions += count;
//            pn = CHashList_get_node( pchlst, cnt);
//            for ( tnc = 0; tnc < count && NULL != pn; tnc++, pn = CoNode_get_next(pn) )
//            {
//                if (TOTAL_MAX_PRT == pn->prta)
//                {
//                    // object A must be a character
//                    if (MAX_PRT == pn->prtb && MAX_TILE == pn->tileb)
//                    {
//                        do_chr_chr_collision(pzone, pn, dUpdate );
//                    }
//                    else if (MAX_CHR == pn->chrb && MAX_TILE == pn->tileb)
//                    {
//                        do_chr_prt_collision(pzone, pn, dUpdate);
//                    }
//                    else
//                    {
//                        // this is an "impossible" situation. must be corrupted data?
//                        EGOBOO_ASSERTbfalse);
//                    }
//                }
//                else
//                {
//                    // object A must be a particle. the only valid action at this point would be to
//                    // do a particle-mesh interaction
//                }
//            }
//        }
//    }
//}

//--------------------------------------------------------------------------------------------
//bool_t detect_chr_chr_interaction( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b )
//{
//    bool_t interact_x  = bfalse;
//    bool_t interact_y  = bfalse;
//    bool_t interact_xy = bfalse;
//    bool_t interact_z  = bfalse;
//
//    float xa, ya, za;
//    float xb, yb, zb;
//    float dxy, dx, dy, depth_z;
//
//    chr_t *pchr_a, *pchr_b;
//    cap_t *pcap_a, *pcap_b;
//
//    if ( !detect_chr_chr_interaction_valid( ichr_a, ichr_b ) ) return bfalse;
//
//    // Ignore invalid characters
//    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
//    pchr_a = ChrList.lst + ichr_a;
//
//    pcap_a = chr_get_pcap( ichr_a );
//    if ( NULL == pcap_a ) return bfalse;
//
//    // Ignore invalid characters
//    if ( !INGAME_CHR( ichr_b ) ) return bfalse;
//    pchr_b = ChrList.lst + ichr_b;
//
//    pcap_b = chr_get_pcap( ichr_b );
//    if ( NULL == pcap_b ) return bfalse;
//
//    xa = pchr_a->pos.x;
//    ya = pchr_a->pos.y;
//    za = pchr_a->pos.z;
//
//    xb = pchr_b->pos.x;
//    yb = pchr_b->pos.y;
//    zb = pchr_b->pos.z;
//
//    // First check absolute value diamond
//    dx = ABS( xa - xb );
//    dy = ABS( ya - yb );
//    dxy = dx + dy;
//
//    // detect z interactions based on the actual vertical extent of the bounding box
//    depth_z = MIN( zb + pchr_b->chr_chr_cv.maxs[OCT_Z], za + pchr_a->chr_chr_cv.maxs[OCT_Z] ) -
//              MAX( zb + pchr_b->chr_chr_cv.mins[OCT_Z], za + pchr_a->chr_chr_cv.mins[OCT_Z] );
//
//    // detect x-y interactions based on a potentially gigantor bounding box
//    interact_x  = ( dx  <= pchr_a->bump_1.size    + pchr_b->bump_1.size );
//    interact_y  = ( dy  <= pchr_a->bump_1.size    + pchr_b->bump_1.size );
//    interact_xy = ( dxy <= pchr_a->bump_1.size_big + pchr_b->bump_1.size_big );
//
//    if (( pchr_a->platform && pchr_b->canuseplatforms ) ||
//        ( pchr_b->platform && pchr_a->canuseplatforms ) )
//    {
//        interact_z  = ( depth_z > -PLATTOLERANCE );
//    }
//    else
//    {
//        interact_z  = ( depth_z > 0 );
//    }
//
//    return interact_x && interact_y && interact_xy && interact_z;
//}

//--------------------------------------------------------------------------------------------
//bool_t detect_chr_prt_interaction( const CHR_REF by_reference ichr_a, const PRT_REF by_reference iprt_b )
//{
//    bool_t interact_x  = bfalse;
//    bool_t interact_y  = bfalse;
//    bool_t interact_xy = bfalse;
//    bool_t interact_z  = bfalse;
//    bool_t interact_platform = bfalse;
//
//    float dxy, dx, dy, depth_z;
//
//    chr_t * pchr_a;
//    prt_t * pprt_b;
//
//    if ( !detect_chr_prt_interaction_valid( ichr_a, iprt_b ) ) return bfalse;
//
//    // Ignore invalid characters
//    if ( !INGAME_CHR( ichr_a ) ) return bfalse;
//    pchr_a = ChrList.lst + ichr_a;
//
//    // Ignore invalid characters
//    if ( !INGAME_PRT( iprt_b ) ) return bfalse;
//    pprt_b = PrtList.lst + iprt_b;
//
//    // First check absolute value diamond
//    dx = ABS( pchr_a->pos.x - pprt_b->pos.x );
//    dy = ABS( pchr_a->pos.y - pprt_b->pos.y );
//    dxy = dx + dy;
//
//    // estimate the horizontal interactions this frame
//    interact_x  = ( dx  <= ( pchr_a->bump_1.size     + pprt_b->bump_padded.size ) );
//    interact_y  = ( dy  <= ( pchr_a->bump_1.size     + pprt_b->bump_padded.size ) );
//    interact_xy = ( dxy <= ( pchr_a->bump_1.size_big + pprt_b->bump_padded.size_big ) );
//
//    if ( !interact_x || !interact_y || !interact_xy ) return bfalse;
//
//    interact_platform = bfalse;
//    if ( pchr_a->platform && !INGAME_CHR( pprt_b->attachedto_ref ) )
//    {
//        // estimate the vertical interactions this frame
//        depth_z = pprt_b->pos.z - ( pchr_a->pos.z + pchr_a->bump_1.height );
//        interact_platform = ( depth_z > -PLATTOLERANCE && depth_z < PLATTOLERANCE );
//    }
//
//    depth_z     = MIN( pchr_a->pos.z + pchr_a->chr_prt_cv.maxs[OCT_Z], pprt_b->pos.z + pprt_b->bump_padded.height ) - MAX( pchr_a->pos.z + pchr_a->chr_prt_cv.mins[OCT_Z], pprt_b->pos.z - pprt_b->bump_padded.height );
//    interact_z  = ( depth_z > 0 ) || interact_platform;
//
//    return interact_z;
//}
