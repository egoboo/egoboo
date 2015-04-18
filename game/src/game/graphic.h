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

/// @file game/graphic.h

#pragma once

#include "game/egoboo_typedef.h"

#include "game/mesh.h"
#include "game/mad.h"
#include "game/Graphics/CameraSystem.hpp"
#include "game/egoboo.h"
#include "egolib/DynamicArray.hpp"

//--------------------------------------------------------------------------------------------
// external structs
//--------------------------------------------------------------------------------------------

// Forward declarations.
class Object;
struct egoboo_config_t;
struct chr_instance_t;

//--------------------------------------------------------------------------------------------
// internal structs
//--------------------------------------------------------------------------------------------

struct s_GLvertex;
typedef struct s_GLvertex GLvertex;



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the default icon size in pixels
#define ICON_SIZE 32



/// max number of blips on the minimap
#define MAXBLIP        128                          ///<Max blips on the screen

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// special return values
enum gfx_rv
{
    gfx_error   = -1,
    gfx_fail    = false,
    gfx_success = true
};

#define GFX_ERROR_MAX 256

struct gfx_error_state_t
{
    STRING file;
    STRING function;
    int    line;

    int    type;
    STRING string;
};

#define GFX_ERROR_STATE_INIT { "UNKNOWN", "UNKNOWN", -1, -1, "NONE" }

struct gfx_error_stack_t
{
    size_t count;
    gfx_error_state_t lst[GFX_ERROR_MAX];
};

#define GFX_ERROR_STACK_INIT { 0, { GFX_ERROR_STATE_INIT } }

egolib_rv           gfx_error_add( const char * file, const char * function, int line, int id, const char * sz );
gfx_error_state_t * gfx_error_pop();
void                gfx_error_clear();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAPSIZE 96

#define TABX                            32// 16      ///< Size of little name tag on the bar
#define BARX                            112// 216         ///< Size of bar
#define BARY                            16// 8
#define NUMTICK                         10// 50          ///< Number of ticks per row
#define TICKX                           8// 4           ///< X size of each tick
#define MAXTICK                         (NUMTICK*10) ///< Max number of ticks to draw
#define XPTICK                          6.00f

#define NUMBAR                          6               ///< Number of status bars
#define NUMXPBAR                        2               ///< Number of xp bars

#define MAXLIGHTLEVEL                   16          ///< Number of premade light intensities
#define MAXSPEKLEVEL                    16          ///< Number of premade specularities
#define MAXLIGHTROTATION                256         ///< Number of premade light maps

#define DONTFLASH                       255
#define SEEKURSEAND                     31          ///< Blacking flash

#define SHADOWRAISE                       5

/// The supported colors of bars and blips
enum e_color
{
    COLOR_WHITE = 0,
    COLOR_RED,
    COLOR_YELLOW,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_PURPLE,
    COLOR_MAX
};

// The map file only supports 256 tile textures:
// Four texture atlases each with 8 x 8 textures hence 4 * (8 * 8) = 256 different tile textures.
#define MESH_IMG_COUNT 256

#define VALID_MESH_TX_RANGE(VAL) ( ((VAL)>=0) && ((VAL)<MESH_IMG_COUNT) )

//#define CALC_OFFSET_X(IMG) ((( (IMG) >> 0 ) & 7 ) / 8.0f)
//#define CALC_OFFSET_Y(IMG) ((( (IMG) >> 3 ) & 7 ) / 8.0f)

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define XX 0

struct renderlist_lst_t
{
    struct element_t
    {
        Uint32 index;             ///< which tile
        float distance;          ///< how far it is
#if XX ==  1
        element_t() :
            index(), distance(-1.0f)
        {}
        element_t(const element_t& other) :
            index(other.index), distance(other.distance)
        {}
        virtual ~element_t()
        {}
        element_t& operator=(const element_t& other)
        {
            index = other.index;
            distance = other.distance;
            return *this;
        }
#endif
    };
    /**
    * @brief
    *    The maximum capacity of a renderlist
    *    i.e. the maximum number of tiles in a render list
    *    i.e. the maximum number of tiles to draw.
    */
    static const size_t CAPACITY = 1024;
    size_t size;              ///< how many in the list
    element_t lst[CAPACITY];  ///< the list

#if XX == 1
    renderlist_lst_t() :
        size(0), lst(),
    {}
    virtual ~renderlist_lst_t()
    {}
#endif

