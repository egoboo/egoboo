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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - proto.h
 * Function prototypes for a huge portion of the game code.
 */

#include "egoboo_typedef.h"

struct s_script_state;

void empty_import_directory();

int load_ai_script(  const char *loadname );

void release_all_ai_scripts();
int what_action( char cTmp );
float get_level( float x, float y, bool_t waterwalk );
bool_t load_one_icon(  const char *szLoadName );


// Exporting stuff
void export_one_character( Uint16 character, Uint16 owner, int number, bool_t is_local );
void export_all_local_players( void );
void export_all_players( bool_t require_local );

//Quitting stuff
void quit_module( void );
void quit_game( void );
void release_module();
void close_session();

//Read/write values from/to files
IDSZ get_idsz( FILE* fileread );
void goto_colon( FILE* fileread );
Uint8 goto_colon_yesno( FILE* fileread );
char get_first_letter( FILE* fileread );
bool_t read_tag( FILE *fileread );
void get_name( FILE* fileread,  char *szName );
void ftruthf( FILE* filewrite,  const char* text, Uint8 truth );
void fdamagf( FILE* filewrite,  const char* text, Uint8 damagetype );
void factiof( FILE* filewrite,  const char* text, Uint8 action );
void fgendef( FILE* filewrite,  const char* text, Uint8 gender );
void fpairof( FILE* filewrite,  const char* text, int base, int rand );
void funderf( FILE* filewrite,  const char* text,  const char* usename );

//Getting input control
void reset_tags();
void read_all_tags(  const char *szFilename );
int tag_value(  const char *string );
char* tag_to_string( Sint32 device, Sint32 tag, bool_t onlykeys );
bool_t control_is_pressed( Uint32 idevice, Uint8 icontrol );

//Messages
void display_message( struct s_script_state * pstate, int message, Uint16 character );

//String handling
char * get_file_path( const char *character);

void load_action_names(  const char* loadname );
void log_madused(  const char *savename );

//This one is unused
int vertexconnected( Uint16 modelindex, int vertex );

void keep_weapons_with_holders();
void make_one_character_matrix( Uint16 cnt );
void free_one_character( Uint16 character );
void free_inventory( Uint16 character );
void attach_particle_to_character( Uint16 particle, Uint16 character, int grip );
void make_one_weapon_matrix( Uint16 cnt );
void make_character_matrices();
int get_free_character();

void debug_message(  const char *text );
void reset_end_text();
void append_end_text( struct s_script_state * pstate, int message, Uint16 character );

//Particles
void make_prtlist( void );
void free_one_particle_no_sound( Uint16 particle );
void play_particle_sound( Uint16 particle, Sint8 sound );
void free_one_particle( Uint16 particle );
int get_free_particle( int force );
Uint16 spawn_one_particle( float x, float y, float z,
                           Uint16 facing, Uint16 model, Uint16 pip,
                           Uint16 characterattach, Uint16 grip, Uint8 team,
                           Uint16 characterorigin, Uint16 multispawn, Uint16 oldtarget );
Uint8 __prthitawall( Uint16 particle );
void disaffirm_attached_particles( Uint16 character );
Uint16 number_of_attached_particles( Uint16 character );
void reaffirm_attached_particles( Uint16 character );
void setup_particles();
void spawn_bump_particles( Uint16 character, Uint16 particle );
void move_particles( void );

void do_enchant_spawn();
void attach_particles();
void free_all_particles();
void free_all_characters();
void show_stat( Uint16 statindex );
void check_stats();

void add_stat( Uint16 character );
void move_to_top( Uint16 character );
void sort_stat();

Uint16 terp_dir( Uint16 majordir, Uint16 minordir );
Uint16 terp_dir_fast( Uint16 majordir, Uint16 minordir );
Uint8 __chrhitawall( Uint16 character );
void play_action( Uint16 character, Uint16 action, Uint8 actionready );
void set_frame( Uint16 character, int frame, Uint16 lip );

int generate_number( int numbase, int numrand );

void naming_names( Uint16 profile );
void read_naming( Uint16 profile,  const char *szLoadname );
void prime_names( void );
void tilt_characters_to_terrain();
int spawn_one_character( float x, float y, float z, Uint16 profile, Uint8 team,
                         Uint8 skin, Uint16 facing,  const char *name, int override );
void respawn_character( Uint16 character );
Uint16 change_armor( Uint16 character, Uint16 skin );
void change_character( Uint16 cnt, Uint16 profile, Uint8 skin,
                       Uint8 leavewhich );
Uint8 cost_mana( Uint16 character, int amount, Uint16 killer );
void switch_team( Uint16 character, Uint8 team );
void issue_clean( Uint16 character );
int restock_ammo( Uint16 character, IDSZ idsz );
void issue_order( Uint16 character, Uint32 order );
void issue_special_order( Uint32 order, IDSZ idsz );
void set_alerts( Uint16 character );
void attach_character_to_mount( Uint16 character, Uint16 mount,
                                Uint16 grip );
