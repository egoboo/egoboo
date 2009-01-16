/* Egoboo - proto.h
 * Function prototypes for a huge portion of the game code.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
*/

#ifndef _PROTO_H_
#define _PROTO_H_

#include "egobootypedef.h"
#include <SDL_mixer.h> // for Mix_* stuff.

// typedef struct glmatrix { float v[16]; } glMatrix;  // was v[4][4], changed for OGL compatibility
// typedef struct glvector { float x,y,z; } glVector;
/*glVector vsub(glVector A, glVector B);
glVector Normalize(glVector vec);
glVector CrossProduct(glVector A, glVector B);
float DotProduct(glVector A, glVector B);*/
void load_graphics();
void save_settings();
bool_t save_input_settings(char* whichfile);
void empty_import_directory( void );
void insert_space( int position );
void copy_one_line( int write );
int load_one_line( int read );
int load_parsed_line( int read );
void surround_space( int position );
void parse_null_terminate_comments();
int get_indentation();
void fix_operators();
int starts_with_capital_letter();
Uint32 get_high_bits();
int tell_code( int read );
void add_code( Uint32 highbits );
void parse_line_by_line();
Uint32 jump_goto( int index );
void parse_jumps( int ainumber );
void log_code( int ainumber, char* savename );
int ai_goto_colon( int read );
void get_code( int read );
void load_ai_codes( char* loadname );
int load_ai_script( char *loadname );
void reset_ai_script();
int what_action( char cTmp );
int get_level( Uint8 x, Uint8 y, Uint32 fan, Uint8 waterwalk );
bool_t load_one_icon( char *szLoadName );
bool_t load_all_global_icons();

//Free memory functions
void prime_titleimage();
void prime_icons();
void release_all_icons();
void release_all_titleimages();
void release_all_models();
void release_map();
void release_module();
void close_session();
void release_all_textures();
void memory_cleanUp();

//Sound
bool_t sdlmixer_initialize();
int play_sound( float xpos, float ypos, Mix_Chunk *loadedwave );
void stop_sound( int whichchannel );
void play_music( Sint8 songnumber, Uint16 fadetime, Sint8 loops );
void make_newloadname( char *modname, char *appendname, char *newloadname );
void load_global_waves( char *modname );

// Exporting stuff
void export_one_character( int character, int owner, int number, bool_t is_local );
void export_all_local_players( void );
void export_all_players( bool_t require_local );

void quit_module( void );
void quit_game( void );
void goto_colon( FILE* fileread );
Uint8 goto_colon_yesno( FILE* fileread );
char get_first_letter( FILE* fileread );
void reset_tags();
int read_tag( FILE *fileread );
void read_all_tags( char *szFilename );
int tag_value( char *string );
char* tag_to_string( Uint32 tag, bool_t onlykeys );
void read_controls( char *szFilename );
Uint8 control_key_is_pressed( Uint8 control );
Uint8 control_mouse_is_pressed( Uint8 control );
Uint8 control_joya_is_pressed( Uint8 control );
Uint8 control_joyb_is_pressed( Uint8 control );
void free_all_enchants();
void load_one_enchant_type( char* szLoadName, Uint16 profile );
Uint16 get_free_enchant();
void unset_enchant_value( Uint16 enchantindex, Uint8 valueindex );
void remove_enchant_value( Uint16 enchantindex, Uint8 valueindex );
int get_free_message( void );
void display_message( int message, Uint16 character );


//IDSZ handling
char *  undo_idsz( IDSZ idsz );
IDSZ get_idsz( FILE* fileread );

//String handling
char * get_file_path(char *character);

//Enchants
void remove_enchant( Uint16 enchantindex );
Uint16 enchant_value_filled( Uint16 enchantindex, Uint8 valueindex );
void set_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype );
void getadd( int min, int value, int max, int* valuetoadd );
void fgetadd( float min, float value, float max, float* valuetoadd );
void add_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype );
Uint16 spawn_enchant( Uint16 owner, Uint16 target,
                      Uint16 spawner, Uint16 enchantindex, Uint16 modeloptional );


