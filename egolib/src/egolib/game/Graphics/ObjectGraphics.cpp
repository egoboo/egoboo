#include "egolib/game/Graphics/ObjectGraphics.hpp"
#include "egolib/Entities/_Include.hpp"
#include "egolib/game/graphic.h"
#include "egolib/game/game.h" //only for character_swipe()

namespace Ego
{
namespace Graphics
{

// the flip tolerance is the default flip increment / 2
static constexpr float FLIP_TOLERANCE = 0.25f * 0.5f;

ObjectGraphics::ObjectGraphics(Object &object) :
    matrix_cache(),

    alpha(0xFF),
    light(0),
    sheen(0),

    colorshift(),

    uoffset(0),
    voffset(0),

    _object(object),
    _vertexList(),
    _matrix(Matrix4f4f::identity()),
    _reflectionMatrix(Matrix4f4f::identity()),

    // graphical optimizations
    _vertexCache(),

    // lighting info
    _ambientColour(0),
    _maxLight(-0xFF),
    _lastLightingUpdateFrame(-1),

    _modelDescriptor(nullptr),
    _animationRate(1.0f),
    _animationProgress(0.0f),
    _animationProgressInteger(0),
    _targetFrameIndex(0),
    _sourceFrameIndex(0),

    _canBeInterrupted(true),
    _freezeAtLastFrame(false),
    _loopAnimation(false),
    _currentAnimation(ACTION_DA),
    _nextAnimation(ACTION_DA)    
{
    // initalize the character instance
    setObjectProfile(_object.getProfile());
}

ObjectGraphics::~ObjectGraphics() 
{
    //dtor
}

void ObjectGraphics::updateLighting()
{
    static constexpr uint32_t FRAME_SKIP = 1 << 2;
    static constexpr uint32_t FRAME_MASK = FRAME_SKIP - 1;

    // make sure the matrix is valid
    //chr_update_matrix(&_object, true);

    // has this already been calculated in the last FRAME_SKIP update frames?
	if (_lastLightingUpdateFrame >= 0 && static_cast<uint32_t>(_lastLightingUpdateFrame) >= update_wld) {
		return;
	}

    // reduce the amount of updates to one every FRAME_SKIP frames, but dither
    // the updating so that not all objects update on the same frame
    _lastLightingUpdateFrame = update_wld + ((update_wld + _object.getObjRef().get()) & FRAME_MASK);

    // interpolate the lighting for the origin of the object
	lighting_cache_t global_light;
    GridIllumination::grid_lighting_interpolate(*_currentModule->getMeshPointer(), global_light, Vector2f(_object.getPosX(), _object.getPosY()));

    // rotate the lighting data to body_centered coordinates
	lighting_cache_t loc_light;
	lighting_cache_t::lighting_project_cache(loc_light, global_light, getMatrix());

    //Low-pass filter to smooth lighting transitions?
    //_ambientColour = 0.9f * _ambientColour + 0.1f * (loc_light.hgh._lighting[LVEC_AMB] + loc_light.low._lighting[LVEC_AMB]) * 0.5f;
    //_ambientColour = (loc_light.hgh._lighting[LVEC_AMB] + loc_light.low._lighting[LVEC_AMB]) * 0.5f;
    _ambientColour = get_ambient_level();

    _maxLight = -0xFF;
    for (size_t cnt = 0; cnt < _vertexList.size(); cnt++ )
    {
        float lite = 0.0f;

        GLvertex *pvert = &_vertexList[cnt];

        // a simple "height" measurement
        float hgt = pvert->pos[ZZ] * _matrix(3, 3) + _matrix(3, 3);

        if (pvert->nrm[0] == 0.0f && pvert->nrm[1] == 0.0f && pvert->nrm[2] == 0.0f)
        {
            // this is the "ambient only" index, but it really means to sum up all the light
            lite  = lighting_cache_t::lighting_evaluate_cache(loc_light, Vector3f(+1.0f,+1.0f,+1.0f), hgt, _currentModule->getMeshPointer()->_tmem._bbox, nullptr, nullptr);
            lite += lighting_cache_t::lighting_evaluate_cache(loc_light, Vector3f(-1.0f,-1.0f,-1.0f), hgt, _currentModule->getMeshPointer()->_tmem._bbox, nullptr, nullptr);

            // average all the directions
            lite /= 6.0f;
        }
        else
        {
            lite = lighting_cache_t::lighting_evaluate_cache(loc_light, Vector3f(pvert->nrm[0],pvert->nrm[1],pvert->nrm[2]), hgt, _currentModule->getMeshPointer()->_tmem._bbox, nullptr, nullptr);
        }

        pvert->color_dir = lite;

        _maxLight = std::max(_maxLight, pvert->color_dir);
    }

    // ??coerce this to reasonable values in the presence of negative light??
    _maxLight = std::max(_maxLight, 0);
}

int ObjectGraphics::getAmbientColour() const
{
    return _ambientColour;
}

gfx_rv ObjectGraphics::needs_update(int vmin, int vmax, bool *verts_match, bool *frames_match)
{
	bool local_verts_match, local_frames_match;

    // ensure that the pointers point to something
    if ( NULL == verts_match ) verts_match  = &local_verts_match;
    if ( NULL == frames_match ) frames_match = &local_frames_match;

    // initialize the boolean pointers
    *verts_match  = false;
    *frames_match = false;

    // check to see if the _vertexCache has been marked as invalid.
    // in this case, everything needs to be updated
	if (!isVertexCacheValid()) {
		return gfx_success;
	}

    // get the last valid vertex from the chr_instance
    int maxvert = static_cast<int>(_vertexList.size()) - 1;

    // check to make sure the lower bound of the saved data is valid.
    // it is initialized to an invalid value (_vertexCache.vmin = _vertexCache.vmax = -1)
	if (_vertexCache.vmin < 0 || _vertexCache.vmax < 0) {
		return gfx_success;
	}
    // check to make sure the upper bound of the saved data is valid.
	if (_vertexCache.vmin > maxvert || _vertexCache.vmax > maxvert) {
		return gfx_success;
	}
    // make sure that the min and max vertices are in the correct order
	if (vmax < vmin) {
		std::swap(vmax, vmin);
	}
    // test to see if we have already calculated this data
    *verts_match = (vmin >= _vertexCache.vmin) && (vmax <= _vertexCache.vmax);

	bool flips_match = (std::abs(_vertexCache.flip - _animationProgress) < FLIP_TOLERANCE);

    *frames_match = (_targetFrameIndex == _sourceFrameIndex && _vertexCache.frame_nxt == _targetFrameIndex && _vertexCache.frame_lst == _sourceFrameIndex ) ||
                    (flips_match && _vertexCache.frame_nxt == _targetFrameIndex && _vertexCache.frame_lst == _sourceFrameIndex);

    return (!(*verts_match) || !( *frames_match )) ? gfx_success : gfx_fail;
}

void ObjectGraphics::interpolateVerticesRaw(const std::vector<MD2_Vertex> &lst_ary, const std::vector<MD2_Vertex> &nxt_ary, int vmin, int vmax, float flip )
{
    /// raw indicates no bounds checking, so be careful

    if ( 0.0f == flip )
    {
        for (size_t i = vmin; i <= vmax; i++)
        {
            GLvertex* dst = &_vertexList[i];
            const MD2_Vertex &srcLast = lst_ary[i];

			dst->pos[XX] = srcLast.pos[kX];
			dst->pos[YY] = srcLast.pos[kY];
			dst->pos[ZZ] = srcLast.pos[kZ];
            dst->pos[WW] = 1.0f;

			dst->nrm[XX] = srcLast.nrm[kX];
			dst->nrm[YY] = srcLast.nrm[kY];
			dst->nrm[ZZ] = srcLast.nrm[kZ];

            dst->env[XX] = indextoenvirox[srcLast.normal];
            dst->env[YY] = 0.5f * ( 1.0f + dst->nrm[ZZ] );
        }
    }
    else if ( 1.0f == flip )
    {
        for (size_t i = vmin; i <= vmax; i++ )
        {
            GLvertex* dst = &_vertexList[i];
            const MD2_Vertex &srcNext = nxt_ary[i];

			dst->pos[XX] = srcNext.pos[kX];
			dst->pos[YY] = srcNext.pos[kY];
			dst->pos[ZZ] = srcNext.pos[kZ];
            dst->pos[WW] = 1.0f;

            dst->nrm[XX] = srcNext.nrm[kX];
            dst->nrm[YY] = srcNext.nrm[kY];
            dst->nrm[ZZ] = srcNext.nrm[kZ];

            dst->env[XX] = indextoenvirox[srcNext.normal];
            dst->env[YY] = 0.5f * ( 1.0f + dst->nrm[ZZ] );
        }
    }
    else
    {
        uint16_t vrta_lst, vrta_nxt;

        for (size_t i = vmin; i <= vmax; i++)
        {
            GLvertex* dst = &_vertexList[i];
            const MD2_Vertex &srcLast = lst_ary[i];
            const MD2_Vertex &srcNext = nxt_ary[i];

            dst->pos[XX] = srcLast.pos[kX] + ( srcNext.pos[kX] - srcLast.pos[kX] ) * flip;
            dst->pos[YY] = srcLast.pos[kY] + ( srcNext.pos[kY] - srcLast.pos[kY] ) * flip;
            dst->pos[ZZ] = srcLast.pos[kZ] + ( srcNext.pos[kZ] - srcLast.pos[kZ] ) * flip;
            dst->pos[WW] = 1.0f;

            dst->nrm[XX] = srcLast.nrm[kX] + ( srcNext.nrm[kX] - srcLast.nrm[kX] ) * flip;
            dst->nrm[YY] = srcLast.nrm[kY] + ( srcNext.nrm[kY] - srcLast.nrm[kY] ) * flip;
            dst->nrm[ZZ] = srcLast.nrm[kZ] + ( srcNext.nrm[kZ] - srcLast.nrm[kZ] ) * flip;

            vrta_lst = srcLast.normal;
            vrta_nxt = srcNext.normal;

            dst->env[XX] = indextoenvirox[vrta_lst] + ( indextoenvirox[vrta_nxt] - indextoenvirox[vrta_lst] ) * flip;
            dst->env[YY] = 0.5f * ( 1.0f + dst->nrm[ZZ] );
        }
    }
}

gfx_rv ObjectGraphics::updateVertices(int vmin, int vmax, bool force)
{
    bool vertices_match, frames_match;
    float  loc_flip;

    int vdirty1_min = -1, vdirty1_max = -1;
    int vdirty2_min = -1, vdirty2_max = -1;

    // get the model
    const std::shared_ptr<MD2Model> &pmd2 = getModelDescriptor()->getMD2();

    // make sure we have valid data
    if (_vertexList.size() != pmd2->getVertexCount())
    {
        Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "character instance vertex data does not match its md2", Log::EndOfEntry);
        return gfx_error;
    }

