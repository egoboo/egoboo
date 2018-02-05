//********************************************************************************************
//*
//*    This file is part of Cartman.
//*
//*    Cartman is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Cartman is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Cartman.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "egolib/egolib.h"

#include "cartman/cartman.h"

#include "cartman/cartman_map.h"
#include "cartman/cartman_functions.h"
#include "cartman/cartman_input.h"
#include "cartman/cartman_gui.h"
#include "cartman/cartman_gfx.h"
#include "cartman/cartman_select.h"
#include "egolib/FileFormats/Globals.hpp"
#include "cartman/cartman_math.h"

//--------------------------------------------------------------------------------------------

struct light_t
{
    int           x;
    int           y;
    uint8_t level;
    int           radius;
};

//--------------------------------------------------------------------------------------------


static void cart_mouse_data_toggle_fx( int fxmask );

// helper functions
static void cart_mouse_data_mesh_set_tile( uint16_t tiletoset );
static void cart_mouse_data_flatten_mesh();
static void cart_mouse_data_clear_mesh();
static void cart_mouse_data_three_e_mesh();
static void cart_mouse_data_mesh_replace_tile( bool tx_only, bool at_floor_level );
static void cart_mouse_data_mesh_set_fx();
static void cart_mouse_data_rect_select();
static void cart_mouse_data_rect_unselect();
static void cart_mouse_data_mesh_replace_fx();

//--------------------------------------------------------------------------------------------

std::string egoboo_path = "";

int      onscreen_count = 0;
uint32_t onscreen_vert[MAXPOINTS];

//--------------------------------------------------------------------------------------------

static float cartman_zoom_hrz = 1.0f;
static float cartman_zoom_vrt = 1.0f;

static STRING  loadname;        // Text

static int     brushsize = 3;      // Size of raise/lower terrain brush
static int     brushamount = 50;   // Amount of raise/lower

static float   debugx = -1;        // Blargh
static float   debugy = -1;        //

static bool addinglight = false;

static int numlight;
static light_t light_lst[MAXLIGHT];

static int ambi = 22;
static int ambicut = 1;
static int direct = 16;

static bool _ttf_atexit_registered = false;

//--------------------------------------------------------------------------------------------

// ui functions
static void draw_cursor_in_window(Cartman::Gui::Window& pwin);
static void unbound_mouse();
static void bound_mouse();
static bool cartman_check_keys( const char *modname, cartman_mpd_t * pmesh );
static bool cartman_check_mouse( const char *modname, cartman_mpd_t * pmesh );
static void   cartman_check_input( const char *modname, cartman_mpd_t * pmesh );

// loading
static bool load_module( const std::string& modname, cartman_mpd_t * pmesh );
static void gfx_system_load_basic_textures( const std::string& modname );
static void cartman_create_mesh( cartman_mpd_t * pmesh );

// saving
static void cartman_save_mesh( const char *modname, cartman_mpd_t * pmesh );

// gfx functions
static void load_all_windows( cartman_mpd_t& mesh );

#include "cartman/Views/FxView.hpp"
#include "cartman/Views/SideView.hpp"
#include "cartman/Views/TileView.hpp"
#include "cartman/Views/VertexView.hpp"

struct Views
{
    static Cartman::TileView tileView;
    static Cartman::FxView fxView;
    static Cartman::VertexView vertexView;
    static Cartman::SideView sideView;
};

Cartman::TileView Views::tileView;
Cartman::FxView Views::fxView;
Cartman::VertexView Views::vertexView;
Cartman::SideView Views::sideView;

static void draw_lotsa_stuff( cartman_mpd_t * pmesh );

static void draw_main( cartman_mpd_t * pmesh );

// camera stuff
static void move_camera( cartman_mpd_info_t * pinfo );
static void bound_camera( cartman_mpd_info_t * pinfo );

// misc
static void mesh_calc_vrta( cartman_mpd_t * pmesh );
static void fan_calc_vrta( cartman_mpd_t * pmesh, int fan );
static int  vertex_calc_vrta( cartman_mpd_t * pmesh, uint32_t vert );
static void make_onscreen();
static void onscreen_add_fan( cartman_mpd_t * pmesh, uint32_t fan );
static void ease_up_mesh( cartman_mpd_t& mesh, float zoom_vrt );

// cartman versions of these functions
static int cartman_get_vertex(cartman_mpd_t *pmesh, Index2D index2d, int num);



static void cartman_check_mouse_side(std::shared_ptr<Cartman::Gui::Window> pwin, float zoom_hrz, float zoom_vrt);
static void cartman_check_mouse_tile(std::shared_ptr<Cartman::Gui::Window> pwin, float zoom_hrz, float zoom_vrt);
static void cartman_check_mouse_fx(std::shared_ptr<Cartman::Gui::Window> pwin, float zoom_hrz, float zoom_vrt);
static void cartman_check_mouse_vertex(std::shared_ptr<Cartman::Gui::Window> pwin, float zoom_hrz, float zoom_vrt);

// shutdown function
static void main_end();

//--------------------------------------------------------------------------------------------

#include "cartman/Clocks.h"

int          dunframe = 0;
int          secframe = 0;
uint8_t      lastsecframe = 0;
volatile int minsecframe = 10000;
int          worldclock = 0;
int          seed;
uint16_t     entry;
char         charread;
FILE        *fileread;
char         nameread[80];
char         fpstext[256][8];

//--------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------
void sectim(void) {
    if (secframe < minsecframe) {
        minsecframe = secframe;
    }
    lastsecframe = secframe;
    secframe = 0;
}

//--------------------------------------------------------------------------------------------
void reset_minfps(void) {
    minsecframe = 0;
    secframe = -9999;
    while (minsecframe > -9999) {
    }
    minsecframe = 10000;
    lastsecframe = 0;

    return;
}

//--------------------------------------------------------------------------------------------
void comment(void) {
    fscanf(fileread, "%c", &charread);
    while ('\n' != charread) {
        fscanf(fileread, "%c", &charread);
    }

    return;
}

//--------------------------------------------------------------------------------------------
void show_info(void) {
    printf("%s - %s\n", NAME, VERSION_STR);

    if (Clocks::timePassed<Time::Unit::Ticks, int>() != 0) {
        printf("Seconds = %15f\n", Clocks::timePassed<Time::Unit::Seconds, double>());
        printf("Avg FPS = %15f\n", dunframe * 1000.0 / Clocks::timePassed<Time::Unit::Milliseconds, double>());
        if (minsecframe != 10000) {
            printf("Min FPS = %8d.000000\n", minsecframe);
        }
    }
    printf("\n\n");

    return;
}

//--------------------------------------------------------------------------------------------
void fill_fpstext(void) {
    int cnt;

    for (cnt = 0; cnt < 256; cnt++) {
        fpstext[cnt][0] = 'F';
        fpstext[cnt][1] = 'P';
        fpstext[cnt][2] = 'S';
        fpstext[cnt][3] = ' ';
        if (cnt >= 100) {
            fpstext[cnt][4] = '0' + (cnt / 100);
        } else {
            fpstext[cnt][4] = ' ';
        }
        if (cnt >= 10) {
            fpstext[cnt][5] = '0' + ((cnt / 10) % 10);
        } else {
            fpstext[cnt][5] = ' ';
        }
        fpstext[cnt][6] = '0' + (cnt % 10);
        fpstext[cnt][7] = 0;
    }

    return;
}

//--------------------------------------------------------------------------------------------
void scan_pair(int *one, int *two) {
    char temp;

    fscanf(fileread, "%d", one);
    fscanf(fileread, "%c", &temp);
    if (temp == '-') {
        fscanf(fileread, "%d", two);
        *two = (*two) - (*one) + 1;
        *two = abs(*two);
    } else {
        *two = 1;
    }

    return;
}

//--------------------------------------------------------------------------------------------
void limit(int low, int *value, int high) {
    if (*value < low) {
        *value = low;
        return;
    }
    if (*value > high) {
        *value = high;
    }

    return;
}


//--------------------------------------------------------------------------------------------

struct LightModel {
    static void add_light(int x, int y, int radius, int level) {
        if (numlight >= MAXLIGHT)  numlight = MAXLIGHT - 1;

        light_lst[numlight].x = x;
        light_lst[numlight].y = y;
        light_lst[numlight].radius = radius;
        light_lst[numlight].level = level;
        numlight++;
    }

    static void alter_light(int x, int y) {
        numlight--;
        if (numlight < 0)  numlight = 0;

        int level = std::abs(light_lst[numlight].y - y);

        int radius = std::abs(light_lst[numlight].x - x);
        if (radius > MAXRADIUS / cartman_zoom_hrz)  radius = MAXRADIUS / cartman_zoom_hrz;
        if (radius < MINRADIUS / cartman_zoom_hrz)  radius = MINRADIUS / cartman_zoom_hrz;

        light_lst[numlight].radius = radius;
        if (level > MAP_MAXLEVEL) level = MAP_MAXLEVEL;
        if (level < MAP_MINLEVEL) level = MAP_MINLEVEL;
        light_lst[numlight].level = level;

        numlight++;
    }

