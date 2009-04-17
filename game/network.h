#pragma once

#include "egoboo_typedef.h"

//---------------------------------------------------------------------------------------------
// Networking functions

void listen_for_packets();
void unbuffer_player_latches();
void close_session();

void net_initialize();
void net_shutDown();
void net_logf( const char *format, ... );

void net_startNewPacket();

void packet_addUnsignedByte( Uint8 uc );
void packet_addSignedByte( Sint8 sc );
void packet_addUnsignedShort( Uint16 us );
void packet_addSignedShort( Sint16 ss );
void packet_addUnsignedInt( Uint32 ui );
void packet_addSignedInt( Sint32 si );
void packet_addString(  const char *string );

void net_sendPacketToHost();
void net_sendPacketToAllPlayers();
void net_sendPacketToHostGuaranteed();
void net_sendPacketToAllPlayersGuaranteed();
void net_sendPacketToOnePlayerGuaranteed( int player );
void net_send_message();

void net_updateFileTransfers();
int  net_pendingFileTransfers();

void net_copyFileToAllPlayers(  const char *source,  const char *dest );
void net_copyFileToHost(  const char *source,  const char *dest );
void net_copyDirectoryToHost(  const char *dirname,  const char *todirname );
void net_copyDirectoryToAllPlayers(  const char *dirname,  const char *todirname );
void net_sayHello();
void cl_talkToHost();
void sv_talkToRemotes();

int sv_hostGame();
int cl_joinGame( const char *hostname );

void find_open_sessions();
void sv_letPlayersJoin();
void stop_players_from_joining();
// int create_player(int host);
// void turn_on_service(int service);