    // get the vertex list size from the chr_instance
    int maxvert = static_cast<int>(_vertexList.size()) - 1;

    // handle the default parameters
    if ( vmin < 0 ) vmin = 0;
    if ( vmax < 0 ) vmax = maxvert;

    // are they in the right order?
    if ( vmax < vmin ) std::swap(vmax, vmin);

    // make sure that the vertices are within the max range
    vmin = Ego::Math::constrain(vmin, 0, maxvert);
    vmax = Ego::Math::constrain(vmax, 0, maxvert);

    if (force)
    {
        // force an update of vertices

        // select a range that encompases the requested vertices and the saved vertices
        // if this is the 1st update, the saved vertices may be set to invalid values, as well

        // grab the dirty vertices
        vdirty1_min = vmin;
        vdirty1_max = vmax;

        // force the routine to update
        vertices_match = false;
        frames_match   = false;
    }
    else
    {
        // do we need to update?
        gfx_rv retval = needs_update(vmin, vmax, &vertices_match, &frames_match );
        if ( gfx_error == retval ) return gfx_error;            // gfx_error == retval means some pointer or reference is messed up
        if ( gfx_fail  == retval ) return gfx_success;          // gfx_fail  == retval means we do not need to update this round

        if ( !frames_match )
        {
            // the entire frame is dirty
            vdirty1_min = vmin;
            vdirty1_max = vmax;
        }
        else
        {
            // grab the dirty vertices
            if ( vmin < _vertexCache.vmin )
            {
                vdirty1_min = vmin;
                vdirty1_max = _vertexCache.vmin - 1;
            }

            if ( vmax > _vertexCache.vmax )
            {
                vdirty2_min = _vertexCache.vmax + 1;
                vdirty2_max = vmax;
            }
        }
    }