    static void draw_light(int number, std::shared_ptr<Cartman::Gui::Window> pwin, float zoom_hrz) {
        int xdraw = (light_lst[number].x / FOURNUM * zoom_hrz) - cam.x + (pwin->size.x() / 2) - SMALLXY;
        int ydraw = (light_lst[number].y / FOURNUM * zoom_hrz) - cam.y + (pwin->size.y() / 2) - SMALLXY;
        int radius = std::abs(light_lst[number].radius) / FOURNUM * zoom_hrz;
        uint8_t color = light_lst[number].level;

        //color = make_rgb(pwin->bmp, color, color, color);
        //circle(pwin->bmp, xdraw, ydraw, radius, color);
    }
};

//--------------------------------------------------------------------------------------------

void draw_cursor_in_window(Cartman::Gui::Window& pwin)
{
    using namespace Cartman;

    if ( !pwin.on ) return;

    if ( -1 != mdata.win_id && pwin.id != mdata.win_id )
    {
        int size = POINT_SIZE( 10 );

        int x = pwin.position.x() + ( Input::get()._mouse.position.x() - Gui::Manager::get().windowList[mdata.win_id]->position.x() );
        int y = pwin.position.y() + ( Input::get()._mouse.position.y() - Gui::Manager::get().windowList[mdata.win_id]->position.y() );

        ogl_draw_sprite_2d(Resources::get().tx_pointon, x - size / 2, y - size / 2, size, size );
    }

}

//--------------------------------------------------------------------------------------------

int cartman_get_vertex( cartman_mpd_t * pmesh, Index2D index2d, int num )
{
    if ( NULL == pmesh ) pmesh = &mesh;

    int vert = pmesh->get_ivrt_xy(index2d, num);

    if ( vert == -1 )
    {
        return vert;
        printf( "BAD GET_VERTEX NUMBER(2nd), %d at %d, %d...\n", num, index2d.x(), index2d.y() );
        exit( -1 );
    }

    return vert;
}

//--------------------------------------------------------------------------------------------

