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
#include "game/Graphics/CameraSystem.hpp"
#include "game/egoboo.h"
#include "game/Graphics/TileList.hpp"
#include "game/Graphics/EntityList.hpp"
#include "game/Graphics/Vertex.hpp"
#include "game/Graphics/RenderPasses.hpp"
#include "egolib/Graphics/MD2Model.hpp"



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the default icon size in pixels
#define ICON_SIZE 32


/// max number of blips on the minimap
#define MAXBLIP        128                          ///<Max blips on the screen

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#include "game/Graphics/TileList.hpp"

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
#define TABX                            32// 16      ///< Size of little name tag on the bar
#define BARX                            112// 216         ///< Size of bar
#define BARY                            16// 8
#define NUMTICK                         10// 50          ///< Number of ticks per row
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
enum HUDColors : uint8_t
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

//--------------------------------------------------------------------------------------------

using namespace std;

/**
 * @brief
 *  A pool is a small, fixed set of objects. The set of objects is created with the pool and destroy with the pool.
 *  That is, a pool is suited for storng a small fixed set of objects, which are expensive to create and destroy.
 *  <br/>
 *  Users can acquire elements from the pool.
 *  If a user is in possession of an element,
 *      the element can not be acquired by another user
 *          until the user in posession of the element relinquishes the element to the pool.
 * @param _Type
 *  the type of the elements in this pool.
 *  Must be default-constructible.
 * @param _Capacity
 *  the capacity of the pool.
 *  Must be greater than @a 0.
 * @author
 *  Michael Heilmann
 */
template <typename _Type, size_t _Capacity>
struct Pool
{
    /**
     * @brief
     *  The type of the pool elements.
     */
    typedef _Type Type;
    /**
     * @brief
     *  The capacity of the pool.
     */
    static const size_t Capacity;

private:

#if defined(_DEBUG)
    bool isFree(size_t index) const
    {
        for (size_t i = 0; i < free_count; ++i)
        {
            if (index == free_lst[i])
            {
                return true;
            }
        }
        return false;
    }
#endif
    /**
     * @brief
     *  A special value indicating in the free list that an index is used.
     * @invariant
     *  Always greater or equal to @a Capacity.
     */
    static const size_t InvalidIndex;
    size_t free_count;
    size_t free_lst[_Capacity];
    Type _elements[_Capacity];
protected:
    void free(size_t index)
    {
        EGOBOO_ASSERT(index < getCapacity()); // Index is not valid.
        EGOBOO_ASSERT(!isFree(index)); // Index already free.
        EGOBOO_ASSERT(free_count < getCapacity()); // Free list is full.

        // Add the the index to the free list and increment the number of free indices.
        free_lst[free_count] = index;
        free_count++;
    }

public:

    /**
     * @brief
     *  Get the pool size.
     * @return
     *  the pool size
     */
    size_t getSize() const
    {
        return getCapacity() - free_count;
    }

    /**
     * @brief
     *  Get the pool capacity.
     * @return
     *  the pool capacity
     */
    size_t getCapacity() const
    {
        return Capacity;
    }

    /**
     * @brief
     *  Acquire a pool element.
     * @return
     *  the shared pointer to the pool element on success, a shared null pointer on failure
     */
    shared_ptr<Type> acquire()
    {
        // If no free elements are available ...
        if (free_count <= 0)
        {
            // ... return the null pointer.
            return nullptr;
        }

        // Reduce the number of free elements by 1.
        free_count--;

        // Get the index into the array of lists.
        size_t index = free_count;

        // Mark the element as used.
        free_lst[index] = InvalidIndex;

        // Create and return a shared pointer with a custom deallocator for the element.
        return shared_ptr<Type>(&(_elements[index]), [index, this](Type *dummy) { free(index); });
    }

    /**
     * @brief
     *  Construct this pool.
     */
    Pool() :
        free_lst()
    {
        for (size_t i = 0; i < Capacity; ++i)
        {
            free_lst[i] = i;
            _elements[i].init();
        }
        free_count = Capacity;
    }

    /**
     * @brief
     *  Destruct this pool.
     */
    virtual ~Pool()
    {
        //for (size_t i = 0; i < Capacity; ++i)
        //{
        //    free_lst[i] = i;
        //    _elements[i].init();
        //}
        //free_count = Capacity;
    }
};

template <typename _Type, size_t _Capacity>
const size_t Pool<_Type, _Capacity>::Capacity = _Capacity;

template <typename _Type, size_t _Capacity>
const size_t Pool<_Type, _Capacity>::InvalidIndex = std::numeric_limits<size_t>::max();

/// @todo Use Ego::Core::System/Ego::Core::Singleton
struct dolist_mgr_t : public Pool<Ego::Graphics::EntityList, MAX_CAMERAS>, public Id::NonCopyable
{
private:
    static dolist_mgr_t *_singleton;
private:
    dolist_mgr_t();
    virtual ~dolist_mgr_t();
public:
    static void initialize();
    static void uninitialize();
    static dolist_mgr_t& get();
};

/// @todo Use Ego::Core::System/Ego::Core::Singleton
struct renderlist_mgr_t : public Pool<Ego::Graphics::TileList, MAX_CAMERAS>
{
private:
    static renderlist_mgr_t *_singleton;
private:
    renderlist_mgr_t();
    virtual ~renderlist_mgr_t();
public:
    static void initialize();
    static void uninitialize();
    static renderlist_mgr_t& get();
};