    // make sure the frames are in the valid range
    const auto& frameList = pmd2->getFrames();
    if ( _targetFrameIndex >= frameList.size() || _sourceFrameIndex >= frameList.size() )
    {
		Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "character instance frame is outside "
                                         "the range of its MD2", Log::EndOfEntry);
        return gfx_error;
    }

    // grab the frame data from the correct model
    const auto& nextFrame = frameList[_targetFrameIndex];
    const auto& lastFrame = frameList[_sourceFrameIndex];

    // fix the flip for objects that are not animating
    loc_flip = _animationProgress;
    if ( _targetFrameIndex == _sourceFrameIndex ) {
        loc_flip = 0.0f;
    }

    // interpolate the 1st dirty region
    if ( vdirty1_min >= 0 && vdirty1_max >= 0 )
    {
		interpolateVerticesRaw(lastFrame.vertexList, nextFrame.vertexList, vdirty1_min, vdirty1_max, loc_flip);
    }

    // interpolate the 2nd dirty region
    if ( vdirty2_min >= 0 && vdirty2_max >= 0 )
    {
		interpolateVerticesRaw(lastFrame.vertexList, nextFrame.vertexList, vdirty2_min, vdirty2_max, loc_flip);
    }

    // update the saved parameters
    return updateVertexCache(vmax, vmin, force, vertices_match, frames_match);
}

gfx_rv ObjectGraphics::updateVertexCache(int vmax, int vmin, bool force, bool vertices_match, bool frames_match)
{
    // this is getting a bit ugly...
    // we need to do this calculation as little as possible, so it is important that the
    // _vertexCache.* values be tested and stored properly

	int maxvert = static_cast<int>(_vertexList.size()) - 1;

    // the save_vmin and save_vmax is the most complex
    bool verts_updated = false;
    if ( force )
    {
        // to get here, either the specified range was outside the clean range or
        // the animation was updated. In any case, the only vertices that are
        // clean are in the range [vmin, vmax]

        _vertexCache.vmin   = vmin;
        _vertexCache.vmax   = vmax;
        verts_updated = true;
    }
    else if ( vertices_match && frames_match )
    {
        // everything matches, so there is nothing to do
    }
    else if ( vertices_match )
    {
        // The only way to get here is to fail the frames_match test, and pass vertices_match

        // This means that all of the vertices were SUPPOSED TO BE updated,
        // but only the ones in the range [vmin, vmax] actually were.
        _vertexCache.vmin = vmin;
        _vertexCache.vmax = vmax;
        verts_updated = true;
    }
    else if ( frames_match )
    {
        // The only way to get here is to fail the vertices_match test, and pass frames_match test

        // There was no update to the animation,  but there was an update to some of the vertices
        // The clean verrices should be the union of the sets of the vertices updated this time
        // and the oned updated last time.
        //
        // If these ranges are disjoint, then only one of them can be saved. Choose the larger set

        if ( vmax >= _vertexCache.vmin && vmin <= _vertexCache.vmax )
        {
            // the old list [save_vmin, save_vmax] and the new list [vmin, vmax]
            // overlap, so we can merge them
            _vertexCache.vmin = std::min( _vertexCache.vmin, vmin );
            _vertexCache.vmax = std::max( _vertexCache.vmax, vmax );
            verts_updated = true;
        }
        else
        {
            // the old list and the new list are disjoint sets, so we are out of luck
            // save the set with the largest number of members
            if (( _vertexCache.vmax - _vertexCache.vmin ) >= ( vmax - vmin ) )
            {
                // obviously no change...
                //_vertexCache.vmin = _vertexCache.vmin;
                //_vertexCache.vmax = _vertexCache.vmax;
                verts_updated = true;
            }
            else
            {
                _vertexCache.vmin = vmin;
                _vertexCache.vmax = vmax;
                verts_updated = true;
            }
        }
    }
    else
    {
        // The only way to get here is to fail the vertices_match test, and fail the frames_match test

        // everything was dirty, so just save the new vertex list
        _vertexCache.vmin = vmin;
        _vertexCache.vmax = vmax;
        verts_updated = true;
    }

    _vertexCache.frame_nxt = _targetFrameIndex;
    _vertexCache.frame_lst = _sourceFrameIndex;
    _vertexCache.flip      = _animationProgress;

    // store the last time there was an update to the animation
    bool frames_updated = false;
    if ( !frames_match )
    {
        _vertexCache.frame_wld = update_wld;
        frames_updated   = true;
    }

    // store the time of the last full update
    if ( 0 == vmin && maxvert == vmax )
    {
        _vertexCache.vert_wld  = update_wld;
    }

    return ( verts_updated || frames_updated ) ? gfx_success : gfx_fail;
}

