#pragma once

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

/// @file EncList.inl
/// @brief
/// @details

#include "EncList.h"

#include "enchant.h"

//--------------------------------------------------------------------------------------------
// testing macros
//--------------------------------------------------------------------------------------------

#define VALID_ENC_RANGE( IENC )    ( ((ENC_REF)(IENC)) < MAX_ENC )
#define DEFINED_ENC( IENC )        ( VALID_ENC_RANGE( IENC ) && DEFINED_PENC_RAW   ( EncList.lst + (IENC)) )
#define ALLOCATED_ENC( IENC )      ( VALID_ENC_RANGE( IENC ) && ALLOCATED_PENC_RAW ( EncList.lst + (IENC)) )
#define ACTIVE_ENC( IENC )         ( VALID_ENC_RANGE( IENC ) && ACTIVE_PENC_RAW    ( EncList.lst + (IENC)) )
#define WAITING_ENC( IENC )        ( VALID_ENC_RANGE( IENC ) && WAITING_PENC_RAW   ( EncList.lst + (IENC)) )
#define TERMINATED_ENC( IENC )     ( VALID_ENC_RANGE( IENC ) && TERMINATED_PENC_RAW( EncList.lst + (IENC)) )

#define GET_INDEX_PENC( PENC )      LAMBDA(NULL == (PENC), INVALID_ENC_IDX, (size_t)GET_INDEX_POBJ( PENC, INVALID_ENC_IDX ))
#define GET_REF_PENC( PENC )        ((ENC_REF)GET_INDEX_PENC( PENC ))
#define VALID_ENC_PTR( PENC )       ( (NULL != (PENC)) && VALID_ENC_RANGE( GET_REF_POBJ( PENC, INVALID_ENC_REF) ) )
#define DEFINED_PENC( PENC )        ( VALID_ENC_PTR( PENC ) && DEFINED_PENC_RAW   ( PENC ) )
#define ALLOCATED_PENC( PENC )      ( VALID_ENC_PTR( PENC ) && ALLOCATED_PENC_RAW ( PENC ) )
#define ACTIVE_PENC( PENC )         ( VALID_ENC_PTR( PENC ) && ACTIVE_PENC_RAW    ( PENC ) )
#define WAITING_PENC( PENC )        ( VALID_ENC_PTR( PENC ) && WAITING_PENC_RAW   ( PENC ) )
#define TERMINATED_PENC( PENC )     ( VALID_ENC_PTR( PENC ) && TERMINATED_PENC_RAW( PENC ) )

// Macros to determine whether the enchant is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define INGAME_ENC_BASE(IENC)       ( VALID_ENC_RANGE( IENC ) && INGAME_PENC_BASE_RAW( EncList.lst + (IENC) ) )
#define INGAME_PENC_BASE(PENC)      ( VALID_ENC_PTR( PENC ) && INGAME_PENC_BASE_RAW( PENC ) )

#define INGAME_ENC(IENC)            LAMBDA( ego_object_spawn_depth > 0, DEFINED_ENC(IENC), INGAME_ENC_BASE(IENC) )
#define INGAME_PENC(PENC)           LAMBDA( ego_object_spawn_depth > 0, DEFINED_PENC(PENC), INGAME_PENC_BASE(PENC) )

// macros without range checking
#define INGAME_PENC_BASE_RAW(PENC)      ( ACTIVE_PBASE( POBJ_GET_PBASE(PENC) ) && ON_PBASE( POBJ_GET_PBASE(PENC) ) )
#define DEFINED_PENC_RAW( PENC )        ( ALLOCATED_PBASE ( POBJ_GET_PBASE(PENC) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PENC) ) )
#define ALLOCATED_PENC_RAW( PENC )      ALLOCATED_PBASE( POBJ_GET_PBASE(PENC) )
#define ACTIVE_PENC_RAW( PENC )         ACTIVE_PBASE( POBJ_GET_PBASE(PENC) )
#define WAITING_PENC_RAW( PENC )        WAITING_PBASE   ( POBJ_GET_PBASE(PENC) )
#define TERMINATED_PENC_RAW( PENC )     TERMINATED_PBASE( POBJ_GET_PBASE(PENC) )