    static gfx_rv reset(renderlist_lst_t *self);
    static gfx_rv push(renderlist_lst_t *self, const TileIndex& index, float distance);
};

/// Which tiles are to be drawn, arranged by MAPFX_* bits
struct renderlist_t
{
    ego_mesh_t *mesh;
    size_t index;
    renderlist_lst_t  all;     ///< List of which to render, total
    renderlist_lst_t  ref;     ///< ..., is reflected in the floor
    renderlist_lst_t  sha;     ///< ..., is not reflected in the floor
    renderlist_lst_t  drf;     ///< ..., draws character reflections
    renderlist_lst_t  ndr;     ///< ..., draws no character reflections
    renderlist_lst_t  wat;     ///< ..., draws a water tile

#if XX == 1
    renderlist_t() :
        mesh(nullptr), index(std::numeric_limits<size_t>::max()),
        all(), ref(), sha(), drf(), ndr(), wat()
    {}
    virtual ~renderlist_t() :
    {}
#endif

    static renderlist_t *init(renderlist_t *self, size_t index);
    /// @brief Clear a render list
    static gfx_rv reset(renderlist_t *self);
    /// @brief Insert a tile into this render list.
    /// @param index the tile index
    /// @param camera the camera
    static gfx_rv insert(renderlist_t *self, const TileIndex& index, const std::shared_ptr<Camera>& camera);
    /// @brief Get mesh this render list is attached to.
    /// @return the mesh or @a nullptr
    /// @post If the render list is attached to a mesh, that mesh is returned.
    ///       Otherwise a null pointer is returned.
    static ego_mesh_t *getMesh(const renderlist_t *self);
    /// @brief Set mesh this render list is attached to.
    /// @param mesh the mesh or @a nullptr
    /// @post If @a mesh is not a null pointer, then this render list is attached to that mesh.
    ///       Otherwise it is detached.
    static gfx_rv setMesh(renderlist_t *self, ego_mesh_t *mesh);
    /// @brief Insert tiles into this render list.
    /// @param leaves a list of tile BSP leaves
    /// @param camera the camera
    /// @remark A tile
    static gfx_rv add(renderlist_t *self, const Ego::DynamicArray<BSP_leaf_t *> *leaves, const std::shared_ptr<Camera>& camera);
};



//--------------------------------------------------------------------------------------------

/**
 * @brief
 *    List of character and particle entities to be draw by a renderer.
 *
 *    Entities in a do list are sorted based on their position from the camera before drawing.
 */
struct dolist_t
{
    /**
     * @brief
     *    The (fixed) capacity of a do list.
     */
    static const size_t CAPACITY = OBJECTS_MAX + PARTICLES_MAX;
    /**
     * @brief
     *    An eleemnt of a do list.
     */
    struct element_t
    {
        CHR_REF ichr;
        PRT_REF iprt;
        float dist;
        
        element_t() :
            ichr(INVALID_CHR_REF), iprt(INVALID_PRT_REF), dist(0.0f)
        {}
#if XX == 1
        element_t(const element_t& other) :
            ichr(other.ichr), iprt(other.iprt), dist(other.dist)
        {}
        element_t& operator=(const element_t& other)
        {
            ichr = other.ichr;
            iprt = other.iprt;
            dist = other.dist;
            return *this;
        }
        virtual ~element_t()
        {}
#endif

        static element_t *init(element_t *self);
        static int cmp(const void *left, const void *right);
    };

    size_t index;            /**< A "name" for the dolist. */
    size_t size;             /**< The size of the do list:
                                  How many character and particle entities are in the do list. */
    element_t lst[CAPACITY]; /**< List of which objects to draw. */
    dolist_t();
#if XX == 1
    virtual ~dolist_t()
    {}
#endif
    static dolist_t *init(dolist_t *self, const size_t index = 0);
    static gfx_rv reset(dolist_t *self, const size_t index);
    static gfx_rv sort(dolist_t *self, std::shared_ptr<Camera> camera, const bool reflect);
    static gfx_rv test_chr(dolist_t *self, const Object *pchr);
    static gfx_rv add_chr_raw(dolist_t *self, Object *pchr);
    static gfx_rv test_prt(dolist_t *self, const prt_t *pprt);
    static gfx_rv add_prt_raw(dolist_t *self, prt_t *pprt);
    /// @brief Insert character or particle entities into this dolist.
    /// @param leaves
    static gfx_rv add_colst(dolist_t *self, const Ego::DynamicArray<BSP_leaf_t *> *collisions);
};