bool ObjectGraphics::updateGripVertices(const uint16_t vrt_lst[], const size_t vrt_count)
{
    if ( nullptr == vrt_lst || 0 == vrt_count ) {
        return false;
    }

    // count the valid attachment points
    int vmin = 0xFFFF;
    int vmax = 0;
    size_t count = 0;
    for (size_t cnt = 0; cnt < vrt_count; cnt++ )
    {
        if ( 0xFFFF == vrt_lst[cnt] ) continue;

        vmin = std::min<uint16_t>(vmin, vrt_lst[cnt]);
        vmax = std::max<uint16_t>(vmax, vrt_lst[cnt]);
        count++;
    }

    // if there are no valid points, there is nothing to do
    if (0 == count) {
        return false;
    }

    // force the vertices to update
    return updateVertices(vmin, vmax, true) == gfx_success;
}

bool ObjectGraphics::playAction(const ModelAction action, const bool action_ready)
{
    return startAnimation(getModelDescriptor()->getAction(action), action_ready, true);
}

void ObjectGraphics::clearCache()
{
    /// @author BB
    /// @details force chr_instance_update_vertices() recalculate the vertices the next time
    ///     the function is called

    _vertexCache.clear();
    this->matrix_cache = matrix_cache_t();

    _lastLightingUpdateFrame = -1;
}

int ObjectGraphics::getMaxLight() const
{
    return _maxLight;    
}

const GLvertex& ObjectGraphics::getVertex(const size_t index) const
{
    return _vertexList[index];
}

bool ObjectGraphics::setModel(const std::shared_ptr<Ego::ModelDescriptor> &model)
{
    bool updated = false;

    if (getModelDescriptor() != model) {
        updated = true;
        _modelDescriptor = model;
    }

    // set the vertex size
    size_t vlst_size = getModelDescriptor()->getMD2()->getVertexCount();
    if (_vertexList.size() != vlst_size) {
        updated = true;
        _vertexList.resize(vlst_size);
    }

    // set the frames to frame 0 of this object's data
    if (0 != _targetFrameIndex || 0 != _sourceFrameIndex) {
        updated = true;
        _sourceFrameIndex = 0;
        _targetFrameIndex = 0;
    }

    if (updated) {
        // update the vertex and lighting cache
        clearCache();
        updateVertices(-1, -1, true);
    }

    return updated;
}

const Matrix4f4f& ObjectGraphics::getMatrix() const
{
    return _matrix;
}

const Matrix4f4f& ObjectGraphics::getReflectionMatrix() const
{
    return _reflectionMatrix;
}

uint8_t ObjectGraphics::getReflectionAlpha() const
{
    // determine the reflection alpha based on altitude above the mesh
    const float altitudeAboveGround = std::max(0.0f, _object.getPosZ() - _object.getFloorElevation());
    float alphaFade = (255.0f - altitudeAboveGround)*0.5f;
    alphaFade = Ego::Math::constrain(alphaFade, 0.0f, 255.0f);

    return this->alpha * alphaFade * idlib::fraction<float, 1, 255>();
}

void ObjectGraphics::setObjectProfile(const std::shared_ptr<ObjectProfile> &profile)
{
    //Reset data
    // Remember any previous color shifts in case of lasting enchantments
    _matrix = Matrix4f4f::identity();
    _reflectionMatrix = Matrix4f4f::identity();
    uoffset = 0;
    voffset = 0;

    _ambientColour = 0;
    _maxLight = -0xFF;
    _vertexList.clear();
    clearCache();

    //Animation and 3D model
    _modelDescriptor = nullptr;
    _targetFrameIndex = 0;
    _sourceFrameIndex = 0;
    _animationProgressInteger = 0;
    _animationProgress = 0.0f;
    _animationRate = 1.0f;

    //Action specific variables
    _canBeInterrupted = true;
    _freezeAtLastFrame = false;
    _loopAnimation = false;
    _currentAnimation = ACTION_DA;
    _nextAnimation = ACTION_DA;

    // lighting parameters
	this->alpha = profile->getAlpha();
	this->light = profile->getLight();
	this->sheen = profile->getSheen();

    // model parameters
    setModel(profile->getModel());

    // set the initial action, all actions override it
    setActionReady(false);
    setActionLooped(false);
    if (_object.isAlive()) {
        playAction(ACTION_DA, false);
        setActionKeep(false);
    }
    else {
        playAction(profile->getModel()->randomizeAction(ACTION_KA), false);
        setActionKeep(true);
    }
}

BIT_FIELD ObjectGraphics::getFrameFX() const
{
    return getNextFrame().framefx;
}

