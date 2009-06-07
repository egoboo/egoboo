#include "ogl_debug.h"

#include <stdio.h>
#include <assert.h>

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
char * next_cmd = NULL;
int    next_line = -1;
char * next_file = "BAD FILE";

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void handle_gl_error()
{
    GLint err = glGetError();
    if (GL_NO_ERROR != err)
    {
        const char * err_str = gluErrorString(err);
        fprintf(stdout, "%s (\"%s\" - %d)- %s\n", next_cmd, next_file, next_line, err_str );
        assert(0);
    }
};

//------------------------------------------------------------------------------
void gl_grab_texture_state(GLenum target, GLint level, gl_texture_t * pi)
{
    // which texture id
    if (GL_TEXTURE_1D == target)
    {
        pi->target = GL_TEXTURE_1D;
        glGetIntegerv(GL_TEXTURE_BINDING_1D, &pi->binding);
    }
    else if (GL_TEXTURE_2D == target)
    {
        pi->target = GL_TEXTURE_2D;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &pi->binding);
    }

    // basic parameters
    glGetTexParameterfv(target, GL_TEXTURE_PRIORITY,     &pi->priority );
    glGetTexParameteriv(target, GL_TEXTURE_RESIDENT,     &pi->resident );
    glGetTexParameteriv(target, GL_TEXTURE_WRAP_S,       &pi->wrap_s );
    glGetTexParameteriv(target, GL_TEXTURE_WRAP_T,       &pi->wrap_t );
    glGetTexParameteriv(target, GL_TEXTURE_MIN_FILTER,   &pi->min_filter );
    glGetTexParameteriv(target, GL_TEXTURE_MAG_FILTER,   &pi->mag_filter );
    glGetTexParameterfv(target, GL_TEXTURE_BORDER_COLOR,  pi->border_color );

    // format
    glGetTexLevelParameterfv(target, level, GL_TEXTURE_WIDTH,           &pi->width );
    glGetTexLevelParameterfv(target, level, GL_TEXTURE_HEIGHT,          &pi->height );
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_INTERNAL_FORMAT, &pi->internal_format );
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_RED_SIZE,        &pi->red_size );
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_GREEN_SIZE,      &pi->green_size );
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_BLUE_SIZE,       &pi->blue_size );
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_ALPHA_SIZE,      &pi->alpha_size );
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_LUMINANCE_SIZE,  &pi->luminance_size );
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_INTENSITY_SIZE,  &pi->intensity_size );
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_BORDER,          &pi->border );
}

//------------------------------------------------------------------------------
void gl_grab_unpacking_state(gl_packing_t * pp)
{
    glGetBooleanv(GL_UNPACK_SWAP_BYTES,  pp->swap_bytes);
    glGetBooleanv(GL_UNPACK_LSB_FIRST,   pp->lsb_first);
    glGetIntegerv(GL_UNPACK_ROW_LENGTH,  pp->row_length);
    glGetIntegerv(GL_UNPACK_SKIP_ROWS,   pp->skip_rows);
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, pp->skip_pixels);
    glGetIntegerv(GL_UNPACK_ALIGNMENT,   pp->alignment);
};

//------------------------------------------------------------------------------
void gl_grab_packing_state(gl_packing_t * pp)
{
    glGetBooleanv(GL_PACK_SWAP_BYTES,  pp->swap_bytes);
    glGetBooleanv(GL_PACK_LSB_FIRST,   pp->lsb_first);
    glGetIntegerv(GL_PACK_ROW_LENGTH,  pp->row_length);
    glGetIntegerv(GL_PACK_SKIP_ROWS,   pp->skip_rows);
    glGetIntegerv(GL_PACK_SKIP_PIXELS, pp->skip_pixels);
    glGetIntegerv(GL_PACK_ALIGNMENT,   pp->alignment);
};

