#include "AStar.h"
#include "mesh.h"
#include "mesh.inl"

#include "graphic.h" //for point debugging
#include "script.h"  //for waypoint list control

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define MAX_ASTAR_NODES 512    //Maximum number of nodes to explore
#define MAX_ASTAR_PATH  128    //Maximum length of the final path (before pruning)

//------------------------------------------------------------------------------
//Local private variables
static AStar_Node_t node_list[MAX_ASTAR_NODES];
static int          node_list_length = 0;
static AStar_Node_t *final_node = NULL;
static AStar_Node_t *start_node = NULL;

//------------------------------------------------------------------------------
//"Private" functions
static AStar_Node_t* AStar_get_next_node();
static AStar_Node_t* AStar_add_node( const int x, const int y, AStar_Node_t *parent, float weight, bool_t closed );
static void AStar_reset();


//------------------------------------------------------------------------------
AStar_Node_t* AStar_get_next_node()
{
    //@details ZF@> This function finds and returns the next cheapest open node
    int i, best_node = -1;

    for( i = 0; i < node_list_length; i++ )
    {
        if( node_list[i].closed ) continue;
        if( best_node == -1 || node_list[i].weight < node_list[best_node].weight ) best_node = i;
    }

    //return the node if found, NULL otherwise
    return (best_node != -1) ? &node_list[best_node] : NULL;
}

//------------------------------------------------------------------------------
AStar_Node_t* AStar_add_node( const int x, const int y, AStar_Node_t *parent, float weight, bool_t closed )
{
    //@details ZF@> Adds one new node to the end of the node list
    if( node_list_length >= MAX_ASTAR_NODES ) return NULL;

    //add the node
    node_list[node_list_length].ix      = x;
    node_list[node_list_length].iy      = y;
    node_list[node_list_length].closed  = closed;
    node_list[node_list_length].weight  = weight;
    node_list[node_list_length].parent  = parent;
    node_list_length++;

    //return the new node
    return &node_list[node_list_length-1];
}

//------------------------------------------------------------------------------
void AStar_reset()
{
    //@details ZF@> Reset AStar memory. This doesn't actually clear anything to make it work as fast as possible
    node_list_length = 0;
    final_node = NULL;
    start_node = NULL;
}