//--------------------------------------------------------------------------------------------
// testing functions
//--------------------------------------------------------------------------------------------

static INLINE ego_bool _VALID_ENC_RANGE( const ENC_REF IENC );
static INLINE ego_bool _DEFINED_ENC( const ENC_REF IENC );
static INLINE ego_bool _ALLOCATED_ENC( const ENC_REF IENC );
static INLINE ego_bool _ACTIVE_ENC( const ENC_REF IENC );
static INLINE ego_bool _WAITING_ENC( const ENC_REF IENC );
static INLINE ego_bool _TERMINATED_ENC( const ENC_REF IENC );

static INLINE size_t  _GET_INDEX_PENC( const enc_t * PENC );
static INLINE ENC_REF _GET_REF_PENC( const enc_t * PENC );
static INLINE ego_bool  _DEFINED_PENC( const enc_t * PENC );
static INLINE ego_bool  _VALID_ENC_PTR( const enc_t * PENC );
static INLINE ego_bool  _ALLOCATED_PENC( const enc_t * PENC );
static INLINE ego_bool  _ACTIVE_PENC( const enc_t * PENC );
static INLINE ego_bool  _TERMINATED_PENC( const enc_t * PENC );

static INLINE ego_bool _INGAME_ENC_BASE( const ENC_REF IENC );
static INLINE ego_bool _INGAME_PENC_BASE( const enc_t * PENC );

static INLINE ego_bool _INGAME_ENC( const ENC_REF IENC );
static INLINE ego_bool _INGAME_PENC( const enc_t * PENC );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE ego_bool _VALID_ENC_RANGE( const ENC_REF IENC ) { return VALID_ENC_RANGE( IENC ); }
static INLINE ego_bool _DEFINED_ENC( const ENC_REF IENC )     { return DEFINED_ENC( IENC );     }
static INLINE ego_bool _ALLOCATED_ENC( const ENC_REF IENC )   { return ALLOCATED_ENC( IENC );   }
static INLINE ego_bool _ACTIVE_ENC( const ENC_REF IENC )      { return ACTIVE_ENC( IENC );      }
static INLINE ego_bool _WAITING_ENC( const ENC_REF IENC )     { return WAITING_ENC( IENC );     }
static INLINE ego_bool _TERMINATED_ENC( const ENC_REF IENC )  { return TERMINATED_ENC( IENC );  }

static INLINE size_t  _GET_INDEX_PENC( const enc_t * PENC )  { return _GET_INDEX_PENC( PENC );  }
static INLINE ENC_REF _GET_REF_PENC( const enc_t * PENC )    { return _GET_REF_PENC( PENC );    }
static INLINE ego_bool  _DEFINED_PENC( const enc_t * PENC )    { return _DEFINED_PENC( PENC );    }
static INLINE ego_bool  _VALID_ENC_PTR( const enc_t * PENC )   { return _VALID_ENC_PTR( PENC );   }
static INLINE ego_bool  _ALLOCATED_PENC( const enc_t * PENC )  { return _ALLOCATED_PENC( PENC );  }
static INLINE ego_bool  _ACTIVE_PENC( const enc_t * PENC )     { return _ACTIVE_PENC( PENC );     }
static INLINE ego_bool  _TERMINATED_PENC( const enc_t * PENC ) { return _TERMINATED_PENC( PENC ); }

static INLINE ego_bool _INGAME_ENC_BASE( const ENC_REF IENC )  { return _INGAME_ENC_BASE( IENC );  }
static INLINE ego_bool _INGAME_PENC_BASE( const enc_t * PENC ) { return _INGAME_PENC_BASE( PENC ); }

static INLINE ego_bool _INGAME_ENC( const ENC_REF IENC )       { return _INGAME_ENC( IENC );  }
static INLINE ego_bool _INGAME_PENC( const enc_t * PENC )      { return _INGAME_PENC( PENC ); }