//------------------------------------------------------------------------------
void gl_grab_blend_state(gl_blend_t * pb)
{
    pb->enabled = glIsEnabled(GL_BLEND);
    if (pb->enabled)
    {
        glGetIntegerv(GL_BLEND_SRC, pb->src);
        switch (pb->src[0])
        {
            case GL_ZERO:                pb->src_sz = "GL_ZERO";                break;
            case GL_ONE:                 pb->src_sz = "GL_ONE";                 break;
            case GL_DST_COLOR:           pb->src_sz = "GL_DST_COLOR";           break;
            case GL_SRC_COLOR:           pb->src_sz = "GL_SRC_COLOR";           break;
            case GL_ONE_MINUS_DST_COLOR: pb->src_sz = "GL_ONE_MINUS_DST_COLOR"; break;
            case GL_ONE_MINUS_SRC_COLOR: pb->src_sz = "GL_ONE_MINUS_SRC_COLOR"; break;
            case GL_SRC_ALPHA:           pb->src_sz = "GL_SRC_ALPHA";           break;
            case GL_ONE_MINUS_SRC_ALPHA: pb->src_sz = "GL_ONE_MINUS_SRC_ALPHA"; break;
            case GL_DST_ALPHA:           pb->src_sz = "GL_DST_ALPHA";           break;
            case GL_ONE_MINUS_DST_ALPHA: pb->src_sz = "GL_ONE_MINUS_DST_ALPHA"; break;
            case GL_SRC_ALPHA_SATURATE:  pb->src_sz = "GL_SRC_ALPHA_SATURATE";  break;
            default:                     pb->src_sz = "UNKNOWN";
        };

        glGetIntegerv(GL_BLEND_DST, pb->dst);
        switch (pb->dst[0])
        {
            case GL_ZERO:                pb->dst_sz = "GL_ZERO";                break;
            case GL_ONE:                 pb->dst_sz = "GL_ONE";                 break;
            case GL_DST_COLOR:           pb->dst_sz = "GL_DST_COLOR";           break;
            case GL_SRC_COLOR:           pb->dst_sz = "GL_SRC_COLOR";           break;
            case GL_ONE_MINUS_DST_COLOR: pb->dst_sz = "GL_ONE_MINUS_DST_COLOR"; break;
            case GL_ONE_MINUS_SRC_COLOR: pb->dst_sz = "GL_ONE_MINUS_SRC_COLOR"; break;
            case GL_SRC_ALPHA:           pb->dst_sz = "GL_SRC_ALPHA";           break;
            case GL_ONE_MINUS_SRC_ALPHA: pb->dst_sz = "GL_ONE_MINUS_SRC_ALPHA"; break;
            case GL_DST_ALPHA:           pb->dst_sz = "GL_DST_ALPHA";           break;
            case GL_ONE_MINUS_DST_ALPHA: pb->dst_sz = "GL_ONE_MINUS_DST_ALPHA"; break;
            case GL_SRC_ALPHA_SATURATE:  pb->dst_sz = "GL_SRC_ALPHA_SATURATE";  break;
            default:                     pb->dst_sz = "UNKNOWN";
        };
    };

}

//------------------------------------------------------------------------------
void gl_grab_alpha_state(gl_alpha_t * pa)
{
    pa->test_enabled = glIsEnabled(GL_ALPHA_TEST);

    if (GL_TRUE == pa->test_enabled)
    {
        glGetIntegerv(GL_ALPHA_TEST_FUNC, pa->test_func);
        switch (pa->test_func[0])
        {
            case GL_NEVER:    pa->test_func_sz = "GL_NEVER";  break;
            case GL_ALWAYS:   pa->test_func_sz = "GL_ALWAYS"; break;
            case GL_LESS:     pa->test_func_sz = "GL_LESS";   break;
            case GL_LEQUAL:   pa->test_func_sz = "GL_LEQUAL"; break;
            case GL_EQUAL:    pa->test_func_sz = "GL_EQUAL";  break;
            case GL_GEQUAL:   pa->test_func_sz = "GL_GEQUAL"; break;
            case GL_GREATER:  pa->test_func_sz = "GL_EQUAL";  break;
            case GL_NOTEQUAL: pa->test_func_sz = "GL_GEQUAL"; break;
            default:          pa->test_func_sz = "UNKNOWN";
        };

        glGetIntegerv(GL_ALPHA_TEST_REF, pa->test_ref);
    };

    glGetFloatv(GL_ALPHA_SCALE, pa->scale);
    glGetFloatv(GL_ALPHA_BIAS,  pa->bias);
}