Uint16 stack_in_pack( Uint16 item, Uint16 character );
void add_item_to_character_pack( Uint16 item, Uint16 character );
Uint16 get_item_from_character_pack( Uint16 character, Uint16 grip, Uint8 ignorekurse );
void drop_keys( Uint16 character );
void drop_all_items( Uint16 character );
void character_grab_stuff( Uint16 chara, int grip, Uint8 people );
void character_swipe( Uint16 cnt, Uint8 slot );
void move_characters();

int module_reference_matches(  const char *szLoadName, IDSZ idsz );
void add_module_idsz(  const char *szLoadName, IDSZ idsz );

void make_textureoffset();
int add_player( Uint16 character, Uint16 player, Uint32 device );
void clear_messages();
void setup_characters(  const char *modname );

void setup_alliances(  const char *modname );
void load_mesh_fans();
void make_twist();
int  load_mesh(  const char *modname );

void set_one_player_latch( Uint16 player );
void set_local_latches( void );

void make_onwhichfan( void );
void bump_characters( void );
int  prt_is_over_water( Uint16 cnt );
void do_weather_spawn();
void stat_return();
void update_pits();
void reset_players();
int  find_module(  const char *smallname );
void listen_for_packets();
void unbuffer_player_latches();
void resize_characters();
void update_game();
void update_timers();
void load_basic_textures(  const char *modname );
Uint16 action_number();
Uint16 action_frame();
Uint16 test_frame_name( char letter );
void action_copy_correct( Uint16 object, Uint16 actiona, Uint16 actionb );
void get_walk_frame( Uint16 object, int lip, int action );
void get_framefx( int frame );
void make_framelip( Uint16 object, int action );
void get_actions( Uint16 object );
void read_pair( FILE* fileread );
void undo_pair( int base, int rand );
void export_one_character_name(  const char *szSaveName, Uint16 character );
void export_one_character_profile(  const char *szSaveName, Uint16 character );
void export_one_character_skin(  const char *szSaveName, Uint16 character );
int load_one_character_profile(  const char *szLoadName );
int load_one_particle(  const char *szLoadName, Uint16 object, Uint16 pip );
void reset_particles(  const char* modname );
void make_mad_equally_lit( int model );
void get_message( FILE* fileread );
void load_all_messages(  const char *loadname, Uint16 object );
void check_copy(  const char* loadname, Uint16 object );
int  load_one_object( int skin,  const char* tmploadname );
int  load_all_objects(  const char *modname );
void load_all_global_objects(int skin);

void font_init();
void font_release();
void font_load(  const char* szBitmap,  const char* szSpacing );

void make_water();
void read_wawalite(  const char *modname );
void reset_teams();
void reset_messages();
void make_randie();
void load_module(  const char *smallname );

int    load_one_title_image( int titleimage,  const char *szLoadName );
bool_t load_valid_module( int modnumber,  const char *szLoadName );
int    get_module_summary(  const char *szLoadName );
void   load_all_menu_images();
bool_t load_blip_bitmap();

int   get_skin(  const char *filename );
void  check_player_import(  const char *dirname, bool_t initialize );


void reset_timers();


void gltitle();
int DirGetAttrib(  const char *fromdir );

int check_skills( Uint16 who, IDSZ whichskill );

//---------------------------------------------------------------------------------------------
//AI targeting
Uint16 get_target( Uint16 character, Uint32 maxdistance, TARGET_TYPE team, bool_t targetitems, bool_t targetdead, IDSZ idsz, bool_t excludeidsz);
Uint16 get_particle_target( float xpos, float ypos, float zpos, Uint16 facing,
                            Uint16 particletype, Uint8 team, Uint16 donttarget,
                            Uint16 oldtarget );

//---------------------------------------------------------------------------------------------
// Quest system
bool_t add_quest_idsz(  const char *whichplayer, IDSZ idsz );
Sint16    modify_quest_idsz(  const char *whichplayer, IDSZ idsz, Sint16 adjustment );
Sint16    check_player_quest(  const char *whichplayer, IDSZ idsz );

//---------------------------------------------------------------------------------------------
// Filesystem functions
void fs_init();
const char *fs_getTempDirectory();
const char *fs_getImportDirectory();
const char *fs_getGameDirectory();
const char *fs_getSaveDirectory();
int fs_fileExists( const char *filename );
int fs_fileIsDirectory( const char *filename );
int fs_createDirectory( const char *dirname );
int fs_removeDirectory( const char *dirname );
void fs_deleteFile( const char *filename );
void fs_copyFile( const char *source, const char *dest );
void fs_removeDirectoryAndContents( const char *dirname );
void fs_copyDirectory( const char *sourceDir, const char *destDir );
bool_t fcopy_line(FILE * fileread, FILE * filewrite);
Sint32 fget_int( FILE* fileread );

// Enumerate directory contents
const char *fs_findFirstFile( const char *path, const char *extension );
const char *fs_findNextFile( void );
void fs_findClose();

//---------------------------------------------------------------------------------------------
// Networking functions
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

//---------------------------------------------------------------------------------------------
// Linux hook into the main function
int SDL_main( int argc, char **argv );

#define _PROTO_H_
