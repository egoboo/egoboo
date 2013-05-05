#include "../egolib/network_file.h"

#include "../egolib/log.h"

#include "../egolib/network.h"
#include "../egolib/strutil.h"
#include "../egolib/vfs.h"
#include "../egolib/platform.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_NetFileTransfer;
typedef struct s_NetFileTransfer NetFileTransfer;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define COPYSIZE    0x1000
#define TOTALSIZE   0x00200000L

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Data for network file transfers
struct s_NetFileTransfer
{
    char sourceName[NET_MAX_FILE_NAME];
    char destName[NET_MAX_FILE_NAME];
    ENetPeer *target;
};

NetFileTransfer * NetFileTransfer__ctor( NetFileTransfer * );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Network file transfer queue
static NetFileTransfer netfile_transferStates[NET_MAX_FILE_TRANSFERS];
static int netfile_numTransfers = 0;  ///< Queue count
static int netfile_transferHead = 0;  ///< Queue start index
static int netfile_transferTail = 0;  ///< Queue end index
static int netfile_waitingForXferAck = 0; ///< Queue state

static Uint8  * netfile_transferBuffer = NULL;
static size_t   netfile_transferSize = 0;

// Receiving files
static NetFileTransfer netfile_receiveState;

static int  netfile_count = 0;                                // For network copy
static int  netfile_sent = 0;                            // For network copy
static int  netfile_expected = 0;                        // For network copy
static int  netfile_playerrespond = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void netfile_initialize( void )
{
    int cnt;

    for ( cnt = 0; cnt < NET_MAX_FILE_TRANSFERS; cnt++ )
    {
        NetFileTransfer__ctor( netfile_transferStates + cnt );
    };

    NetFileTransfer__ctor( &netfile_receiveState );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void netfile_copyToAllPlayers( const char *source, const char *dest )
{
    /// @author JF
    /// @details This function queues up files to send to all the hosts.
    /// @todo Deal with having to send to up to MAX_PLAYER players...

    NetFileTransfer *state;
    const ENetHost * MyHost = egonet_get_myHost();

    if ( netfile_numTransfers < NET_MAX_FILE_TRANSFERS )
    {
        // netfile_transferTail should already be pointed at an open
        // slot in the queue.
        state = &( netfile_transferStates[netfile_transferTail] );
        EGOBOO_ASSERT( CSTR_END == state->sourceName[0] );

        // Just do the first player for now
        state->target = MyHost->peers + 0;
        strncpy( state->sourceName, source, NET_MAX_FILE_NAME );
        strncpy( state->destName, dest, NET_MAX_FILE_NAME );

        // advance the tail index
        netfile_numTransfers++;
        netfile_transferTail++;
        if ( netfile_transferTail >= NET_MAX_FILE_TRANSFERS )
        {
            netfile_transferTail = 0;
        }
        if ( netfile_transferTail == netfile_transferHead )
        {
            log_warning( "netfile_copyToAllPlayers: Warning!  Queue tail caught up with the head!\n" );
        }
    }
}

//--------------------------------------------------------------------------------------------
void netfile_copyToPeer( const char *source, const char *dest, ENetPeer *peer )
{
    NetFileTransfer *state;

    /// @author JF
    /// @details New function merely queues up a new file to be sent

    // If this is the host, just copy the file locally
    if ( egonet_get_hostactive() )
    {
        // Simulate a network transfer
        if ( vfs_isDirectory( source ) )
        {
            vfs_mkdir( dest );
        }
        else
        {
            vfs_copyFile( source, dest );
        }

        return;
    }
    if ( netfile_numTransfers < NET_MAX_FILE_TRANSFERS )
    {
        // netfile_transferTail should already be pointed at an open
        // slot in the queue.
        state = &( netfile_transferStates[netfile_transferTail] );
        EGOBOO_ASSERT( CSTR_END == state->sourceName[0] );

        state->target = peer;
        strncpy( state->sourceName, source, NET_MAX_FILE_NAME );
        strncpy( state->destName, dest, NET_MAX_FILE_NAME );

        // advance the tail index
        netfile_numTransfers++;
        netfile_transferTail++;
        if ( netfile_transferTail >= NET_MAX_FILE_TRANSFERS )
        {
            netfile_transferTail = 0;
        }
        if ( netfile_transferTail == netfile_transferHead )
        {
            log_warning( "netfile_copyToPeer: Warning!  Queue tail caught up with the head!\n" );
        }
    }
}

//--------------------------------------------------------------------------------------------
void netfile_copyDirectoryToAllPlayers( const char *dirname, const char *todirname )
{
    /// @author ZZ
    /// @details This function copies all files in a directory

    vfs_search_context_t * ctxt;
    const char *searchResult;

    STRING fromname;
    STRING toname;

    log_info( "netfile_copyDirectoryToAllPlayers: %s, %s\n", dirname, todirname );

    // Search for all files
    ctxt = vfs_findFirst( dirname, NULL, VFS_SEARCH_FILE | VFS_SEARCH_BARE );
    searchResult = vfs_search_context_get_current( ctxt );

    if ( NULL != ctxt && VALID_CSTR( searchResult ) )
    {
        // Make the new directory
        netfile_copyToAllPlayers( dirname, todirname );

        // Copy each file
        while ( VALID_CSTR( searchResult ) )
        {
            // If a file begins with a dot, assume it's something
            // that we don't want to copy.  This keeps repository
            // directories, /., and /.. from being copied
            if ( '.' == searchResult[0] )
            {
                ctxt = vfs_findNext( &ctxt );
                searchResult = vfs_search_context_get_current( ctxt );

                continue;
            }

            snprintf( fromname, SDL_arraysize( fromname ), "%s/%s", dirname, searchResult );
            snprintf( toname, SDL_arraysize( toname ), "%s/%s", todirname, searchResult );
            netfile_copyToAllPlayers( fromname, toname );

            ctxt = vfs_findNext( &ctxt );
            searchResult = vfs_search_context_get_current( ctxt );
        }
    }

    vfs_findClose( &ctxt );
}

//--------------------------------------------------------------------------------------------
void netfile_copyDirectoryToPeer( const char *dirname, const char *todirname, ENetPeer * peer )
{
    /// @author ZZ
    /// @details This function copies all files in a directory

    vfs_search_context_t * ctxt;
    const char *searchResult;

    STRING fromname;
    STRING toname;

    log_info( "netfile_copyDirectoryToPeer: %s, %s\n", dirname, todirname );

    // Search for all files
    ctxt = vfs_findFirst( dirname, NULL, VFS_SEARCH_FILE | VFS_SEARCH_BARE );
    searchResult = vfs_search_context_get_current( ctxt );

    if ( NULL != ctxt && VALID_CSTR( searchResult ) )
    {
        // Make the new directory
        netfile_copyToPeer( dirname, todirname, peer );

        // Copy each file
        while ( VALID_CSTR( searchResult ) )
        {
            // If a file begins with a dot, assume it's something
            // that we don't want to copy.  This keeps repository
            // directories, /., and /.. from being copied
            // Also avoid copying directories in general.
            snprintf( fromname, SDL_arraysize( fromname ), "%s/%s", dirname, searchResult );
            if ( '.' == searchResult[0] || vfs_isDirectory( fromname ) )
            {
                ctxt = vfs_findNext( &ctxt );
                searchResult = vfs_search_context_get_current( ctxt );
                continue;
            }

            snprintf( fromname, SDL_arraysize( fromname ), "%s/%s", dirname, searchResult );
            snprintf( toname, SDL_arraysize( toname ), "%s/%s", todirname, searchResult );

            netfile_copyToPeer( fromname, toname, peer );

            ctxt = vfs_findNext( &ctxt );
            searchResult = vfs_search_context_get_current( ctxt );
        }
    }

    vfs_findClose( &ctxt );
}

//--------------------------------------------------------------------------------------------
int  netfile_pendingTransfers( void )
{
    return netfile_numTransfers;
}

//--------------------------------------------------------------------------------------------
void netfile_updateTransfers( void )
{
    NetFileTransfer *state;
    ENetPacket *packet;
    size_t nameLen, fileSize;
    Uint32 networkSize;
    vfs_FILE *file;
    char *p;

    ego_packet_t ego_pkt;

    ego_packet_ctor( &ego_pkt );

    // Are there any pending file sends?
    if ( netfile_numTransfers > 0 )
    {
        if ( !netfile_waitingForXferAck )
        {
            state = &netfile_transferStates[netfile_transferHead];

            // Check and see if this is a directory, instead of a file
            if ( vfs_isDirectory( state->sourceName ) )
            {
                // Tell the target to create a directory
                log_info( "egonet_updateFileTranfers: Creating directory %s on target\n", state->destName );
                ego_packet_begin( &ego_pkt );
                ego_packet_addUint16( &ego_pkt, NETFILE_CREATE_DIRECTORY );
                ego_packet_addString( &ego_pkt, state->destName );
                egonet_sendPacketToPeerGuaranteed( &ego_pkt, state->target );

                netfile_waitingForXferAck = 1;
            }
            else
            {
                file = vfs_openReadB( state->sourceName );
                if ( file )
                {
                    log_info( "netfile_updateTransfers: Attempting to send %s to %s\n", state->sourceName, state->destName );

                    fileSize = vfs_fileLength( file );
                    vfs_seek( file, 0 );

                    // Make room for the file's name
                    nameLen = strlen( state->destName ) + 1;
                    netfile_transferSize = nameLen;

                    // And for the file's size
                    netfile_transferSize += 6;  // Uint32 size, and Uint16 message type
                    netfile_transferSize += fileSize;

                    netfile_transferBuffer = EGOBOO_NEW_ARY( Uint8, netfile_transferSize );
                    *( Uint16* )netfile_transferBuffer = ENET_HOST_TO_NET_16( NETFILE_TRANSFER );

                    // Add the string and file length to the buffer
                    p = ( char * )( netfile_transferBuffer + 2 );
                    strcpy( p, state->destName );
                    p += nameLen;

                    networkSize = ENET_HOST_TO_NET_32(( Uint32 )fileSize );
                    *( size_t* )p = networkSize;
                    p += 4;

                    vfs_read( p, 1, fileSize, file );
                    vfs_close( file );

                    packet = enet_packet_create( netfile_transferBuffer, netfile_transferSize, ENET_PACKET_FLAG_RELIABLE );
                    enet_peer_send( state->target, NET_GUARANTEED_CHANNEL, packet );

                    EGOBOO_DELETE_ARY( netfile_transferBuffer );
                    netfile_transferSize = 0;

                    netfile_waitingForXferAck = 1;
                }
                else
                {
                    log_warning( "netfile_updateTransfers: Could not open file %s to send it!\n", state->sourceName );
                }
            }

            // update transfer queue state
            BLANK_STRUCT_PTR( state )
            netfile_transferHead++;
            if ( netfile_transferHead >= NET_MAX_FILE_TRANSFERS )
            {
                netfile_transferHead = 0;
            }

        } // end if waiting for ack
    } // end if netfile_numTransfers > 0

    // Let the recieve loop run at least once
    egonet_listen_for_packets();

    ego_packet_dtor( &ego_pkt );
}

//--------------------------------------------------------------------------------------------
egolib_rv netfile_handleEvent( ENetEvent * event )
{
    Uint16 header;
    STRING filename;      // also used for reading various strings
    int filesize, newfilesize, fileposition;
    char newfile;

    vfs_FILE *file;
    size_t fileSize;
    enet_packet_t enet_pkt;
    ego_packet_t  ego_pkt;
    bool_t handled;

    Uint8  ub;
    Uint16 us;
    Uint32 ui;

    enet_packet_ctor( &enet_pkt );
    ego_packet_ctor( &ego_pkt );

    if ( !egonet_on() || NULL == event )
    {
        return rv_fail;
    }

    // assume the best
    handled = btrue;

    // log the packet
    log_info( "netfile_handleEvent: Received " );

    // initialize the packet container
    if ( !enet_packet_startReading( &enet_pkt, event->packet ) )
    {
        return rv_fail;
    }

    // begin cracking
    enet_packet_readUint16( &enet_pkt, &header );

    switch ( header )
    {

        case NETFILE_TRANSFER:
            enet_packet_readString( &enet_pkt, filename, sizeof( filename ) );
            enet_packet_readUint32( &enet_pkt, &fileSize );

            log_info( "NETFILE_TRANSFER: %s with size %d.\n", filename, fileSize );

            // Try and save the file
            file = vfs_openWriteB( filename );
            if ( NULL != file )
            {
                vfs_write( &enet_pkt.ptr->data + enet_pkt.read_location, 1, fileSize, file );
                vfs_close( file );
            }
            else
            {
                log_warning( "egonet_dispatchEvent: Couldn't write new file!\n" );
            }

            // Acknowledge that we got this file
            ego_packet_begin( &ego_pkt );
            ego_packet_addUint16( &ego_pkt, NETFILE_TRANSFER_OK );
            egonet_sendPacketToPeer( &ego_pkt, event->peer );

            // And note that we've gotten another one
            netfile_count++;
            break;

        case NETFILE_TRANSFER_OK:
            log_info( "NETFILE_TRANSFER_OK. The last file sent was successful.\n" );
            netfile_waitingForXferAck = 0;
            netfile_numTransfers--;

            break;

        case NETFILE_CREATE_DIRECTORY:
            enet_packet_readString( &enet_pkt,  filename, sizeof( filename ) );
            log_info( "NETFILE_CREATE_DIRECTORY: %s\n", filename );

            vfs_mkdir( filename );

            // Acknowledge that we got this file
            ego_packet_begin( &ego_pkt );
            ego_packet_addUint16( &ego_pkt, NETFILE_TRANSFER_OK );
            egonet_sendPacketToPeer( &ego_pkt, event->peer );

            netfile_count++;  // The client considers directories it sends to be files, so ya.
            break;

        case NETFILE_DONE_SENDING:
            log_info( "NETFILE_DONE_SENDING\n" );
            netfile_playerrespond++;
            break;

        case NETFILE_NUM_TO_SEND:
            log_info( "NETFILE_NUM_TO_SEND\n" );
            if ( enet_packet_readUint16( &enet_pkt, &us ) )
            {
                netfile_expected = ( int ) us;
            }
            else
            {
                netfile_expected = 0;
            }
            break;

        case NETFILE_TO_HOST_FILE:
            log_info( "NETFILE_TO_HOST_FILE\n" );
            enet_packet_readString( &enet_pkt,  filename, sizeof( filename ) );
            enet_packet_readUint32( &enet_pkt, &ui );
            newfilesize = ( int )ui;

            // Change the size of the file if need be
            newfile = 0;
            file = vfs_openReadB( filename );
            if ( file )
            {
                filesize = vfs_fileLength( file );
                vfs_close( file );
                if ( filesize != newfilesize )
                {
                    // Destroy the old file
                    newfile = 1;
                }
            }
            else
            {
                newfile = 1;
            }
            if ( newfile )
            {
                // file must be created.  Write zeroes to the file to do it
                netfile_count++;
                file = vfs_openWriteB( filename );
                if ( file )
                {
                    filesize = 0;

                    while ( filesize < newfilesize )
                    {
                        vfs_putc( 0, file );
                        filesize++;
                    }

                    vfs_close( file );
                }
            }

            // Go to the position in the file and copy data
            enet_packet_readUint32( &enet_pkt, &ui );
            fileposition = ( int ) ui;

            file = vfs_openReadB( filename );
            if ( file )
            {
                if ( 0 == vfs_seek( file, fileposition ) )
                {
                    while ( enet_packet_readUint8( &enet_pkt, &ub ) )
                    {
                        vfs_putc( ub, file );
                    }
                }

                vfs_close( file );
            }
            break;

        case NETFILE_TO_HOST_DIR:
            log_info( "NETFILE_TO_HOST_DIR\n" );
            if ( egonet_get_hostactive() )
            {
                if ( enet_packet_readString( &enet_pkt, filename, sizeof( filename ) ) )
                {
                    vfs_mkdir( filename );
                }
            }
            break;

        case NETFILE_TO_HOST_SENT:
            log_info( "NETFILE_TO_HOST_SENT\n" );
            if ( egonet_get_hostactive() )
            {
                if ( enet_packet_readUint32( &enet_pkt, &ui ) )
                {
                    netfile_expected += ui;
                    netfile_playerrespond++;
                }
            }
            break;

        case NETFILE_TO_REMOTE_SENT:
            log_info( "NETFILE_TO_REMOTE_SENT\n" );
            if ( !egonet_get_hostactive() )
            {
                if ( enet_packet_readUint32( &enet_pkt, &ui ) )
                {
                    netfile_expected += ui;
                    netfile_playerrespond++;
                }
            }
            break;

        case NETFILE_TO_REMOTE_FILE:
            log_info( "NETFILE_TO_REMOTE_FILE\n" );
            if ( !egonet_get_hostactive() )
            {
                enet_packet_readString( &enet_pkt,  filename, sizeof( filename ) );
                enet_packet_readUint32( &enet_pkt, &ui );
                newfilesize = ( int )ui;

                // Change the size of the file if need be
                newfile = 0;
                file = vfs_openReadB( filename );
                if ( file )
                {
                    filesize = vfs_fileLength( file );
                    vfs_close( file );
                    if ( filesize != newfilesize )
                    {
                        // Destroy the old file
                        newfile = 1;
                    }
                }
                else
                {
                    newfile = 1;
                }
                if ( newfile )
                {
                    // file must be created.  Write zeroes to the file to do it
                    netfile_count++;
                    file = vfs_openWriteB( filename );
                    if ( file )
                    {
                        filesize = 0;

                        while ( filesize < newfilesize )
                        {
                            vfs_putc( 0, file );
                            filesize++;
                        }

                        vfs_close( file );
                    }
                }

                // Go to the position in the file and copy data
                enet_packet_readUint32( &enet_pkt, &ui );
                fileposition = ( int )ui;

                file = vfs_openReadB( filename );
                if ( file )
                {
                    if ( 0 == vfs_seek( file, fileposition ) )
                    {
                        while ( enet_packet_readUint8( &enet_pkt, &ub ) )
                        {
                            vfs_putc( ub, file );
                        }
                    }

                    vfs_close( file );
                }
            }
            break;

        case NETFILE_TO_REMOTE_DIR:
            log_info( "NETFILE_TO_REMOTE_DIR\n" );
            if ( !egonet_get_hostactive() )
            {
                enet_packet_readString( &enet_pkt,  filename, sizeof( filename ) );
                vfs_mkdir( filename );
            }
            break;

        default:
            handled = bfalse;
            break;
    }

    // deconstruct the packet(s)
    enet_packet_dtor( &enet_pkt );
    ego_packet_dtor( &ego_pkt );

    return handled ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

NetFileTransfer * NetFileTransfer__ctor( NetFileTransfer * ptr )
{
    if ( NULL == ptr ) return ptr;

    ptr->sourceName[0] = CSTR_END;
    ptr->destName[0] = CSTR_END;
    ptr->target = NULL;

    return ptr;
}

//--------------------------------------------------------------------------------------------
// OBSOLETE?
//--------------------------------------------------------------------------------------------
//
//static void netfile_copyToAllPlayersOld_vfs( const char *source, const char *dest );
//static void netfile_copyToPeerOld_vfs( BaseClientState_t * pc, const char *source, const char *dest, ENetPeer *peer );
//
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//
//void netfile_copyToPeerOld_vfs( BaseClientState_t * pc, const char *source, const char *dest, ENetPeer *peer )
//{
//    /// @author ZZ
//    /// @details This function copies a file on the remote to the host computer.
//    ///    Packets are sent in chunks of COPYSIZE bytes.  The max file size
//    ///    that can be sent is 2 Megs ( TOTALSIZE ).
//
//    vfs_FILE* fileread;
//    int packet_size, packet_start;
//    int filesize;
//    int fileisdir;
//    char cTmp;
//    ego_packet_t ego_pkt;
//
//    ego_packet_ctor( &ego_pkt );
//
//    log_info( "netfile_copyToPeer: " );
//    fileisdir = vfs_isDirectory( source );
//    if ( egonet_get_hostactive() )
//    {
//        // Simulate a network transfer
//        if ( fileisdir )
//        {
//            log_info( "Creating local directory %s\n", dest );
//            vfs_mkdir( dest );
//        }
//        else
//        {
//            log_info( "Copying local file %s --> %s\n", source, dest );
//            vfs_copyFile( source, dest );
//        }
//    }
//    else
//    {
//        if ( fileisdir )
//        {
//            log_info( "Creating directory on host: %s\n", dest );
//            ego_packet_begin( &ego_pkt );
//            ego_packet_addUint16( &ego_pkt, NETFILE_TO_HOST_DIR );
//            ego_packet_addString( &ego_pkt, dest );
////     egonet_broadcastPacketGuaranteed( &ego_pkt );
//            egonet_sendPacketToPeer( &ego_pkt, pc->gameHost );
//        }
//        else
//        {
//            log_info( "Copying local file to host file: %s --> %s\n", source, dest );
//            fileread = vfs_openReadB( source );
//            if ( fileread )
//            {
//                filesize = vfs_fileLength( fileread );
//                vfs_seek( fileread, 0 );
//                if ( filesize > 0 && filesize < TOTALSIZE )
//                {
//                    netfile_sent++;
//                    packet_size = 0;
//                    packet_start = 0;
//                    ego_packet_begin( &ego_pkt );
//                    ego_packet_addUint16( &ego_pkt, NETFILE_TO_HOST_FILE );
//                    ego_packet_addString( &ego_pkt, dest );
//                    ego_packet_addUint32( &ego_pkt, filesize );
//                    ego_packet_addUint32( &ego_pkt, packet_start );
//
//                    while ( packet_start < filesize )
//                    {
//                        vfs_scanf( fileread, "%c", &cTmp );
//                        ego_packet_addUint8( &ego_pkt, cTmp );
//                        packet_size++;
//                        packet_start++;
//                        if ( packet_size >= COPYSIZE )
//                        {
//                            // Send off the packet
//                            egonet_sendPacketToPeerGuaranteed( &ego_pkt, peer );
//                            enet_host_flush(( ENetHost* )egonet_get_myHost() );
//
//                            // Start on the next 4K
//                            packet_size = 0;
//                            ego_packet_begin( &ego_pkt );
//                            ego_packet_addUint16( &ego_pkt, NETFILE_TO_HOST_FILE );
//                            ego_packet_addString( &ego_pkt, dest );
//                            ego_packet_addUint32( &ego_pkt, filesize );
//                            ego_packet_addUint32( &ego_pkt, packet_start );
//                        }
//                    }
//
//                    // Send off the packet
//                    egonet_sendPacketToPeerGuaranteed( &ego_pkt, peer );
//                }
//
//                vfs_close( fileread );
//            }
//        }
//    }
//
//    ego_packet_dtor( &ego_pkt );
//}
//
//--------------------------------------------------------------------------------------------
//void netfile_copyToAllPlayersOld_vfs( const char *source, const char *dest )
//{
//    /// @author ZZ
//    /// @details This function copies a file on the host to every remote computer.
//    ///    Packets are sent in chunks of COPYSIZE bytes.  The max file size
//    ///    that can be sent is 2 Megs ( TOTALSIZE ).
//
//    vfs_FILE* fileread;
//    int packet_size, packet_start;
//    int filesize;
//    int fileisdir;
//    char cTmp;
//    ego_packet_t ego_pkt;
//
//    ego_packet_ctor( &ego_pkt );
//
//    log_info( "netfile_copyToAllPlayers: %s, %s\n", source, dest );
//    if ( egonet_on() && egonet_get_hostactive() )
//    {
//        fileisdir = vfs_isDirectory( source );
//        if ( fileisdir )
//        {
//            ego_packet_begin( &ego_pkt );
//            ego_packet_addUint16( &ego_pkt, NETFILE_TO_REMOTE_DIR );
//            ego_packet_addString( &ego_pkt, dest );
//            egonet_broadcastPacketGuaranteed( &ego_pkt );
//        }
//        else
//        {
//            fileread = vfs_openReadB( source );
//            if ( fileread )
//            {
//                filesize = vfs_fileLength( fileread );
//                vfs_seek( fileread, 0 );
//                if ( filesize > 0 && filesize < TOTALSIZE )
//                {
//                    packet_size = 0;
//                    packet_start = 0;
//                    netfile_sent++;
//
//                    ego_packet_begin( &ego_pkt );
//                    ego_packet_addUint16( &ego_pkt, NETFILE_TO_REMOTE_FILE );
//                    ego_packet_addString( &ego_pkt, dest );
//                    ego_packet_addUint32( &ego_pkt, filesize );
//                    ego_packet_addUint32( &ego_pkt, packet_start );
//
//                    while ( packet_start < filesize )
//                    {
//                        // This will probably work...
//                        // vfs_read((egonet_packet.buffer + egonet_packet.head), COPYSIZE, 1, fileread);
//
//                        // But I'll leave it alone for now
//                        vfs_scanf( fileread, "%c", &cTmp );
//
//                        ego_packet_addUint8( &ego_pkt, cTmp );
//                        packet_size++;
//                        packet_start++;
//                        if ( packet_size >= COPYSIZE )
//                        {
//                            // Send off the packet
//                            egonet_broadcastPacketGuaranteed( &ego_pkt );
//                            enet_host_flush(( ENetHost* )egonet_get_myHost() );
//
//                            // Start on the next 4K
//                            packet_size = 0;
//                            ego_packet_begin( &ego_pkt );
//                            ego_packet_addUint16( &ego_pkt, NETFILE_TO_REMOTE_FILE );
//                            ego_packet_addString( &ego_pkt, dest );
//                            ego_packet_addUint32( &ego_pkt, filesize );
//                            ego_packet_addUint32( &ego_pkt, packet_start );
//                        }
//                    }
//
//                    // Send off the packet
//                    egonet_broadcastPacketGuaranteed( &ego_pkt );
//                }
//
//                vfs_close( fileread );
//            }
//        }
//    }
//
//    ego_packet_dtor( &ego_pkt );
//}
