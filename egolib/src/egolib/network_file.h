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

/// @file egolib/network_file.h
/// @brief file transfer protocol

#include "egolib/typedef.h"

#include <enet/enet.h>

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    struct s_egonet_instance;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// All the supported netfile messages
    enum NetworkMessage
    {
        NETFILE_TRANSFER       = 10001,  ///< Packet contains a file.
        NETFILE_TRANSFER_OK         = 10002,  ///< Acknowledgement packet for a file send
        NETFILE_CREATE_DIRECTORY    = 10003,  ///< Tell the peer to create the named directory
        NETFILE_DONE_SENDING  = 10009,  ///< Sent when there are no more files to send.
        NETFILE_NUM_TO_SEND   = 10010  ///< Let the other person know how many files you're sending
    };

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
    void netfile_initialize( void );
    int  netfile_pendingTransfers( void );
    void netfile_updateTransfers( void );

    void netfile_copyToAllPlayers( const char *source, const char *dest );
    void netfile_copyToPeer( const char *source, const char *dest, ENetPeer *peer );

    void netfile_copyDirectoryToAllPlayers( const char *dirname, const char *todirname );
    void netfile_copyDirectoryToPeer( const char *dirname, const char *todirname, ENetPeer * peer );

    egolib_rv netfile_handleEvent( ENetEvent * event );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _egolib_network_file_h
