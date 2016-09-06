#include "ModelGraphics.hpp"
#include "game/Entities/_Include.hpp"
#include "game/game.h"

// the flip tolerance is the default flip increment / 2
static constexpr float FLIP_TOLERANCE = 0.25f * 0.5f;

chr_instance_t::chr_instance_t(const Object &object) :
    matrix_cache(),

    facing_z(0),

    alpha(0xFF),
    light(0),
    sheen(0),

    colorshift(),

    uoffset(0),
    voffset(0),

    animationState(),
    actionState(),

    // linear interpolated frame vertices
    _object(object),
    _vertexList(),
    _matrix(Matrix4f4f::identity()),
    _reflectionMatrix(Matrix4f4f::identity()),

    // graphical optimizations
    _vertexCache(*this),

    // lighting info
    _ambientColour(0),
    _maxLight(-0xFF),
    _lastLightingUpdateFrame(-1)
{
    // initalize the character instance
    setObjectProfile(_object.getProfile());
}

chr_instance_t::~chr_instance_t() 
{
    //dtor
}

void chr_instance_t::updateLighting()
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

int chr_instance_t::getAmbientColour() const
{
    return _ambientColour;
}

oct_bb_t chr_instance_t::getBoundingBox() const
{
    //Beginning of a frame animation
    if (this->animationState.getTargetFrameIndex() == this->animationState.getSourceFrameIndex() || this->animationState.flip == 0.0f) {
        return getLastFrame().bb;
    } 

    //Finished frame animation
    if (this->animationState.flip == 1.0f) {
        return getNextFrame().bb;
    } 

    //We are middle between two animation frames
    return oct_bb_t::interpolate(getLastFrame().bb, getNextFrame().bb, this->animationState.flip);
}

gfx_rv chr_instance_t::needs_update(int vmin, int vmax, bool *verts_match, bool *frames_match)
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
	if (!_vertexCache.isValid()) {
		return gfx_success;
	}

    // get the last valid vertex from the chr_instance
    int maxvert = ((int)this->_vertexList.size()) - 1;

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

	bool flips_match = (std::abs(_vertexCache.flip - this->animationState.flip) < FLIP_TOLERANCE);

    *frames_match = (this->animationState.getTargetFrameIndex() == this->animationState.getSourceFrameIndex() && _vertexCache.frame_nxt == this->animationState.getTargetFrameIndex() && _vertexCache.frame_lst == this->animationState.getSourceFrameIndex() ) ||
                    (flips_match && _vertexCache.frame_nxt == this->animationState.getTargetFrameIndex() && _vertexCache.frame_lst == this->animationState.getSourceFrameIndex());

    return (!(*verts_match) || !( *frames_match )) ? gfx_success : gfx_fail;
}

void chr_instance_t::interpolateVerticesRaw(const std::vector<MD2_Vertex> &lst_ary, const std::vector<MD2_Vertex> &nxt_ary, int vmin, int vmax, float flip )
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