const MD2_Frame& ObjectGraphics::getNextFrame() const
{
    assertFrameIndex(_targetFrameIndex);
    return getModelDescriptor()->getMD2()->getFrames()[_targetFrameIndex];
}

const MD2_Frame& ObjectGraphics::getLastFrame() const
{
    assertFrameIndex(_sourceFrameIndex);
    return getModelDescriptor()->getMD2()->getFrames()[_sourceFrameIndex];
}

bool ObjectGraphics::isVertexCacheValid() const
{
    if (_sourceFrameIndex != _vertexCache.frame_nxt) {
        return false;
    }

    if (_sourceFrameIndex != _vertexCache.frame_lst) {
        return false;
    }

    if ((_sourceFrameIndex != _vertexCache.frame_lst) && std::abs(_animationProgress - _vertexCache.flip) > Ego::Graphics::FLIP_TOLERANCE) {
        return false;
    }

    return true;
}

void ObjectGraphics::getTint(GLXvector4f tint, const bool reflection, const int type)
{
	int local_alpha;
	int local_light;
	int local_sheen;
    colorshift_t local_colorshift;

	if (reflection)
	{
		// this is a reflection, use the reflected parameters
		local_alpha = getReflectionAlpha();

        if(this->light == 0xFF) {
            local_light = 0xFF;
        }
        else {
            local_light = this->light * local_alpha * idlib::fraction<float, 1, 255>();
        }

		local_sheen = this->sheen / 2; //half of normal sheen
        local_colorshift = colorshift_t(this->colorshift.red + 1, this->colorshift.green + 1, this->colorshift.blue + 1);
	}
	else
	{
		// this is NOT a reflection, use the normal parameters
		local_alpha = this->alpha;
		local_light = this->light;
		local_sheen = this->sheen;
        local_colorshift = this->colorshift;
	}

	// modify these values based on local character abilities
    if(local_stats.seeinvis_level > 0.0f) {
        local_alpha = std::max(local_alpha, SEEINVISIBLE);
    }
	local_light = get_light(local_light, local_stats.seedark_mag);

	// clear out the tint
    tint[RR] = 1.0f / (1 << local_colorshift.red);
    tint[GG] = 1.0f / (1 << local_colorshift.green);
    tint[BB] = 1.0f / (1 << local_colorshift.blue);
    tint[AA] = 1.0f;

    switch(type)
    {
        case CHR_LIGHT:
        case CHR_ALPHA:
            // alpha characters are blended onto the canvas using the alpha channel
            tint[AA] = local_alpha * idlib::fraction<float, 1, 255>();

            // alpha characters are blended onto the canvas by adding their color
            // the more black the colors, the less visible the character
            // the alpha channel is not important
            tint[RR] = local_light * idlib::fraction<float, 1, 255>() / (1 << local_colorshift.red);
            tint[GG] = local_light * idlib::fraction<float, 1, 255>() / (1 << local_colorshift.green);
            tint[BB] = local_light * idlib::fraction<float, 1, 255>() / (1 << local_colorshift.blue);
        break;

        case CHR_PHONG:
            // phong is essentially the same as light, but it is the
            // sheen that sets the effect
            float amount = (Ego::Math::constrain(local_sheen, 0, 15) << 4) / 240.0f;

            tint[RR] += tint[RR] * 0.5f + amount;
            tint[GG] += tint[GG] * 0.5f + amount;
            tint[BB] += tint[BB] * 0.5f + amount;

            tint[RR] /= 2.0f;
            tint[GG] /= 2.0f;
            tint[BB] /= 2.0f;
        break;
    }

}

void ObjectGraphics::flash(uint8_t value)
{
	const float flash_val = value * idlib::fraction<float, 1, 255>();

	// flash the ambient color
	_ambientColour = flash_val;

	// flash the directional lighting
	for (size_t i = 0; i < _vertexList.size(); ++i) {
		_vertexList[i].color_dir = flash_val;
	}
}


size_t ObjectGraphics::getVertexCount() const
{
    return _vertexList.size();
}

void ObjectGraphics::flashVariableHeight(const uint8_t valuelow, const int16_t low, const uint8_t valuehigh, const int16_t high)
{
    for (size_t cnt = 0; cnt < _vertexList.size(); cnt++)
    {
        int16_t z = _vertexList[cnt].pos[ZZ];

        if ( z < low )
        {
            _vertexList[cnt].col[RR] =
                _vertexList[cnt].col[GG] =
                    _vertexList[cnt].col[BB] = valuelow;
        }
        else if ( z > high )
        {
            _vertexList[cnt].col[RR] =
                _vertexList[cnt].col[GG] =
                    _vertexList[cnt].col[BB] = valuehigh;
        }
        else if ( high != low )
        {
            uint8_t valuemid = ( valuehigh * ( z - low ) / ( high - low ) ) +
                             ( valuelow * ( high - z ) / ( high - low ) );

            _vertexList[cnt].col[RR] =
                _vertexList[cnt].col[GG] =
                    _vertexList[cnt].col[BB] =  valuemid;
        }
        else
        {
            // z == high == low
            uint8_t valuemid = ( valuehigh + valuelow ) * 0.5f;

            _vertexList[cnt].col[RR] =
                _vertexList[cnt].col[GG] =
                    _vertexList[cnt].col[BB] =  valuemid;
        }
    }
}

void ObjectGraphics::setMatrix(const Matrix4f4f& matrix)
{
    //Set the normal model matrix
    _matrix = matrix;

    //Compute the reflected matrix as well
    _reflectionMatrix = matrix;
    _reflectionMatrix(2, 0) = -_reflectionMatrix(0, 2);
    _reflectionMatrix(2, 1) = -_reflectionMatrix(1, 2);
    _reflectionMatrix(2, 2) = -_reflectionMatrix(2, 2);
    _reflectionMatrix(2, 3) = 2.0f * _object.getFloorElevation() - _object.getPosZ();
}