//------------------------------------------------------------------------------
bool_t AStar_find_path( ego_mpd_t *PMesh, Uint32 stoppedby, const int src_ix, const int src_iy, int dst_ix, int dst_iy )
{
    //@details ZF@> Explores up to MAX_ASTAR_NODES number of nodes to find a path between the source coordinates and destination coordinates.
    //              The result is stored in a node list and can be accessed through AStar_get_path(). Returns bfalse if no path was found.
    int j,k, cnt;
    bool_t done;
    int deadend_count;
    AStar_Node_t * popen;
    float weight;

    // do not start if the initial point is off the mesh
    if(  mesh_get_tile_int( PMesh, src_ix, src_iy ) == INVALID_TILE )
    {
#ifdef DEBUG_ASTAR
        printf("AStar failed because source position is off the mesh.\n");
#endif
        return bfalse;
    }

    //be a bit flexible if the destination is inside a wall
    if ( mesh_tile_has_bits( PMesh, dst_ix, dst_iy, stoppedby ) )
    {
        //check all tiles edging to this one, including corners
        for( j = -1; j <= 1; j++ )
            for( k = -1; k <= 1; k++ )
            {
                //we already checked this one
                if( j == 0 && k == 0 ) continue;

                //Did we find a free tile?
                if ( !mesh_tile_has_bits( PMesh, dst_ix+j, dst_iy+k, stoppedby ) )
                {
                    dst_ix = dst_ix+j;
                    dst_iy = dst_iy+k;
                    goto flexible_destination;
                }
            }
#ifdef DEBUG_ASTAR
            printf("AStar failed because goal position is impassable (and no nearby non-impassable tile found).\n");
#endif
            return bfalse;
    }        
flexible_destination:

    // restart the algorithm
    done = bfalse;
    AStar_reset();

    // initialize the starting node
    weight = SQRT( ( src_ix-dst_ix )*( src_ix-dst_ix ) + ( src_iy-dst_iy )*( src_iy-dst_iy ) );
    start_node = AStar_add_node( src_ix, src_iy, NULL, weight, bfalse );

    // do the algorithm
    while ( !done )
    {
        int tmp_x, tmp_y;
        bool_t stop;

        // list is completely full... we failed
        if( node_list_length == MAX_ASTAR_NODES ) break;

        //Get the cheapest open node
        popen = AStar_get_next_node();
        if( popen != NULL )
        {
            // find some child nodes
            deadend_count = 0;
            for ( j = -1; j <= 1; j++ )
            {
                tmp_x = popen->ix + j;

                for ( k = -1; k <= 1; k++ )
                {
                    Uint32 itile;

                    // do not recompute the open node!
                    if ( j == 0 && k == 0 ) continue;

                    // do not check diagonals
                    if( j != 0 && k != 0 ) continue;

                    tmp_y = popen->iy + k;

                    // check for the simplest case, is this the destination node?
                    if ( tmp_x == dst_ix && tmp_y == dst_iy )
                    {
                        weight = ( tmp_x-popen->ix )*( tmp_x-popen->ix ) + ( tmp_y-popen->iy )*( tmp_y-popen->iy );
                        weight = sqrt( weight );
                        final_node = AStar_add_node( tmp_x, tmp_y, popen, weight, bfalse );
                        done = btrue;
                        continue;
                    }

                    // is the test node on the mesh?
                    itile = mesh_get_tile_int( PMesh, tmp_x, tmp_y );
                    if( INVALID_TILE == itile )
                    {
                        deadend_count++;
                        continue;
                    }

                    // is this already in the list? (must be checked before wall or fanoff)
                    stop = bfalse;
                    for( cnt = 0; cnt < node_list_length; cnt++ )
                    {
                        if( node_list[cnt].ix == tmp_x && node_list[cnt].iy == tmp_y )
                        {
                            deadend_count++;
                            stop = btrue;
                            break;
                        }
                    }
                    if( stop ) continue;

                    //Dont walk into pits
                    //@todo: might need to check tile Z level here instead
                    if ( TILE_IS_FANOFF( PMesh->tmem.tile_list[itile] ) )
                    {
                        // add the invalid tile to the list as a closed tile
                        AStar_add_node( tmp_x, tmp_y, popen, 0xFFFF, btrue );
                        deadend_count++;
                        continue;
                    }

                    // is this a wall or impassable?
                    if ( mesh_tile_has_bits( PMesh, tmp_x, tmp_y, stoppedby ) )
                    {
                        // add the invalid tile to the list as a closed tile
                        AStar_add_node( tmp_x, tmp_y, popen, 0xFFFF, btrue );
                        deadend_count++;
                        continue;
                    }

                    ///
                    /// @todo  I need to check for collisions with static objects, like trees

                    // OK. determine the weight (F + H)
                    weight  = ( tmp_x-popen->ix )*( tmp_x-popen->ix ) + ( tmp_y-popen->iy )*( tmp_y-popen->iy );
                    weight += ( tmp_x-dst_ix )*( tmp_x-dst_ix ) + ( tmp_y-dst_iy )*( tmp_y-dst_iy );
                    weight  = sqrt( weight );
                    AStar_add_node( tmp_x, tmp_y, popen, weight, bfalse );
                }
            }


            if ( deadend_count == 4 )
            {
                // this node is no longer active.
                // move it to the closed list so that we do not get any loops
                popen->closed = btrue;
            }
        }

        //Found no open nodes
        else
        {
            break;
        }
    }

#ifdef DEBUG_ASTAR
        if( !done && node_list_length == MAX_ASTAR_NODES ) printf("AStar failed because maximum number of nodes were explored (%d)\n", MAX_ASTAR_NODES);
#endif

    return done;
}