//------------------------------------------------------------------------------
void gl_grab_stencil_state( gl_stencil_t * ps )
{
    ps->test_enabled = glIsEnabled(GL_STENCIL_TEST);
    if (ps->test_enabled)
    {
        glGetIntegerv(GL_STENCIL_FUNC, ps->test_func);
        switch (ps->test_func[0])
        {
            case GL_NEVER:    ps->test_func_sz = "GL_NEVER"; break;
            case GL_ALWAYS:   ps->test_func_sz = "GL_ALWAYS"; break;
            case GL_LESS:     ps->test_func_sz = "GL_LESS"; break;
            case GL_LEQUAL:   ps->test_func_sz = "GL_LEQUAL"; break;
            case GL_EQUAL:    ps->test_func_sz = "GL_EQUAL"; break;
            case GL_GEQUAL:   ps->test_func_sz = "GL_GEQUAL"; break;
            case GL_GREATER:  ps->test_func_sz = "GL_EQUAL"; break;
            case GL_NOTEQUAL: ps->test_func_sz = "GL_GEQUAL"; break;
            default:          ps->test_func_sz = "UNKNOWN";
        };
        glGetIntegerv(GL_STENCIL_REF, ps->test_ref);

        glGetIntegerv(GL_STENCIL_FAIL, ps->fail);
        switch (ps->fail[0])
        {
            case GL_KEEP:    ps->fail_sz = "GL_KEEP";    break;
            case GL_ZERO:    ps->fail_sz = "GL_ZERO";    break;
            case GL_REPLACE: ps->fail_sz = "GL_REPLACE"; break;
            case GL_INCR:    ps->fail_sz = "GL_INCR";    break;
            case GL_DECR:    ps->fail_sz = "GL_DECR";    break;
            case GL_INVERT:  ps->fail_sz = "GL_INVERT";  break;
            default:         ps->fail_sz = "UNKNOWN";
        };

        glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, ps->pass_depth_fail);
        switch (ps->pass_depth_fail[0])
        {
            case GL_KEEP:    ps->pass_depth_fail_sz = "GL_KEEP"; break;
            case GL_ZERO:    ps->pass_depth_fail_sz = "GL_ZERO"; break;
            case GL_REPLACE: ps->pass_depth_fail_sz = "GL_REPLACE"; break;
            case GL_INCR:    ps->pass_depth_fail_sz = "GL_INCR"; break;
            case GL_DECR:    ps->pass_depth_fail_sz = "GL_DECR"; break;
            case GL_INVERT:  ps->pass_depth_fail_sz = "GL_INVERT"; break;
            default:         ps->pass_depth_fail_sz = "UNKNOWN";
        };

        glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, ps->pass_depth_pass);
        switch (ps->pass_depth_pass[0])
        {
            case GL_KEEP:    ps->pass_depth_pass_sz = "GL_KEEP";    break;
            case GL_ZERO:    ps->pass_depth_pass_sz = "GL_ZERO";    break;
            case GL_REPLACE: ps->pass_depth_pass_sz = "GL_REPLACE"; break;
            case GL_INCR:    ps->pass_depth_pass_sz = "GL_INCR";    break;
            case GL_DECR:    ps->pass_depth_pass_sz = "GL_DECR";    break;
            case GL_INVERT:  ps->pass_depth_pass_sz = "GL_INVERT";  break;
            default:         ps->pass_depth_pass_sz = "UNKNOWN";
        };

    }

    glGetIntegerv(GL_STENCIL_VALUE_MASK, ps->value_mask);

    glGetIntegerv(GL_STENCIL_WRITEMASK, ps->writemask);
    glGetIntegerv(GL_STENCIL_CLEAR_VALUE, ps->clear_value);

}

//------------------------------------------------------------------------------
void gl_grab_clip_plane_state( gl_clip_plane_t * ps, GLint which )
{
    ps->enabled = glIsEnabled(GL_CLIP_PLANE0 + which);
    glGetClipPlane(GL_CLIP_PLANE0 + which, ps->coeffs);
}

//------------------------------------------------------------------------------
void gl_grab_depth_state( gl_depth_t * pd )
{
    pd->test_enabled = glIsEnabled(GL_DEPTH_TEST);
    if (GL_TRUE == pd->test_enabled)
    {
        glGetIntegerv(GL_DEPTH_FUNC, pd->test_func);
        switch (pd->test_func[0])
        {
            case GL_NEVER:    pd->test_func_sz = "GL_NEVER";  break;
            case GL_ALWAYS:   pd->test_func_sz = "GL_ALWAYS"; break;
            case GL_LESS:     pd->test_func_sz = "GL_LESS";   break;
            case GL_LEQUAL:   pd->test_func_sz = "GL_LEQUAL"; break;
            case GL_EQUAL:    pd->test_func_sz = "GL_EQUAL";  break;
            case GL_GEQUAL:   pd->test_func_sz = "GL_GEQUAL"; break;
            case GL_GREATER:  pd->test_func_sz = "GL_EQUAL";  break;
            case GL_NOTEQUAL: pd->test_func_sz = "GL_GEQUAL"; break;
            default:          pd->test_func_sz = "UNKNOWN";
        };
    };

    glGetBooleanv(GL_DEPTH_WRITEMASK, pd->writemask_enabled);
    glGetFloatv(GL_DEPTH_CLEAR_VALUE, pd->clear_value);

    glGetFloatv(GL_DEPTH_RANGE, pd->range);
    glGetFloatv(GL_DEPTH_SCALE, pd->scale);
    glGetFloatv(GL_DEPTH_BIAS,  pd->bias);
};