static void chr_invalidate_child_instances(Object &object)
{
    if(object.getLeftHandItem()) {
        object.getLeftHandItem()->inst.matrix_cache.valid = false;
    }
    if(object.getRightHandItem()) {
        object.getRightHandItem()->inst.matrix_cache.valid = false;
    }
}

const std::shared_ptr<Ego::ModelDescriptor>& ObjectGraphics::getModelDescriptor() const {
    return _modelDescriptor;
}

void ObjectGraphics::assertFrameIndex(int frameIndex) const {
    if (frameIndex > getModelDescriptor()->getMD2()->getFrames().size()) {
        auto e = Log::Entry::create(Log::Level::Error, __FILE__, __LINE__, "invalid frame ", frameIndex, "/", 
                                    getModelDescriptor()->getMD2()->getFrames().size(), Log::EndOfEntry);
        Log::get() << e;
        throw idlib::runtime_error(__FILE__, __LINE__, e.getText());
    }
}

void ObjectGraphics::setAnimationSpeed(const float rate)
{
    _animationRate = Ego::Math::constrain(rate, 0.1f, 3.0f);
}

void ObjectGraphics::updateAnimation()
{
    float flip_diff  = 0.25f * _animationRate;
    float flip_next = getRemainingFlip();

    while ( flip_next > 0.0f && flip_diff >= flip_next )
    {
        flip_diff -= flip_next;

        //Update one linear interpolated frame
        _animationProgressInteger += 1;
        _animationProgress = 0.25f * _animationProgressInteger;

        // handle frame FX for the new frame
        if ( 3 == _animationProgressInteger )
        {
            handleAnimationFX();
        }

        if ( 4 == _animationProgressInteger )
        {
            incrementFrame();
        }

        if ( _animationProgressInteger > 4 )
        {
            Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "invalid ilip", Log::EndOfEntry);
            _animationProgressInteger = 0;
            break;
        }

        flip_next = getRemainingFlip();
    }

    if ( flip_diff > 0.0f )
    {
        int ilip_old = _animationProgressInteger;

        // update the lips
        _animationProgress += flip_diff;
        _animationProgressInteger = ((int)std::floor(_animationProgress * 4)) % 4;

        if ( ilip_old != _animationProgressInteger )
        {
            // handle frame FX for the new frame
            if ( 3 == _animationProgressInteger )
            {
                handleAnimationFX();
            }

            if ( 4 == _animationProgressInteger )
            {
                incrementFrame();
            }

            if ( _animationProgressInteger > 4 )
            {
                Log::get() << Log::Entry::create(Log::Level::Warning, __FILE__, __LINE__, "invalid ilip", Log::EndOfEntry);
                _animationProgressInteger = 0;
            }
        }
    }

    updateAnimationRate();
}

float ObjectGraphics::getRemainingFlip() const
{
    return (_animationProgressInteger + 1) * 0.25f - _animationProgress;
}

bool ObjectGraphics::handleAnimationFX() const
{
    uint32_t framefx = _object.inst.getFrameFX();

    if ( 0 == framefx ) return true;

    // Check frame effects
    if ( HAS_SOME_BITS( framefx, MADFX_ACTLEFT ) )
    {
        character_swipe( _object.getObjRef(), SLOT_LEFT );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_ACTRIGHT ) )
    {
        character_swipe( _object.getObjRef(), SLOT_RIGHT );
    }

    if ( HAS_SOME_BITS( framefx, MADFX_GRABLEFT ) )
    {
        _object.getObjectPhysics().grabStuff(GRIP_LEFT, false);
    }

    if ( HAS_SOME_BITS( framefx, MADFX_GRABRIGHT ) )
    {
        _object.getObjectPhysics().grabStuff(GRIP_RIGHT, false);
    }

    if ( HAS_SOME_BITS( framefx, MADFX_CHARLEFT ) )
    {
        _object.getObjectPhysics().grabStuff(GRIP_LEFT, true);
    }

    if ( HAS_SOME_BITS( framefx, MADFX_CHARRIGHT ) )
    {
        _object.getObjectPhysics().grabStuff(GRIP_RIGHT, true);
    }

    if ( HAS_SOME_BITS( framefx, MADFX_DROPLEFT ) )
    {
        if(_object.getLeftHandItem()) {
            _object.getLeftHandItem()->detatchFromHolder(false, true);
        }
    }

    if ( HAS_SOME_BITS( framefx, MADFX_DROPRIGHT ) )
    {
        if(_object.getRightHandItem()) {
            _object.getRightHandItem()->detatchFromHolder(false, true);
        }
    }

    if ( HAS_SOME_BITS( framefx, MADFX_POOF ) && !_object.isPlayer() )
    {
        _object.ai.poof_time = update_wld;
    }

    //Do footfall sound effect
    if (egoboo_config_t::get().sound_footfallEffects_enable.getValue() && HAS_SOME_BITS(framefx, MADFX_FOOTFALL))
    {
        AudioSystem::get().playSound(_object.getPosition(), _object.getProfile()->getFootFallSound());
    }

    return true;
}