void onscreen_add_fan( cartman_mpd_t * pmesh, uint32_t fan )
{
    // ZZ> This function flags a fan's points as being "onscreen"
    int cnt;
    uint32_t vert, fan_type, vert_count;
    cartman_mpd_tile_t * pfan;

    if ( NULL == pmesh ) pmesh = &mesh;

    pfan = CART_MPD_FAN_PTR( pmesh, fan );
    if ( NULL == pfan ) return;

    fan_type    = pfan->type;
    vert_count  = tile_dict.def_lst[fan_type].numvertices;

    for ( cnt = 0, vert = pfan->vrtstart;
          cnt < vert_count && CHAINEND != vert;
          cnt++, vert = pmesh->vrt2[vert].next )
    {
        if ( !CART_VALID_VERTEX_RANGE( vert ) ) break;

        if ( VERTEXUNUSED == pmesh->vrt2[vert].a ) continue;

        if ( onscreen_count < MAXPOINTS )
        {
            onscreen_vert[onscreen_count] = vert;
            onscreen_count++;
        }
        else
        {
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------

void make_onscreen( cartman_mpd_t * pmesh )
{
    if ( NULL == pmesh ) pmesh = &mesh;

    int mapxstt = std::floor(( cam.x - cam.w  * 0.5f ) / Info<float>::Grid::Size()) - 1.0f;
    int mapystt = std::floor(( cam.y - cam.h  * 0.5f ) / Info<float>::Grid::Size()) - 1.0f;

    int mapxend = std::ceil(( cam.x + cam.w  * 0.5f ) / Info<float>::Grid::Size()) + 1.0f;
    int mapyend = std::ceil(( cam.y + cam.h  * 0.5f ) / Info<float>::Grid::Size()) + 1.0f;

    onscreen_count = 0;
    for (int mapy = mapystt; mapy <= mapyend; mapy++ )
    {
        if ( mapy < 0 || mapy >= pmesh->info.getTileCountY() ) continue;

        for (int mapx = mapxstt; mapx <= mapxend; mapx++ )
        {
            if ( mapx < 0 || mapx >= pmesh->info.getTileCountX() ) continue;

            int fan = pmesh->get_ifan({mapx, mapy});
            if ( !VALID_MPD_TILE_RANGE( fan ) ) continue;

            onscreen_add_fan( pmesh, fan );
        }
    }
}

//--------------------------------------------------------------------------------------------

void gfx_system_load_basic_textures( const std::string& modname )
{
    // ZZ> This function loads the standard textures for a module

    /*
    // Uncomment to display the search paths.
    vfs_listSearchPaths();
    */

    SDL_Surface *bmptemp;       // A temporary bitmap

    std::string prefix = modname + C_SLASH_STR + "gamedat" + C_SLASH_STR;

    bmptemp = cartman_LoadIMG(prefix + "tile0.bmp");
    get_tiles( bmptemp );
    SDL_FreeSurface( bmptemp );

    bmptemp = cartman_LoadIMG(prefix + "tile1.bmp");
    get_tiles( bmptemp );
    SDL_FreeSurface( bmptemp );

    bmptemp = cartman_LoadIMG(prefix + "tile2.bmp");
    get_tiles( bmptemp );
    SDL_FreeSurface( bmptemp );

    bmptemp = cartman_LoadIMG(prefix + "tile3.bmp");
    get_tiles( bmptemp );
    SDL_FreeSurface( bmptemp );
}

//--------------------------------------------------------------------------------------------

bool load_module( const std::string& modname, cartman_mpd_t * pmesh )
{
    wawalite_data_t *pdata;

    if (!pmesh) pmesh = &mesh;

    std::string mod_path = std::string("/modules/") + modname;

    if (!setup_init_module_vfs_paths(modname))
    {
        return false;
    }

    //  show_name(mod_path);
    gfx_system_load_basic_textures( mod_path );

    // read the mpd file from the module directory
    if ( NULL == cartman_mpd_load_vfs( /*mod_path,*/ pmesh ) )
    {
        cartman_create_mesh( pmesh );
    }

    // read the wawalite file from the module directory
    pdata = wawalite_data_read("mp_data/wawalite.txt", &wawalite_data);

    // make sure the values are in range
    pdata = wawalite_limit( pdata );

    numlight = 0;
    addinglight = 0;

    return true;
}

//--------------------------------------------------------------------------------------------

void Cartman::Gui::Window::render()
{
    if ( !on ) return;

    glPushAttrib( GL_SCISSOR_BIT );
    {
        auto& renderer = Ego::Renderer::get();
		renderer.setScissorTestEnabled(true);
        auto drawableSize = Ego::GraphicsSystem::get().window->getDrawableSize();
        renderer.setScissorRectangle(position.x(), drawableSize.y() - ( position.y() + size.y() ),
                                     size.x(), size.y());

        make_onscreen( pmesh );

        if ( HAS_BITS( mode, WINMODE_TILE ) )
        {
            Views::tileView.render( *this, cartman_zoom_hrz, cartman_zoom_vrt );
        }

        if ( HAS_BITS( mode, WINMODE_VERTEX ) )
        {
            Views::vertexView.render( *this, cartman_zoom_hrz, cartman_zoom_vrt );
        }

        if ( HAS_BITS( mode, WINMODE_SIDE ) )
        {
            Views::sideView.render( *this, cartman_zoom_hrz, cartman_zoom_vrt );
        }

        if ( HAS_BITS( mode, WINMODE_FX ) )
        {
            Views::fxView.render( *this, cartman_zoom_hrz, cartman_zoom_vrt );
        }

        draw_cursor_in_window( *this );

    }
    glPopAttrib();
}

//--------------------------------------------------------------------------------------------

void load_all_windows( cartman_mpd_t& mesh )
{
    using namespace Cartman;
    static const auto windowSize = Vector2f(Gui::Window::defaultWidth, Gui::Window::defaultHeight);
    static const auto borderSize = Vector2f(7, 9);
    auto& windowList = Gui::Manager::get().windowList;
    windowList[0]->load_window(0, "editor/window.png", Point2f(180, 16),  borderSize, windowSize, WINMODE_VERTEX, &mesh );
    windowList[1]->load_window(1, "editor/window.png", Point2f(410, 16),  borderSize, windowSize, WINMODE_TILE,   &mesh );
    windowList[2]->load_window(2, "editor/window.png", Point2f(180, 248), borderSize, windowSize, WINMODE_SIDE,   &mesh );
    windowList[3]->load_window(3, "editor/window.png", Point2f(410, 248), borderSize, windowSize, WINMODE_FX,     &mesh );
}

//--------------------------------------------------------------------------------------------

void bound_camera( cartman_mpd_info_t * pinfo )
{
	cam.x = Ego::Math::constrain(cam.x, 0.0f, pinfo->getEdgeX());
	cam.y = Ego::Math::constrain(cam.y, 0.0f, pinfo->getEdgeY());
	cam.z = Ego::Math::constrain(cam.z, -pinfo->getEdgeZ(), +pinfo->getEdgeZ());
}

//--------------------------------------------------------------------------------------------

void unbound_mouse()
{
    using namespace Cartman;
    if (!Input::get()._mouse.drag)
    {
        Input::get()._mouse.tlx = 0;
        Input::get()._mouse.tly = 0;
        Input::get()._mouse.brx = Ego::GraphicsSystem::get().window->getSize().x() - 1;
        Input::get()._mouse.bry = Ego::GraphicsSystem::get().window->getSize().y() - 1;
    }
}

//--------------------------------------------------------------------------------------------

void bound_mouse()
{
    using namespace Cartman;
    if (mdata.win_id != -1)
    {
        auto window = Gui::Manager::get().windowList[mdata.win_id];
        Input::get()._mouse.tlx = window->position.x() + window->border.size.x();
        Input::get()._mouse.tly = window->position.y() + window->border.size.y();
        Input::get()._mouse.brx = Input::get()._mouse.tlx + window->size.x() - 1;
        Input::get()._mouse.bry = Input::get()._mouse.tly + window->size.y() - 1;
    }
}

//--------------------------------------------------------------------------------------------

int vertex_calc_vrta( cartman_mpd_t * pmesh, uint32_t vert )
{
    int newa, cnt;
    float x, y, z;
    float brx, bry, brz, deltaz;
    float newlevel, distance, disx, disy;

    if ( NULL == pmesh ) pmesh = &mesh;

    if ( CHAINEND == vert || !CART_VALID_VERTEX_RANGE( vert ) ) return ~0;

    if ( VERTEXUNUSED == pmesh->vrt2[vert].a ) return ~0;

    // To make life easier
    x = pmesh->vrt2[vert].x;
    y = pmesh->vrt2[vert].y;
    z = pmesh->vrt2[vert].z;

    // Directional light
    brx = x + 64;
    bry = y + 64;
    brz = pmesh->get_level( brx, y ) +
          pmesh->get_level( x, bry ) +
          pmesh->get_level( x + 46, y + 46 );
    if ( z < -128 ) z = -128;
    if ( brz < -128*3 ) brz = -128 * 3;
    deltaz = z + z + z - brz;
    newa = ( deltaz * direct / 256.0f );

    // Point lights !!!BAD!!!
    newlevel = 0;
    for ( cnt = 0; cnt < numlight; cnt++ )
    {
        disx = x - light_lst[cnt].x;
        disy = y - light_lst[cnt].y;
        distance = sqrt(( float )( disx * disx + disy * disy ) );
        if ( distance < light_lst[cnt].radius )
        {
            newlevel += (( light_lst[cnt].level * ( light_lst[cnt].radius - distance ) ) / light_lst[cnt].radius );
        }
    }
    newa += newlevel;

    // Bounds
    if ( newa < -ambicut ) newa = -ambicut;
    newa += ambi;
    pmesh->vrt2[vert].a = Ego::Math::constrain( newa, 1, 255 );

    // Edge fade
    //dist = dist_from_border( pmesh->vrt[vert].x, pmesh->vrt[vert].y );
    //if ( dist <= FADEBORDER )
    //{
    //    newa = newa * dist / FADEBORDER;
    //    if ( newa == VERTEXUNUSED )  newa = 1;
    //    pmesh->vrt[vert].a = newa;
    //}

    return newa;
}

//--------------------------------------------------------------------------------------------

void fan_calc_vrta( cartman_mpd_t * pmesh, int fan )
{
    int num, cnt;
    uint32_t vert;

    tile_definition_t * pdef;
    cartman_mpd_tile_t * pfan;

    if ( NULL == pmesh ) pmesh = &mesh;

    pfan = CART_MPD_FAN_PTR( pmesh, fan );
    if ( NULL == pfan ) return;

    pdef = tile_dict.get( pfan->type );
    if ( NULL == pdef ) return;

    num = pdef->numvertices;
    if ( 0 == num ) return;

    for ( cnt = 0, vert = pfan->vrtstart;
          cnt < num && CHAINEND != vert;
          cnt++, vert = pmesh->vrt2[vert].next )
    {
        vertex_calc_vrta( pmesh, vert );
    }
}

//--------------------------------------------------------------------------------------------

void mesh_calc_vrta(cartman_mpd_t *self)
{
    if (NULL == self) self = &mesh;

    for (auto it = self->info.begin(); it != self->info.end(); ++it) {
            int fan = self->get_ifan(*it);

            fan_calc_vrta(self, fan);
    }
}

//--------------------------------------------------------------------------------------------

void move_camera( cartman_mpd_info_t * pinfo )
{
    using namespace Cartman;
    if ((-1 != mdata.win_id) && (CART_BUTTONDOWN(SDL_BUTTON_MIDDLE) || CART_KEYDOWN(SDLK_m)))
    {
        auto delta = Input::get()._mouse.position - Input::get()._mouse.positionOld;
        cam.x += delta.x();
        cam.y += delta.y();

        Input::get()._mouse.position = Input::get()._mouse.positionOld;

        bound_camera( pinfo );
    }
}

//--------------------------------------------------------------------------------------------

void cartman_check_mouse_side(std::shared_ptr<Cartman::Gui::Window> pwin, float zoom_hrz, float zoom_vrt)
{
    using namespace Cartman;
    int    mpix_x, mpix_z;
    float  mpos_x, mpos_z;
    float  mpos_y0, mpos_y1;
    bool inside;

    if ( NULL == pwin || !pwin->on || !HAS_BITS( pwin->mode, WINMODE_SIDE ) ) return;

    if ( NULL == pwin->pmesh ) pwin->pmesh = &mesh;

    mpix_x = Input::get()._mouse.position.x() - (pwin->position.x() + pwin->size.x() / 2);
    mpix_z = Input::get()._mouse.position.y() - (pwin->position.y() + pwin->size.y() / 2);

    inside = ( mpix_x >= -( pwin->size.x() / 2 ) ) && ( mpix_x <= ( pwin->size.x() / 2 ) ) &&
             ( mpix_z >= -( pwin->size.y() / 2 ) ) && ( mpix_z <= ( pwin->size.y() / 2 ) );

    mpos_x = SCREEN_TO_REAL( mpix_x, cam.x, zoom_hrz );
    mpos_z = SCREEN_TO_REAL( mpix_z, cam.z, zoom_vrt );

    mpos_y0 = SCREEN_TO_REAL( - pwin->size.x() / 2, cam.y, zoom_hrz );
    mpos_y1 = SCREEN_TO_REAL( + pwin->size.x() / 2, cam.y, zoom_hrz );

    if ( pwin->id == mdata.rect_drag && !inside )
    {
        // scroll the window
        int dmpix_x = 0, dmpix_z = 0;

        if ( mpix_x < - pwin->size.x() / 2 )
        {
            dmpix_x = mpix_x + pwin->size.x() / 2;
        }
        else if ( mpix_x > pwin->size.x() / 2 )
        {
            dmpix_x = mpix_x - pwin->size.x() / 2;
        }

        /// @todo Shouldn't this be <tt>pwin->size.y()</tt>?
        if ( mpix_z < - pwin->size.x() / 2 )
        {
            dmpix_z = mpix_z + pwin->size.y() / 2;
        }
        else if ( mpix_z > pwin->size.y() / 2 )
        {
            dmpix_z = mpix_z - pwin->size.y() / 2;
        }

        if ( 0 != dmpix_x && 0 != dmpix_z )
        {
            cam.x += dmpix_x * FOURNUM / zoom_hrz;
            cam.z += dmpix_z * FOURNUM / zoom_vrt;

            bound_camera( &( pwin->pmesh->info ) );
        }
    }
    else if ( inside )
    {
        mdata.win_id     = pwin->id;
        mdata.win_mode   = pwin->mode;
        mdata.win_mesh   = pwin->pmesh;
        mdata.win_mpos_x = mpos_x;
        mdata.win_mpos_y = mpos_z;
        mdata.win_fan_x  = std::floor( mpix_x / Info<float>::Grid::Size());
        mdata.win_fan_y  = -1;

        mdata.win_select.set_mesh( pwin->pmesh );

        debugx = mpos_x;
        debugy = mpos_z;

        if (CART_BUTTONDOWN(SDL_BUTTON_LEFT))
        {
            if ( -1 == mdata.rect_drag )
            {
                mdata.rect_draw = true;
                mdata.rect_drag = pwin->id;
                mdata.rect_done = -1;

                mdata.rect_x0 = mdata.rect_x1 = mpos_x;
                mdata.rect_y0 = mpos_y0;
                mdata.rect_y1 = mpos_y1;
                mdata.rect_z0 = mdata.rect_z1 = mpos_z;

            }
            else if ( pwin->id == mdata.rect_drag )
            {
                mdata.rect_x1 = mpos_x;
                mdata.rect_y0 = mpos_y0;
                mdata.rect_y1 = mpos_y1;
                mdata.rect_z1 = mpos_z;
            }
        }
        else
        {
            if ( pwin->id == mdata.rect_drag )
            {
                mdata.rect_drag = -1;
                mdata.rect_done = pwin->id;
            }
        }

        if ( pwin->id == mdata.rect_done )
        {
            if ( mdata.win_select.count() > 0 && !CART_KEYMOD( KMOD_ALT ) && !CART_KEYDOWN( SDLK_MODE ) &&
                 !CART_KEYMOD( KMOD_LCTRL ) && !CART_KEYMOD( KMOD_RCTRL ) )
            {
                mdata.win_select.clear();
            }

            if ( CART_KEYMOD( KMOD_ALT ) || CART_KEYDOWN( SDLK_MODE ) )
            {
                cart_mouse_data_rect_unselect();
            }
            else
            {
                cart_mouse_data_rect_select();
            }

            mdata.rect_draw = false;
            mdata.rect_drag = -1;
            mdata.rect_done = -1;
        }

        if (CART_BUTTONDOWN(SDL_BUTTON_RIGHT))
        {
            mesh_select_move( mdata.win_select, Input::get()._mouse.cx / zoom_hrz, 0, -Input::get()._mouse.cy / zoom_vrt );
            bound_mouse();
        }

        if ( CART_KEYDOWN( SDLK_y ) )
        {
            mesh_select_move( mdata.win_select, 0, 0, -Input::get()._mouse.cy / zoom_vrt );
            bound_mouse();
        }

        if ( CART_KEYDOWN( SDLK_u ) )
        {
            if ( mdata.type >= tile_dict.offset )
            {
                MeshEditor::move_mesh_z(mdata.win_mesh != nullptr ? *mdata.win_mesh : mesh, -Input::get()._mouse.cy / zoom_vrt, mdata.tx, 0xC0);
            }
            else
            {
                MeshEditor::move_mesh_z(mdata.win_mesh != nullptr ? *mdata.win_mesh : mesh, -Input::get()._mouse.cy / zoom_vrt, mdata.tx, 0xF0);
            }
            bound_mouse();
        }

        if ( CART_KEYDOWN( SDLK_n ) )
        {
            if ( CART_KEYDOWN( SDLK_RSHIFT ) )
            {
                // Move the first 16 up and down
                MeshEditor::move_mesh_z(mdata.win_mesh != nullptr ? *mdata.win_mesh : mesh, -Input::get()._mouse.cy / zoom_vrt, 0, 0xF0);
            }
            else
            {
                // Move the entire mesh up and down
                MeshEditor::move_mesh_z(mdata.win_mesh != nullptr ? *mdata.win_mesh : mesh, -Input::get()._mouse.cy / zoom_vrt, 0, 0);
            }
            bound_mouse();
        }
    }
}

//--------------------------------------------------------------------------------------------

void cartman_check_mouse_tile(std::shared_ptr<Cartman::Gui::Window> pwin, float zoom_hrz, float zoom_vrt)
{
    using namespace Cartman;
    int fan_tmp;
    int mpix_x, mpix_y;
    float mpos_x, mpos_y;
    bool inside;

    if ( NULL == pwin || !pwin->on || !HAS_BITS( pwin->mode, WINMODE_TILE ) ) return;

    if ( NULL == pwin->pmesh ) pwin->pmesh = &mesh;

    mpix_x = Input::get()._mouse.position.x() - (pwin->position.x() + pwin->border.size.x() + pwin->size.x() / 2);
    mpix_y = Input::get()._mouse.position.y() - (pwin->position.y() + pwin->border.size.y() + pwin->size.y() / 2);

    inside = ( mpix_x >= -( pwin->size.x() / 2 ) ) && ( mpix_x <= ( pwin->size.x() / 2 ) ) &&
             ( mpix_y >= -( pwin->size.y() / 2 ) ) && ( mpix_y <= ( pwin->size.y() / 2 ) );

    mpos_x = SCREEN_TO_REAL( mpix_x, cam.x, zoom_hrz );
    mpos_y = SCREEN_TO_REAL( mpix_y, cam.y, zoom_hrz );

    if ( pwin->id == mdata.rect_drag && !inside )
    {
        // scroll the window
        int dmpix_x = 0, dmpix_y = 0;

        if ( mpix_x < - pwin->size.x() / 2 )
        {
            dmpix_x = mpix_x + pwin->size.x() / 2;
        }
        else if ( mpix_x > pwin->size.x() / 2 )
        {
            dmpix_x = mpix_x - pwin->size.x() / 2;
        }

        /// @todo Shouldn't this be <tt>pwin->size.y()</tt>?
        if ( mpix_y < - pwin->size.x() / 2 )
        {
            dmpix_y = mpix_y + pwin->size.y() / 2;
        }
        else if ( mpix_y > pwin->size.y() / 2 )
        {
            dmpix_y = mpix_y - pwin->size.y() / 2;
        }

        if ( 0 != dmpix_x && 0 != dmpix_y )
        {
            cam.x += dmpix_x  * FOURNUM / zoom_hrz;
            cam.y += dmpix_y  * FOURNUM / zoom_hrz;

            bound_camera( &( pwin->pmesh->info ) );
        }
    }
    else if ( inside )
    {
        mdata.win_id    = pwin->id;
        mdata.win_mode  = pwin->mode;
        mdata.win_mesh     = pwin->pmesh;
        mdata.win_mpos_x      = mpos_x;
        mdata.win_mpos_y      = mpos_y;
        mdata.win_fan_x = std::floor( mpos_x / Info<float>::Grid::Size());
        mdata.win_fan_y = std::floor( mpos_y / Info<float>::Grid::Size());

        mdata.win_select.set_mesh( pwin->pmesh );

        debugx = mpos_x;
        debugy = mpos_y;

        // update mdata.win_fan only if the tile is valid
        fan_tmp = pwin->pmesh->get_ifan({mdata.win_fan_x, mdata.win_fan_y});
        if ( VALID_MPD_TILE_RANGE( fan_tmp ) ) mdata.win_fan = fan_tmp;

        if (CART_BUTTONDOWN(SDL_BUTTON_LEFT))
        {
            cart_mouse_data_mesh_replace_tile( CART_KEYDOWN( SDLK_t ), CART_KEYDOWN( SDLK_v ) );
        }

        if (CART_BUTTONDOWN(SDL_BUTTON_RIGHT))
        {
            // force an update of mdata.win_fan
            mdata.win_fan = fan_tmp;

            if ( VALID_MPD_TILE_RANGE( mdata.win_fan ) )
            {
                mdata.type  = pwin->pmesh->fan2[mdata.win_fan].type;
                mdata.tx    = TILE_GET_LOWER_BITS( pwin->pmesh->fan2[mdata.win_fan].tx_bits );
                mdata.upper = TILE_GET_UPPER_BITS( pwin->pmesh->fan2[mdata.win_fan].tx_bits );
            }
            else
            {
                mdata.type  = 0;
                mdata.tx    = TILE_GET_LOWER_BITS( MAP_FANOFF );
                mdata.upper = TILE_GET_UPPER_BITS( MAP_FANOFF );
            }
        }

        if ( !CART_KEYDOWN( SDLK_k ) )
        {
            addinglight = false;
        }
        if ( CART_KEYDOWN( SDLK_k ) && !addinglight )
        {
            LightModel::add_light( mdata.win_mpos_x, mdata.win_mpos_y, MINRADIUS / zoom_hrz, MAP_MAXLEVEL / zoom_hrz );
            addinglight = true;
        }
        if ( addinglight )
        {
            LightModel::alter_light( mdata.win_mpos_x, mdata.win_mpos_y );
        }
    }
}

//--------------------------------------------------------------------------------------------

void cartman_check_mouse_fx(std::shared_ptr<Cartman::Gui::Window> pwin, float zoom_hrz, float zoom_vrt)
{
    using namespace Cartman;
    int mpix_x, mpix_y;
    float mpos_x, mpos_y;
    bool inside;

    if ( NULL == pwin || !pwin->on || !HAS_BITS( pwin->mode, WINMODE_FX ) ) return;

    if ( NULL == pwin->pmesh ) pwin->pmesh = &mesh;

    mpix_x = Input::get()._mouse.position.x() - (pwin->position.x() + pwin->border.size.x() + pwin->size.x() / 2);
    mpix_y = Input::get()._mouse.position.y() - (pwin->position.y() + pwin->border.size.y() + pwin->size.y() / 2);

    inside = ( mpix_x >= -( pwin->size.x() / 2 ) ) && ( mpix_x <= ( pwin->size.x() / 2 ) ) &&
             ( mpix_y >= -( pwin->size.y() / 2 ) ) && ( mpix_y <= ( pwin->size.y() / 2 ) );

    mpos_x = SCREEN_TO_REAL( mpix_x, cam.x, zoom_hrz );
    mpos_y = SCREEN_TO_REAL( mpix_y, cam.y, zoom_hrz );

    if ( pwin->id == mdata.rect_drag && !inside )
    {
        // scroll the window
        int dmpix_x = 0, dmpix_y = 0;

        if ( mpix_x < - pwin->size.x() / 2 )
        {
            dmpix_x = mpix_x + pwin->size.x() / 2;
        }
        else if ( mpix_x > pwin->size.x() / 2 )
        {
            dmpix_x = mpix_x - pwin->size.x() / 2;
        }

        /// @todo Shouldn't this be <tt>pwin->size.y()</tt>?
        if ( mpix_y < - pwin->size.x() / 2 )
        {
            dmpix_y = mpix_y + pwin->size.y() / 2;
        }
        else if ( mpix_y > pwin->size.y() / 2 )
        {
            dmpix_y = mpix_y - pwin->size.y() / 2;
        }

        if ( 0 != dmpix_x && 0 != dmpix_y )
        {
            cam.x += dmpix_x * FOURNUM / zoom_hrz;
            cam.y += dmpix_y * FOURNUM / zoom_hrz;

            bound_camera( &( pwin->pmesh->info ) );
        }
    }
    else if ( inside )
    {
        int fan_tmp;

        mdata.win_id     = pwin->id;
        mdata.win_mode   = pwin->mode;
        mdata.win_mesh   = pwin->pmesh;
        mdata.win_mpos_x = mpos_x;
        mdata.win_mpos_y = mpos_y;
        mdata.win_fan_x  = std::floor( mpos_x / Info<float>::Grid::Size());
        mdata.win_fan_y  = std::floor( mpos_y / Info<float>::Grid::Size());

        mdata.win_select.set_mesh( pwin->pmesh );

        debugx = mpos_x;
        debugy = mpos_y;

        fan_tmp = pwin->pmesh->get_ifan({mdata.win_fan_x, mdata.win_fan_y});
        if ( VALID_MPD_TILE_RANGE( fan_tmp ) ) mdata.win_fan = fan_tmp;

        if (CART_BUTTONDOWN(SDL_BUTTON_LEFT))
        {
            if ( !CART_KEYDOWN( SDLK_LSHIFT ) )
            {
                cart_mouse_data_mesh_set_fx();
            }
            else
            {
                cart_mouse_data_mesh_replace_fx();
            }
        }

        if (CART_BUTTONDOWN(SDL_BUTTON_RIGHT))
        {
            mdata.win_fan = fan_tmp;

            if ( VALID_MPD_TILE_RANGE( mdata.win_fan ) )
            {
                mdata.fx = pwin->pmesh->fan2[mdata.win_fan].fx;
            }
            else
            {
                mdata.fx = MAPFX_WALL | MAPFX_IMPASS;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------

void cartman_check_mouse_vertex(std::shared_ptr<Cartman::Gui::Window> pwin, float zoom_hrz, float zoom_vrt)
{
    using namespace Cartman;
    int mpix_x, mpix_y;
    float mpos_x, mpos_y;
    float mpos_z0, mpos_z1;
    bool inside;

    if ( NULL == pwin || !pwin->on || !HAS_BITS( pwin->mode, WINMODE_VERTEX ) ) return;

    if ( NULL == pwin->pmesh ) pwin->pmesh = &mesh;

    mpix_x = Input::get()._mouse.position.x() - (pwin->position.x() + pwin->size.x() / 2);
    mpix_y = Input::get()._mouse.position.y() - (pwin->position.y() + pwin->size.y() / 2);

    inside = ( mpix_x >= -( pwin->size.x() / 2 ) ) && ( mpix_x <= ( pwin->size.x() / 2 ) ) &&
             ( mpix_y >= -( pwin->size.y() / 2 ) ) && ( mpix_y <= ( pwin->size.y() / 2 ) );

    mpos_x = SCREEN_TO_REAL( mpix_x, cam.x, zoom_hrz );
    mpos_y = SCREEN_TO_REAL( mpix_y, cam.y, zoom_hrz );

    mpos_z0 = SCREEN_TO_REAL( - pwin->size.y() / 2, cam.z, zoom_vrt );
    mpos_z1 = SCREEN_TO_REAL( + pwin->size.y() / 2, cam.z, zoom_vrt );

    if ( pwin->id == mdata.rect_drag && !inside )
    {
        // scroll the window
        int dmpix_x = 0, dmpix_y = 0;

        if ( mpix_x < - pwin->size.x() / 2 )
        {
            dmpix_x = mpix_x + pwin->size.x() / 2;
        }
        else if ( mpix_x > pwin->size.x() / 2 )
        {
            dmpix_x = mpix_x - pwin->size.x() / 2;
        }

        /// @todo Shouldn't this be <tt>pwin->size.y()</tt>?
        if ( mpix_y < - pwin->size.x() / 2 )
        {
            dmpix_y = mpix_y + pwin->size.y() / 2;
        }
        else if ( mpix_y > pwin->size.y() / 2 )
        {
            dmpix_y = mpix_y - pwin->size.y() / 2;
        }

        if ( 0 != dmpix_x && 0 != dmpix_y )
        {
            cam.x += dmpix_x * FOURNUM / zoom_hrz;
            cam.y += dmpix_y * FOURNUM / zoom_hrz;

            bound_camera( &( pwin->pmesh->info ) );
        }
    }
    else if ( inside )
    {
        mdata.win_id     = pwin->id;
        mdata.win_mode   = pwin->mode;
        mdata.win_mesh   = pwin->pmesh;
        mdata.win_mpos_x = mpos_x;
        mdata.win_mpos_y = mpos_y;
        mdata.win_fan_x  = std::floor( mpos_x / Info<float>::Grid::Size());
        mdata.win_fan_y  = std::floor( mpos_y / Info<float>::Grid::Size());

        mdata.win_select.set_mesh( pwin->pmesh );

        debugx = mpos_x;
        debugy = mpos_y;

        if (CART_BUTTONDOWN(SDL_BUTTON_LEFT))
        {
            if ( -1 == mdata.rect_drag )
            {
                mdata.rect_draw = true;
                mdata.rect_drag = pwin->id;
                mdata.rect_done = -1;

                mdata.rect_x0 = mdata.rect_x1 = mpos_x;
                mdata.rect_y0 = mdata.rect_y1 = mpos_y;
                mdata.rect_z0 = mpos_z0;
                mdata.rect_z1 = mpos_z1;
            }
            else if ( pwin->id == mdata.rect_drag )
            {
                mdata.rect_x1 = mpos_x;
                mdata.rect_y1 = mpos_y;
                mdata.rect_z0 = mpos_z0;
                mdata.rect_z1 = mpos_z1;
            }
        }
        else
        {
            if ( pwin->id == mdata.rect_drag )
            {
                mdata.rect_drag = -1;
                mdata.rect_done = pwin->id;
            }
        }

        if ( pwin->id == mdata.rect_done )
        {
            if ( mdata.win_select.count() > 0 && !CART_KEYMOD( KMOD_ALT ) && !CART_KEYDOWN( SDLK_MODE ) &&
                 !CART_KEYMOD( KMOD_LCTRL ) && !CART_KEYMOD( KMOD_RCTRL ) )
            {
                mdata.win_select.clear();
            }
            if ( CART_KEYMOD( KMOD_ALT ) || CART_KEYDOWN( SDLK_MODE ) )
            {
                cart_mouse_data_rect_unselect();
            }
            else
            {
                cart_mouse_data_rect_select();
            }

            mdata.rect_draw = false;
            mdata.rect_drag = -1;
            mdata.rect_done = -1;
        }

        if (CART_BUTTONDOWN(SDL_BUTTON_RIGHT))
        {
            mesh_select_move( mdata.win_select, Input::get()._mouse.cx / zoom_vrt, Input::get()._mouse.cy / zoom_vrt, 0 );
            bound_mouse();
        }

        if ( CART_KEYDOWN( SDLK_f ) )
        {
            //    weld_corner_verts({mdata.win_mpos_x>>7, mdata.win_mpos_y>>7});
            fix_vertices(pwin->pmesh != nullptr ? *pwin->pmesh : mesh, Index2D(std::floor(mdata.win_mpos_x / Info<float>::Grid::Size()), std::floor(mdata.win_mpos_y / Info<float>::Grid::Size())));
        }

        if (CART_KEYDOWN(SDLK_p) || (CART_BUTTONDOWN(SDL_BUTTON_RIGHT) && 0 == mdata.win_select.count()))
        {
            MeshEditor::raise_mesh( mdata.win_mesh != nullptr ? *mdata.win_mesh : mesh, onscreen_vert, onscreen_count, mdata.win_mpos_x, mdata.win_mpos_y, brushamount, brushsize );
        }
    }
}

//--------------------------------------------------------------------------------------------

bool cartman_check_mouse( const char * modulename, cartman_mpd_t * pmesh )
{
    using namespace Cartman;

    if (!Input::get()._mouse.on) return false;

    if ( NULL == pmesh ) pmesh = &mesh;

    unbound_mouse();
    move_camera( &( pmesh->info ) );

    // place this after move_camera()
    Input::get()._mouse.update();

    // handle all window-specific commands
    //if( mos.drag && NULL != mos.drag_window )
    //{
    //    // we are dragging something in a specific window
    //    cartman_check_mouse_tile( mos.drag_window, cartman_zoom_hrz, 1.0f / 16.0f );
    //    cartman_check_mouse_vertex( mos.drag_window, cartman_zoom_hrz, 1.0f / 16.0f );
    //    cartman_check_mouse_side( mos.drag_window, cartman_zoom_hrz, 1.0f / 16.0f );
    //    cartman_check_mouse_fx( mos.drag_window, cartman_zoom_hrz, 1.0f / 16.0f );
    //}
    //else
    {
        mdata.win_id = -1;

        for (auto& window : Cartman::Gui::Manager::get().windowList)
        {
            cartman_check_mouse_tile(window, cartman_zoom_hrz, cartman_zoom_vrt);
            cartman_check_mouse_vertex(window, cartman_zoom_hrz, cartman_zoom_vrt);
            cartman_check_mouse_side(window, cartman_zoom_hrz, cartman_zoom_vrt);
            cartman_check_mouse_fx(window, cartman_zoom_hrz, cartman_zoom_vrt);
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------

void ease_up_mesh( cartman_mpd_t& mesh, float zoom_vrt )
{
    // ZZ> This function lifts the entire mesh
    using namespace Cartman;
    Input::get()._mouse.position = Input::get()._mouse.positionOld;
    MeshEditor::mesh_move( mesh, 0, 0, -Input::get()._mouse.cy / zoom_vrt );
}

//--------------------------------------------------------------------------------------------

bool cartman_check_keys( const char * modname, cartman_mpd_t * pmesh )
{
    using namespace Cartman;
    if (!check_keys(20)) return false;

    if ( NULL == pmesh ) pmesh = &mesh;

    // Hurt
    if ( CART_KEYDOWN( SDLK_h ) )
    {
        cart_mouse_data_toggle_fx( MAPFX_DAMAGE );
        Input::get()._keyboard.delay = KEYDELAY;
    }
    // Impassable
    if ( CART_KEYDOWN( SDLK_i ) )
    {
        cart_mouse_data_toggle_fx( MAPFX_IMPASS );
        Input::get()._keyboard.delay = KEYDELAY;
    }
    // Barrier
    if ( CART_KEYDOWN( SDLK_b ) )
    {
        cart_mouse_data_toggle_fx( MAPFX_WALL );
        Input::get()._keyboard.delay = KEYDELAY;
    }
    // Overlay
    if ( CART_KEYDOWN( SDLK_o ) )
    {
        cart_mouse_data_toggle_fx( MAPFX_WATER );
        Input::get()._keyboard.delay = KEYDELAY;
    }
    // Reflective
    if ( CART_KEYDOWN( SDLK_r ) )
    {
        cart_mouse_data_toggle_fx( MAPFX_SHA );
        Input::get()._keyboard.delay = KEYDELAY;
    }
    // Reflective
    if ( CART_KEYDOWN( SDLK_d ) )
    {
        cart_mouse_data_toggle_fx( MAPFX_REFLECTIVE );
        Input::get()._keyboard.delay = KEYDELAY;
    }
    // Animated
    if ( CART_KEYDOWN( SDLK_a ) )
    {
        cart_mouse_data_toggle_fx( MAPFX_ANIM );
        Input::get()._keyboard.delay = KEYDELAY;
    }
    // Slippy
    if ( CART_KEYDOWN( SDLK_s ) )
    {
        cart_mouse_data_toggle_fx( MAPFX_SLIPPY );
        Input::get()._keyboard.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_g ) )
    {
        fix_mesh( pmesh != nullptr ? *pmesh : mesh);
        Input::get()._keyboard.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_z ) )
    {
        if ( VALID_MPD_TILE_RANGE( mdata.win_fan ) )
        {
            uint16_t tx_bits = pmesh->fan2[mdata.win_fan].tx_bits;
            cart_mouse_data_mesh_set_tile( tx_bits );
        }
        Input::get()._keyboard.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_x ) )
    {
        if ( VALID_MPD_TILE_RANGE( mdata.win_fan ) )
        {
            uint8_t  type    = pmesh->fan2[mdata.win_fan].type;
            uint16_t tx_bits = pmesh->fan2[mdata.win_fan].tx_bits;

            if ( type >= tile_dict.offset )
            {
                trim_mesh_tile( pmesh != nullptr ? *pmesh : mesh, tx_bits, 0xC0 );
            }
            else
            {
                trim_mesh_tile( pmesh != nullptr ? *pmesh : mesh, tx_bits, 0xF0 );
            }
        }

        Input::get()._keyboard.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_e ) )
    {
        ease_up_mesh( pmesh != nullptr ? *pmesh : mesh, cartman_zoom_vrt );
    }

    if ( CART_KEYDOWN( SDLK_LEFTBRACKET ) || CART_KEYDOWN( SDLK_RIGHTBRACKET ) )
    {
        mesh_select_verts_connected( mdata.win_select );
    }
    if ( CART_KEYDOWN( SDLK_8 ) )
    {
        cart_mouse_data_three_e_mesh();
        Input::get()._keyboard.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_j ) )
    {
        if ( 0 == mdata.win_select.count() ) { MeshEditor::jitter_mesh( pmesh != nullptr ? *pmesh : mesh ); }
        else { mesh_select_jitter( mdata.win_select ); }
        Input::get()._keyboard.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_w ) )
    {
        //impass_edges(2);
        mesh_calc_vrta( pmesh );
        cartman_save_mesh( modname, pmesh );
        Input::get()._keyboard.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_SPACE ) )
    {
        mesh_select_weld( mdata.win_select );
        Input::get()._keyboard.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_INSERT ) )
    {
        mdata.type = ( mdata.type - 1 ) % tile_dict.def_count;
        while ( 0 == tile_dict_lines[mdata.type].count )
        {
            mdata.type = ( mdata.type - 1 ) % tile_dict.def_count;
        }
        Input::get()._keyboard.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_DELETE ) )
    {
        mdata.type = ( mdata.type + 1 ) % tile_dict.def_count;
        while ( 0 == tile_dict_lines[mdata.type].count )
        {
            mdata.type = ( mdata.type + 1 ) % tile_dict.def_count;
        }
        Input::get()._keyboard.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_KP_PLUS ) )
    {
        mdata.tx = ( mdata.tx + 1 ) & 0xFF;
        Input::get()._keyboard.delay = KEYDELAY;
    }
    if ( CART_KEYDOWN( SDLK_KP_MINUS ) )
    {
        mdata.tx = ( mdata.tx - 1 ) & 0xFF;
        Input::get()._keyboard.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_UP ) || CART_KEYDOWN( SDLK_LEFT ) || CART_KEYDOWN( SDLK_DOWN ) || CART_KEYDOWN( SDLK_RIGHT ) )
    {
        if ( CART_KEYDOWN( SDLK_RIGHT ) )
        {
            cam.x += 8 * CAMRATE;
        }
        if ( CART_KEYDOWN( SDLK_LEFT ) )
        {
            cam.x -= 8 * CAMRATE;
        }

        if ( WINMODE_SIDE == mdata.win_mode )
        {
            if ( CART_KEYDOWN( SDLK_DOWN ) )
            {
                cam.z += 8 * CAMRATE * ( pmesh->info.getEdgeZ() / DEFAULT_Z_SIZE );
            }
            if ( CART_KEYDOWN( SDLK_UP ) )
            {
                cam.z -= 8 * CAMRATE * ( pmesh->info.getEdgeZ() / DEFAULT_Z_SIZE );
            }
        }
        else
        {
            if ( CART_KEYDOWN( SDLK_DOWN ) )
            {
                cam.y += 8 * CAMRATE;
            }
            if ( CART_KEYDOWN( SDLK_UP ) )
            {
                cam.y -= 8 * CAMRATE;
            }
        }
        bound_camera( &( pmesh->info ) );
    }

    if ( CART_KEYDOWN( SDLK_PLUS ) || CART_KEYDOWN( SDLK_EQUALS ) )
    {
        cartman_zoom_hrz *= 2;
        if ( cartman_zoom_hrz > 4 )
        {
            cartman_zoom_hrz = 4;
        }
        else
        {
            Input::get()._keyboard.delay = KEYDELAY;
        }
    }

    if ( CART_KEYDOWN( SDLK_MINUS ) || CART_KEYDOWN( SDLK_UNDERSCORE ) )
    {
        cartman_zoom_hrz /= 2;
        if ( cartman_zoom_hrz < 0.25f )
        {
            cartman_zoom_hrz = 0.25f;
        }
        else
        {
            Input::get()._keyboard.delay = KEYDELAY;
        }
    }

    //------------------
    // from cartman_check_mouse_side() and cartman_check_mouse_tile() functions
    if ( CART_KEYDOWN( SDLK_f ) )
    {
        cart_mouse_data_flatten_mesh();
        Input::get()._keyboard.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_q ) )
    {
        MeshEditor::fix_walls( pmesh != nullptr ? *pmesh : mesh);
        Input::get()._keyboard.delay = KEYDELAY;
    }

    //------------------
    // "fixed" jeys

    if ( CART_KEYDOWN( SDLK_5 ) )
    {
        mesh_select_set_z_no_bound( mdata.win_select, -8000 * 4 );
        Input::get()._keyboard.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_6 ) )
    {
        mesh_select_set_z_no_bound( mdata.win_select, -127 * 4 );
        Input::get()._keyboard.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN( SDLK_7 ) )
    {
        mesh_select_set_z_no_bound( mdata.win_select, 127 * 4 );
        Input::get()._keyboard.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN_MOD( SDLK_c, KMOD_SHIFT ) )
    {
        cart_mouse_data_clear_mesh();
        Input::get()._keyboard.delay = KEYDELAY;
    }

    if ( CART_KEYDOWN_MOD( SDLK_l, KMOD_SHIFT ) )
    {
        MeshEditor::level_vrtz( pmesh != nullptr ? *pmesh : mesh);
    }

    // brush size
    if ( CART_KEYDOWN( SDLK_END ) || CART_KEYDOWN( SDLK_KP_1 ) )
    {
        brushsize = 0;
    }
    if ( CART_KEYDOWN( SDLK_PAGEDOWN ) || CART_KEYDOWN( SDLK_KP_3 ) )
    {
        brushsize = 1;
    }
    if ( CART_KEYDOWN( SDLK_HOME ) || CART_KEYDOWN( SDLK_KP_7 ) )
    {
        brushsize = 2;
    }
    if ( CART_KEYDOWN( SDLK_PAGEUP ) || CART_KEYDOWN( SDLK_KP_9 ) )
    {
        brushsize = 3;
    }

    // presser
    if ( CART_KEYDOWN( SDLK_1 ) )
    {
        mdata.presser = 0;
    }
    if ( CART_KEYDOWN( SDLK_2 ) )
    {
        mdata.presser = 1;
    }
    if ( CART_KEYDOWN( SDLK_3 ) )
    {
        mdata.presser = 2;
    }
    if ( CART_KEYDOWN( SDLK_4 ) )
    {
        mdata.presser = 3;
    }

    return true;
}

