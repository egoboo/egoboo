#include "id_md2.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/* Table of precalculated normals */
float kid_md2_normals[MD2_MAX_NORMALS][3] =
{
#    include "id_normals.inl"
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Load an MD2 model from file.

// Note: MD2 format stores model's data in little-endian ordering.  On
// big-endian machines, you'll have to perform proper conversions.

id_md2_model_t * id_md2_load (const char *filename, id_md2_model_t * mdl)
{
    FILE *fp;
    int i;

    fp = fopen (filename, "rb");
    if (!fp)
    {
        fprintf (stderr, "Error: couldn't open \"%s\"!\n", filename);
        return 0;
    }

    if ( NULL == mdl )
    {
        mdl = SDL_calloc( 1, sizeof(id_md2_model_t) );
    }

    if ( NULL == mdl ) return NULL;

    /* Read header */
    fread (&mdl->header, 1, sizeof (id_md2_header_t), fp);

    if ((mdl->header.ident != MD2_MAGIC_NUMBER) ||
            (mdl->header.version != MD2_VERSION))
    {
        /* Error! */
        fprintf (stderr, "Error: bad version or identifier\n");
        fclose (fp);
        return 0;
    }

    /* Memory allocations */
    mdl->skins     = (id_md2_skin_t *) SDL_calloc (mdl->header.num_skins, sizeof (id_md2_skin_t) );
    mdl->texcoords = (id_md2_texcoord_t *) SDL_calloc (mdl->header.num_st, sizeof (id_md2_texcoord_t));
    mdl->triangles = (id_md2_triangle_t *) SDL_calloc (mdl->header.num_tris, sizeof (id_md2_triangle_t));
    mdl->frames    = (id_md2_frame_t *) SDL_calloc (mdl->header.num_frames, sizeof (id_md2_frame_t) );
    mdl->glcmds    = (int *)SDL_calloc (mdl->header.size_glcmds, sizeof (int));

    /* Read model data */
    fseek (fp, mdl->header.offset_skins, SEEK_SET);
    fread (mdl->skins, sizeof (id_md2_skin_t), mdl->header.num_skins, fp);

    fseek (fp, mdl->header.offset_st, SEEK_SET);
    fread (mdl->texcoords, sizeof (id_md2_texcoord_t), mdl->header.num_st, fp);

    fseek (fp, mdl->header.offset_tris, SEEK_SET);
    fread (mdl->triangles, sizeof (id_md2_triangle_t), mdl->header.num_tris, fp);

    fseek (fp, mdl->header.offset_glcmds, SEEK_SET);
    fread (mdl->glcmds, sizeof (int), mdl->header.size_glcmds, fp);

    /* Read frames */
    fseek (fp, mdl->header.offset_frames, SEEK_SET);
    for (i = 0; i < mdl->header.num_frames; ++i)
    {
        /* Memory allocation for vertices of this frame */
        mdl->frames[i].verts = (id_md2_vertex_t *)
                               SDL_calloc (mdl->header.num_vertices, sizeof (id_md2_vertex_t));

        /* Read frame data */
        fread (mdl->frames[i].scale, sizeof (float), 3, fp);
        fread (mdl->frames[i].translate, sizeof (float), 3, fp);
        fread (mdl->frames[i].name, sizeof (char), 16, fp);
        fread (mdl->frames[i].verts, sizeof (id_md2_vertex_t), mdl->header.num_vertices, fp);
    }

    fclose (fp);
    return mdl;
}


//--------------------------------------------------------------------------------------------
// Free resources allocated for the model.

void id_md2_free ( id_md2_model_t * mdl )
{
    int i;

    if (mdl->skins)
    {
        SDL_free (mdl->skins);
        mdl->skins = NULL;
    }

    if (mdl->texcoords)
    {
        SDL_free (mdl->texcoords);
        mdl->texcoords = NULL;
    }

    if (mdl->triangles)
    {
        SDL_free (mdl->triangles);
        mdl->triangles = NULL;
    }

    if (mdl->glcmds)
    {
        SDL_free (mdl->glcmds);
        mdl->glcmds = NULL;
    }

    if (mdl->frames)
    {
        for (i = 0; i < mdl->header.num_frames; ++i)
        {
            SDL_free (mdl->frames[i].verts);
            mdl->frames[i].verts = NULL;
        }

        SDL_free (mdl->frames);
        mdl->frames = NULL;
    }
}
