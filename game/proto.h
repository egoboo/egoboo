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
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _PROTO_H_
#define _PROTO_H_

#include "egobootypedef.h"
#include "mathstuff.h"

#include <SDL_mixer.h> // for Mix_* stuff.
#include <SDL_opengl.h>

typedef enum control_t CONTROL;
typedef enum slot_e SLOT;
typedef enum grip_e GRIP;
typedef enum action_e ACTION;
typedef enum lip_transition_e LIPT;
typedef enum damage_e DAMAGE;
typedef enum experience_e EXPERIENCE;
typedef enum team_e TEAM;
typedef enum gender_e GENDER;
typedef enum dynalight_mode_e DYNA_MODE;
typedef enum particle_type PRTTYPE;
typedef enum blud_level_e BLUD_LEVEL;
typedef enum respawn_mode_e RESPAWN_MODE;
typedef enum idsz_index_e IDSZ_INDEX;
typedef enum color_e COLR;

typedef struct gltexture_t GLTexture;

void load_graphics();

void empty_import_directory( void );
void insert_space( size_t position );
void copy_one_line( size_t write );
size_t load_one_line( size_t read );
size_t load_parsed_line( size_t read );
void surround_space( size_t position );
void parse_null_terminate_comments();
int get_indentation();
void fix_operators();
int starts_with_capital_letter();
Uint32 get_high_bits();
size_t tell_code( size_t read );
void add_code( Uint32 highbits );
void parse_line_by_line();
Uint32 jump_goto( int index );
void parse_jumps( int ainumber );
void log_code( int ainumber, char* savename );
int ai_goto_colon( int read );
void fget_code( FILE * pfile );
void load_ai_codes( char* loadname );
Uint32 load_ai_script( char *loadname );
void reset_ai_script();
ACTION what_action( char cTmp );
float mesh_get_level( Uint32 fan, float x, float y, bool_t waterwalk );
void release_all_textures();
void load_one_icon( char *szLoadName );
void prime_titleimage();
void prime_icons();
void release_all_icons();
void release_all_titleimages();
void reset_sounds();
void release_map();
void release_module( void );
void close_session();
bool_t memory_cleanUp();
void make_newloadname( char *modname, char *appendname, char *newloadname );
void export_one_character( CHR_REF character, Uint16 owner, int number );
void export_all_local_players( void );
void quit_module( void );
void quit_game( void );
void fgoto_colon( FILE* fileread );
bool_t fgoto_colon_yesno( FILE* fileread );
char fget_first_letter( FILE* fileread );

//Tags
void reset_tags();
int read_tag( FILE *fileread );
void read_all_tags( char *szFilename );
int tag_value( char *string );
void read_controls( char *szFilename );
bool_t control_key_is_pressed( CONTROL control );
bool_t control_mouse_is_pressed( CONTROL control );
bool_t control_joya_is_pressed( CONTROL control );
bool_t control_joyb_is_pressed( CONTROL control );

//Enchants
void free_all_enchants();


void load_one_enchant_profile( char* szLoadName, Uint16 profile );
Uint16 get_free_enchant();
void unset_enchant_value( Uint16 enchantindex, Uint8 valueindex );
void remove_enchant_value( Uint16 enchantindex, Uint8 valueindex );
int get_free_message( void );
void display_message( int message, CHR_REF character );
void remove_enchant( Uint16 enchantindex );
Uint16 enchant_value_filled( Uint16 enchantindex, Uint8 valueindex );
void set_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype );
void getadd( int MIN, int value, int MAX, int* valuetoadd );
void fgetadd( float MIN, float value, float MAX, float* valuetoadd );
void add_enchant_value( Uint16 enchantindex, Uint8 valueindex,
                        Uint16 enchanttype );
Uint16 spawn_enchant( Uint16 owner, Uint16 target,
                      Uint16 spawner, Uint16 enchantindex, Uint16 modeloptional );
void load_action_names( char* loadname );
void read_setup( char* filename );
void log_madused( char *savename );
void make_lightdirectionlookup();
float spek_global_lighting( int rotation, int inormal, vect3 lite );
void make_spektable( vect3 lite );
void make_lighttospek( void );


//Sound and music stuff
void load_all_music_sounds();
void stop_music(int fadetime);
void load_global_waves( char *modname );
bool_t sdlmixer_initialize();
void sound_apply_mods( int channel, float intensity, vect3 snd_pos, vect3 ear_pos, Uint16 ear_turn_lr  );
int play_sound( float intensity, vect3 pos, Mix_Chunk *loadedwave, int loops  );
void stop_sound( int whichchannel );
void play_music( int songnumber, int fadetime, int loops );