gfx_rv chr_instance_t::updateVertices(int vmin, int vmax, bool force)
{
    bool vertices_match, frames_match;
    float  loc_flip;

    int vdirty1_min = -1, vdirty1_max = -1;
    int vdirty2_min = -1, vdirty2_max = -1;

    // get the model
    const std::shared_ptr<MD2Model> &pmd2 = this->animationState.getModelDescriptor()->getMD2();

    // make sure we have valid data
    if (_vertexList.size() != pmd2->getVertexCount())
    {
		Log::get().warn( "chr_instance_update_vertices() - character instance vertex data does not match its md2\n" );
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
    const std::vector<MD2_Frame> &frameList = pmd2->getFrames();
    if ( this->animationState.getTargetFrameIndex() >= frameList.size() || this->animationState.getSourceFrameIndex() >= frameList.size() )
    {
		Log::get().warn("%s:%d:%s: character instance frame is outside the range of its MD2\n", __FILE__, __LINE__, __FUNCTION__ );
        return gfx_error;
    }

    // grab the frame data from the correct model
    const MD2_Frame &nextFrame = frameList[this->animationState.getTargetFrameIndex()];
    const MD2_Frame &lastFrame = frameList[this->animationState.getSourceFrameIndex()];

    // fix the flip for objects that are not animating
    loc_flip = this->animationState.flip;
    if ( this->animationState.getTargetFrameIndex() == this->animationState.getSourceFrameIndex() ) {
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

gfx_rv chr_instance_t::updateVertexCache(int vmax, int vmin, bool force, bool vertices_match, bool frames_match)
{
    // this is getting a bit ugly...
    // we need to do this calculation as little as possible, so it is important that the
    // _vertexCache.* values be tested and stored properly

	int maxvert = ((int)this->_vertexList.size()) - 1;

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

    _vertexCache.frame_nxt = this->animationState.getTargetFrameIndex();
    _vertexCache.frame_lst = this->animationState.getSourceFrameIndex();
    _vertexCache.flip      = this->animationState.flip;

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

bool chr_instance_t::updateGripVertices(const uint16_t vrt_lst[], const size_t vrt_count)
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

gfx_rv chr_instance_t::setAction(const ModelAction action, const bool action_ready, const bool override_action)
{
    // is the chosen action valid?
	if (!animationState.getModelDescriptor()->isActionValid(action)) {
		return gfx_fail;
	}

    // are we going to check action_ready?
	if (!override_action && !actionState.action_ready) {
		return gfx_fail;
	}

    // set up the action
	actionState.action_which = action;
	actionState.action_next = ACTION_DA;
	actionState.action_ready = action_ready;

    return gfx_success;
}

gfx_rv chr_instance_t::setFrame(int frame)
{
    if (this->actionState.action_which < 0 || this->actionState.action_which > ACTION_COUNT) {
        gfx_error_add(__FILE__, __FUNCTION__, __LINE__, this->actionState.action_which, "invalid action range");
        return gfx_error;
    }

    // is the frame within the valid range for this action?
    if(!this->animationState.getModelDescriptor()->isFrameValid(this->actionState.action_which, frame)) {
        return gfx_fail;
    }

    // jump to the next frame
	this->animationState.flip = 0.0f;
	this->animationState.ilip = 0;
	this->animationState.setSourceFrameIndex(this->animationState.getTargetFrameIndex());
	this->animationState.setTargetFrameIndex(frame);

    return gfx_success;
}

gfx_rv chr_instance_t::startAnimation(const ModelAction action, const bool action_ready, const bool override_action)
{
    gfx_rv retval = setAction(action, action_ready, override_action);
    if ( rv_success != retval ) return retval;

    retval = setFrame(animationState.getModelDescriptor()->getFirstFrame(action));
    if ( rv_success != retval ) return retval;

    return gfx_success;
}

gfx_rv chr_instance_t::incrementAction()
{
    // get the correct action
	ModelAction action = animationState.getModelDescriptor()->getAction(actionState.action_next);

    // determine if the action is one of the types that can be broken at any time
    // D == "dance" and "W" == walk
    // @note ZF> Can't use ACTION_IS_TYPE(action, D) because of GCC compile warning
    bool action_ready = action < ACTION_DD || ACTION_IS_TYPE(action, W);

	return startAnimation(action, action_ready, true);
}

gfx_rv chr_instance_t::incrementFrame(const ObjectRef imount, const ModelAction mount_action)
{
    // fix the ilip and flip
	this->animationState.ilip = this->animationState.ilip % 4;
	this->animationState.flip = fmod(this->animationState.flip, 1.0f);

    // Change frames
	int frame_lst = this->animationState.getTargetFrameIndex();
	int frame_nxt = this->animationState.getTargetFrameIndex() + 1;

    // detect the end of the animation and handle special end conditions
	if (frame_nxt > this->animationState.getModelDescriptor()->getLastFrame(this->actionState.action_which))
    {
		if (this->actionState.action_keep)
        {
            // Freeze that animation at the last frame
            frame_nxt = frame_lst;

            // Break a kept action at any time
			this->actionState.action_ready = true;
        }
		else if (this->actionState.action_loop)
        {
            // Convert the action into a riding action if the character is mounted
            if (_currentModule->getObjectHandler().exists(imount))
            {
				startAnimation(mount_action, true, true);
            }

            // set the frame to the beginning of the action
			frame_nxt = this->animationState.getModelDescriptor()->getFirstFrame(this->actionState.action_which);

            // Break a looped action at any time
			this->actionState.action_ready = true;
        }
        else
        {
            // Go on to the next action. don't let just anything interrupt it?
			incrementAction();

            // incrementAction() actually sets this value properly. just grab the new value.
			frame_nxt = this->animationState.getTargetFrameIndex();
        }
    }

	this->animationState.setSourceFrameIndex(frame_lst);
	this->animationState.setTargetFrameIndex(frame_nxt);


    return gfx_success;
}

gfx_rv chr_instance_t::playAction(const ModelAction action, const bool action_ready)
{
    return startAnimation(animationState.getModelDescriptor()->getAction(action), action_ready, true);
}

void chr_instance_t::clearCache()
{
    /// @author BB
    /// @details force chr_instance_update_vertices() recalculate the vertices the next time
    ///     the function is called

    _vertexCache.clear();
    this->matrix_cache = matrix_cache_t();

    _lastLightingUpdateFrame = -1;
}

int chr_instance_t::getMaxLight() const
{
    return _maxLight;    
}

const GLvertex& chr_instance_t::getVertex(const size_t index) const
{
    return _vertexList[index];
}

bool chr_instance_t::setModel(const std::shared_ptr<Ego::ModelDescriptor> &model)
{
    bool updated = false;

    if (this->animationState.getModelDescriptor() != model) {
        updated = true;
        this->animationState.setModelDescriptor(model);
    }

    // set the vertex size
    size_t vlst_size = this->animationState.getModelDescriptor()->getMD2()->getVertexCount();
    if (this->_vertexList.size() != vlst_size) {
        updated = true;
        this->_vertexList.resize(vlst_size);
    }

    // set the frames to frame 0 of this object's data
    if (0 != this->animationState.getTargetFrameIndex() || 0 != this->animationState.getSourceFrameIndex()) {
        updated = true;
        this->animationState.setSourceFrameIndex(0);
        this->animationState.setTargetFrameIndex(0);
    }

    if (updated) {
        // update the vertex and lighting cache
        clearCache();
        updateVertices(-1, -1, true);
    }

    return updated;
}

const Matrix4f4f& chr_instance_t::getMatrix() const
{
    return _matrix;
}

const Matrix4f4f& chr_instance_t::getReflectionMatrix() const
{
    return _reflectionMatrix;
}

uint8_t chr_instance_t::getReflectionAlpha() const
{
    // determine the reflection alpha based on altitude above the mesh
    const float altitudeAboveGround = std::max(0.0f, _object.getPosZ() - _object.getFloorElevation());
    float alphaFade = (255.0f - altitudeAboveGround)*0.5f;
    alphaFade = Ego::Math::constrain(alphaFade, 0.0f, 255.0f);

    return this->alpha * alphaFade * INV_FF<float>();
}

void chr_instance_t::setObjectProfile(const std::shared_ptr<ObjectProfile> &profile)
{
    //Reset data
    // Remember any previous color shifts in case of lasting enchantments
    _matrix = Matrix4f4f::identity();
    _reflectionMatrix = Matrix4f4f::identity();
    facing_z = 0;
    uoffset = 0;
    voffset = 0;
    animationState = AnimationState();
    actionState = ActionState();
    _ambientColour = 0;
    _maxLight = -0xFF;
    _vertexList.clear();
    clearCache();

    // lighting parameters
	this->alpha = profile->getAlpha();
	this->light = profile->getLight();
	this->sheen = profile->getSheen();

    // model parameters
    setModel(profile->getModel());

    // set the initial action, all actions override it
    playAction(ACTION_DA, true);
}

BIT_FIELD chr_instance_t::getFrameFX() const
{
    return getNextFrame().framefx;
}

gfx_rv chr_instance_t::setFrameFull(int frame_along, int ilip)
{
    // handle optional parameters
	const std::shared_ptr<Ego::ModelDescriptor> &imad = this->animationState.getModelDescriptor();

    // we have to have a valid action range
	if (this->actionState.action_which > ACTION_COUNT) {
		return gfx_fail;
	}

    // try to heal a bad action
    this->actionState.action_which = imad->getAction(this->actionState.action_which);

    // reject the action if it is cannot be made valid
	if (this->actionState.action_which == ACTION_COUNT) {
		return gfx_fail;
	}

    // get some frame info
    int frame_stt   = imad->getFirstFrame(this->actionState.action_which);
    int frame_end   = imad->getLastFrame(this->actionState.action_which);
    int frame_count = 1 + ( frame_end - frame_stt );

    // try to heal an out of range value
    frame_along %= frame_count;

    // get the next frames
    int new_nxt = frame_stt + frame_along;
    new_nxt = std::min(new_nxt, frame_end);

    this->animationState.setTargetFrameIndex(new_nxt);
    this->animationState.ilip      = ilip;
    this->animationState.flip      = ilip * 0.25f;

    // set the validity of the cache

    return gfx_success;
}

void chr_instance_t::setActionKeep(bool val) {
	actionState.action_keep = val;
}

void chr_instance_t::setActionReady(bool val) {
    actionState.action_ready = val;
}

void chr_instance_t::setActionLooped(bool val) {
    actionState.action_loop = val;
}

void chr_instance_t::setNextAction(const ModelAction val) {
    actionState.action_next = val;
}

void chr_instance_t::removeInterpolation()
{
    if (this->animationState.getSourceFrameIndex() != this->animationState.getTargetFrameIndex() ) {
		this->animationState.setSourceFrameIndex(this->animationState.getTargetFrameIndex());
		this->animationState.ilip = 0;
		this->animationState.flip = 0.0f;


    }
}

const MD2_Frame& chr_instance_t::getNextFrame() const
{
    return animationState.getTargetFrame();
}

const MD2_Frame& chr_instance_t::getLastFrame() const
{
    return animationState.getSourceFrame();
}

void chr_instance_t::updateOneLip() {
    this->animationState.ilip += 1;
    this->animationState.flip = 0.25f * this->animationState.ilip;

}

bool chr_instance_t::isVertexCacheValid() const
{
    return _vertexCache.isValid();
}

bool chr_instance_t::updateOneFlip(const float dflip)
{
	if (0.0f == dflip) {
		return false;
	}

    // update the lips
    this->animationState.flip += dflip;
	this->animationState.ilip = ((int)std::floor(this->animationState.flip * 4)) % 4;


    return true;
}

float chr_instance_t::getRemainingFlip() const
{
	return (this->animationState.ilip + 1) * 0.25f - this->animationState.flip;
}

void chr_instance_t::getTint(GLXvector4f tint, const bool reflection, const int type)
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
            local_light = this->light * local_alpha * INV_FF<float>();
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
            tint[AA] = local_alpha * INV_FF<float>();

            // alpha characters are blended onto the canvas by adding their color
            // the more black the colors, the less visible the character
            // the alpha channel is not important
            tint[RR] = local_light * INV_FF<float>() / (1 << local_colorshift.red);
            tint[GG] = local_light * INV_FF<float>() / (1 << local_colorshift.green);
            tint[BB] = local_light * INV_FF<float>() / (1 << local_colorshift.blue);
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

bool VertexListCache::isValid() const
{
    if (_instance.animationState.getSourceFrameIndex() != frame_nxt) {
        return false;
    }

    if (_instance.animationState.getSourceFrameIndex() != frame_lst) {
        return false;
    }

    if ((_instance.animationState.getSourceFrameIndex() != frame_lst) && std::abs(_instance.animationState.flip - flip) > FLIP_TOLERANCE) {
        return false;
    }

    return true;
}

void chr_instance_t::flash(uint8_t value)
{
	const float flash_val = value * INV_FF<float>();

	// flash the ambient color
	_ambientColour = flash_val;

	// flash the directional lighting
	for (size_t i = 0; i < _vertexList.size(); ++i) {
		_vertexList[i].color_dir = flash_val;
	}
}


size_t chr_instance_t::getVertexCount() const
{
    return _vertexList.size();
}

void chr_instance_t::flashVariableHeight(const uint8_t valuelow, const int16_t low, const uint8_t valuehigh, const int16_t high)
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

void chr_instance_t::setMatrix(const Matrix4f4f& matrix)
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

bool matrix_cache_t::isValid() const {
    return valid && matrix_valid;
}