//--------------------------------------------------------------------------------------------

void draw_lotsa_stuff( cartman_mpd_t * pmesh )
{
    int x, cnt, todo, tile, add;

    if ( NULL == pmesh ) pmesh = &mesh;

#if defined(CARTMAN_DEBUG)
    // Tell which tile we're in
    {
        std::stringstream tempBuffer{};
        tempBuffer << "X = " << std::setprecision(2) << std::setw(6) << debugx;
        gfx_font_ptr->drawText(tempBuffer.str(), 0, 226);
    }
    {
        std::stringstream tempBuffer{};
        tempBuffer << "Y = " << std::setprecision(2) << std::setw(6) << debugy;
        gfx_font_ptr->drawText(tempBuffer.str(), 0, 234);
    }
#endif

    // Tell user what keys are important
    int y = Ego::GraphicsSystem::get().window->getSize().y() - 120, step = 8;
    gfx_font_ptr->drawText("O = Overlay (Water)", 0, y); y -= step;
    gfx_font_ptr->drawText("R = Reflective", 0, y); y -= step;
    gfx_font_ptr->drawText("D = Draw Reflection", 0, y); y -= step;
    gfx_font_ptr->drawText("A = Animated", 0, y); y -= step;
    gfx_font_ptr->drawText("B = Barrier (Slit)", 0, y); y -= step;
    gfx_font_ptr->drawText("I = Impassable (Wall)", 0, y); y -= step;
    gfx_font_ptr->drawText("H = Hurt", 0, y); y -= step;
    gfx_font_ptr->drawText("S = Slippy", 0, y); y -= step;

    // Vertices left
    gfx_font_ptr->drawText("Vertices " + std::to_string(pmesh->vrt_free), 0, y); y -= step;

    // Misc data
    gfx_font_ptr->drawText("Ambient   " + std::to_string(ambi), 0, y); y -= step;
    gfx_font_ptr->drawText("Ambicut   " + std::to_string(ambicut), 0, y); y -= step;
    gfx_font_ptr->drawText("Direct    " + std::to_string(direct), 0, y); y -= step;
    gfx_font_ptr->drawText("Brush amount " + std::to_string(brushamount), 0, y); y -= step;
    gfx_font_ptr->drawText("Brush size   " + std::to_string(brushsize), 0, y); y -= step;

    // Cursor
    //if (mos.x >= 0 && mos.x < Ego::GraphicsSystem::window->getDrawableSize().width() && mos.y >= 0 && mos.y < Ego::GraphicsSystem::window->getDrawableSize().height())
    //{
    //    draw_sprite(theSurface, bmpcursor, mos.x, mos.y);
    //}

    // Tile picks
    todo = 0;
    tile = 0;
    add  = 1;
    if ( mdata.tx < MAXTILE )
    {
        switch ( mdata.presser )
        {
            case 0:
                todo = 1;
                tile = mdata.tx;
                add = 1;
                break;
            case 1:
                todo = 2;
                tile = mdata.tx & 0xFE;
                add = 1;
                break;
            case 2:
                todo = 4;
                tile = mdata.tx & 0xFC;
                add = 1;
                break;
            case 3:
                todo = 4;
                tile = mdata.tx & 0xF8;
                add = 2;
                break;
        }

        x = 0;
        for ( cnt = 0; cnt < todo; cnt++ )
        {
            if ( mdata.type >= tile_dict.offset )
            {
                ogl_draw_sprite_2d(Resources::get().tx_bigtile[tile], x, 0, SMALLXY, SMALLXY );
            }
            else
            {
                ogl_draw_sprite_2d(Resources::get().tx_smalltile[tile], x, 0, SMALLXY, SMALLXY );
            }
            x += SMALLXY;
            tile += add;
        }
        
        {
            std::stringstream tempBuffer{};
            tempBuffer << "Tile 0x" << std::hex << std::setfill('0') << std::setw(2) << (uint16_t)mdata.upper <<
                            " 0x" << std::setw(2) << (uint16_t)mdata.tx;

            gfx_font_ptr->drawText(tempBuffer.str(), 0, 32);
        }

        gfx_font_ptr->drawText("Eats " + std::to_string(tile_dict.def_lst[mdata.type].numvertices) + " verts", 0, 40);

        if ( mdata.type >= tile_dict.offset )
        {
            gfx_font_ptr->drawText("63x63 Tile", 0, 56);
        }
        else
        {
            gfx_font_ptr->drawText("31x31 Tile", 0, 56);
        }
        draw_schematic( NULL, mdata.type, 0, 64 );
    }

    // FX selection
    draw_tile_fx( 0, 193, mdata.fx, 1.0f );

    if ( numattempt > 0 )
    {
        gfx_font_ptr->drawText("numwritten " + std::to_string(numwritten) + "/" + std::to_string(numattempt), 0, 0);
    }

#if defined(CARTMAN_DEBUG)
    gfx_font_ptr->drawText("<" + std::to_string(mos.x) + ", " std::to_string(mos.y) + ">", 0, 0);
#endif

}

