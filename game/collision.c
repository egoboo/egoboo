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

#include "char.h"
#include "particle.h"
#include "enchant.h"
#include "profile.h"

#include "game.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define CHR_MAX_COLLISIONS    512*16
#define COLLISION_HASH_NODES (CHR_MAX_COLLISIONS*2)

#define MAKE_HASH(AA,BB) CLIP_TO_08BITS( ((AA) * 0x0111 + 0x006E) + ((BB) * 0x0111 + 0x006E) )

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
    Uint16  chr;                     // For character collisions
    Uint16  chrnum;                  // Number on the block
    Uint16  prt;                     // For particle collisions
    Uint16  prtnum;                  // Number on the block
};
typedef struct s_bumplist bumplist_t;

//--------------------------------------------------------------------------------------------
/// element for storing pair-wise "collision" data
struct s_collision_data
{
    Uint16 chra, chrb;
    Uint16 prtb;
};

typedef struct s_collision_data co_data_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t add_chr_chr_interaction( Uint16 ichr_a, Uint16 ichr_b, co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count );
static bool_t add_chr_prt_interaction( Uint16 ichr_a, Uint16 iprt_b, co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count );

static bool_t detect_chr_chr_interaction( Uint16 ichr_a, Uint16 ichr_b );
static bool_t detect_chr_prt_interaction( Uint16 ichr_a, Uint16 iprt_b );

static bool_t do_chr_platform_detection( Uint16 ichr_a, Uint16 ichr_b );
static bool_t do_prt_platform_detection( Uint16 ichr_a, Uint16 iprt_b );

static bool_t attach_chr_to_platform( chr_t * pchr, chr_t * pplat );
static bool_t attach_prt_to_platform( prt_t * pprt, chr_t * pplat );

static void fill_interaction_list( co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count );
static void fill_bumplists( void );

static void bump_all_platforms( void );
static void bump_all_mounts( void );
static void bump_all_collisions( void );


static bool_t do_mounts( Uint16 ichr_a, Uint16 ichr_b );
static bool_t do_chr_platform_physics( chr_t * pitem, chr_t * pplat );
static float  estimate_chr_prt_normal( chr_t * pchr, prt_t * pprt, fvec3_base_t nrm, fvec3_base_t vdiff );
static bool_t do_chr_chr_collision( Uint16 ichr_a, Uint16 ichr_b );