//--------------------------------------------------------------------------------------------
// encapsulation of all graphics options
struct gfx_config_t
{
    bool gouraudShading_enable;
    bool refon;
    bool antialiasing;
    bool dither;
    bool perspective;
    bool phongon;
    bool shadows_enable;
    bool shadows_highQuality_enable;

    bool clearson;          ///< Do we clear every time?
    bool draw_background;   ///< Do we draw the background image?
    bool draw_overlay;      ///< Draw overlay?
    bool draw_water_0;      ///< Do we draw water layer 1 (TX_WATER_LOW)
    bool draw_water_1;      ///< Do we draw water layer 2 (TX_WATER_TOP)

    size_t dynalist_max;     ///< Max number of dynamic lights to draw
    bool exploremode;       ///< fog of war mode for mesh display
    bool usefaredge;        ///< Far edge maps? (Outdoor)

    static void init(gfx_config_t& self);
    static void download(gfx_config_t& self, egoboo_config_t& cfg);
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern Uint32          game_frame_all;             ///< The total number of frames drawn so far
extern Uint32          menu_frame_all;             ///< The total number of frames drawn so far

extern gfx_config_t gfx;

extern int GFX_WIDTH;
extern int GFX_HEIGHT;

extern float           indextoenvirox[EGO_NORMAL_COUNT];                    ///< Environment map
extern float           lighttoenviroy[256];                                ///< Environment map

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
void gfx_system_load_assets();

renderlist_mgr_t *gfx_system_get_renderlist_mgr();
dolist_mgr_t *gfx_system_get_dolist_mgr();

// the render engine callback
void gfx_system_render_world(const std::shared_ptr<Camera> camera, std::shared_ptr<Ego::Graphics::TileList> tl, std::shared_ptr<Ego::Graphics::EntityList> el);

void gfx_request_clear_screen();
void gfx_do_clear_screen();
bool gfx_flip_pages_requested();
void gfx_request_flip_pages();
void gfx_do_flip_pages();

float draw_icon_texture(const oglx_texture_t *ptex, float x, float y, Uint8 sparkle_color, Uint32 sparkle_timer, float size, bool useAlpha = false);
float draw_game_icon(const oglx_texture_t* icontype, float x, float y, Uint8 sparkle, Uint32 delta_update, float size);
void draw_blip(float sizeFactor, Uint8 color, float x, float y);
void draw_mouse_cursor();

/// The active dynamic lights
struct dynalist_t
{
	int frame; ///< The last frame in shich the list was updated. @a -1 if there was no update yet.
	size_t size; ///< The size of the list.
	dynalight_data_t lst[TOTAL_MAX_DYNA];  ///< The list.
	static void init(dynalist_t& self);
};




#define DYNALIST_INIT { -1 /* frame */, 0 /* count */, {} }

/// Illuminate the "grid".
struct GridIllumination {
private:
	static void test_one_corner(const ego_mesh_t& mesh, GLXvector3f pos, float& pdelta);
	static bool test_corners(const ego_mesh_t& mesh, ego_tile_info_t& tile, float threshold);
	static void light_one_corner(ego_mesh_t& mesh, ego_tile_info_t& tile, const bool reflective, const Vector3f& pos, const Vector3f& nrm, float& plight);
	static float grid_lighting_test(const ego_mesh_t& mesh, GLXvector3f pos, float& low_diff, float& hgh_diff);
	static void light_fans_update_clst(Ego::Graphics::TileList& tl);
	static gfx_rv light_fans_throttle_update(ego_mesh_t * mesh, ego_tile_info_t& tile, const Index1D& tileIndex, float threshold);
	static void light_fans_update_lcache(Ego::Graphics::TileList& tl);
public:
	static gfx_rv do_grid_lighting(Ego::Graphics::TileList& tl, dynalist_t& dyl, Camera& cam);
	static void light_fans(Ego::Graphics::TileList& tl);
	static float light_corners(ego_mesh_t& mesh, ego_tile_info_t& tile, bool reflective, float mesh_lighting_keep);
	static bool grid_lighting_interpolate(const ego_mesh_t& mesh, lighting_cache_t& dst, const Vector2f& pos);
	static bool light_corner(ego_mesh_t& mesh, const Index1D& fan, float height, float nrm[], float& plight);
};


float  get_ambient_level();



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
    static oglx_texture_t *sml[MESH_IMG_COUNT];
    static int sml_cnt;

    // the "large" textures
    static oglx_texture_t *big[MESH_IMG_COUNT];
    static int big_cnt;

#if TEXTUREATLASMANAGER_VERSION > 3
    TextureAtlasManager();
    virtual ~TextureAtlasManager();
#endif

    // decimate one tiled texture of a mesh
    static int decimate_one_mesh_texture(const oglx_texture_t *src_tx, oglx_texture_t *(&tx_lst)[MESH_IMG_COUNT], size_t tx_lst_cnt, int minification);

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

//--------------------------------------------------------------------------------------------
// The following functions/variables manipulate/represent a per-mesh rendering cache;
// per-mesh and global variables - 'nough said.

// variables to optimize calls to bind the textures
extern bool mesh_tx_none;           ///< use blank textures?
extern TX_REF mesh_tx_image;          ///< Last texture used
extern uint8_t mesh_tx_size;           ///< what size texture?
void mesh_texture_invalidate();
oglx_texture_t *ego_mesh_get_texture(Uint8 image, Uint8 size);
oglx_texture_t *mesh_texture_bind(const ego_tile_info_t * ptile);
