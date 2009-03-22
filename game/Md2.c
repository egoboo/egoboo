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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

#include "Md2.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float kMd2Normals[][3] =
{
#include "id_normals.c"
    ,
    {0, 0, 0}  // Spiky Mace
};

// TEMPORARY: Global list of Md2Models.  It's declared in egoboo.h, which
// is why I have to include it here at the moment.
struct Md2Model *md2_models[MAXMODEL];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void md2_freeModel( Md2Model *model )
{
    if ( model != NULL )
    {
        if ( model->texCoords != NULL ) free( model->texCoords );

        if ( model->triangles != NULL ) free( model->triangles );

        if ( model->skins != NULL ) free( model->skins );

        if ( model->frames != NULL )
        {
            int i;

            for ( i = 0; i < model->numFrames; i++ )
            {
                if ( model->frames[i].vertices != NULL ) free( model->frames[i].vertices );
            }

            free( model->frames );
        }

        free( model );
    }
}

//--------------------------------------------------------------------------------------------
Md2Model* md2_loadFromFile( const char *filename )
{
    return NULL;
}