void ObjectGraphics::incrementFrame()
{
    // fix the ilip and flip
    _animationProgressInteger %= 4;
    _animationProgress = fmod(_animationProgress, 1.0f);

    // Change frames
    int frame_lst = _targetFrameIndex;
    int frame_nxt = _targetFrameIndex + 1;

    // detect the end of the animation and handle special end conditions
    if (frame_nxt > getModelDescriptor()->getLastFrame(_currentAnimation))
    {
        if (_freezeAtLastFrame)
        {
            // Freeze that animation at the last frame
            frame_nxt = frame_lst;

            // Break a kept action at any time
            _canBeInterrupted = true;
        }
        else if (_loopAnimation)
        {
            // Convert the action into a riding action if the character is mounted
            if (_object.isBeingHeld())
            {
                ModelAction mount_action;

                // determine what kind of action we are going to substitute for a riding character
                if (_object.getLeftHandItem() || _object.getRightHandItem()) {
                    // if the character is holding anything, make the animation
                    // ACTION_MH == "sitting" so that it does not look so silly
                    mount_action = _object.getProfile()->getModel()->getAction(ACTION_MH);
                }
                else {
                    // if it is not holding anything, go for the riding animation
                    mount_action = _object.getProfile()->getModel()->getAction(ACTION_MI);
                }
                
                startAnimation(mount_action, true, true);
            }

            // set the frame to the beginning of the action
            frame_nxt = getModelDescriptor()->getFirstFrame(_currentAnimation);

            // Break a looped action at any time
            _canBeInterrupted = true;
        }
        else
        {
            // Go on to the next action. don't let just anything interrupt it?
            incrementAction();

            // incrementAction() actually sets this value properly. just grab the new value.
            frame_nxt = _targetFrameIndex;
        }
    }

    _sourceFrameIndex = frame_lst;
    _targetFrameIndex = frame_nxt;

    // if the instance is invalid, invalidate everything that depends on this object
    if (!_object.inst.isVertexCacheValid()) {
        chr_invalidate_child_instances(_object);
    }
}

bool ObjectGraphics::startAnimation(const ModelAction action, const bool action_ready, const bool override_action)
{
    if (!setAction(action, action_ready, override_action)) {
        return false;
    }

    if(!setFrame(getModelDescriptor()->getFirstFrame(action))) {
        return false;
    }

    // if the instance is invalid, invalidate everything that depends on this object
    if (!_object.inst.isVertexCacheValid()) {
        chr_invalidate_child_instances(_object);
    }

    return true;
}

bool ObjectGraphics::setFrame(int frame)
{
    // is the frame within the valid range for this action?
    if(!getModelDescriptor()->isFrameValid(_currentAnimation, frame)) {
        return false;
    }

    // jump to the next frame
    _animationProgress = 0.0f;
    _animationProgressInteger = 0;
    _sourceFrameIndex = _targetFrameIndex;
    _targetFrameIndex = frame;

    return true;
}

bool ObjectGraphics::incrementAction()
{
    // get the correct action
    ModelAction action = getModelDescriptor()->getAction(_nextAnimation);

    // determine if the action is one of the types that can be broken at any time
    // D == "dance" and "W" == walk
    // @note ZF> Can't use ACTION_IS_TYPE(action, D) because of GCC compile warning
    bool action_ready = action < ACTION_DD || ACTION_IS_TYPE(action, W);

    return startAnimation(action, action_ready, true);
}