//Graphical Functions
void render_particles();
void render_particle_reflections();
void render_mad_lit( CHR_REF character );
void render_water_fan_lit( Uint32 fan, Uint8 layer, Uint8 mode );

bool_t open_passage( Uint32 passage );
void check_passage_music();
int break_passage( Uint32 passage, Uint16 starttile, Uint16 frames,
                   Uint16 become, Uint32 meshfxor );
void flash_passage( Uint32 passage, Uint8 color );
bool_t  search_tile_in_passage( Uint32 passage, Uint32 tiletype );
Uint16 who_is_blocking_passage( Uint32 passage );
Uint16 who_is_blocking_passage_ID( Uint32 passage, IDSZ idsz );
bool_t close_passage( Uint32 passage );
void clear_passages();
Uint32 add_shop_passage( Uint16 owner, Uint32 passage );
Uint32 add_passage( int tlx, int tly, int brx, int bry, bool_t open, Uint32 mask );
void flash_character_height( CHR_REF character, Uint8 valuelow, Sint16 low,
                             Uint8 valuehigh, Sint16 high );
void flash_character( CHR_REF character, Uint8 value );
void add_to_dolist( Uint16 cnt );
void order_dolist( void );
void make_dolist( void );


void keep_weapons_with_holders();
void make_enviro( void );
void make_prtlist( void );
void make_turntosin( void );
bool_t make_one_character_matrix( CHR_REF cnt );
void free_one_particle_no_sound( PRT_REF particle );
void play_particle_sound( float intensity, PRT_REF particle, Sint8 sound );
void free_one_particle( PRT_REF particle );
void free_one_character( CHR_REF character );
void free_inventory( CHR_REF character );
void attach_particle_to_character( PRT_REF particle, CHR_REF character, Uint16 vertoffset );
bool_t make_one_weapon_matrix( Uint16 cnt );
void make_character_matrices();
int get_free_particle( int force );
int get_free_character();
void debug_message( int time, const char *format, ... );
void reset_end_text();
void append_end_text( int message, CHR_REF character );
PRT_REF spawn_one_particle( float intensity, vect3 pos,
                           Uint16 facing, Uint16 model, Uint16 pip,
                           CHR_REF characterattach, GRIP grip, TEAM team,
                           CHR_REF characterorigin, Uint16 multispawn, CHR_REF oldtarget );
Uint32 __prthitawall( PRT_REF particle, vect3 * norm );
void disaffirm_attached_particles( CHR_REF character );
Uint16 number_of_attached_particles( CHR_REF character );
void reaffirm_attached_particles( CHR_REF character );
void do_enchant_spawn( float dUpdate );
void despawn_particles();
void move_particles( float dUpdate );
void attach_particles();
void free_all_particles();
void free_all_characters();
void show_stat( Uint16 statindex );
void check_stats();
void check_screenshot();
bool_t dump_screenshot();
void add_stat( CHR_REF character );
void move_to_top( CHR_REF character );
void sort_stat();
void setup_particles();
Uint16 terp_dir( Uint16 majordir, float dx, float dy, float dUpdate );
Uint16 terp_dir_fast( Uint16 majordir, float dx, float dy, float dUpdate );
Uint32 __chrhitawall( CHR_REF character, vect3 * norm );
void move_water( float dUpdate );
void play_action( CHR_REF character, ACTION action, bool_t actionready );
void set_frame( CHR_REF character, Uint16 frame, Uint8 lip );
void reset_character_alpha( CHR_REF character );
void reset_character_accel( CHR_REF character );
bool_t detach_character_from_mount( CHR_REF character, bool_t ignorekurse, bool_t doshop );
void spawn_bump_particles( CHR_REF character, PRT_REF particle );
Uint32 generate_unsigned( PAIR * ppair );
Sint32 generate_signed( PAIR * ppair );
Sint32 generate_dither( PAIR * ppair, Uint16 strength_fp8 );
void drop_money( CHR_REF character, Uint16 money );
CHR_REF search_best_leader( TEAM team, Uint16 exclude );
void call_for_help( CHR_REF character );
void give_experience( CHR_REF character, int amount, EXPERIENCE xptype );
void give_team_experience( TEAM team, int amount, EXPERIENCE xptype );
void disenchant_character( Uint16 cnt );
void damage_character( CHR_REF character, Uint16 direction,
                       PAIR * ppair, DAMAGE damagetype, TEAM team,
                       Uint16 attacker, Uint16 effects );