//--------------------------------------------------------------------------------------------

void draw_main( cartman_mpd_t * pmesh )
{
    bool recalc_lighting = false;

    if ( NULL == pmesh ) pmesh = &mesh;

    Ego::Renderer::get().getColourBuffer().clear();

    ogl_beginFrame();
    {
        int itmp;

        Cartman::Gui::Manager::get().render();

        itmp = ambi;
        draw_slider( 0, 250, 19, 350, &ambi,          0, 200 );
        if ( itmp != ambi ) recalc_lighting = true;

        itmp = ambicut;
        draw_slider( 20, 250, 39, 350, &ambicut,       0, ambi );
        if ( itmp != ambicut ) recalc_lighting = true;

        itmp = direct;
        draw_slider( 40, 250, 59, 350, &direct,        0, 100 );
        if ( itmp != direct ) recalc_lighting = true;

        draw_slider( 60, 250, 79, 350, &brushamount, -50,  50 );

        draw_lotsa_stuff( pmesh );
    }
    ogl_endFrame();

    if ( recalc_lighting )
    {
        mesh_calc_vrta( pmesh );
    }

    Ego::Core::Console::get().draw();

    dunframe++;
    secframe++;

    SDL_GL_SwapWindow(Ego::GraphicsSystem::get().window->get());
}