static bool_t do_chr_prt_collision_deflect( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata  );
static bool_t do_chr_prt_collision_recoil( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_damage( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_bump( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision_handle_bump(chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata);
static bool_t do_chr_prt_collision_init( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata );
static bool_t do_chr_prt_collision( Uint16 ichr_a, Uint16 iprt_b );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bumplist_t bumplist[MAXMESHFAN/16];

static int           chr_co_count = 0;
static hash_list_t * chr_co_list;

// collision data
static int       cdata_count = 0;
static co_data_t cdata[CHR_MAX_COLLISIONS];

// collision data hash nodes
static int         hn_count = 0;
static hash_node_t hnlst[COLLISION_HASH_NODES];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t detect_chr_chr_interaction( Uint16 ichr_a, Uint16 ichr_b )
{
    bool_t interact_x  = bfalse;
    bool_t interact_y  = bfalse;
    bool_t interact_xy = bfalse;
    bool_t interact_z  = bfalse;

    float xa, ya, za;
    float xb, yb, zb;
    float dxy, dx, dy, depth_z;

    chr_t *pchr_a, *pchr_b;
    cap_t *pcap_a, *pcap_b;

    // Don't interact with self
    if ( ichr_a == ichr_b ) return bfalse;

    // Ignore invalid characters
    if ( !ACTIVE_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    pcap_a = chr_get_pcap( ichr_a );
    if ( NULL == pcap_a ) return bfalse;

    // Ignore invalid characters
    if ( !ACTIVE_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    pcap_b = chr_get_pcap( ichr_b );
    if ( NULL == pcap_b ) return bfalse;

    xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    xb = pchr_b->pos.x;
    yb = pchr_b->pos.y;
    zb = pchr_b->pos.z;

    // don't interact if there is no interaction
    if ( 0 == pchr_a->bump.size || 0 == pchr_b->bump.size ) return bfalse;

    // don't interact with your mount, or your held items
    if ( ichr_a == pchr_b->attachedto || ichr_b == pchr_a->attachedto ) return bfalse;

    // reject characters that are hidden
    if ( pchr_a->is_hidden || pchr_b->is_hidden ) return bfalse;

    // First check absolute value diamond
    dx = ABS( xa - xb );
    dy = ABS( ya - yb );
    dxy = dx + dy;

    // detect z interactions based on the actual vertical extent of the bounding box
    depth_z = MIN( zb + pchr_b->chr_chr_cv.maxs[OCT_Z], za + pchr_a->chr_chr_cv.maxs[OCT_Z] ) -
              MAX( zb + pchr_b->chr_chr_cv.mins[OCT_Z], za + pchr_a->chr_chr_cv.mins[OCT_Z] );

    // detect x-y interactions based on a potentially gigantor bounding box
    interact_x  = ( dx  <= pchr_a->bump_1.size    + pchr_b->bump_1.size );
    interact_y  = ( dy  <= pchr_a->bump_1.size    + pchr_b->bump_1.size );
    interact_xy = ( dxy <= pchr_a->bump_1.sizebig + pchr_b->bump_1.sizebig );

    if (( pchr_a->platform && pcap_b->canuseplatforms ) ||
        ( pchr_b->platform && pcap_a->canuseplatforms ) )
    {
        interact_z  = ( depth_z > -PLATTOLERANCE );
    }
    else
    {
        interact_z  = ( depth_z > 0 );
    }

    return interact_x && interact_y && interact_xy && interact_z;
}

//--------------------------------------------------------------------------------------------
bool_t detect_chr_prt_interaction( Uint16 ichr_a, Uint16 iprt_b )
{
    bool_t interact_x  = bfalse;
    bool_t interact_y  = bfalse;
    bool_t interact_xy = bfalse;
    bool_t interact_z  = bfalse;
    bool_t interact_platform = bfalse;

    float dxy, dx, dy, depth_z;

    chr_t * pchr_a;
    prt_t * pprt_b;

    // Ignore invalid characters
    if ( !ACTIVE_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    // Ignore invalid characters
    if ( !ACTIVE_PRT( iprt_b ) ) return bfalse;
    pprt_b = PrtList.lst + iprt_b;

    // reject characters that are hidden
    if ( pchr_a->is_hidden || pprt_b->is_hidden ) return bfalse;

    // particles don't "collide" with anything they are attached to.
    // that only happes through doing bump particle damamge
    if ( ichr_a == pprt_b->attachedto_ref ) return bfalse;

    // don't interact if there is no interaction
    //if ( 0 == pchr_a->bump_1.size || 0 == pprt_b->bumpsize ) return bfalse;

    // First check absolute value diamond
    dx = ABS( pchr_a->pos.x - pprt_b->pos.x );
    dy = ABS( pchr_a->pos.y - pprt_b->pos.y );
    dxy = dx + dy;

    // estimate the horizontal interactions this frame
    interact_x  = ( dx  <= ( pchr_a->bump_1.size    + pprt_b->bump.size ) );
    interact_y  = ( dy  <= ( pchr_a->bump_1.size    + pprt_b->bump.size ) );
    interact_xy = ( dxy <= ( pchr_a->bump_1.sizebig + pprt_b->bump.sizebig ) );

    if ( !interact_x || !interact_y || !interact_xy ) return bfalse;

    interact_platform = bfalse;
    if( pchr_a->platform && !ACTIVE_CHR( pprt_b->attachedto_ref ) )
    {
        // estimate the vertical interactions this frame
        depth_z = pprt_b->pos.z - ( pchr_a->pos.z + pchr_a->bump_1.height );
        interact_platform = ( depth_z > -PLATTOLERANCE && depth_z < PLATTOLERANCE );
    }

    depth_z     = MIN( pchr_a->pos.z + pchr_a->chr_prt_cv.maxs[OCT_Z], pprt_b->pos.z + pprt_b->bump.height ) - MAX( pchr_a->pos.z + pchr_a->chr_prt_cv.mins[OCT_Z], pprt_b->pos.z - pprt_b->bump.height );
    interact_z  = ( depth_z > 0 ) || interact_platform;

    return interact_z;
}

//--------------------------------------------------------------------------------------------
bool_t add_chr_chr_interaction( Uint16 ichr_a, Uint16 ichr_b, co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count )
{
    Uint32 hashval = 0;
    int count;
    bool_t found;

    hash_node_t * n;
    co_data_t   * d;

    // there is no situation in the game where we allow characters to interact with themselves
    if ( ichr_a == ichr_b ) return bfalse;

    // create a hash that is order-independent
    hashval = MAKE_HASH( ichr_a, ichr_b );

    found = bfalse;
    count = chr_co_list->subcount[hashval];
    if ( count > 0 )
    {
        int i;

        // this hash already exists. check to see if the binary collision exists, too
        n = chr_co_list->sublist[hashval];
        for ( i = 0; i < count; i++ )
        {
            d = ( co_data_t * )( n->data );

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
        assert(( *cdata_count ) < CHR_MAX_COLLISIONS );
        d = cdata + ( *cdata_count );
        ( *cdata_count )++;

        // fill it in
        d->chra = ichr_a;
        d->chrb = ichr_b;
        d->prtb = TOTAL_MAX_PRT;

        // generate a new hash node
        assert(( *hn_count ) < COLLISION_HASH_NODES );
        n = hnlst + ( *hn_count );
        ( *hn_count )++;
        hash_node_ctor( n, ( void* )d );

        // insert the node
        chr_co_list->subcount[hashval]++;
        chr_co_list->sublist[hashval] = hash_node_insert_before( chr_co_list->sublist[hashval], n );
    }

    return !found;
}

//--------------------------------------------------------------------------------------------
bool_t add_chr_prt_interaction( Uint16 ichr_a, Uint16 iprt_b, co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count )
{
    bool_t found;
    int    count;
    Uint32 hashval = 0;

    hash_node_t * n;
    co_data_t   * d;

    // create a hash that is order-independent
    hashval = MAKE_HASH( ichr_a, iprt_b );

    found = bfalse;
    count = chr_co_list->subcount[hashval];
    if ( count > 0 )
    {
        int i ;

        // this hash already exists. check to see if the binary collision exists, too
        n = chr_co_list->sublist[hashval];
        for ( i = 0; i < count; i++ )
        {
            d = ( co_data_t * )( n->data );
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
        assert(( *cdata_count ) < CHR_MAX_COLLISIONS );
        d = cdata + ( *cdata_count );
        ( *cdata_count )++;

        // fill it in
        d->chra = ichr_a;
        d->chrb = MAX_CHR;
        d->prtb = iprt_b;

        // generate a new hash node
        assert(( *hn_count ) < COLLISION_HASH_NODES );
        n = hnlst + ( *hn_count );
        ( *hn_count )++;
        hash_node_ctor( n, ( void* )d );

        // insert the node
        chr_co_list->subcount[hashval]++;
        chr_co_list->sublist[hashval] = hash_node_insert_before( chr_co_list->sublist[hashval], n );
    }

    return !found;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void fill_interaction_list( co_data_t cdata[], int * cdata_count, hash_node_t hnlst[], int * hn_count )
{
    Uint16 ichr_a, ichr_b, iprt_b;

    Uint32 fanblock;
    int tnc, chrinblock, prtinblock;
    int cnt;

    if ( NULL == chr_co_list )
    {
        chr_co_list = hash_list_create( -1 );
        assert( NULL != chr_co_list );
    }

    // renew the collision list. Since we are filling this list with pre-allocated hash_node_t's,
    // there is no need to delete any of the existing chr_co_list->sublist elements
    for ( cnt = 0; cnt < 256; cnt++ )
    {
        chr_co_list->subcount[cnt] = 0;
        chr_co_list->sublist[cnt]  = NULL;
    }
    ( *cdata_count ) = 0;
    ( *hn_count )    = 0;

    for ( ichr_a = 0; ichr_a < MAX_CHR; ichr_a++ )
    {
        int ixmax, ixmin;
        int iymax, iymin;

        int ix_block, ixmax_block, ixmin_block;
        int iy_block, iymax_block, iymin_block;

        chr_t * pchr_a;

        // make sure that it is on
        if ( !ACTIVE_CHR( ichr_a ) ) continue;
        pchr_a = ChrList.lst + ichr_a;

        // make sure we have a good collision size for this object
        //chr_update_collision_size( pchr_a, btrue );

        // reject characters that are in packs, or are marked as non-colliding
        if ( pchr_a->pack_ispacked ) continue;

        // reject characters that are hidden
        if ( pchr_a->is_hidden ) continue;

        // determine the size of this object in blocks
        ixmin = pchr_a->pos.x - pchr_a->bump.size; ixmin = CLIP( ixmin, 0, PMesh->info.edge_x );
        ixmax = pchr_a->pos.x + pchr_a->bump.size; ixmax = CLIP( ixmax, 0, PMesh->info.edge_x );

        iymin = pchr_a->pos.y - pchr_a->bump.size; iymin = CLIP( iymin, 0, PMesh->info.edge_y );
        iymax = pchr_a->pos.y + pchr_a->bump.size; iymax = CLIP( iymax, 0, PMesh->info.edge_y );

        ixmin_block = ixmin >> BLOCK_BITS; ixmin_block = CLIP( ixmin_block, 0, MAXMESHBLOCKY );
        ixmax_block = ixmax >> BLOCK_BITS; ixmax_block = CLIP( ixmax_block, 0, MAXMESHBLOCKY );

        iymin_block = iymin >> BLOCK_BITS; iymin_block = CLIP( iymin_block, 0, MAXMESHBLOCKY );
        iymax_block = iymax >> BLOCK_BITS; iymax_block = CLIP( iymax_block, 0, MAXMESHBLOCKY );

        // handle all the interactions on this block
        for ( ix_block = ixmin_block; ix_block <= ixmax_block; ix_block++ )
        {
            for ( iy_block = iymin_block; iy_block <= iymax_block; iy_block++ )
            {
                // Allow raw access here because we were careful :)
                fanblock = mesh_get_block_int( PMesh, ix_block, iy_block );
                if ( INVALID_BLOCK == fanblock ) continue;

                chrinblock = bumplist[fanblock].chrnum;
                prtinblock = bumplist[fanblock].prtnum;

                // detect all the character-character interactions
                for ( tnc = 0, ichr_b = bumplist[fanblock].chr;
                      tnc < chrinblock && MAX_CHR != ichr_b;
                      tnc++, ichr_b = ChrList.lst[ichr_b].bumplist_next )
                {
                    chr_t * pchr_b;
                    if ( !ACTIVE_CHR( ichr_b ) ) continue;

                    pchr_b = ChrList.lst + ichr_b;

                    // make sure we have a good collision size for this object
                    //chr_update_collision_size( pchr_b, btrue );

                    if ( detect_chr_chr_interaction( ichr_a, ichr_b ) )
                    {
                        add_chr_chr_interaction( ichr_a, ichr_b, cdata, cdata_count, hnlst, hn_count );
                    }
                }

                // detect all the character-particle interactions
                // for living characters
                if ( pchr_a->alive )
                {
                    for ( tnc = 0, iprt_b = bumplist[fanblock].prt;
                          tnc < prtinblock && TOTAL_MAX_PRT != iprt_b;
                          tnc++, iprt_b = PrtList.lst[iprt_b].bumplist_next )
                    {
                        if ( detect_chr_prt_interaction( ichr_a, iprt_b ) )
                        {
                            add_chr_prt_interaction( ichr_a, iprt_b, cdata, cdata_count, hnlst, hn_count );
                        }
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void fill_bumplists()
{
    Uint16 character, particle;
    Uint32 fanblock;

    // Clear the lists
    for ( fanblock = 0; fanblock < PMesh->info.blocks_count; fanblock++ )
    {
        bumplist[fanblock].chr    = MAX_CHR;
        bumplist[fanblock].chrnum = 0;

        bumplist[fanblock].prt    = TOTAL_MAX_PRT;
        bumplist[fanblock].prtnum = 0;
    }

    // Fill 'em back up
    for ( character = 0; character < MAX_CHR; character++ )
    {
        chr_t * pchr;

        if ( !ACTIVE_CHR( character ) ) continue;
        pchr = ChrList.lst + character;

        // reset the platform stuff each update
        pchr->holdingweight   = 0;
        pchr->onwhichplatform = MAX_CHR;
        pchr->enviro.level    = pchr->enviro.floor_level;

        // reset the fan and block position
        pchr->onwhichfan   = mesh_get_tile( PMesh, pchr->pos.x, pchr->pos.y );
        pchr->onwhichblock = mesh_get_block( PMesh, pchr->pos.x, pchr->pos.y );

        // reject characters that are in packs, or are marked as non-colliding
        if ( pchr->pack_ispacked ) continue;

        // reject characters that are hidden
        if ( pchr->is_hidden ) continue;

        if ( INVALID_BLOCK != pchr->onwhichblock )
        {
            // Insert before any other characters on the block
            pchr->bumplist_next = bumplist[pchr->onwhichblock].chr;
            bumplist[pchr->onwhichblock].chr = character;
            bumplist[pchr->onwhichblock].chrnum++;
        }
    }

    for ( particle = 0; particle < maxparticles; particle++ )
    {
        prt_t * pprt;

        // reject invalid particles
        if ( !ACTIVE_PRT( particle ) ) continue;
        pprt = PrtList.lst + particle;

        pprt->onwhichplatform = MAX_CHR;
        particle_set_level( pprt, pprt->enviro.floor_level );

        // reject characters that are hidden
        if ( pprt->is_hidden ) continue;

        // reset the fan and block position
        pprt->onwhichfan   = mesh_get_tile( PMesh, pprt->pos.x, pprt->pos.y );
        pprt->onwhichblock = mesh_get_block( PMesh, pprt->pos.x, pprt->pos.y );

        if ( INVALID_BLOCK != pprt->onwhichblock )
        {
            // Insert before any other particles on the block
            pprt->bumplist_next = bumplist[pprt->onwhichblock].prt;
            bumplist[pprt->onwhichblock].prt = particle;
            bumplist[pprt->onwhichblock].prtnum++;
        }
    }
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_platform_detection( Uint16 ichr_a, Uint16 ichr_b )
{
    chr_t * pchr_a, * pchr_b;
    cap_t * pcap_a, * pcap_b;

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
    if ( !ACTIVE_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    // make sure that B is valid
    if ( !ACTIVE_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    // if you are mounted, only your mount is affected by platforms
    if ( ACTIVE_CHR( pchr_a->attachedto ) || ACTIVE_CHR( pchr_b->attachedto ) ) return bfalse;

    pcap_a = chr_get_pcap( ichr_a );
    if ( NULL == pcap_a ) return bfalse;

    pcap_b = chr_get_pcap( ichr_b );
    if ( NULL == pcap_b ) return bfalse;

    // only check possible object-platform interactions
    platform_a = pcap_b->canuseplatforms && pchr_a->platform;
    platform_b = pcap_a->canuseplatforms && pchr_b->platform;
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
                pchr_a->onwhichplatform = ichr_b;
            }
        }
        else
        {
            if ( pchr_a->pos.z + pchr_a->chr_chr_cv.maxs[OCT_Z] > pchr_b->enviro.level )
            {
                // set, but do not attach the platforms yet
                pchr_b->enviro.level    = pchr_a->pos.z + pchr_a->chr_chr_cv.maxs[OCT_Z];
                pchr_b->onwhichplatform = ichr_a;
            }
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t do_prt_platform_detection( Uint16 ichr_a, Uint16 iprt_b )
{
    chr_t * pchr_a;
    prt_t * pprt_b;

    oct_vec_t odepth;

    // make sure that B is valid
    if ( !ACTIVE_PRT( iprt_b ) ) return bfalse;
    pprt_b = PrtList.lst + iprt_b;

    // if the particle is attached to something, it can't be affected by a platform
    if ( ACTIVE_CHR( pprt_b->attachedto_ref ) ) return bfalse;

    // make sure that A is valid
    if ( !ACTIVE_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    // only check possible particle-platform interactions
    if ( !pchr_a->platform ) return bfalse;

    // is this calculation going to matter, even if it succeeds?
    if ( pchr_a->pos.z + pchr_a->chr_chr_cv.maxs[OCT_Z] > pprt_b->enviro.level ) return bfalse;

    //---- determine the interaction depth for each dimension
    odepth[OCT_Z] = ( pchr_a->pos.z + pchr_a->chr_chr_cv.maxs[OCT_Z] ) - ( pprt_b->pos.z - pprt_b->bump.height );
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

    pchr_cap = pro_get_pcap( pchr->iprofile );
    if ( NULL == pchr_cap ) return bfalse;

    // check if they can be connected
    if ( !pchr_cap->canuseplatforms || ( 0 != pchr->flyheight ) ) return bfalse;
    if ( !pplat->platform ) return bfalse;

    // do the attachment
    pchr->onwhichplatform = GET_INDEX_PCHR( pplat );

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
    ai_state_set_bumplast( &( pplat->ai ), GET_INDEX_PCHR( pchr ) );

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

    pprt_pip = prt_get_ppip( GET_INDEX_PPRT( pprt ) );
    if ( NULL == pprt_pip ) return bfalse;

    // check if they can be connected
    if ( !pplat->platform ) return bfalse;

    // do the attachment
    pprt->onwhichplatform = GET_INDEX_PCHR( pplat );

    // update the character's relationship to the ground
    particle_set_level( pprt, MAX( pprt->enviro.level, pplat->pos.z + pplat->chr_chr_cv.maxs[OCT_Z] ) );

    return btrue;
}


//--------------------------------------------------------------------------------------------
void bump_all_objects( void )
{
    /// @details ZZ@> This function sets handles characters hitting other characters or particles

    // fill up the bumplists
    fill_bumplists();

    // fill the collision list with all possible binary interactions
    fill_interaction_list( cdata, &cdata_count, hnlst, &hn_count );

    // handle interaction with platforms
    bump_all_platforms();

    // handle interaction with mounts
    bump_all_mounts();

    // handle all the collisions
    bump_all_collisions();

    // The following functions need to be called any time you actually change a charcter's position
    keep_weapons_with_holders();
    attach_particles();
    make_all_character_matrices( update_wld != 0 );
}

//--------------------------------------------------------------------------------------------
void bump_all_platforms( void )
{
    /// @details BB@> Detect all character and particle interactions platforms. Then attach them.
    ///             @note it is important to only attach the character to a platform once, so its
    ///              weight does not get applied to multiple platforms

    int         cnt, tnc;
    co_data_t * d;

    // Find the best platforms
    for ( cnt = 0; cnt < chr_co_list->allocated; cnt++ )
    {
        hash_node_t * n;
        int count = chr_co_list->subcount[cnt];

        n = chr_co_list->sublist[cnt];
        for ( tnc = 0; tnc < count && NULL != n; tnc++, n = n->next )
        {
            // only look at character-character interactions
            d = ( co_data_t * )( n->data );
            if ( TOTAL_MAX_PRT != d->prtb ) continue;

            if ( TOTAL_MAX_PRT == d->prtb )
            {
                do_chr_platform_detection( d->chra, d->chrb );
            }
            else if ( MAX_CHR == d->chrb )
            {
                do_prt_platform_detection( d->chra, d->prtb );
            }
        }
    }

    // Do the actual platform attachments.
    // Doing the attachments after detecting the best platform
    // prevents an object from attaching it to multiple platforms as it
    // is still trying to find the best one
    for ( cnt = 0; cnt < chr_co_list->allocated; cnt++ )
    {
        hash_node_t * n;
        int count = chr_co_list->subcount[cnt];

        n = chr_co_list->sublist[cnt];
        for ( tnc = 0; tnc < count && NULL != n; tnc++, n = n->next )
        {
            // only look at character-character interactions
            d = ( co_data_t * )( n->data );
            if ( TOTAL_MAX_PRT != d->prtb ) continue;

            if ( TOTAL_MAX_PRT == d->prtb )
            {
                if ( ACTIVE_CHR( d->chra ) && ACTIVE_CHR( d->chrb ) )
                {
                    if ( ChrList.lst[d->chra].onwhichplatform == d->chrb )
                    {
                        attach_chr_to_platform( ChrList.lst + d->chra, ChrList.lst + d->chrb );
                    }
                    else if ( ChrList.lst[d->chrb].onwhichplatform == d->chra )
                    {
                        attach_chr_to_platform( ChrList.lst + d->chrb, ChrList.lst + d->chra );
                    }
                }
            }
            else if ( MAX_CHR == d->chrb )
            {
                if ( ACTIVE_CHR( d->chra ) && ACTIVE_PRT( d->prtb ) )
                {
                    if ( PrtList.lst[d->prtb].onwhichplatform == d->chra )
                    {
                        attach_prt_to_platform( PrtList.lst + d->prtb, ChrList.lst + d->chra );
                    }
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void bump_all_mounts()
{
    /// @details BB@> Detect all character interactions mounts. Then attach them.

    int         cnt, tnc;
    co_data_t * d;

    // Do mounts
    for ( cnt = 0; cnt < chr_co_list->allocated; cnt++ )
    {
        hash_node_t * n;
        int count = chr_co_list->subcount[cnt];

        n = chr_co_list->sublist[cnt];
        for ( tnc = 0; tnc < count && NULL != n; tnc++, n = n->next )
        {
            // only look at character-character interactions
            d = ( co_data_t * )( n->data );
            if ( TOTAL_MAX_PRT != d->prtb ) continue;

            do_mounts( d->chra, d->chrb );
        }
    }
}

//-------------------------------------------------------------------------------------------
void bump_all_collisions()
{
    /// @details BB@> Detect all character-character and character-particle collsions (with exclusions
    ///               for the mounts and platforms found in the previous steps)

    int cnt, tnc;
    co_data_t   * d;

    // blank the accumulators
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        ChrList.lst[cnt].phys.apos_0.x = 0.0f;
        ChrList.lst[cnt].phys.apos_0.y = 0.0f;
        ChrList.lst[cnt].phys.apos_0.z = 0.0f;

        ChrList.lst[cnt].phys.apos_1.x = 0.0f;
        ChrList.lst[cnt].phys.apos_1.y = 0.0f;
        ChrList.lst[cnt].phys.apos_1.z = 0.0f;

        ChrList.lst[cnt].phys.avel.x = 0.0f;
        ChrList.lst[cnt].phys.avel.y = 0.0f;
        ChrList.lst[cnt].phys.avel.z = 0.0f;
    }

    // do all interactions
    for ( cnt = 0; cnt < chr_co_list->allocated; cnt++ )
    {
        hash_node_t * n;
        int count = chr_co_list->subcount[cnt];

        n = chr_co_list->sublist[cnt];
        for ( tnc = 0; tnc < count && NULL != n; tnc++, n = n->next )
        {
            // only look at character-character interactions
            d = ( co_data_t * )( n->data );

            if ( TOTAL_MAX_PRT == d->prtb )
            {
                do_chr_chr_collision( d->chra, d->chrb );
            }
            else if ( MAX_CHR == d->chrb )
            {
                do_chr_prt_collision( d->chra, d->prtb );
            }
        }
    }

    // accumulate the accumulators
    for ( cnt = 0; cnt < MAX_CHR; cnt++ )
    {
        float tmpx, tmpy, tmpz;
        chr_t * pchr;
        float bump_str;

        if ( !ACTIVE_CHR( cnt ) ) continue;
        pchr = ChrList.lst + cnt;

        bump_str = 1.0f;
        if ( ACTIVE_CHR( pchr->attachedto ) )
        {
            bump_str = 0;
        }
        else if ( pchr->dismount_timer > 0 )
        {
            bump_str = 1.0f - ( float )pchr->dismount_timer / PHYS_DISMOUNT_TIME;
            bump_str = bump_str * bump_str * 0.5f;
        };

        // decrement the dismount timer
        if ( pchr->dismount_timer > 0 ) pchr->dismount_timer--;

        // do the "integration" of the accumulated accelerations
        pchr->vel.x += pchr->phys.avel.x;
        pchr->vel.y += pchr->phys.avel.y;
        pchr->vel.z += pchr->phys.avel.z;

        // do the "integration" on the position
        if ( ABS( pchr->phys.apos_0.x + pchr->phys.apos_1.x ) > 0 )
        {
            tmpx = pchr->pos.x;
            pchr->pos.x += pchr->phys.apos_0.x + pchr->phys.apos_1.x;
            if ( __chrhitawall( pchr, NULL ) )
            {
                // restore the old values
                pchr->pos.x = tmpx;
            }
            else
            {
                pchr->vel.x += pchr->phys.apos_1.x * bump_str;
                pchr->pos_safe.x = tmpx;
            }
        }

        if ( ABS( pchr->phys.apos_0.y + pchr->phys.apos_1.y ) > 0 )
        {
            tmpy = pchr->pos.y;
            pchr->pos.y += pchr->phys.apos_0.y + pchr->phys.apos_1.y;
            if ( __chrhitawall( pchr, NULL ) )
            {
                // restore the old values
                pchr->pos.y = tmpy;
            }
            else
            {
                pchr->vel.y += pchr->phys.apos_1.y * bump_str;
                pchr->pos_safe.y = tmpy;
            }
        }

        if ( ABS( pchr->phys.apos_0.z + pchr->phys.apos_1.z ) > 0 )
        {
            tmpz = pchr->pos.z;
            pchr->pos.z += pchr->phys.apos_0.z + pchr->phys.apos_1.z;
            if ( pchr->pos.z < pchr->enviro.floor_level )
            {
                // restore the old values
                pchr->pos.z = tmpz;
            }
            else
            {
                pchr->vel.z += pchr->phys.apos_1.z * bump_str;
                pchr->pos_safe.z = tmpz;
            }
        }

        pchr->safe_valid = ( 0 == __chrhitawall( pchr, NULL ) );
    }
}

//--------------------------------------------------------------------------------------------
bool_t do_mounts( Uint16 ichr_a, Uint16 ichr_b )
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
    if ( !ACTIVE_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    pcap_a = chr_get_pcap( ichr_a );
    if ( NULL == pcap_a ) return bfalse;

    // make sure that B is valid
    if ( !ACTIVE_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    pcap_b = chr_get_pcap( ichr_b );
    if ( NULL == pcap_b ) return bfalse;

    xa = pchr_a->pos.x;
    ya = pchr_a->pos.y;
    za = pchr_a->pos.z;

    xb = pchr_b->pos.x;
    yb = pchr_b->pos.y;
    zb = pchr_b->pos.z;

    // can either of these objects mount the other?
    mount_a = chr_can_mount( ichr_b, ichr_a );
    mount_b = chr_can_mount( ichr_a, ichr_b );

    if ( !mount_a && !mount_b ) return bfalse;

    mounted = bfalse;
    if ( !mounted && mount_b && ( pchr_a->vel.z - pchr_b->vel.z ) < 0 )
    {
        // A falling on B?
        fvec4_t   point[1], nupoint[1];

        // determine the actual location of the mount point
        {
            int vertex;
            chr_instance_t * pinst = &( pchr_b->inst );

            vertex = ego_md2_data[MadList[pinst->imad].md2_ref].vertices - GRIP_LEFT;

            // do the automatic update
            chr_instance_update_vertices( pinst, vertex, vertex, bfalse );

            // Calculate grip point locations with linear interpolation and other silly things
            point[0].x = pinst->vlst[vertex].pos[XX];
            point[0].y = pinst->vlst[vertex].pos[YY];
            point[0].z = pinst->vlst[vertex].pos[ZZ];
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
            collide_xy = ( dist <= pchr_a->bump.sizebig * 2 );

            if ( collide_x && collide_y && collide_xy )
            {
                attach_character_to_mount( ichr_a, ichr_b, GRIP_ONLY );
                mounted = ACTIVE_CHR( pchr_a->attachedto );
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

            vertex = ego_md2_data[MadList[pinst->imad].md2_ref].vertices - GRIP_LEFT;

            // do the automatic update
            chr_instance_update_vertices( pinst, vertex, vertex, bfalse );

            // Calculate grip point locations with linear interpolation and other silly things
            point[0].x = pinst->vlst[vertex].pos[XX];
            point[0].y = pinst->vlst[vertex].pos[YY];
            point[0].z = pinst->vlst[vertex].pos[ZZ];
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
            collide_xy = ( dist <= pchr_b->bump.sizebig * 2 );

            if ( collide_x && collide_y && collide_xy )
            {
                attach_character_to_mount( ichr_b, ichr_a, GRIP_ONLY );
                mounted = ACTIVE_CHR( pchr_a->attachedto );
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

    if ( pitem->onwhichplatform != GET_INDEX_PCHR( pplat ) ) return bfalse;

    // grab the pre-computed zlerp value, and map it to our needs
    lerp_z = 1.0f - pitem->enviro.zlerp;

    // if your velocity is going up much faster then the
    // platform, there is no need to suck you to the level of the platform
    // this was one of the things preventing you from jumping from platforms
    // properly
    vlerp_z = ABS( pitem->vel.z - pplat->vel.z ) / 5;
    vlerp_z  = 1.0f - CLIP( vlerp_z, 0.0f, 1.0f );

    // determine the rotation rates
    rot_b = pitem->turn_z - pitem->turn_old_z;
    rot_a = pplat->turn_z - pplat->turn_old_z;

    if ( lerp_z == 1.0f )
    {
        pitem->phys.apos_0.z += ( pitem->enviro.level - pitem->pos.z ) * 0.125f;
        pitem->phys.avel.z += ( pplat->vel.z  - pitem->vel.z ) * 0.25f;
        pitem->turn_z      += ( rot_a         - rot_b ) * platstick;
    }
    else
    {
        pitem->phys.apos_0.z += ( pitem->enviro.level - pitem->pos.z ) * 0.125f * lerp_z * vlerp_z;
        pitem->phys.avel.z += ( pplat->vel.z  - pitem->vel.z ) * 0.25f * lerp_z * vlerp_z;
        pitem->turn_z      += ( rot_a         - rot_b ) * platstick * lerp_z * vlerp_z;
    };

    return btrue;
}

//--------------------------------------------------------------------------------------------
float estimate_chr_prt_normal( chr_t * pchr, prt_t * pprt, fvec3_base_t nrm, fvec3_base_t vdiff )
{
    fvec3_t collision_size;
    float dot;

    collision_size.x = MAX(pchr->chr_prt_cv.maxs[OCT_X] - pchr->chr_prt_cv.mins[OCT_X], 2.0f*pprt->bump.size  );
    collision_size.y = MAX(pchr->chr_prt_cv.maxs[OCT_Y] - pchr->chr_prt_cv.mins[OCT_Y], 2.0f*pprt->bump.size  );
    collision_size.z = MAX(pchr->chr_prt_cv.maxs[OCT_Z] - pchr->chr_prt_cv.mins[OCT_Z], 2.0f*pprt->bump.height);

    // estimate the "normal" for the collision, using the center-of-mass difference
    nrm[kX] = pprt->pos.x - pchr->pos.x;
    nrm[kY] = pprt->pos.y - pchr->pos.y;
    nrm[kZ] = pprt->pos.z - ( pchr->pos.z + 0.5f * (pchr->chr_prt_cv.maxs[OCT_Z] + pchr->chr_prt_cv.mins[OCT_Z]) );

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
            nrm[kZ] = pprt->pos_old.z - ( pchr->pos_old.z + 0.5f * (pchr->chr_prt_cv.maxs[OCT_Z] + pchr->chr_prt_cv.mins[OCT_Z]) );

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
        memcpy( nrm, ntmp.v, sizeof(fvec3_base_t) );

        // determine the actual dot product
        dot = fvec3_dot_product( vdiff, nrm );
    }

    return dot;
};

//--------------------------------------------------------------------------------------------
bool_t do_chr_chr_collision( Uint16 ichr_a, Uint16 ichr_b )
{
    chr_t * pchr_a, * pchr_b;
    cap_t * pcap_a, * pcap_b;

    float interaction_strength = 1.0f;
    float wta, wtb;

    fvec3_t   nrm;
    int exponent = 1;

    oct_vec_t opos_a, opos_b, odepth, odepth_old;
    bool_t    collision = bfalse, bump = bfalse;

    // make sure that it is on
    if ( !ACTIVE_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    pcap_a = chr_get_pcap( ichr_a );
    if ( NULL == pcap_a ) return bfalse;

    // make sure that it is on
    if ( !ACTIVE_CHR( ichr_b ) ) return bfalse;
    pchr_b = ChrList.lst + ichr_b;

    pcap_b = chr_get_pcap( ichr_b );
    if ( NULL == pcap_b ) return bfalse;

    // platform interaction. if the onwhichplatform is set, then
    // all collision tests have been met
    if ( ichr_a == pchr_b->onwhichplatform )
    {
        if ( do_chr_platform_physics( pchr_b, pchr_a ) )
        {
            // this is handled
            return btrue;
        }
    }

    // platform interaction. if the onwhichplatform is set, then
    // all collision tests have been met
    if ( ichr_b == pchr_a->onwhichplatform )
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
    if ( pcap_b->canuseplatforms && pchr_a->platform )
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

    if ( pcap_a->canuseplatforms && pchr_b->platform )
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
    if( !get_depth_2( pchr_a->chr_chr_cv, pchr_a->pos, pchr_b->chr_chr_cv, pchr_b->pos, btrue, odepth ) )
    {
        // return if there was no collision
        return bfalse;
    }

    // measure the collision depth in the last update
    // the objects were not touching last frame, so they must have collided this frame
    collision = !get_depth_2( pchr_a->chr_chr_cv, pchr_a->pos_old, pchr_b->chr_chr_cv, pchr_b->pos_old, btrue, odepth_old );

    //------------------
    // do character-character interactions

    // calculate a "mass" for each object, taking into account possible infinite masses
    chr_get_mass_pair( pchr_a, pchr_b, &wta, &wtb );

    // create an "octagonal position" for each object
    vec_to_oct_vec( pchr_a->pos, opos_a );
    vec_to_oct_vec( pchr_b->pos, opos_b );

    // adjust the center-of-mass
    opos_a[OCT_Z] += (pchr_a->chr_chr_cv.maxs[OCT_Z] + pchr_a->chr_chr_cv.mins[OCT_Z]) * 0.5f;
    opos_b[OCT_Z] += (pchr_b->chr_chr_cv.maxs[OCT_Z] + pchr_b->chr_chr_cv.mins[OCT_Z]) * 0.5f;

    // make the object more like a table if there is a platform-like interaction
    if ( pcap_a->canuseplatforms && pchr_b->platform ) exponent += 2;
    if ( pcap_b->canuseplatforms && pchr_a->platform ) exponent += 2;

    if( phys_estimate_chr_chr_normal( opos_a, opos_b, odepth, exponent, nrm.v ) )
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

    if( NULL == pdata ) return bfalse;

    // is the platform a platform?
    if( !ACTIVE_PCHR(pplat) || !pplat->platform ) return bfalse;

    // can the particle interact with it?
    if( !ACTIVE_PPRT(pprt) || ACTIVE_CHR( pprt->attachedto_ref ) ) return bfalse;

    // this is handled elsewhere 
    if( GET_INDEX_PCHR( pplat ) == pprt->onwhichplatform ) return bfalse;

    // Test to see whether the particle is in the right position to interact with the platform.
    // You have to be closer to a platform to interact with it then for a general object,
    // but the vertical distance is looser.
    plat_collision = test_interaction_close_1( pplat->chr_prt_cv, pplat->pos, pprt->bump, pprt->pos, btrue );

    if( !plat_collision ) return bfalse;

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
        if ( pdata->ppip->endground && pplat->platform )
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
bool_t do_chr_prt_collision_deflect( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata  )
{
    bool_t prt_deflected = bfalse;

    bool_t chr_is_invictus, chr_can_deflect;
    bool_t prt_wants_deflection;

    if( NULL == pdata ) return bfalse;

    if( !ACTIVE_PCHR( pchr ) ) return bfalse;

    if( !ACTIVE_PPRT( pprt ) ) return bfalse;

    // shield block?
    chr_is_invictus = ( 0 != ( Md2FrameList[pchr->inst.frame_nxt].framefx & MADFX_INVICTUS ) );

    // shield block can be counteracted by an unblockable particle
    chr_is_invictus = chr_is_invictus && ( 0 == ( pdata->ppip->damfx & DAMFX_NBLOC ) );

    // determine whether the character is magically protected from missile attacks
    prt_wants_deflection  = ( MISSILE_NORMAL != pchr->missiletreatment ) && 
                            ( pprt->owner_ref != GET_INDEX_PCHR( pchr ) ) && !pdata->ppip->bumpmoney;

    chr_can_deflect = chr_is_invictus || (( 0 != pchr->damagetime ) && ( pdata->max_damage > 0 ) );

    // try to deflect the particle
    prt_deflected = bfalse;
    pdata->mana_paid = bfalse;
    if ( prt_wants_deflection || chr_can_deflect )
    {
        // magically deflect the particle or make a ricochet if the character is invictus

        int treatment;

        treatment = MISSILE_DEFLECT;
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
            else if ( treatment == MISSILE_DEFLECT )
            {
                // Reflect it back in the direction it came
                pdata->impulse.x -= 2.0f * pprt->vel.x;
                pdata->impulse.y -= 2.0f * pprt->vel.y;
                pdata->impulse.z -= 2.0f * pprt->vel.z;
            }

            // Change the owner of the missile
            pprt->team      = pchr->team;
            pprt->owner_ref = GET_INDEX_PCHR( pchr );
            pdata->ppip->homing    = bfalse;

            // Change the direction of the particle
            if ( pdata->ppip->rotatetoface )
            {
                // Turn to face new direction
                pprt->facing = vec_to_facing( pprt->vel.x , pprt->vel.y );
            }
        }
    }

    return prt_deflected;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_recoil( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata )
{
    /// @details BB@> make the character and particle recoil from the collision

    if( NULL == pdata ) return 0;

    if( !ACTIVE_PCHR( pchr ) ) return bfalse;

    if( !ACTIVE_PPRT( pprt ) ) return bfalse;

    if( 0.0f == ABS(pdata->impulse.x) + ABS(pdata->impulse.y) + ABS(pdata->impulse.z) ) return btrue;

    // do the reaction force of the particle on the character
    if ( pdata->ppip->allowpush )
    {
        float prt_mass;
        float attack_factor;

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
            factor *= 2.0f * ( float )pdata->actual_damage / ( float )( ABS( pdata->actual_damage ) + pchr->lifemax );

            factor = CLIP( factor, 0.0f, 3.0f );

            // calculate the "impulse"
            pchr->phys.avel.x -= pdata->impulse.x * factor;
            pchr->phys.avel.y -= pdata->impulse.y * factor;
            pchr->phys.avel.z -= pdata->impulse.z * factor;
        }

        // if the particle is attached to a weapon, the particle can force the
        // weapon (actually, the weapon's holder) to rebound.
        if ( ACTIVE_CHR( pprt->attachedto_ref ) )
        {
            chr_t * ptarget;
            Uint16 iholder;

            ptarget = NULL;

            // transmit the force of the blow back to the character that is
            // holding the weapon

            iholder = chr_get_lowest_attachment( pprt->attachedto_ref, bfalse );
            if ( ACTIVE_CHR( iholder ) )
            {
                ptarget = ChrList.lst + iholder;
            }
            else
            {
                iholder = chr_get_lowest_attachment( pprt->owner_ref, bfalse );
                if ( ACTIVE_CHR( iholder ) )
                {
                    ptarget = ChrList.lst + iholder;
                }
            }

            if ( ptarget->phys.weight != INFINITE_WEIGHT )
            {
                float factor = attack_factor;

                if ( ptarget->phys.weight > 0 )
                {
                    // limit the prt_mass to be something relatice to this object
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
    Uint16 enchant, enc_next;
    bool_t prt_needs_impact;

    if( NULL == pdata ) return bfalse;
    if( !ACTIVE_PCHR( pchr ) ) return bfalse;
    if( !ACTIVE_PPRT( pprt ) ) return bfalse;

    if( pchr->damagetime > 0 ) return bfalse;

    // clean up the enchant list before doing anything
    pchr->firstenchant = cleanup_enchant_list( pchr->firstenchant );

    // Check all enchants to see if they are removed
    enchant = pchr->firstenchant;
    while ( enchant != MAX_ENC )
    {
        enc_next = EncList.lst[enchant].nextenchant_ref;
        if ( enc_is_removed( enchant, pprt->profile_ref ) )
        {
            remove_enchant( enchant );
        }
        enchant = enc_next;
    }

    // Do confuse effects
    // the particle would have already been deflected if this frame was a
    // invictus frame
    if ( 0 != ( pdata->ppip->damfx & DAMFX_NBLOC ) /* || 0 == ( Md2FrameList[pchr->inst.frame_nxt].framefx & MADFX_INVICTUS ) */ )
    {
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
    }

    //---- Damage the character, if necessary

    prt_needs_impact = pdata->ppip->rotatetoface || ACTIVE_CHR( pprt->attachedto_ref );
    if ( ACTIVE_CHR( pprt->owner_ref ) )
    {
        chr_t * powner = ChrList.lst + pprt->owner_ref;
        cap_t * powner_cap = pro_get_pcap( powner->iprofile );

        if ( powner_cap->isranged ) prt_needs_impact = btrue;
    }

    // DAMFX_ARRO means that it only does damage to the one it's attached to
    if ( 0 == ( pdata->ppip->damfx&DAMFX_ARRO ) && !( prt_needs_impact && !(pdata->dot < 0.0f) ) )
    {
        Uint16 direction;
        IPair loc_damage = pprt->damage;

        direction = vec_to_facing( pprt->vel.x , pprt->vel.y );
        direction = pchr->turn_z - direction + ATK_BEHIND;

        // Apply intelligence/wisdom bonus damage for particles with the [IDAM] and [WDAM] expansions (Low ability gives penality)
        // +2% bonus for every point of intelligence and/or wisdom above 14. Below 14 gives -2% instead!
        if ( pdata->ppip->intdamagebonus )
        {
            float percent;
            percent = (( FP8_TO_INT( ChrList.lst[pprt->owner_ref].intelligence ) ) - 14 ) * 2;
            percent /= 100;
            loc_damage.base *= 1.00f + percent;
        }

        if ( pdata->ppip->wisdamagebonus )
        {
            float percent;
            percent = ( FP8_TO_INT( ChrList.lst[pprt->owner_ref].wisdom ) - 14 ) * 2;
            percent /= 100;
            loc_damage.base *= 1.00f + percent;
        }

        // handle vulnerabilities
        if ( chr_has_vulnie( GET_INDEX_PCHR( pchr ), pprt->profile_ref ) )
        {
            loc_damage.base = ( loc_damage.base << 1 );
            loc_damage.rand = ( loc_damage.rand << 1 ) | 1;

            pchr->ai.alert |= ALERTIF_HITVULNERABLE;
        }

        // Damage the character
        pdata->actual_damage = damage_character( GET_INDEX_PCHR( pchr ), direction, loc_damage, pprt->damagetype, pprt->team, pprt->owner_ref, pdata->ppip->damfx, bfalse );

        // we're supposed to blank out the damage here so that swords and such don't
        // kill everything in one swipe?
        pprt->damage = loc_damage;
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

    //---- Notify the attacker of a scored hit
    if ( ACTIVE_CHR( pprt->owner_ref ) )
    {
        Uint16 item;

        chr_get_pai( pprt->owner_ref )->alert |= ALERTIF_SCOREDAHIT;
        chr_get_pai( pprt->owner_ref )->hitlast = GET_INDEX_PCHR( pchr );

        //Tell the weapons who the attacker hit last
        item = ChrList.lst[pprt->owner_ref].holdingwhich[SLOT_LEFT];
        if ( ACTIVE_CHR( item ) )
        {
            ChrList.lst[item].ai.hitlast = GET_INDEX_PCHR( pchr );
        }

        item = ChrList.lst[pprt->owner_ref].holdingwhich[SLOT_RIGHT];
        if ( ACTIVE_CHR( item ) )
        {
            ChrList.lst[item].ai.hitlast = GET_INDEX_PCHR( pchr );
        }
    }

    return btrue;
}
//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_bump( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata )
{
    bool_t prt_belongs_to_chr;
    bool_t prt_hates_chr, prt_attacks_chr;
    bool_t valid_onlydamagefriendly;
    bool_t valid_friendlyfire;

    if( NULL == pdata ) return bfalse;
    if( !ACTIVE_PCHR( pchr ) ) return bfalse;
    if( !ACTIVE_PPRT( pprt ) ) return bfalse;

    // if the particle was deflected, then it can't bump the character
    if( pchr->invictus || pprt->attachedto_ref == GET_INDEX_PCHR( pchr ) ) return 0;

    prt_belongs_to_chr = ( GET_INDEX_PCHR( pchr ) == pprt->owner_ref );

    if ( !prt_belongs_to_chr )
    {
        // no simple owner relationship. Check for something deeper.
        Uint16 prt_owner = prt_get_iowner( GET_INDEX_PPRT( pprt ), 0 );
        if( ACTIVE_CHR(prt_owner) )
        {
            Uint16 chr_wielder = chr_get_lowest_attachment( GET_INDEX_PCHR( pchr ),   btrue );
            Uint16 prt_wielder = chr_get_lowest_attachment( prt_owner, btrue );

            if( !ACTIVE_CHR(chr_wielder) ) chr_wielder = GET_INDEX_PCHR( pchr );
            if( !ACTIVE_CHR(prt_wielder) ) prt_wielder = prt_owner;

            prt_belongs_to_chr = (chr_wielder == prt_wielder);
        }
    }

    // does the particle team hate the character's team
    prt_hates_chr = TeamList[pprt->team].hatesteam[pchr->team];

    // allow neutral particles to attack anything
    prt_attacks_chr = prt_hates_chr || ( (TEAM_NULL != pchr->team) && (TEAM_NULL == pprt->team) );

    // this is the onlydamagefriendly condition from the particle search code
    valid_onlydamagefriendly = ( pdata->ppip->onlydamagefriendly && pprt->team == pchr->team ) ||
                                ( !pdata->ppip->onlydamagefriendly && prt_attacks_chr );

    // I guess "friendly fire" does not mean "self fire", which is a bit unfortunate.
    valid_friendlyfire = ( pdata->ppip->friendlyfire && !prt_hates_chr && !prt_belongs_to_chr ) ||
                            ( !pdata->ppip->friendlyfire && prt_attacks_chr );

    pdata->prt_bumps_chr =  valid_friendlyfire || valid_onlydamagefriendly;

    return pdata->prt_bumps_chr;
}

//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_handle_bump(chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata)
{
    if( NULL == pdata || !pdata->prt_bumps_chr ) return bfalse;

    if( !ACTIVE_PCHR( pchr ) ) return bfalse;
    if( !ACTIVE_PPRT( pprt ) ) return bfalse;

    if( !pdata->prt_bumps_chr ) return bfalse;

    // Catch on fire
    spawn_bump_particles( GET_INDEX_PCHR( pchr ), GET_INDEX_PPRT( pprt ) );

    //handle some special particle interactions
    if( pdata->ppip->endbump )
    {
        if ( pdata->ppip->bumpmoney )
        {
            chr_t * pcollector = pchr;

            // Let mounts collect money for their riders
            if ( pchr->ismount && ACTIVE_CHR( pchr->holdingwhich[SLOT_LEFT] ) )
            {
                pcollector = ChrList.lst + pchr->holdingwhich[SLOT_LEFT];

                // if the mount's rider can't get money, the mount gets to keep the money!
                if( !pcollector->cangrabmoney )
                {
                    pcollector = pchr;
                }
            }

            if ( pcollector->cangrabmoney && pcollector->alive && 0 == pcollector->damagetime && pcollector->money < MAXMONEY )
            {
                pcollector->money += pdata->ppip->bumpmoney;
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

    return btrue;;
}



//--------------------------------------------------------------------------------------------
bool_t do_chr_prt_collision_init( chr_t * pchr, prt_t * pprt, chr_prt_collsion_data_t * pdata )
{
    bool_t full_collision;

    if( NULL == pdata ) return bfalse;

    memset( pdata, 0, sizeof(*pdata) );

    if( !ACTIVE_PCHR( pchr ) ) return bfalse;
    if( !ACTIVE_PPRT( pprt ) ) return bfalse;

    // initialize the collision date
    pdata->pcap = pro_get_pcap( pchr->iprofile );
    if ( NULL == pdata->pcap ) return bfalse;

    if ( !LOADED_PIP( pprt->pip_ref ) ) return bfalse;
    pdata->ppip = PipStack.lst + pprt->pip_ref;

    // measure the collision depth
    full_collision = get_depth_1( pchr->chr_prt_cv, pchr->pos, pprt->bump, pprt->pos, btrue, pdata->odepth );

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
bool_t do_chr_prt_collision( Uint16 ichr_a, Uint16 iprt_b )
{
    /// @details BB@> this funciton goes through all of the steps to handle character-particle
    ///               interactions. A basic interaction has been detected. This needs to be refined
    ///               and then handled. The function returns bfalse if the basic interaction was wrong
    ///               or if the interaction had no effect.
    ///
    /// @note This function is a little more complicated than the character-character case because
    //        of the friend-foe logic as well as the damage and other special effects that particles can do.

    bool_t retval = bfalse;

    chr_t * pchr_a;
    prt_t * pprt_b;

    bool_t prt_deflected;
    bool_t prt_can_hit_chr;

    bool_t plat_collision, full_collision;

    chr_prt_collsion_data_t cdata;

    // make sure that it is on
    if ( !ACTIVE_CHR( ichr_a ) ) return bfalse;
    pchr_a = ChrList.lst + ichr_a;

    if ( !pchr_a->alive ) return bfalse;

    if ( !ACTIVE_PRT( iprt_b ) ) return bfalse;
    pprt_b = PrtList.lst + iprt_b;

    if( ichr_a == pprt_b->attachedto_ref ) return bfalse;

    // detect a full collision
    full_collision = do_chr_prt_collision_init( pchr_a, pprt_b, &cdata );

    // platform interaction. we can still have a platform interaction even if there
    // is not a "full_collision" since the z-distance thes
    plat_collision = bfalse;
    if ( pchr_a->platform && !ACTIVE_CHR( pprt_b->attachedto_ref ) )
    {
        plat_collision = do_prt_platform_physics( pprt_b, pchr_a, &cdata );
    }

    // if there is no collision, no point in going farther
    if( !full_collision && !plat_collision ) return bfalse;

    // estimate the "normal" for the collision, using the center-of-mass difference
    // put this off until this point to reduce calling this "expensive" function
    cdata.dot = estimate_chr_prt_normal( pchr_a, pprt_b, cdata.nrm.v, cdata.vdiff.v );

    // handle particle deflection. 
    // if the platform collision was already handled, there is nothing left to do
    prt_deflected = bfalse;
    if( full_collision && !plat_collision )
    {
        // determine whether the particle is deflected by the character
        if( cdata.dot < 0.0f )
        {
            prt_deflected = do_chr_prt_collision_deflect(pchr_a, pprt_b, &cdata);
            if( prt_deflected )
            {
                retval = btrue;
            }
        }
    }

    // do "damage" to the character
    if( !prt_deflected && 0 == pchr_a->damagetime && ( full_collision || plat_collision ) )
    {
        // Check reaffirmation of particles
        if ( pchr_a->reaffirmdamagetype == pprt_b->damagetype )
        {
            retval = (0 != reaffirm_attached_particles( ichr_a ));
        }

        // refine the logic for a particle to hit a character  
        prt_can_hit_chr = do_chr_prt_collision_bump( pchr_a, pprt_b, &cdata );

        // does the particle damage/heal the character?
        if( prt_can_hit_chr )
        {
            // we can't even get to this point if the character is completely invulnerable (invictus)
            // or can't be damaged this round
            cdata.prt_damages_chr = do_chr_prt_collision_damage( pchr_a, pprt_b, &cdata );
            if( cdata.prt_damages_chr )
            {
                retval = btrue;
            }
        }
    }

    // make the character and particle recoil from the collision
    if( ABS(cdata.impulse.x) + ABS(cdata.impulse.y) + ABS(cdata.impulse.z) > 0.0f )
    {
        if( do_chr_prt_collision_recoil( pchr_a, pprt_b, &cdata ) )
        {
            retval = btrue;
        }
    }

    // handle a couple of special cases
    if( cdata.prt_bumps_chr )
    {
        if( do_chr_prt_collision_handle_bump(pchr_a, pprt_b, &cdata) )
        {
            retval = btrue;
        }
    }

    // terminate the particle if needed
    if ( cdata.terminate_particle )
    {
        prt_request_terminate( iprt_b );
        retval = btrue;
    }

    return retval;
}
