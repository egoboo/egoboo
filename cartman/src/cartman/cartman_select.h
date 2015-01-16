#pragma once

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

#include "cartman/cartman_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_cartman_mpd;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_select_lst;
typedef struct s_select_lst select_lst_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAXSELECT 2560          // Max points that can be select_vertsed

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_select_lst
{
    struct s_cartman_mpd * pmesh;
    int                    count;
    Uint32                 which[MAXSELECT];
};

#define SELECT_LST_INIT \
    {\
        NULL,        /* struct s_cartman_mpd * pmesh */ \
        0,           /* int     count                */ \
        { CHAINEND } /* Uint32  which[MAXSELECT]     */ \
    }

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

select_lst_t * select_lst_default();
select_lst_t * select_lst_synch_mesh( select_lst_t *, struct s_cartman_mpd * );

select_lst_t * select_lst_init( select_lst_t *, struct s_cartman_mpd * );
select_lst_t * select_lst_clear( select_lst_t * );
select_lst_t * select_lst_add( select_lst_t * , int vert );
select_lst_t * select_lst_remove( select_lst_t * , int vert );

int select_lst_count( const select_lst_t * );
int select_lst_find( const select_lst_t * , int vert );

select_lst_t * select_lst_set_mesh( select_lst_t *, struct s_cartman_mpd * );