//--------------------------------------------------------------------------------------------

void main_end( void )
{
    // Ending statistics
    show_info();

    // end the graphics system
    setup_clear_base_vfs_paths();
}

//--------------------------------------------------------------------------------------------

int SDL_main( int argcnt, char* argtext[] )
{
    char modulename[100];

    // register the function to be called to deinitialize the program
    atexit( main_end );

    // Initial text for the console.
    show_info();

    // grab the egoboo directory and the module name from the command line
    if ( argcnt < 2 || argcnt > 3 )
    {
        printf( "USAGE: CARTMAN [PATH] MODULE ( without .MOD )\n\n" );
		return EXIT_FAILURE;
    }
    else if ( argcnt < 3 )
    {
        egoboo_path = ".";
        sprintf( modulename, "%s.mod", argtext[1] );
    }
    else if ( argcnt < 4 )
    {
        size_t len = strlen( argtext[1] );
        char * pstr = argtext[1];
        if ( pstr[0] == '\"' )
        {
            pstr[len-1] = '\0';
            pstr++;
        }
        egoboo_path = pstr;
        sprintf( modulename, "%s.mod", argtext[2] );
    }

    Ego::Core::System::initialize(std::string(argtext[0]), egoboo_path);

    // Construct the input system.
    Cartman::Input::initialize();
    Cartman_MouseData::ctor(&mdata); /// @todo What is this crap?

    GFX::initialize();
    Resources::initialize();

    // Initialize the console.
	auto rectangle = Rectangle2f(id::zero<Point2f>(), { Ego::GraphicsSystem::get().window->getDrawableSize()(0),
		                                                Ego::GraphicsSystem::get().window->getDrawableSize()(1) * 0.25 });

    Ego::Core::Console::initialize(rectangle);

    // Load the module
    if (!load_module(modulename, &mesh))
    {
		Log::get() << Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "unable to load module ", "`", modulename, "`", Log::EndOfEntry);
		return EXIT_FAILURE;
    }

    fill_fpstext();                     // Make the FPS text
    Cartman::Gui::Manager::initialize();
    load_all_windows( mesh );          // Load windows
    load_img();                         // Load cartman icons

    dunframe   = 0;                     // Timer resets
    worldclock = 0;
    Clocks::initialize();
    for ( ;; )  // Main loop
    {
        if ( CART_KEYDOWN( SDLK_ESCAPE ) || CART_KEYDOWN( SDLK_F1 ) ) break;

        cartman_check_input( modulename, &mesh );

        draw_main( &mesh );

        SDL_Delay( 1 );

        Clocks::update();
    }
    Cartman::Gui::Manager::uninitialize();
    Ego::Core::Console::uninitialize();
    Resources::uninitialize();
    GFX::uninitialize();
    Cartman::Input::uninitialize();
    Ego::Core::System::uninitialize();
    return EXIT_SUCCESS;
}