//--------------------------------------------------------------------------------------------

template <typename Type, size_t Capacity>
struct list_ary_t
{
    size_t free_count;
    int free_lst[Capacity];
    Type lst[Capacity];
    static size_t getCapacity()
    {
        return Capacity;
    }
    int get_free_idx()
    {
        // If no free lists are available ...
        if (free_count <= 0)
        {
            // ... return -1.
            return -1;
        }

        // Reduce the number of free lists by 1.
        free_count--;

        // Get the index into the array of lists.
        size_t index = free_count;

        // Mark the list as used.
        free_lst[index] = -1;
        
        // Return the index.
        return index;
    }

    gfx_rv free_one(size_t index)
    {
        // If the index is not valid ...
        if (index >= getCapacity())
        {
            // ... return an error.
            return gfx_error;
        }

        // If the index is already free ...
        for (size_t cnt = 0; cnt < free_count; ++cnt)
        {
            if (index == free_lst[cnt])
            {
                // ... return a failure.
                return gfx_fail;
            }
        }

        // If the free list is not full ...
        if (free_count < getCapacity())
        {
            // ... add the index to the free list and ...
            free_lst[free_count] = index;
            free_count++;
            // ... return success.
            return gfx_success;
        }
        // Otherwise: Return failure.
        return gfx_fail;
    }

    Type *get_ptr(size_t index)
    {
        // If the index is not valid ...
        if (index >= getCapacity())
        {
            // ... return nullptr.
            return nullptr;
        }
        // Otherwise: Return a pointer to the list.
        return &(lst[index]);
    }

    list_ary_t() :
        free_lst()
    {
        for (size_t cnt = 0; cnt < Capacity; ++cnt)
        {
            free_lst[cnt] = cnt;
            Type::init(&(lst[cnt]), cnt);
        }
        free_count = Capacity;
    }

    virtual ~list_ary_t()
    {
        free_count = 0;
        for (size_t cnt = 0; cnt < Capacity; ++cnt)
        {
            free_lst[cnt] = -1;
            Type::init(&(lst[cnt]), cnt);
        }
    }
};

struct dolist_ary_t : public list_ary_t<dolist_t,MAX_CAMERAS>
{
    dolist_ary_t() :
        list_ary_t()
    {}
    virtual ~dolist_ary_t()
    {}
};

struct renderlist_ary_t : public list_ary_t<renderlist_t, MAX_CAMERAS>
{
    renderlist_ary_t() :
        list_ary_t()
    {}
    virtual ~renderlist_ary_t()
    {}
};

struct dolist_mgr_t
{
private:
    static dolist_mgr_t *_singleton;
private:
    dolist_ary_t ary;
    dolist_mgr_t();
    virtual ~dolist_mgr_t();
public:
    static void initialize();
    static void uninitialize();
    static dolist_mgr_t& get();
public:
    int get_free_idx();
    gfx_rv free_one(size_t index);
    dolist_t *get_ptr(size_t index);
};

struct renderlist_mgr_t
{
private:
    static renderlist_mgr_t *_singleton;
private:
    renderlist_ary_t ary;
    renderlist_mgr_t();
    virtual ~renderlist_mgr_t();
public:
    static void initialize();
    static void uninitialize();
    static renderlist_mgr_t& get();
public:
    int get_free_idx();
    gfx_rv free_one(size_t index);
    renderlist_t *get_ptr(size_t index);
};

//--------------------------------------------------------------------------------------------
// encapsulation of all graphics options
struct gfx_config_t
{
    GLuint shading;
    bool refon;
    bool antialiasing;
    bool dither;
    bool perspective;
    bool phongon;
    bool shaon;
    bool shasprite;