void load_action_names( char* loadname );
void get_name( FILE* fileread, char *szName );
void read_setup( char* filename );
void log_madused( char *savename );
void make_lightdirectionlookup();
float light_for_normal( int rotation, int normal, float lx, float ly, float lz, float ambi );
void make_lighttable( float lx, float ly, float lz, float ambi );
void make_lighttospek( void );
int vertexconnected( int modelindex, int vertex );
void get_madtransvertices( int modelindex );
int rip_md2_header( void );
void fix_md2_normals( Uint16 modelindex );
void rip_md2_commands( Uint16 modelindex );
int rip_md2_frame_name( int frame );
void rip_md2_frames( Uint16 modelindex );
int load_one_md2( char* szLoadname, Uint16 modelindex );
void load_all_music_sounds();
void stop_music();

//Passages
int open_passage( int passage );
void check_passage_music();
int break_passage( int passage, Uint16 starttile, Uint16 frames,
                   Uint16 become, Uint8 meshfxor );
void flash_passage( int passage, Uint8 color );
Uint8 find_tile_in_passage( int passage, int tiletype );
Uint16 who_is_blocking_passage( int passage );
Uint16 who_is_blocking_passage_ID( int passage, IDSZ idsz );
int close_passage( int passage );
void clear_passages();
void add_shop_passage( int owner, int passage );
void add_passage( int tlx, int tly, int brx, int bry, Uint8 open, Uint8 mask );


void flash_character_height( int character, Uint8 valuelow, Sint16 low,
                             Uint8 valuehigh, Sint16 high );
void flash_character( int character, Uint8 value );
void add_to_dolist( int cnt );
void order_dolist( void );
void make_dolist( void );
void keep_weapons_with_holders();
void make_enviro( void );
void make_prtlist( void );
void make_one_character_matrix( Uint16 cnt );
void free_one_particle_no_sound( int particle );
void play_particle_sound( int particle, Sint8 sound );
void free_one_particle( int particle );
void free_one_character( int character );
void free_inventory( int character );
void attach_particle_to_character( int particle, int character, int grip );
void make_one_weapon_matrix( Uint16 cnt );
void make_character_matrices();
int get_free_particle( int force );
int get_free_character();

/*Uint8 find_target_in_block( int x, int y, float chrx, float chry, Uint16 facing,
                            Uint8 onlyfriends, Uint8 anyone, Uint8 team,
                            Uint16 donttarget, Uint16 oldtarget );*/

void debug_message( char *text );
void reset_end_text();
void append_end_text( int message, Uint16 character );
Uint16 spawn_one_particle( float x, float y, float z,
                           Uint16 facing, Uint16 model, Uint16 pip,
                           Uint16 characterattach, Uint16 grip, Uint8 team,
                           Uint16 characterorigin, Uint16 multispawn, Uint16 oldtarget );
Uint8 __prthitawall( int particle );
void disaffirm_attached_particles( Uint16 character );
Uint16 number_of_attached_particles( Uint16 character );
void reaffirm_attached_particles( Uint16 character );
void do_enchant_spawn();
void move_particles( void );
void attach_particles();
void free_all_particles();
void free_all_characters();
void show_stat( Uint16 statindex );
void check_stats();
void check_screenshot();
bool_t dump_screenshot();
void add_stat( Uint16 character );
void move_to_top( Uint16 character );
void sort_stat();
void setup_particles();
Uint16 terp_dir( Uint16 majordir, Uint16 minordir );
Uint16 terp_dir_fast( Uint16 majordir, Uint16 minordir );
Uint8 __chrhitawall( int character );
void move_water( void );
void play_action( Uint16 character, Uint16 action, Uint8 actionready );
void set_frame( int character, int frame, Uint16 lip );
void reset_character_alpha( Uint16 character );
void reset_character_accel( Uint16 character );
void detach_character_from_mount( Uint16 character, Uint8 ignorekurse,
                                  Uint8 doshop );
void spawn_bump_particles( Uint16 character, Uint16 particle );
int generate_number( int numbase, int numrand );
void drop_money( Uint16 character, Uint16 money );
void call_for_help( Uint16 character );
void give_experience( int character, int amount, Uint8 xptype );
void give_team_experience( Uint8 team, int amount, Uint8 xptype );
void disenchant_character( Uint16 cnt );
void damage_character( Uint16 character, Uint16 direction,
                       int damagebase, int damagerand, Uint8 damagetype, Uint8 team,
                       Uint16 attacker, Uint16 effects );
