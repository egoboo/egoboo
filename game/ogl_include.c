
/// @file
/// @brief Egoboo OpenGL interface
/// @details Implements the most basic code that Egoboo uses to interface with OpenGL

#include "ogl_include.h"

//--------------------------------------------------------------------------------------------
GLboolean handle_opengl_error()
{
    GLboolean error = GL_TRUE;

    switch ( glGetError() )
    {
        case GL_INVALID_ENUM:      fprintf( stderr, "GLenum argument out of range" ); break;
        case GL_INVALID_VALUE:     fprintf( stderr, "Numeric argument out of range" ); break;
        case GL_INVALID_OPERATION: fprintf( stderr, "Operation illegal in current state" ); break;
        case GL_STACK_OVERFLOW:    fprintf( stderr, "Command would cause a stack overflow" ); break;
        case GL_STACK_UNDERFLOW:   fprintf( stderr, "Command would cause a stack underflow" ); break;
        case GL_OUT_OF_MEMORY:     fprintf( stderr, "Not enough memory left to execute command" ); break;
        default: error = GL_FALSE; break;
    };

    if ( error ) fflush( stderr );

    return error;
};

//--------------------------------------------------------------------------------------------
// MN This probably should be replaced by a call to gluLookAt, don't see why we need to make our own...
void oglx_ViewMatrix( GLXmatrix view,
                 const GLXvector3f from,     // camera location
                 const GLXvector3f at,        // camera look-at target
                 const GLXvector3f world_up,  // world’s up, usually 0, 0, 1
                 const GLfloat roll )     // clockwise roll around viewing direction, in radians
{

    ATTRIB_PUSH( "ViewMatrix", GL_TRANSFORM_BIT );
    {
        glMatrixMode( GL_MODELVIEW );
        glPushMatrix();
        glLoadIdentity();
        glScalef( -1, 1, 1 );

        if ( roll > .001 )
        {
            //GLXmatrix stupid_intermediate_matrix = RotateZ( roll );
            //glMultMatrixf( stupid_intermediate_matrix );
        }

        gluLookAt( from[0], from[1], from[2], at[0], at[1], at[2], world_up[0], world_up[1], world_up[2] );

        glGetFloatv( GL_MODELVIEW_MATRIX, view );

        glPopMatrix();
    }
    ATTRIB_POP( "ViewMatrix" );
}

//--------------------------------------------------------------------------------------------
void oglx_ProjectionMatrix( GLXmatrix proj,
                       const GLfloat near_plane,    // distance to near clipping plane
                       const GLfloat far_plane,      // distance to far clipping plane
                       const GLfloat fov )           // field of view angle, in radians
{
    ATTRIB_PUSH( "ProjectionMatrix", GL_TRANSFORM_BIT );
    {
        GLint viewport[ 4 ];

        //use OpenGl to create our projection matrix
        glGetIntegerv( GL_VIEWPORT, viewport );

        glMatrixMode( GL_PROJECTION );
        glPushMatrix();
        glLoadIdentity();
        gluPerspective( fov, ( GLfloat )( viewport[ 2 ] - viewport[ 0 ] ) / ( GLfloat )( viewport[ 3 ] - viewport[ 1 ] ), near_plane, far_plane );
        glGetFloatv( GL_PROJECTION_MATRIX, proj );
        glPopMatrix();
    }
    ATTRIB_POP( "ProjectionMatrix" );
}