//------------------------------------------------------------------------------
void gl_grab_texture_gen_state( gl_texture_gen_t * pt, GLenum coord )
{
    glGetTexGenfv(coord, GL_EYE_PLANE, pt->eye_plane);
    glGetTexGenfv(coord, GL_OBJECT_PLANE, pt->object_plane);
    glGetTexGenfv(coord, GL_OBJECT_PLANE, pt->object_plane);
    glGetTexGeniv(coord, GL_TEXTURE_GEN_MODE, pt->mode);
};

//------------------------------------------------------------------------------
void gl_grab_matrix_state( gl_matrix_t * pm )
{
    glGetIntegerv(GL_MODELVIEW_STACK_DEPTH, pm->modelview_stack_depth);
    glGetFloatv(GL_MODELVIEW_MATRIX, pm->modelview_matrix);

    glGetIntegerv(GL_PROJECTION_STACK_DEPTH, pm->projection_stack_depth);
    glGetFloatv(GL_PROJECTION_MATRIX, pm->projection_matrix);

    glGetIntegerv(GL_TEXTURE_STACK_DEPTH, pm->texture_stack_depth);
    glGetFloatv(GL_TEXTURE_MATRIX, pm->texture_matrix);

    glGetIntegerv(GL_MATRIX_MODE, pm->mode);
    switch (pm->mode[0])
    {
        case GL_MODELVIEW : pm->mode_sz = "GL_MODELVIEW";  break;
        case GL_PROJECTION: pm->mode_sz = "GL_PROJECTION"; break;
        case GL_TEXTURE   : pm->mode_sz = "GL_TEXTURE";    break;
        default:            pm->mode_sz = "UNKNOWN";
    };
}

//------------------------------------------------------------------------------
void gl_grab_attrib_state( gl_attrib_t * pa )
{
    glGetIntegerv(GL_CLIENT_ATTRIB_STACK_DEPTH, pa->client_stack_depth);
    glGetIntegerv(GL_ATTRIB_STACK_DEPTH, pa->stack_depth);
}

//------------------------------------------------------------------------------
void gl_grab_mapping_state( gl_mapping_t * pm )
{
    glGetBooleanv(GL_MAP_COLOR, pm->map_color);
    glGetBooleanv(GL_MAP_STENCIL, pm->map_stencil);
}

//------------------------------------------------------------------------------
void gl_grab_state( ogl_state_t * ps )
{
    ogl_state_t      tmp_state;
    ogl_state_comp_t tmp_cmp;

    if (NULL == ps) return;

    memset( ps, 0, sizeof(ogl_state_t) );
    memcpy( &tmp_state, ps, sizeof(ogl_state_t) );

    gl_grab_texturing_state( &ps->texturing );
    gl_comp_state(&tmp_cmp, &tmp_state, ps);
    memcpy( &tmp_state, ps, sizeof(ogl_state_t) );

    gl_grab_clipping_state( &ps->clipping );
    gl_comp_state(&tmp_cmp, &tmp_state, ps);
    memcpy( &tmp_state, ps, sizeof(ogl_state_t) );

    gl_grab_render_options_state( &ps->options );
    gl_comp_state(&tmp_cmp, &tmp_state, ps);
    memcpy( &tmp_state, ps, sizeof(ogl_state_t) );

    gl_grab_mapping_state( &ps->mapping );
    gl_comp_state(&tmp_cmp, &tmp_state, ps);
    memcpy( &tmp_state, ps, sizeof(ogl_state_t) );

    gl_grab_unpacking_state( &ps->unpack );
    gl_comp_state(&tmp_cmp, &tmp_state, ps);
    memcpy( &tmp_state, ps, sizeof(ogl_state_t) );

    gl_grab_packing_state( &ps->pack );
    gl_comp_state(&tmp_cmp, &tmp_state, ps);
    memcpy( &tmp_state, ps, sizeof(ogl_state_t) );

    gl_grab_matrix_state( &ps->matrix );
    gl_comp_state(&tmp_cmp, &tmp_state, ps);
    memcpy( &tmp_state, ps, sizeof(ogl_state_t) );

    gl_grab_attrib_state( &ps->attrib );
    gl_comp_state(&tmp_cmp, &tmp_state, ps);
    memcpy( &tmp_state, ps, sizeof(ogl_state_t) );

    gl_grab_logic_op_state( &ps->logic_op );
    gl_comp_state(&tmp_cmp, &tmp_state, ps);
    memcpy( &tmp_state, ps, sizeof(ogl_state_t) );

    // ---- zoom                    ";
    glGetFloatv(GL_ZOOM_X, ps->zoom_x);
    glGetFloatv(GL_ZOOM_Y, ps->zoom_y);

    ps->error_value = glGetError();
    if (GL_NO_ERROR != ps->error_value)
    {
        ps->error_value_sz = gluErrorString(ps->error_value);
    }

};