void kill_character( CHR_REF character, Uint16 killer );
void spawn_poof( CHR_REF character, Uint16 profile );
void naming_names( int profile );
void read_naming( int profile, char *szLoadname );
void prime_names( void );
void tilt_characters_to_terrain();
CHR_REF spawn_one_character( vect3 pos, int profile, TEAM team,
                            Uint8 skin, Uint16 facing, char *name, Uint16 override );

void respawn_character( CHR_REF character );
Uint16 change_armor( CHR_REF character, Uint16 skin );
void change_character( Uint16 cnt, Uint16 profile, Uint8 skin,
                       Uint8 leavewhich );
bool_t cost_mana( CHR_REF character, int amount, Uint16 killer );
void switch_team( CHR_REF character, TEAM team );

void issue_clean( CHR_REF character );
int restock_ammo( CHR_REF character, IDSZ idsz );
void signal_target( Uint16 target, Uint16 upper, Uint16 lower );
void signal_team( CHR_REF character, Uint32 order );
void signal_idsz_index( Uint32 order, IDSZ idsz, IDSZ_INDEX index );
void set_alerts( CHR_REF character, float dUpdate );
bool_t module_reference_matches( char *szLoadName, IDSZ idsz );
void add_module_idsz( char *szLoadName, IDSZ idsz );

bool_t add_quest_idsz( char *whichplayer, IDSZ idsz );
bool_t beat_quest_idsz( char *whichplayer, IDSZ idsz );
int check_player_quest( char *whichplayer, IDSZ idsz );

bool_t run_function( Uint32 value, CHR_REF character );
void set_operand( Uint8 variable );
void run_operand( Uint32 value, CHR_REF character );
void let_character_think( CHR_REF character, float dUpdate );
void let_ai_think( float dUpdate );
bool_t attach_character_to_mount( CHR_REF character, Uint16 mount, SLOT slot );
CHR_REF stack_in_pack( CHR_REF item, CHR_REF character );
bool_t add_item_to_character_pack( Uint16 item, CHR_REF character );
Uint16 get_item_from_character_pack( CHR_REF character, SLOT slot, bool_t ignorekurse );
void drop_keys( CHR_REF character );
void drop_all_items( CHR_REF character );
bool_t character_grab_stuff( CHR_REF chara, SLOT slot, bool_t people );
void character_swipe( Uint16 cnt, SLOT slot );
void despawn_characters();
void move_characters( float dUpdate );
void make_textureoffset( void );
int add_player( CHR_REF character, Uint16 player, Uint8 device );
void clear_messages();
void setup_characters( char *modname );
void setup_passage( char *modname );
void setup_alliances( char *modname );

void read_mouse();
void read_key();
void read_joystick();
void reset_press();
void read_input();
void check_add( Uint8 key, char bigletter, char littleletter );
void camera_calc_turn_lr();
//void project_view();
void make_renderlist();
void make_camera_matrix();
void figure_out_what_to_draw();
void set_one_player_latch( Uint16 player );
void set_local_latches( void );
void adjust_camera_angle( int height );
void move_camera( float dUpdate );
void make_onwhichfan( void );
void do_bumping( float dUpdate );
bool_t prt_is_over_water( int cnt );
void do_weather_spawn( float dUpdate );
void animate_tiles( float dUpdate );
void stat_return( float dUpdate );
void pit_kill( float dUpdate );
void reset_players();
int find_module( char *smallname );
void resize_characters( float dUpdate );
void update_game( float dUpdate );
void update_timers();
void load_basic_textures( char *modname );
ACTION action_number(char * szName);
Uint16 action_frame();
bool_t test_frame_name( char letter );
void action_copy_correct( Uint16 object, ACTION actiona, ACTION actionb );
void get_walk_frame( Uint16 object, LIPT lip_trans, ACTION action );
Uint16 get_framefx( char * szName );
void make_framelip( Uint16 object, ACTION action );
void get_actions( Uint16 object );

bool_t undo_pair_fp8( PAIR * ppair, RANGE * prange );
bool_t fget_pair_fp8( FILE* fileread, PAIR * ppair );
bool_t fget_next_pair_fp8( FILE* fileread, PAIR * ppair );

bool_t undo_pair( PAIR * ppair, RANGE * prange );
bool_t fget_pair( FILE* fileread, PAIR * ppair );
bool_t fget_next_pair( FILE* fileread, PAIR * ppair );

char * undo_idsz( IDSZ idsz );
IDSZ fget_idsz( FILE* fileread );
IDSZ fget_next_idsz( FILE* fileread );