void ObjectGraphics::updateAnimationRate()
{
    ObjectGraphics& pinst = _object.inst;

    // dont change the rate if it is an attack animation
    if ( _object.isAttacking() ) {  
        return;
    }

    // if the action is set to keep then do nothing
    if (_freezeAtLastFrame) {
        return;
    }

    // if the action cannot be changed on the at this time, there's nothing to do.
    // keep the same animation rate
    if ( !canBeInterrupted() )
    {
        if (0.0f == _animationRate) { 
            _animationRate = 1.0f;
        }
        return;
    }

    // go back to a base animation rate, in case the next frame is not a
    // "variable speed frame"
    _animationRate = 1.0f;

    // if the character is mounted or sitting, base the rate off of the mounr
    if ( _object.isBeingHeld() && (( ACTION_MI == _currentAnimation ) || ( ACTION_MH == _currentAnimation ) ) )
    {
        if(_object.getHolder()->isScenery()) {
            //This is a special case to make animation while in the Pot (which is actually a "mount") look better
            _animationRate = 0.0f;
        }
        else {
            // just copy the rate from the mount
            _animationRate = _object.getHolder()->inst._animationRate;
        }

        return;
    }

    // if the animation is not a walking-type animation, ignore the variable animation rates
    // and the automatic determination of the walk animation
    // "dance" is walking with zero speed
    bool is_walk_type = (_currentAnimation < ACTION_DD) || ACTION_IS_TYPE( _currentAnimation, W );
    if ( !is_walk_type ) {
        return;
    }

    // for non-flying objects, you have to be touching the ground
    if (!_object.getObjectPhysics().isTouchingGround() && !_object.isFlying()) {
        return;
    }

    // set the character speed to zero
    float speed = 0.0f;

    // estimate our speed
    if ( _object.isFlying() )
    {
        // for flying objects, the speed is the actual speed
        speed = idlib::euclidean_norm(_object.getVelocity());
    }
    else
    {
        // For non-flying objects, we use the intended speed.
        speed = std::max(idlib::euclidean_norm(xy(_object.getVelocity())), idlib::euclidean_norm(_object.getObjectPhysics().getDesiredVelocity()));
        if (_object.getObjectPhysics().floorIsSlippy())
        {
            // The character is slipping as on ice.
            // Make his little legs move based on his intended speed, for comic effect! :)
            _animationRate = 2.0f;
            speed *= 2.0f;
        }

    }

    //Make bigger Objects have slower animations
    if ( _object.fat > 0.0f ) {
        speed /= _object.fat;  
    }

    //Find out which animation to use depending on movement speed
    ModelAction action = ACTION_DA;
    int lip = 0;
    if (speed <= 1.0f) {
        action = ACTION_DA;     //Stand still
    }
    else {
        if(_object.isStealthed() && getModelDescriptor()->isActionValid(ACTION_WA)) {
            action = ACTION_WA; //Sneak animation
            lip = LIPWA;
        }
        else {
            if(speed <= 4.0f && getModelDescriptor()->isActionValid(ACTION_WB)) {
                action = ACTION_WB; //Walk
                lip = LIPWB;
            }
            else {
                action = ACTION_WC; //Run
                lip = LIPWC;
            }

        }
    }

    // for flying characters, you have to flap like crazy to stand still and
    // do nothing to move quickly
    if ( _object.isFlying() )
    {
        switch(action)
        {
            case ACTION_DA: action = ACTION_WC; break;
            case ACTION_WA: action = ACTION_WB; break;
            case ACTION_WB: action = ACTION_WA; break;
            case ACTION_WC: action = ACTION_DA; break;
            default: /*don't modify action*/    break;
        }
    }

    if ( ACTION_DA == action )
    {
        // Do standstill

        // handle boredom
        _object.bore_timer--;
        if ( _object.bore_timer < 0 )
        {
            _object.resetBoredTimer();

            //Don't yell "im bored!" while stealthed!
            if(!_object.isStealthed())
            {
                SET_BIT( _object.ai.alert, ALERTIF_BORED );

                // set the action to "bored", which is ACTION_DB, ACTION_DC, or ACTION_DD
                int rand_val   = Random::next(std::numeric_limits<uint16_t>::max());
                ModelAction tmp_action = getModelDescriptor()->getAction(ACTION_DB + ( rand_val % 3 ));
                _object.inst.startAnimation(tmp_action, true, true );
            }
        }
        else
        {
            // if the current action is not ACTION_D* switch to ACTION_DA
            if (_currentAnimation > ACTION_DD)
            {
                // get an appropriate version of the idle action
                ModelAction tmp_action = getModelDescriptor()->getAction(ACTION_DA);

                // start the animation
                startAnimation(tmp_action, true, true );
            }
        }
    }
    else
    {
        ModelAction tmp_action = getModelDescriptor()->getAction(action);
        if ( ACTION_COUNT != tmp_action )
        {
            if ( _currentAnimation != tmp_action )
            {
                setAction(tmp_action, true, true);
                setFrame(getModelDescriptor()->getFrameLipToWalkFrame(lip, pinst.getNextFrame().framelip));
                startAnimation(tmp_action, true, true);
            }

            // "loop" the action
            _nextAnimation = tmp_action;
        }
    }

    //Limit final animation speed
    setAnimationSpeed(_animationRate);
}

bool ObjectGraphics::canBeInterrupted() const
{
    return _canBeInterrupted;    
}

bool ObjectGraphics::setAction(const ModelAction action, const bool action_ready, const bool override_action)
{
    // is the chosen action valid?
    if (!getModelDescriptor()->isActionValid(action)) {
        return false;
    }

    // are we going to check action_ready?
    if (!override_action && !_canBeInterrupted) {
        return false;
    }

    // set up the action
    _currentAnimation = action;
    _nextAnimation = ACTION_DA;
    _canBeInterrupted = action_ready;

    return true;
}

bool ObjectGraphics::setFrameFull(int frame_along, int ilip)
{
    // we have to have a valid action range
    if (_currentAnimation > ACTION_COUNT) {
        return false;
    }

    // try to heal a bad action
    _currentAnimation = getModelDescriptor()->getAction(_currentAnimation);

    // reject the action if it is cannot be made valid
    if (_currentAnimation == ACTION_COUNT) {
        return false;
    }

    // get some frame info
    int frame_stt   = getModelDescriptor()->getFirstFrame(_currentAnimation);
    int frame_end   = getModelDescriptor()->getLastFrame(_currentAnimation);
    int frame_count = 1 + ( frame_end - frame_stt );

    // try to heal an out of range value
    frame_along %= frame_count;

    // get the next frames
    int new_nxt = frame_stt + frame_along;
    new_nxt = std::min(new_nxt, frame_end);

    _targetFrameIndex = new_nxt;
    _animationProgressInteger = ilip;
    _animationProgress = _animationProgressInteger * 0.25f;

    // set the validity of the cache
    return true;
}

ModelAction ObjectGraphics::getCurrentAnimation() const
{
    return _currentAnimation;
}

void ObjectGraphics::removeInterpolation()
{
    if (_sourceFrameIndex != _targetFrameIndex ) {
        _sourceFrameIndex = _targetFrameIndex;
        _animationProgressInteger = 0;
        _animationProgress = 0.0f;
    }
}

oct_bb_t ObjectGraphics::getBoundingBox() const
{
    //Beginning of a frame animation
    if (_targetFrameIndex == _sourceFrameIndex || _animationProgress == 0.0f) {
        return _object.inst.getLastFrame().bb;
    } 

    //Finished frame animation
    if (_animationProgress == 1.0f) {
        return _object.inst.getNextFrame().bb;
    } 

    //We are middle between two animation frames
    return oct_bb_t::interpolate(_object.inst.getLastFrame().bb, _object.inst.getNextFrame().bb, _animationProgress);
}

} //namespace Graphics
} //namespace Ego