//------------------------------------------------------------------------------
void gl_comp_state( ogl_state_comp_t * pcomp, ogl_state_t * ps1, ogl_state_t * ps2 )
{
    // compare the entire state
    pcomp->state     = ( 0 != memcmp(  ps1,            ps2,          sizeof(ogl_state_t ) ) );

    // compare the sub states
    pcomp->texturing = ( 0 != memcmp( &ps1->texturing, &ps2->texturing,   sizeof(gl_texturing_t) ) );
    pcomp->clipping  = ( 0 != memcmp( &ps1->clipping,  &ps2->clipping,    sizeof(gl_clipping_t) ) );
    pcomp->options   = ( 0 != memcmp( &ps1->options,  &ps2->options,      sizeof(gl_render_options_t) ) ); ;
    pcomp->mapping   = ( 0 != memcmp( &ps1->mapping,  &ps2->mapping,      sizeof(gl_mapping_t) ) ); ;

    pcomp->pack      = ( 0 != memcmp( &ps1->pack,    &ps2->pack,    sizeof(gl_packing_t) ) );
    pcomp->unpack    = ( 0 != memcmp( &ps1->unpack,  &ps2->unpack,  sizeof(gl_packing_t) ) );

    pcomp->matrix    = ( 0 != memcmp( &ps1->matrix,  &ps2->matrix,      sizeof(gl_matrix_t) ) ); ;;
    pcomp->attrib    = ( 0 != memcmp( &ps1->attrib,  &ps2->attrib,      sizeof(gl_attrib_t) ) ); ;;
    pcomp->logic_op  = ( 0 != memcmp( &ps1->logic_op,  &ps2->logic_op,      sizeof(gl_logic_op_t) ) ); ;;
}

//------------------------------------------------------------------------------
void gl_grab_render_options_state(gl_render_options_t * po)
{
    gl_grab_blend_state( &po->blend );
    gl_grab_alpha_state( &po->alpha );

    po->line.smooth = glIsEnabled(GL_LINE_SMOOTH);
    po->polygon.smooth = glIsEnabled(GL_POLYGON_SMOOTH);

    glGetIntegerv(GL_RENDER_MODE, po->mode.mode);
    switch (po->mode.mode[0])
    {
        case GL_RENDER:   po->mode.mode_sz = "GL_RENDER";   break;
        case GL_SELECT:   po->mode.mode_sz = "GL_SELECT";   break;
        case GL_FEEDBACK: po->mode.mode_sz = "GL_FEEDBACK"; break;
        default:          po->mode.mode_sz = "UNKNOWN";
    };

    //GL_LIST_BASE Setting of glListBase() list 0 glGetIntegerv()
    //GL_LIST_INDEX Number of display list  under construction; 0 if none - 0 glGetIntegerv()
    //GL_LIST_MODE Mode of display list under construction; undefined if none - 0 glGetIntegerv()

    if ( GL_SELECT == po->mode.mode[0] )
    {
        glGetIntegerv(GL_NAME_STACK_DEPTH,         po->mode.selection.name_stack_depth);
        glGetPointerv(GL_SELECTION_BUFFER_POINTER, po->mode.selection.buffer_pointer);
        glGetIntegerv(GL_SELECTION_BUFFER_SIZE,    po->mode.selection.buffer_size);
    }

    if ( GL_FEEDBACK == po->mode.mode[0] )
    {
        glGetPointerv(GL_FEEDBACK_BUFFER_POINTER, po->mode.feedback.buffer_pointer );
        glGetIntegerv(GL_FEEDBACK_BUFFER_SIZE,    po->mode.feedback.buffer_size    );
        glGetIntegerv(GL_FEEDBACK_BUFFER_TYPE,    po->mode.feedback.buffer_type    );
    }

    glGetIntegerv(GL_POLYGON_MODE, po->polygon.mode);
    switch (po->polygon.mode[0])
    {
        case GL_FRONT_AND_BACK: po->polygon.mode_sz = "GL_FRONT_AND_BACK"; break;
        case GL_FRONT:          po->polygon.mode_sz = "GL_FRONT";          break;
        case GL_BACK:           po->polygon.mode_sz = "GL_BACK";           break;
        case GL_POINT:          po->polygon.mode_sz = "GL_POINT";          break;
        case GL_LINE:           po->polygon.mode_sz = "GL_LINE";           break;
        case GL_FILL:           po->polygon.mode_sz = "GL_FILL";           break;
        default:                po->polygon.mode_sz = "UNKNOWN";           break;
    };

    glGetFloatv(GL_POINT_SIZE, po->point.size);
    glGetFloatv(GL_LINE_WIDTH, po->line.width);
    glGetIntegerv(GL_LINE_STIPPLE_PATTERN, po->line.stipple_pattern);
    glGetIntegerv(GL_LINE_STIPPLE_REPEAT, po->line.stipple_repeat);
    po->line.stipple_enabled = glIsEnabled(GL_LINE_STIPPLE );

    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, po->polygon.offset_factor);