void kill_character( Uint16 character, Uint16 killer );
void spawn_poof( Uint16 character, Uint16 profile );
void naming_names( int profile );
void read_naming( int profile, char *szLoadname );
void prime_names( void );
void tilt_characters_to_terrain();
int spawn_one_character( float x, float y, float z, int profile, Uint8 team,
                         Uint8 skin, Uint16 facing, char *name, int override );
void respawn_character( Uint16 character );
Uint16 change_armor( Uint16 character, Uint16 skin );
void change_character( Uint16 cnt, Uint16 profile, Uint8 skin,
                       Uint8 leavewhich );
Uint8 cost_mana( Uint16 character, int amount, Uint16 killer );
void switch_team( int character, Uint8 team );
void issue_clean( Uint16 character );
int restock_ammo( Uint16 character, IDSZ idsz );
void issue_order( Uint16 character, Uint32 order );
void issue_special_order( Uint32 order, IDSZ idsz );
void set_alerts( int character );
int module_reference_matches( char *szLoadName, IDSZ idsz );
void add_module_idsz( char *szLoadName, IDSZ idsz );
Uint8 run_function( Uint32 value, int character );
void set_operand( Uint8 variable );
void run_operand( Uint32 value, int character );
void let_character_think( int character );
void let_ai_think();
void attach_character_to_mount( Uint16 character, Uint16 mount,
                                Uint16 grip );
Uint16 stack_in_pack( Uint16 item, Uint16 character );
void add_item_to_character_pack( Uint16 item, Uint16 character );
Uint16 get_item_from_character_pack( Uint16 character, Uint16 grip, Uint8 ignorekurse );
void drop_keys( Uint16 character );
void drop_all_items( Uint16 character );
void character_grab_stuff( int chara, int grip, Uint8 people );
void character_swipe( Uint16 cnt, Uint8 grip );
void move_characters( void );
void make_textureoffset( void );
int add_player( Uint16 character, Uint16 player, Uint8 device );
void clear_messages();
void setup_characters( char *modname );
void setup_passage( char *modname );
void setup_alliances( char *modname );
void load_mesh_fans();
void make_fanstart();
void make_twist();
int load_mesh( char *modname );
void read_mouse();
void read_key();
void read_joystick();
void read_input();
void check_add( Uint8 key, char bigletter, char littleletter );
void create_szfpstext( int frames );

void project_view();
void make_renderlist();

void figure_out_what_to_draw();

void set_one_player_latch( Uint16 player );
void set_local_latches( void );

//Camera stuff
void adjust_camera_angle( float height );
void move_camera();
void make_camera_matrix();
void camera_look_at( float x, float y );


void make_onwhichfan( void );
void bump_characters( void );
int prt_is_over_water( int cnt );
void do_weather_spawn();
void animate_tiles();
void stat_return();
void update_pits();
void reset_players();
int find_module( char *smallname );
void listen_for_packets();
void unbuffer_player_latches();
void resize_characters();
void update_game();
void update_timers();
void load_basic_textures( char *modname );
Uint16 action_number();
Uint16 action_frame();
Uint16 test_frame_name( char letter );
void action_copy_correct( int object, Uint16 actiona, Uint16 actionb );
void get_walk_frame( int object, int lip, int action );
void get_framefx( int frame );
void make_framelip( int object, int action );
void get_actions( int object );
void read_pair( FILE* fileread );
void undo_pair( int base, int rand );
void ftruthf( FILE* filewrite, char* text, Uint8 truth );
void fdamagf( FILE* filewrite, char* text, Uint8 damagetype );
void factiof( FILE* filewrite, char* text, Uint8 action );
void fgendef( FILE* filewrite, char* text, Uint8 gender );
void fpairof( FILE* filewrite, char* text, int base, int rand );
void funderf( FILE* filewrite, char* text, char* usename );
void export_one_character_name( char *szSaveName, Uint16 character );
void export_one_character_profile( char *szSaveName, Uint16 character );
void export_one_character_skin( char *szSaveName, Uint16 character );
int load_one_character_profile( char *szLoadName );
int load_one_particle( char *szLoadName, int object, int pip );
void reset_particles( char* modname );
void make_mad_equally_lit( int model );
void get_message( FILE* fileread );
void load_all_messages( char *loadname, int object );
void check_copy( char* loadname, int object );
int load_one_object( int skin, char* tmploadname );
void load_all_objects( char *modname );
void load_bars( char* szBitmap );
void load_map( char* szModule );
void load_font( char* szBitmap, char* szSpacing, int sysmem );
void make_water();
void read_wawalite( char *modname );
void reset_teams();
void reset_messages();
void make_randie();
void load_module( char *smallname );
void render_prt();
void render_shadow( int character );
void render_bad_shadow( int character );
void render_refprt();
void render_fan( Uint32 fan );
void render_water_fan( Uint32 fan, Uint8 layer );
void render_enviromad( Uint16 character, Uint8 trans );
void render_texmad( Uint16 character, Uint8 trans );
void render_mad( Uint16 character, Uint8 trans );
void render_refmad( int tnc, Uint8 trans );
void light_characters();
void light_particles();
void set_fan_light( int fanx, int fany, Uint16 particle );
void do_dynalight();
void render_water();
void draw_scene_sadreflection();
void draw_scene_zreflection();
bool_t get_mesh_memory();
void draw_one_icon( int icontype, int x, int y, Uint8 sparkle );
void draw_one_font( int fonttype, int x, int y );
void draw_map( int x, int y );
int draw_one_bar( int bartype, int x, int y, int ticks, int maxticks );
void draw_string( char *szText, int x, int y );
int length_of_word( char *szText );
int draw_wrap_string( char *szText, int x, int y, int maxx );
int draw_status( Uint16 character, int x, int y );
void draw_text();
void flip_pages();
void draw_scene();
void draw_main();