    bool clearson;          ///< Do we clear every time?
    bool draw_background;   ///< Do we draw the background image?
    bool draw_overlay;      ///< Draw overlay?
    bool draw_water_0;      ///< Do we draw water layer 1 (TX_WATER_LOW)
    bool draw_water_1;      ///< Do we draw water layer 2 (TX_WATER_TOP)

    size_t dynalist_max;     ///< Max number of dynamic lights to draw
    bool exploremode;       ///< fog of war mode for mesh display
    bool usefaredge;        ///< Far edge maps? (Outdoor)

    // virtual window parameters
    float vw, vh;
    float vdw, vdh;
};

bool gfx_config_init(gfx_config_t * pgfx);
bool gfx_config_download_from_egoboo_config(gfx_config_t * pgfx, egoboo_config_t * pcfg);

bool oglx_texture_parameters_download_gfx(oglx_texture_parameters_t *ptex, egoboo_config_t *pcfg);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern float time_render_scene_init;
extern float time_render_scene_mesh;
extern float time_render_scene_solid;
extern float time_render_scene_water;
extern float time_render_scene_trans;

extern float time_render_scene_init_renderlist_make;
extern float time_render_scene_init_dolist_make;
extern float time_render_scene_init_do_grid_dynalight;
extern float time_render_scene_init_light_fans;
extern float time_render_scene_init_update_all_chr_instance;
extern float time_render_scene_init_update_all_prt_instance;

extern float time_render_scene_mesh_dolist_sort;
extern float time_render_scene_mesh_ndr;
extern float time_render_scene_mesh_drf_back;
extern float time_render_scene_mesh_ref;
extern float time_render_scene_mesh_ref_chr;
extern float time_render_scene_mesh_drf_solid;
extern float time_render_scene_mesh_render_shadows;

extern Uint32          game_frame_all;             ///< The total number of frames drawn so far
extern Uint32          menu_frame_all;             ///< The total number of frames drawn so far

extern gfx_config_t gfx;

extern Uint8           mapon;
extern Uint8           mapvalid;
extern Uint8           youarehereon;

extern size_t          blip_count;
extern float           blip_x[MAXBLIP];
extern float           blip_y[MAXBLIP];
extern Uint8           blip_c[MAXBLIP];

extern int GFX_WIDTH;
extern int GFX_HEIGHT;

//extern Uint8           lightdirectionlookup[65536];                        ///< For lighting characters
//extern float           lighttable_local[MAXLIGHTROTATION][EGO_NORMAL_COUNT];
//extern float           lighttable_global[MAXLIGHTROTATION][EGO_NORMAL_COUNT];
extern float           indextoenvirox[EGO_NORMAL_COUNT];                    ///< Environment map
extern float           lighttoenviroy[256];                                ///< Environment map
//extern Uint32          lighttospek[MAXSPEKLEVEL][256];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Function prototypes

struct GFX
{
#if 0
    static GFX _singleton;
#endif
#if 0
    /**
     * @brief
     *  Get the GFX singleton.
     * @return
     *  the GFX singleton.
     * @warning
     *  The behavior of this method is undefined if the GFX system is not initialized.
     */
    static GFX& get();
#endif
    /**
     * @brief
     *  Initialize the GFX system.
     * @remark
     *  This method has no effect if the GFX system is initialized.
     * @todo
     *  Rename to @a initialize.
     */
    static void initialize();
    /**
     * @brief
     *  Uninitialize the GFX system.
     * @remark
     *  A call to this method has no effect if the GFX system is not initialized.
     * @todo
     *  Rename to @a uninitialize.
     */
    static void uninitialize();
protected:
    /**
     * @brief
     *  Initialize the OpenGL graphics system.
     */
    static int initializeOpenGL();
    /**
     * @brief
     *  Uninitialize the OpenGL graphics system.
     */
    static void uninitializeOpenGL();
    /**
     * @brief
     *  Initialize the SDL graphics system.
     */
    static void initializeSDLGraphics();

    /**
     * @brief
     *  Uninitialize the SDL graphics system.
     */
    static void uninitializeSDLGraphics();
};

