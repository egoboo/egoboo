#include "game/graphics/Camera.hpp"
#include "game/graphic.h"
#include "game/game.h"

ExtendedCamera::ExtendedCamera(const CameraOptions &options) : Camera(options),
    _trackList(),
    _screen{0, 0, 0, 0},
    _lastFrame(-1),
    _renderList(-1),
    _doList(-1)
{
    // assume that the camera is fullscreen
    setScreen(0, 0, sdl_scr.x, sdl_scr.y);
}

ExtendedCamera::~ExtendedCamera()
{
    // free any locked renderlist
    renderlist_mgr_t *rmgr_ptr = gfx_system_get_renderlist_mgr();
    if ( -1 != _renderList )
    {
        renderlist_mgr_free_one( rmgr_ptr, _renderList );
        _renderList = -1;
    }

    // free any locked dolist
    dolist_mgr_t *dmgr_ptr = gfx_system_get_dolist_mgr();
    if ( -1 != _doList )
    {
        dolist_mgr_free_one( dmgr_ptr, _renderList ); //TODO: check is this correct? or should _renderList be _doList?
        _doList = -1;
    }
}

void ExtendedCamera::initialize(int renderList, int doList)
{
    // make the default viewport fullscreen
    _screen.xmin = 0.0f;
    _screen.xmax = sdl_scr.x;
    _screen.ymin = 0.0f;
    _screen.ymax = sdl_scr.y;

    _renderList = renderList;
    _doList = doList;
}

void ExtendedCamera::updateProjectionExtended()
{
    float frustum_near, frustum_far, aspect_ratio;

    //---- set the camera's projection matrix
    aspect_ratio = ( _screen.xmax - _screen.xmin ) / ( _screen.ymax - _screen.ymin );

    // the nearest we will have to worry about is 1/2 of a tile
    frustum_near = GRID_ISIZE * 0.25f;
    // set the maximum depth to be the "largest possible size" of a mesh
    frustum_far  = GRID_ISIZE * 256 * SQRT_TWO;

    updateProjection(DEFAULT_FOV, aspect_ratio, frustum_near, frustum_far);
 }

void ExtendedCamera::setScreen( float xmin, float ymin, float xmax, float ymax )
{
    // set the screen
    _screen.xmin = xmin;
    _screen.ymin = ymin;
    _screen.xmax = xmax;
    _screen.ymax = ymax;

    //Update projection after setting size
    updateProjectionExtended();
}

void ExtendedCamera::reset(const ego_mesh_t * pmesh)
{
	Camera::reset(pmesh, _trackList);
}

void ExtendedCamera::update(const ego_mesh_t * pmesh)
{
	Camera::update(pmesh, _trackList);
}

void ExtendedCamera::resetTarget(const ego_mesh_t * pmesh)
{
	Camera::resetTarget(pmesh, _trackList);
}

void ExtendedCamera::addTrackTarget(const CHR_REF target)
{
	_trackList.push_front(target);
}