int   fget_int( FILE* fileread );
int   fget_next_int( FILE* fileread );

float fget_float( FILE* fileread );
float fget_next_float( FILE* fileread );

Uint16 fget_fixed( FILE* fileread );
Uint16 fget_next_fixed( FILE* fileread );

bool_t fget_bool( FILE* fileread );
bool_t fget_next_bool( FILE* fileread );

GENDER fget_gender( FILE* fileread );
GENDER fget_next_gender( FILE* fileread );

DAMAGE fget_damage( FILE *fileread );
DAMAGE fget_next_damage( FILE *fileread );

BLUD_LEVEL fget_blud( FILE *fileread );
BLUD_LEVEL fget_next_blud( FILE *fileread );

RESPAWN_MODE fget_respawn( FILE *fileread );
RESPAWN_MODE fget_next_respawn( FILE *fileread );

DYNA_MODE fget_dynamode( FILE *fileread );
DYNA_MODE fget_next_dynamode( FILE *fileread );

bool_t fget_name( FILE* fileread, char *szName, size_t lnName );
bool_t fget_next_name( FILE* fileread, char *szName, size_t lnName );

bool_t fget_string( FILE* fileread, char *szLine, size_t lnLine );
bool_t fget_next_string( FILE* fileread, char *szLine, size_t lnLine );

PRTTYPE fget_prttype( FILE * fileread );
PRTTYPE fget_next_prttype( FILE * fileread );

ACTION fget_action( FILE* fileread );
ACTION fget_next_action( FILE* fileread );



void ftruthf( FILE* filewrite, char* text, bool_t truth );
void fdamagf( FILE* filewrite, char* text, DAMAGE damagetype );
void factiof( FILE* filewrite, char* text, ACTION action );
void fgendef( FILE* filewrite, char* text, GENDER gender );
void fpairof( FILE* filewrite, char* text, PAIR * ppair );
void funderf( FILE* filewrite, char* text, char* usename );

void export_one_character_name( char *szSaveName, CHR_REF character );
void export_one_character_profile( char *szSaveName, CHR_REF character );
void export_one_character_skin( char *szSaveName, CHR_REF character );
int load_one_character_profile( char *szLoadName );
Uint32 load_one_particle_profile( char *szLoadName, Uint16 object, int pip );
void reset_particles( char* modname );
void make_mad_equally_lit( Uint16 model );
bool_t fget_message( FILE* fileread );
bool_t fget_next_message( FILE* fileread );
void load_all_messages( char *loadname, Uint16 object );
void check_copy( char* loadname, Uint16 object );
int load_one_object( int skin, char* tmploadname );
void load_all_objects( char *modname );
bool_t load_bars( char* szBitmap );
void load_map( char* szModule );
bool_t load_font( char* szBitmap, char* szSpacing );
void make_water();
void read_wawalite( char *modname );
void reset_teams();
void reset_messages();
void make_randie();
void load_module( char *smallname );
void render_prt();
void render_shadow( CHR_REF character );
void render_bad_shadow( CHR_REF character );
void render_refprt();
void render_fan( Uint32 fan, char tex_loaded );
void render_fan_ref( Uint32 fan, char tex_loaded, float level );
void render_water_fan( Uint32 fan, Uint8 layer, Uint8 mode );
void render_enviromad( CHR_REF character, Uint8 trans );
void render_texmad( CHR_REF character, Uint8 trans );
void render_mad( CHR_REF character, Uint8 trans );
void render_refmad( int tnc, Uint8 trans );
void light_characters();
void light_particles();
void set_fan_light( int fanx, int fany, PRT_REF particle );
void do_dynalight();
void render_water();
void draw_scene_zreflection();
void draw_blip( COLR color, float x, float y );
void draw_one_icon( int icontype, int x, int y, Uint8 sparkle );
void draw_one_font( int fonttype, float x, float y );
void draw_map( float x, float y );
int draw_one_bar( int bartype, int x, int y, int ticks, int maxticks );
int draw_string( GLTexture * pfnt, float x, float y, GLfloat tint[], char * szFormat, ... );
int length_of_word( char *szText );
int draw_wrap_string( GLTexture * pfnt, float x, float y, GLfloat tint[], float maxx, char * szFormat, ... );
int draw_status( GLTexture * pfnt, CHR_REF character, int x, int y );
void draw_text( GLTexture * pfnt );
bool_t request_pageflip();
bool_t do_pageflip();
bool_t do_clear();
void draw_scene();
void draw_main( float );
bool_t draw_texture_box( GLTexture * ptx, FRect * tx_rect, FRect * sc_rect );