int load_one_title_image( int titleimage, char *szLoadName );
int get_module_data( int modnumber, char *szLoadName );
int get_module_summary( char *szLoadName );
void load_all_menu_images();
bool_t load_blip_bitmap();
void do_cursor();
void menu_service_select();
void menu_start_or_join();
void draw_module_tag( int module, int y );
int get_skin( char *filename );
void check_player_import( char *dirname );
void menu_pick_player( int module );
void menu_module_loading( int module );
void menu_choose_host();
void menu_choose_module();
void menu_boot_players();
void menu_end_text();
void menu_initial_text();
void fiddle_with_menu();
void reset_timers();
void reset_camera();
void sdlinit( int argc, char **argv );
int glinit( int argc, char **argv );
void gltitle();
int DirGetAttrib( char *fromdir );

int check_skills( Uint16 who, IDSZ whichskill );

//---------------------------------------------------------------------------------------------
//AI targeting
Uint16 get_target( Uint16 character, Uint32 maxdistance, TARGET_TYPE team, bool_t targetitems, bool_t targetdead, IDSZ idsz, bool_t excludeidsz);
Uint16 get_particle_target( float xpos, float ypos, float zpos, Uint16 facing,
                    Uint16 particletype, Uint8 team, Uint16 donttarget, 
					Uint16 oldtarget );

//---------------------------------------------------------------------------------------------
// Quest system
bool_t add_quest_idsz( char *whichplayer, IDSZ idsz );
Sint16    modify_quest_idsz( char *whichplayer, IDSZ idsz, Sint16 adjustment );
Sint16    check_player_quest( char *whichplayer, IDSZ idsz );

//---------------------------------------------------------------------------------------------
// Filesystem functions
void fs_init();
const char *fs_getTempDirectory();
const char *fs_getImportDirectory();
const char *fs_getGameDirectory();
const char *fs_getSaveDirectory();
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
void packet_addString( char *string );

void net_sendPacketToHost();
void net_sendPacketToAllPlayers();
void net_sendPacketToHostGuaranteed();
void net_sendPacketToAllPlayersGuaranteed();
void net_sendPacketToOnePlayerGuaranteed( int player );
void input_net_message();

void net_updateFileTransfers();
int  net_pendingFileTransfers();

void net_copyFileToAllPlayers( char *source, char *dest );
void net_copyFileToHost( char *source, char *dest );
void net_copyDirectoryToHost( char *dirname, char *todirname );
void net_copyDirectoryToAllPlayers( char *dirname, char *todirname );
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
// Platform specific functions

// Functions in this section are implemented separately on each platform. (Filesystem stuff
// could go here as well.)

void sys_initialize();  // Allow any setup necessary for platform specific code
void sys_shutdown();  // Allow any necessary cleanup for platform specific code

double sys_getTime();  // Return the current time, in seconds

#endif //#ifndef _PROTO_H_