//------------------------------------------------------------------------------
bool_t AStar_get_path( const int dst_x, const int dst_y, waypoint_list_t *plst )
{
    //@details ZF@> Fills a waypoint list with sensible waypoints. It will return bfalse if it failed to add at least one waypoint.
    //              The function goes through all the AStar_nodes and finds out which one are critical. A critical node is one that
    //              creates a corner. The function automatically prunes away all non-critical nodes. The final waypoint will always be
    //              the destination coordinates.
    int i;
    size_t path_length, waypoint_num;
    //bool_t diagonal_movement = bfalse;
    
    AStar_Node_t *current_node, *last_waypoint, *safe_waypoint;
    AStar_Node_t *node_path[MAX_ASTAR_PATH];

    //Fill the waypoint list as much as we can, the final waypoint will always be the destination waypoint
    waypoint_num = 0;
    current_node = final_node;
    last_waypoint = start_node;

    //Build the local node path tree
    path_length = 0;
    while ( path_length < MAX_ASTAR_PATH && current_node != start_node )
    {
        // add the node to the end of the path
        node_path[path_length++] = current_node;

        // get next node
        current_node = current_node->parent;
    }

    //Begin at the end of the list, which contains the starting node
    safe_waypoint = NULL;
    for( i = path_length-1; i >= 0 && waypoint_num < MAXWAY; i-- )
    {
        bool_t change_direction;

        //get current node
        current_node = node_path[i];

        //the first node should be safe
        if( safe_waypoint == NULL ) safe_waypoint = current_node;

        //is there a change in direction?
        change_direction = (last_waypoint->ix != current_node->ix && last_waypoint->iy != current_node->iy);

        //are we moving diagonally? if so, then don't fill the waypoint list with unessecary waypoints
        /*if( i != 0 )
        {
            if( diagonal_movement ) diagonal_movement = (ABS(current_node->ix - safe_waypoint->ix) == 1)  && (ABS(current_node->iy - safe_waypoint->iy) == 1);
            else                    diagonal_movement = (ABS(current_node->ix - last_waypoint->ix) == 1)  && (ABS(current_node->iy - last_waypoint->iy) == 1);

            if( diagonal_movement )
            {
                safe_waypoint = current_node;
                continue;
            }
        }*/

        //If we have a change in direction, we need to add it as a waypoint, always add the last waypoint
        if( i == 0 || change_direction )
        {
            int way_x;
            int way_y;

            //Special exception for final waypoint, use raw integer
            if( i == 0 )
            {
                way_x = dst_x;
                way_y = dst_y;
            }
            else
            {
                //Translate to raw coordinates
                way_x = safe_waypoint->ix * GRID_ISIZE + (GRID_ISIZE/2);
                way_y = safe_waypoint->iy * GRID_ISIZE + (GRID_ISIZE/2);
            }

#ifdef DEBUG_ASTAR  
            // using >> for division only works if you know for certainty that the value
            // you are shifting is not intended to be neative
            printf( "Waypoint %d: X: %d, Y: %d \n", waypoint_num, way_x / GRID_ISIZE, way_y / GRID_ISIZE );
            point_list_add( way_x, way_y, 200, 800 );
            line_list_add( last_waypoint->ix*GRID_FSIZE+(GRID_ISIZE/2), last_waypoint->iy*GRID_FSIZE+(GRID_ISIZE/2), 200, way_x, way_y, 200, 800 );
#endif

            // add the node to the waypoint list
            last_waypoint = safe_waypoint;
            waypoint_list_push( plst, way_x, way_y );
            waypoint_num++;

            //This one is now safe
            safe_waypoint = current_node;
        }

        //keep track of the last safe node from our previous waypoint
        else
        {
            safe_waypoint = current_node;
        }
    }

#ifdef DEBUG_ASTAR  
    if( waypoint_num > 0 ) point_list_add( start_node->ix*GRID_FSIZE+(GRID_ISIZE/2), start_node->iy*GRID_FSIZE+(GRID_ISIZE/2), 200, 80 );
#endif

    return waypoint_num > 0;
}