//--------------------------------------------------------------------------------------------

void cartman_create_mesh( cartman_mpd_t * pmesh )
{
    cartman_mpd_create_info_t map_info;

    if ( NULL == pmesh ) pmesh = &mesh;

    printf( "Mesh file not found, so creating a new one...\n" );

    printf( "Number of tiles in X direction ( 32-512 ):  " );
    scanf( "%d", &( map_info.tiles_x ) );

    printf( "Number of tiles in Y direction ( 32-512 ):  " );
    scanf( "%d", &( map_info.tiles_y ) );

    cartman_mpd_create( pmesh, map_info.tiles_x, map_info.tiles_y );

    int fan = 0;
    for (int mapy = 0; mapy < pmesh->info.getTileCountY(); mapy++ )
    {
        int y = mapy * Info<int>::Grid::Size();
        for (int mapx = 0; mapx < pmesh->info.getTileCountX(); mapx++ )
        {
            int x = mapx * Info<int>::Grid::Size();
            if (!pmesh->add_ifan(fan, x, y))
            {
                printf( "NOT ENOUGH VERTICES!!!\n\n" );
                exit(EXIT_FAILURE);
            }

            fan++;
        }
    }

    fix_mesh( pmesh != nullptr ? *pmesh : mesh );
}

//--------------------------------------------------------------------------------------------

