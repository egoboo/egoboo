#include "AStar.h"
#include "mesh.h"
#include "script.h"

#include "mesh.inl"

#include "log.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define MAX_ASTAR_NODES 1024    //Maximum number of nodes to explore
#define MAX_ASTAR_PATH  256     //Maximum length of the final path (before pruning)

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
bool_t AStar_find_path( ego_mpd_t *PMesh, Uint32 stoppedby, const int src_ix, const int src_iy, const int dst_ix, const int dst_iy )
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

    //Cannot find path to somewhere inside a wall
    if ( mesh_tile_has_bits( PMesh, dst_ix, dst_iy, stoppedby ) )
    {
#ifdef DEBUG_ASTAR
        printf("AStar failed because target position is inside a wall.\n");
#endif
        return bfalse;
    }

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
                // do not recompute the open node!
                if ( j == 0 && k == 0 ) continue;

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
                if( INVALID_TILE == mesh_get_tile_int( PMesh, tmp_x, tmp_y ) )
                {
                    deadend_count++;
                    continue;
                }

                // is this already in the list?
                for( cnt = 0; cnt < node_list_length; cnt++ )
                {
                    if( node_list[cnt].ix == tmp_x && node_list[cnt].iy == tmp_y )
                    {
                        deadend_count++;
                        continue;
                    }
                }

                ///
                /// @todo  I need to check for collisions with static objects, like trees

                // is this a valid tile?
                if ( mesh_tile_has_bits( PMesh, tmp_x, tmp_y, stoppedby ) )
                {
                    // add the invalid tile to the list as a closed tile
                    AStar_add_node( tmp_x, tmp_y, popen, 0xFFFF, btrue );
                    deadend_count++;
                    continue;
                }

                // OK. determine the weight (F + H)
                weight  = ( tmp_x-popen->ix )*( tmp_x-popen->ix ) + ( tmp_y-popen->iy )*( tmp_y-popen->iy );
                weight += ( tmp_x-dst_ix )*( tmp_x-dst_ix ) + ( tmp_y-dst_iy )*( tmp_y-dst_iy );
                weight  = sqrt( weight );
                AStar_add_node( tmp_x, tmp_y, popen, weight, bfalse );
            }
          }


            if ( deadend_count == 8 )
            {
                // this node is no longer active.
                // move it to the closed list so that we do not get any loops
                popen->closed = btrue;
            }
        }

        //Found no open nodes
        else
        {
#ifdef DEBUG_ASTAR  
            printf("AStar failed: Could not find a path!\n"); 
#endif
            break;
        }
    }

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
    
    AStar_Node_t *current_node, *last_waypoint;
    AStar_Node_t *node_path[MAX_ASTAR_PATH];

    if( start_node == NULL || final_node == NULL )
    {
#ifdef DEBUG_ASTAR  
        printf("ASTAR ERROR: Null pointer exception.\n");
#endif
        return bfalse;
    }

    //Fill the waypoint list as much as we can, the final waypoint will always be the destination waypoint
    waypoint_num = 0;
    last_waypoint = NULL;

    //find the final destination node
    current_node = final_node;

    //Build the local node path tree
    path_length = 0;
    while ( path_length < MAX_ASTAR_PATH && current_node != start_node )
    {
        // add the node to the end of the path
        node_path[path_length++] = current_node;

        // get next node
        current_node = current_node->parent;

        if( current_node == NULL )
        {
#ifdef DEBUG_ASTAR  
            printf("ASTAR ERROR: Null pointer exception.\n");
#endif
            return bfalse;
        }
    }


    //Begin at the end of the list, which contains the starting node
    for( i = path_length-1; i >= 0 && waypoint_num < MAXWAY; i-- )
    {
        //get current node
        current_node = node_path[i];

        if( current_node == NULL )
        {
#ifdef DEBUG_ASTAR  
            printf("ASTAR ERROR: Null pointer exception.\n");
#endif
            return bfalse;
        }

        //If we have a change in direction, we need to add it as a waypoint, always add the last waypoint
        if( last_waypoint == NULL || i == 0 || (last_waypoint->ix != current_node->ix && last_waypoint->iy != current_node->iy) )
        {
            int way_x;
            int way_y;

            last_waypoint = current_node;

            //Special exception for final waypoint, use raw integer (not bit shifted)
            if( i == 0 )
            {
                way_x = dst_x;
                way_y = dst_y;
            }
            else
            {
                //Translate to raw coordinates
                way_x = current_node->ix * GRID_ISIZE;
                way_y = current_node->iy * GRID_ISIZE;
            }

            // add the node to the waypoint list
#ifdef DEBUG_ASTAR  
            // using >> for division only works if you know for certainty that the value
            // you are shifting is not intended to be neative
            printf( "Waypoint %d: X: %d, Y: %d \n", waypoint_num, way_x / GRID_ISIZE, way_y / GRID_ISIZE );
#endif
            waypoint_list_push( plst, way_x, way_y );
            waypoint_num++;
        }
    }

#ifdef DEBUG_ASTAR  
    if( waypoint_num == 0 ) printf( "AStar found a path, but AStar_get_path() did not add any waypoints. Path tree length was %d\n", path_length );
#endif

    return waypoint_num > 0;
}