#ifdef GL_POLYGON_OFFSET_BIAS
    glGetFloatv(GL_POLYGON_OFFSET_BIAS, po->polygon.offset_bias);
#endif
    po->polygon.offset_point_enabled = glIsEnabled(GL_POLYGON_OFFSET_POINT);
    po->polygon.offset_line_enabled = glIsEnabled(GL_POLYGON_OFFSET_LINE);
    po->polygon.offset_fill_enabled = glIsEnabled(GL_POLYGON_OFFSET_FILL);
    po->polygon.stipple_enabled = glIsEnabled(GL_POLYGON_STIPPLE);

    glGetIntegerv(GL_POINT_SMOOTH_HINT, po->point.smooth_hint);
    switch (po->point.smooth_hint[0])
    {
        case GL_FASTEST:   po->point.smooth_hint_sz = "GL_FASTEST";   break;
        case GL_NICEST:    po->point.smooth_hint_sz = "GL_NICEST";    break;
        case GL_DONT_CARE: po->point.smooth_hint_sz = "GL_DONT_CARE"; break;
        default:           po->point.smooth_hint_sz = "UNKNOWN";
    };

    glGetIntegerv(GL_LINE_SMOOTH_HINT, po->line.smooth_hint);
    switch (po->line.smooth_hint[0])
    {
        case GL_FASTEST:   po->line.smooth_hint_sz = "GL_FASTEST";   break;
        case GL_NICEST:    po->line.smooth_hint_sz = "GL_NICEST";    break;
        case GL_DONT_CARE: po->line.smooth_hint_sz = "GL_DONT_CARE"; break;
        default:           po->line.smooth_hint_sz = "UNKNOWN";
    };

    glGetIntegerv(GL_POLYGON_SMOOTH_HINT, po->polygon.smooth_hint);
    switch (po->polygon.smooth_hint[0])
    {
        case GL_FASTEST:   po->polygon.smooth_hint_sz = "GL_FASTEST";   break;
        case GL_NICEST:    po->polygon.smooth_hint_sz = "GL_NICEST";    break;
        case GL_DONT_CARE: po->polygon.smooth_hint_sz = "GL_DONT_CARE"; break;
        default:           po->polygon.smooth_hint_sz = "UNKNOWN";
    };

    //GL_x glPixelMap() translation tables; x is a map name from Table 8-1  - 0's glGetPixelMap*()
    //GL_x_SIZE Size of table x - 1 glGetIntegerv()
    //GL_READ_BUFFER Read source buffer pixel - glGetIntegerv()

    po->normalize_enabled = glIsEnabled(GL_NORMALIZE);
    po->dither_enabled = glIsEnabled(GL_DITHER);
    po->auto_normal_enabled = glIsEnabled(GL_AUTO_NORMAL);

    glGetIntegerv(GL_PERSPECTIVE_CORRECTION_HINT, po->perspective_correction_hint);
    switch (po->perspective_correction_hint[0])
    {
        case GL_FASTEST:   po->perspective_correction_hint_sz = "GL_FASTEST";   break;
        case GL_NICEST:    po->perspective_correction_hint_sz = "GL_NICEST";    break;
        case GL_DONT_CARE: po->perspective_correction_hint_sz = "GL_DONT_CARE"; break;
        default:           po->perspective_correction_hint_sz = "UNKNOWN";
    };

    glGetIntegerv(GL_FOG_HINT, po->fog_hint);
    switch (po->fog_hint[0])
    {
        case GL_FASTEST:   po->fog_hint_sz = "GL_FASTEST";   break;
        case GL_NICEST:    po->fog_hint_sz = "GL_NICEST";    break;
        case GL_DONT_CARE: po->fog_hint_sz = "GL_DONT_CARE"; break;
        default:           po->fog_hint_sz = "UNKNOWN";
    };

    glGetIntegerv(GL_SHADE_MODEL, po->shade_model);
    switch (po->shade_model[0])
    {
        case GL_SMOOTH: po->shade_model_sz = "GL_SMOOTH"; break;
        case GL_FLAT:   po->shade_model_sz = "GL_FLAT";   break;
        default:        po->shade_model_sz = "UNKNOWN";
    };

    //GL_FOG_COLOR Fog color fog 0, 0, 0, 0 glGetFloatv()
    //GL_FOG_INDEX Fog index fog 0 glGetFloatv()
    //GL_FOG_DENSITY Exponential fog density fog 1.0 glGetFloatv()
    //GL_FOG_START Linear fog _ctor fog 0.0 glGetFloatv()
    //GL_FOG_END Linear fog _dtor fog 1.0 glGetFloatv()
    //GL_FOG_MODE Fog mode fog GL_EXP glGetIntegerv()
    //GL_FOG True if fog enabled fog/enable GL_FALSE glIsEnabled()

}