void gfx_system_main();
/// SDL destroys the OpenGL context at various occassions (e.g. when changing the video mode).
/// In particular this happens when the video mode is changed from and to fullscreen. In that
/// case, OpenGL texture become invalid and need to be reloaded. Reloading means here: From
/// the stored state of the texture and its surface, the backing OpenGL texture needs to be
/// reconstructed.
void gfx_system_reload_all_textures();
void gfx_system_make_enviro();
void gfx_system_init_all_graphics();
void gfx_system_release_all_graphics();
void gfx_system_delete_all_graphics();
void gfx_system_load_assets();
void gfx_system_load_basic_textures();

renderlist_mgr_t *gfx_system_get_renderlist_mgr();
dolist_mgr_t *gfx_system_get_dolist_mgr();

// the render engine callback
void gfx_system_render_world(const std::shared_ptr<Camera> cam, const int render_list_index, const int dolist_index);

void gfx_request_clear_screen();
void gfx_do_clear_screen();
bool gfx_flip_pages_requested();
void gfx_request_flip_pages();
void gfx_do_flip_pages();

float draw_icon_texture(oglx_texture_t *ptex, float x, float y, Uint8 sparkle_color, Uint32 sparkle_timer, float size, bool useAlpha = false);
float draw_menu_icon(const TX_REF icontype, float x, float y, Uint8 sparkle, Uint32 delta_update, float size);
float draw_game_icon(const TX_REF icontype, float x, float y, Uint8 sparkle, Uint32 delta_update, float size);
void  draw_map_texture(float x, float y);
float draw_one_bar(Uint8 bartype, float x, float y, int ticks, int maxticks);
float draw_status(const CHR_REF character, float x, float y);
void  draw_one_character_icon(const CHR_REF item, float x, float y, bool draw_ammo, Uint8 sparkle_override);
void  draw_blip(float sizeFactor, Uint8 color, float x, float y, bool mini_map);

//void   make_lightdirectionlookup();

bool grid_lighting_interpolate(const ego_mesh_t *pmesh, lighting_cache_t * dst, const fvec2_t& pos);
float grid_lighting_test(ego_mesh_t *pmesh, GLXvector3f pos, float * low_diff, float * hgh_diff);

void release_all_profile_textures();

gfx_rv gfx_load_blips();
gfx_rv gfx_load_bars();
gfx_rv gfx_load_map();
gfx_rv gfx_load_icons();

float  get_ambient_level();

void   draw_mouse_cursor();

gfx_rv chr_instance_flash(chr_instance_t *inst, Uint8 value);

//void gfx_calc_rotmesh();

#define TEXTUREATLASMANAGER_VERSION 1

struct TextureAtlasManager
{
protected:
#if TEXTUREATLASMANAGER_VERSION > 3
    static TextureAtlasManager *_singleton;
#else
    /// has this system been initialized?
    static bool initialized;
#endif

    // the "small" textures
    static oglx_texture_t sml[MESH_IMG_COUNT];
    static int sml_cnt;

    // the "large" textures
    static oglx_texture_t big[MESH_IMG_COUNT];
    static int big_cnt;

#if TEXTUREATLASMANAGER_VERSION > 3
    TextureAtlasManager();
    virtual ~TextureAtlasManager();
#endif

    // decimate one tiled texture of a mesh
    static int decimate_one_mesh_texture(oglx_texture_t *src_tx, oglx_texture_t *tx_lst, size_t tx_lst_cnt, int minification);

public:
    static oglx_texture_t *get_sml(int which);
    static oglx_texture_t *get_big(int which);

public:

    /**
     * @brief
     *  Initialize the texture atlas manager singleton.
     * @post
     *  The texture atlas manager is initialized if no exception was raised by this call,
     *  otherwise it is not initialized.
     * @remark
     *  If the texture atlas manager is not initialized, a call to this method is a no-op.
     */
    static void initialize();
    /**
     * @brief
     *  Uninitialize the texture atlas manager singleton.
     * @post
     *  The texture atlas manager is uninitialized.
     * @remark
     *  If the texture atlas manager is not initialized, a call to this method is a no-op.
     */
    static void uninitialize();
    static void reinitialize();
    static void reload_all();

    // decimate all tiled textures of a mesh
    static void decimate_all_mesh_textures();

};