bool_t prt_search_target_in_block( int block_x, int block_y, float prtx, float prty, Uint16 facing,
                                   bool_t request_friends, bool_t allow_anyone, TEAM team,
                                   Uint16 donttarget, Uint16 oldtarget );
CHR_REF prt_search_target( float prtx, float prty, Uint16 facing,
                           Uint16 targetangle, bool_t request_friends, bool_t allow_anyone,
                           TEAM team, Uint16 donttarget, Uint16 oldtarget );

CHR_REF chr_search_distant_target( CHR_REF character, int maxdist, bool_t ask_enemies, bool_t ask_dead );

void chr_search_nearest_in_block( int block_x, int block_y, CHR_REF character, bool_t ask_items,
                                  bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, bool_t seeinvisible, IDSZ idsz );

CHR_REF chr_search_nearest_target( CHR_REF character, bool_t ask_items,
                                   bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz );

CHR_REF chr_search_wide_target( CHR_REF character, bool_t ask_items,
                                bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz, bool_t excludeid );

CHR_REF chr_search_target_in_block( int block_x, int block_y, CHR_REF character, bool_t ask_items,
                                    bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, bool_t seeinvisible, IDSZ idsz,
                                    bool_t excludeid );

CHR_REF chr_search_nearby_target( CHR_REF character, bool_t ask_items,
                                  bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ ask_idsz );


bool_t load_one_title_image( int titleimage, char *szLoadName );
bool_t get_module_data( int modnumber, char *szLoadName );
bool_t get_module_summary( char *szLoadName );
void load_all_menu_images();
void load_blip_bitmap( char * modname );
void draw_trimx( int x, int y, int length );
void draw_trimy( int x, int y, int length );
void draw_trim_box( int left, int top, int right, int bottom );
void draw_trim_box_opening( int left, int top, int right, int bottom, float amount );
void draw_titleimage( int image, int x, int y );
void do_cursor();
void draw_module_tag( int module, int y );
int get_skin( char *filename );
bool_t check_skills( int who, Uint32 whichskill );
void check_player_import( char *dirname );
void reset_timers();
void reset_camera();
void sdlinit( int argc, char **argv );
int  glinit( int argc, char **argv );
void gltitle();


//---------------------------------------------------------------------------------------------
// Filesystem functions

typedef enum priority_e
{
  PRI_NONE = 0,
  PRI_WARN,
  PRI_FAIL
} PRIORITY;

int DirGetAttrib( char *fromdir );
void fs_init();
FILE * fs_fileOpen( PRIORITY pri, const char * src, const char * fname, const char * mode );
void fs_fileClose( FILE * pfile );
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

// Enumerate directory contents
const char *fs_findFirstFile( const char *path, const char *extension );
const char *fs_findNextFile( void );
void fs_findClose();



void render_mad_lit( CHR_REF character );
void render_particle_reflections();
void render_water_fan_lit( Uint32 fan, Uint8 layer, Uint8 mode );

void BeginText( GLTexture * pfnt );
void EndText( void );

void Begin2DMode( void );
void End2DMode( void );

void make_speklut();

bool_t handle_opengl_error();

char * convert_spaces( char *strout, size_t insize, char * strin );
char * convert_underscores( char *strout, size_t insize, char * strin );

bool_t passage_check_any( CHR_REF ichr, Uint16 pass, Uint16 * powner );
bool_t passage_check_all( CHR_REF ichr, Uint16 pass, Uint16 * powner );
bool_t passage_check( CHR_REF ichr, Uint16 pass, Uint16 * powner );

// MD2 Stuff
typedef struct ego_md2_model_t MD2_Model;

int vertexconnected( MD2_Model * m, int vertex );
int count_madtransvertices( MD2_Model * m );
int rip_md2_header( void );
char * rip_md2_frame_name( MD2_Model * m, int frame );
int load_one_md2( char* szLoadname, Uint16 modelindex );
void free_one_md2( Uint16 imdl );
void release_all_models();
void init_all_models();

void md2_blend_vertices(CHR_REF ichr, Sint32 vrtmin, Sint32 vrtmax);
void md2_blend_lighting(CHR_REF ichr);
bool_t md2_calculate_bumpers(CHR_REF ichr, int level);
bool_t prt_calculate_bumpers(PRT_REF iprt);

typedef struct collision_volume_t CVolume;
void cv_list_add( CVolume * cv);
void cv_list_clear();
void cv_list_draw();
void draw_CVolume( CVolume * cv );


#endif //#ifndef _PROTO_H_