//------------------------------------------------------------------------------
void gl_grab_clipping_state(gl_clipping_t * ps)
{
    ps->scissor.test_enabled = glIsEnabled(GL_SCISSOR_TEST);
    if (ps->scissor.test_enabled)
    {
        glGetIntegerv(GL_SCISSOR_BOX, ps->scissor.box);
    }

    gl_grab_clip_plane_state( ps->clip_plane + 0, 0 );
    gl_grab_clip_plane_state( ps->clip_plane + 1, 1 );
    gl_grab_clip_plane_state( ps->clip_plane + 2, 2 );
    gl_grab_clip_plane_state( ps->clip_plane + 3, 3 );
    gl_grab_clip_plane_state( ps->clip_plane + 4, 4 );
    gl_grab_clip_plane_state( ps->clip_plane + 5, 5 );

    glGetIntegerv(GL_VIEWPORT, ps->viewport);

    ps->cull_face_enabled = glIsEnabled(GL_CULL_FACE);
    if (GL_TRUE == ps->cull_face_enabled)
    {
        glGetIntegerv(GL_CULL_FACE_MODE, ps->cull_face_mode);
        switch (ps->cull_face_mode[0])
        {
            case GL_FRONT:          ps->cull_face_mode_sz = "GL_FRONT";          break;
            case GL_BACK:           ps->cull_face_mode_sz = "GL_BACK";           break;
            case GL_FRONT_AND_BACK: ps->cull_face_mode_sz = "GL_FRONT_AND_BACK"; break;
            default:                ps->cull_face_mode_sz = "UNKNOWN";
        };

        glGetIntegerv(GL_FRONT_FACE, ps->front_face);
        ps->front_face_sz = (GL_CW == ps->front_face[0]) ? "GL_CW" : "GL_CCW";
    }
}