void cartman_save_mesh( const char * modname, cartman_mpd_t * pmesh )
{
    if ( NULL == pmesh ) pmesh = &mesh;

    numwritten = 0;
    numattempt = 0;

	std::string newloadname = egoboo_path + SLASH_STR + "modules" + SLASH_STR + modname + SLASH_STR + "gamedat" + SLASH_STR + "plan.bmp";
	
    make_planmap( pmesh );
    if (Resources::get().bmphitemap )
    {
        Ego::ImageManager::get().save_as_bmp(Resources::get().bmphitemap, newloadname);
    }

    //  make_newloadname(modname, SLASH_STR "gamedat" SLASH_STR "level.png", newloadname);
    //  make_hitemap();
    //  if(bmphitemap)
    //  {
    //    make_graypal();
    //    save_pcx(newloadname, bmphitemap);
    //  }

    cartman_mpd_save_vfs( /*modname,*/ pmesh );

    show_name( newloadname, Ego::Math::Colour4f::white() );
}

//--------------------------------------------------------------------------------------------

void cartman_check_input( const char * modulename, cartman_mpd_t * pmesh )
{
    if ( NULL == pmesh ) pmesh = &mesh;

    debugx = -1;
    debugy = -1;

    Cartman::Input::get().checkInput();

    cartman_check_mouse( modulename, pmesh );
    cartman_check_keys( modulename, pmesh );
}

//--------------------------------------------------------------------------------------------

bool config_download(egoboo_config_t& cfg)
{
    return Ego::Setup::download(cfg);
}

bool config_upload(egoboo_config_t& cfg)
{
    return Ego::Setup::upload(cfg);
}

//--------------------------------------------------------------------------------------------

Cartman_MouseData *Cartman_MouseData::ctor(Cartman_MouseData *self)
{
    if (!self)
    {
        return nullptr;
    }
    memset(self, 0, sizeof(*self));

    self->win_id = -1;
    self->win_mode = (uint16_t)(~0);
    self->win_fan = -1;
    self->win_fan_x = -1;
    self->win_fan_y = -1;

    self->fx = MAPFX_SHA;

    self->rect_drag = -1;
    self->rect_done = -1;

    return self;
}

void cart_mouse_data_mesh_set_tile( uint16_t tiletoset )
{
    MeshEditor::mesh_set_tile(mdata.win_mesh != nullptr ? *mdata.win_mesh : mesh, tiletoset, mdata.upper, mdata.presser, mdata.tx );
}

void cart_mouse_data_flatten_mesh()
{
    MeshEditor::flatten_mesh( mdata.win_mesh != nullptr ? *mdata.win_mesh : mesh, mdata.win_mpos_y );
}

void cart_mouse_data_clear_mesh()
{
	MeshEditor::clear_mesh(mdata.win_mesh != nullptr ? *mdata.win_mesh : mesh, mdata.upper, mdata.presser, mdata.tx, mdata.type);
}

void cart_mouse_data_three_e_mesh()
{
    MeshEditor::three_e_mesh( mdata.win_mesh != nullptr ? *mdata.win_mesh : mesh, mdata.upper, mdata.tx );
}

void cart_mouse_data_mesh_replace_tile( bool tx_only, bool at_floor_level )
{
    MeshEditor::mesh_replace_tile( mdata.win_mesh != nullptr ? *mdata.win_mesh : mesh, mdata.win_fan_x, mdata.win_fan_y, mdata.win_fan, mdata.tx, mdata.upper, mdata.fx, mdata.type, mdata.presser, tx_only, at_floor_level );
}

void cart_mouse_data_mesh_set_fx()
{
    MeshEditor::setFX( mdata.win_mesh != nullptr ? *mdata.win_mesh : mesh, mdata.win_fan, mdata.fx );
}

void cart_mouse_data_toggle_fx( int fxmask )
{
    mdata.fx ^= fxmask;
}

void cart_mouse_data_rect_select()
{
    select_lst_add_rect( mdata.win_select, Vector3f(mdata.rect_x0, mdata.rect_y0, mdata.rect_z0),
                                           Vector3f(mdata.rect_x1, mdata.rect_y1, mdata.rect_z1),
                        mdata.win_mode );
}

void cart_mouse_data_rect_unselect()
{
    select_lst_remove_rect( mdata.win_select, Vector3f(mdata.rect_x0, mdata.rect_y0, mdata.rect_z0),
                                              Vector3f(mdata.rect_x1, mdata.rect_y1, mdata.rect_z1),
                           mdata.win_mode );
}

void cart_mouse_data_mesh_replace_fx()
{
    if ( !VALID_MPD_TILE_RANGE( mdata.win_fan ) ) return;
    uint8_t type = mdata.win_mesh->fan2[mdata.win_fan].type;

    tile_definition_t *pdef = tile_dict.get(type);
    if ( NULL == pdef ) return;

    uint16_t tx_bits = mdata.win_mesh->fan2[mdata.win_fan].tx_bits;
    if ( TILE_IS_FANOFF( tx_bits ) ) return;

    if ( type >= tile_dict.offset )
    {
        MeshEditor::mesh_replace_fx( mdata.win_mesh != nullptr ? *mdata.win_mesh : mesh, tx_bits, 0xC0, mdata.fx );
    }
    else
    {
        MeshEditor::mesh_replace_fx( mdata.win_mesh != nullptr ? *mdata.win_mesh : mesh, tx_bits, 0xF0, mdata.fx );
    }
}