//------------------------------------------------------------------------------
void gl_grab_texturing_state(gl_texturing_t * pt)
{
    // texture environment
    glGetTexEnviv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,  pt->env_mode );
    glGetTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, pt->env_color );

    pt->texture_1d_enabled = glIsEnabled(GL_TEXTURE_1D);
    if (GL_TRUE == pt->texture_1d_enabled)
    {
        gl_grab_texture_state(GL_TEXTURE_1D, 0, &pt->texture_1d);
    }

    pt->texture_2d_enabled = glIsEnabled(GL_TEXTURE_2D);
    if (GL_TRUE == pt->texture_2d_enabled)
    {
        gl_grab_texture_state(GL_TEXTURE_2D, 0, &pt->texture_2d);
    }

    pt->texture_gen[0].enabled = glIsEnabled(GL_TEXTURE_GEN_S);
    if (pt->texture_gen[0].enabled)
    {
        gl_grab_texture_gen_state(pt->texture_gen + 0, GL_S);
    }

    pt->texture_gen[1].enabled = glIsEnabled(GL_TEXTURE_GEN_T);
    if (pt->texture_gen[1].enabled)
    {
        gl_grab_texture_gen_state(pt->texture_gen + 1, GL_T);
    }

    pt->texture_gen[2].enabled = glIsEnabled(GL_TEXTURE_GEN_R);
    if (pt->texture_gen[2].enabled)
    {
        gl_grab_texture_gen_state(pt->texture_gen + 2, GL_R);
    }

    pt->texture_gen[3].enabled = glIsEnabled(GL_TEXTURE_GEN_Q);
    if (pt->texture_gen[3].enabled)
    {
        gl_grab_texture_gen_state(pt->texture_gen + 3, GL_Q);
    }

}

//------------------------------------------------------------------------------
void gl_grab_logic_op_state(gl_logic_op_t * pl)
{
    pl->index_enabled = glIsEnabled(GL_INDEX_LOGIC_OP);

    pl->color_enabled = glIsEnabled(GL_COLOR_LOGIC_OP);

    if ( pl->index_enabled || pl->color_enabled )
    {
        glGetIntegerv(GL_LOGIC_OP_MODE, pl->mode);
        switch (pl->mode[0])
        {
            case GL_CLEAR:         pl->mode_sz = "GL_CLEAR";         break;
            case GL_AND:           pl->mode_sz = "GL_AND";           break;
            case GL_COPY:          pl->mode_sz = "GL_COPY";          break;
            case GL_OR:            pl->mode_sz = "GL_OR";            break;
            case GL_NOOP:          pl->mode_sz = "GL_NOOP";          break;
            case GL_NAND:          pl->mode_sz = "GL_NAND";          break;
            case GL_SET:           pl->mode_sz = "GL_SET";           break;
            case GL_NOR:           pl->mode_sz = "GL_NOR";           break;
            case GL_COPY_INVERTED: pl->mode_sz = "GL_COPY_INVERTED"; break;
            case GL_XOR:           pl->mode_sz = "GL_XOR";           break;
            case GL_INVERT:        pl->mode_sz = "GL_INVERT";        break;
            case GL_EQUIV:         pl->mode_sz = "GL_EQUIV";         break;
            case GL_AND_REVERSE:   pl->mode_sz = "GL_AND_REVERSE";   break;
            case GL_AND_INVERTED:  pl->mode_sz = "GL_AND_INVERTED";  break;
            case GL_OR_REVERSE:    pl->mode_sz = "GL_OR_REVERSE";    break;
            case GL_OR_INVERTED:   pl->mode_sz = "GL_OR_INVERTED";   break;
            default:               pl->mode_sz = "UNKNOWN";
        };
    }

}

//------------------------------------------------------------------------------
void gl_grab_buffer_state( gl_buffer_t * pb )
{
    gl_grab_depth_state( &pb->depth );
    gl_grab_stencil_state( &pb->stencil );

    glGetIntegerv(GL_DRAW_BUFFER, pb->draw_buffer);

    glGetIntegerv(GL_INDEX_WRITEMASK, pb->index_writemask);
    glGetFloatv(GL_INDEX_CLEAR_VALUE, pb->index_clear_value);

    glGetBooleanv(GL_COLOR_WRITEMASK, pb->color_writemask);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, pb->color_clear_value);

    glGetFloatv(GL_ACCUM_CLEAR_VALUE, pb->accum_clear_value);

    glGetIntegerv(GL_INDEX_SHIFT, pb->index_shift);
    glGetIntegerv(GL_INDEX_OFFSET, pb->index_offset);

    // ---- color scale             ";
    glGetFloatv(GL_RED_SCALE,   pb->red_scale);
    glGetFloatv(GL_GREEN_SCALE, pb->green_scale);
    glGetFloatv(GL_BLUE_SCALE,  pb->blue_scale);

    // ---- color bias              ";
    glGetFloatv(GL_RED_BIAS,   pb->red_bias  );
    glGetFloatv(GL_GREEN_BIAS, pb->green_bias);
    glGetFloatv(GL_BLUE_BIAS,  pb->blue_bias );
};

