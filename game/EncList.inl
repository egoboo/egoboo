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

#define _VALID_ENC_RANGE( IENC )    ( ((IENC) < MAX_ENC) && ((IENC) >= 0) )
#define _DEFINED_ENC( IENC )        ( _VALID_ENC_RANGE( IENC ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define _ALLOCATED_ENC( IENC )      ( _VALID_ENC_RANGE( IENC ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define _ACTIVE_ENC( IENC )         ( _VALID_ENC_RANGE( IENC ) && ACTIVE_PBASE    ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define _WAITING_ENC( IENC )        ( _VALID_ENC_RANGE( IENC ) && WAITING_PBASE   ( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define _TERMINATED_ENC( IENC )     ( _VALID_ENC_RANGE( IENC ) && TERMINATED_PBASE( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )

#define _GET_INDEX_PENC( PENC )      ((NULL == (PENC)) ? MAX_ENC : (size_t)GET_INDEX_POBJ( PENC, MAX_ENC ))
#define _GET_REF_PENC( PENC )        ((ENC_REF)_GET_INDEX_PENC( PENC ))
#define _DEFINED_PENC( PENC )        ( _VALID_ENC_PTR( PENC ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(PENC) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PENC) ) )
#define _VALID_ENC_PTR( PENC )       ( (NULL != (PENC)) && _VALID_ENC_RANGE( GET_REF_POBJ( PENC, MAX_ENC) ) )
#define _ALLOCATED_PENC( PENC )      ( _VALID_ENC_PTR( PENC ) && ALLOCATED_PBASE( POBJ_GET_PBASE(PENC) ) )
#define _ACTIVE_PENC( PENC )         ( _VALID_ENC_PTR( PENC ) && ACTIVE_PBASE( POBJ_GET_PBASE(PENC) ) )
#define _TERMINATED_PENC( PENC )     ( _VALID_ENC_PTR( PENC ) && TERMINATED_PBASE( POBJ_GET_PBASE(PENC) ) )

// Macros to determine whether the enchant is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define _INGAME_ENC_BASE(IENC)       ( _VALID_ENC_RANGE( IENC ) && ACTIVE_PBASE( POBJ_GET_PBASE(EncList.lst + (IENC)) ) && ON_PBASE( POBJ_GET_PBASE(EncList.lst + (IENC)) ) )
#define _INGAME_PENC_BASE(PENC)      ( _VALID_ENC_PTR( PENC ) && ACTIVE_PBASE( POBJ_GET_PBASE(PENC) ) && ON_PBASE( POBJ_GET_PBASE(PENC) ) )

#define _INGAME_ENC(IENC)            ( (ego_object_spawn_depth) > 0 ? _DEFINED_ENC(IENC) : _INGAME_ENC_BASE(IENC) )
#define _INGAME_PENC(PENC)           ( (ego_object_spawn_depth) > 0 ? _DEFINED_PENC(PENC) : _INGAME_PENC_BASE(PENC) )

//--------------------------------------------------------------------------------------------
// testing functions
//--------------------------------------------------------------------------------------------

static INLINE bool_t VALID_ENC_RANGE( const ENC_REF IENC );
static INLINE bool_t DEFINED_ENC( const ENC_REF IENC );
static INLINE bool_t ALLOCATED_ENC( const ENC_REF IENC );
static INLINE bool_t ACTIVE_ENC( const ENC_REF IENC );
static INLINE bool_t WAITING_ENC( const ENC_REF IENC );
static INLINE bool_t TERMINATED_ENC( const ENC_REF IENC );

static INLINE size_t  GET_INDEX_PENC( const enc_t * PENC );
static INLINE ENC_REF GET_REF_PENC( const enc_t * PENC );
static INLINE bool_t  DEFINED_PENC( const enc_t * PENC );
static INLINE bool_t  VALID_ENC_PTR( const enc_t * PENC );
static INLINE bool_t  ALLOCATED_PENC( const enc_t * PENC );
static INLINE bool_t  ACTIVE_PENC( const enc_t * PENC );
static INLINE bool_t  TERMINATED_PENC( const enc_t * PENC );

static INLINE bool_t INGAME_ENC_BASE( const ENC_REF IENC );
static INLINE bool_t INGAME_PENC_BASE( const enc_t * PENC );

static INLINE bool_t INGAME_ENC( const ENC_REF IENC );
static INLINE bool_t INGAME_PENC( const enc_t * PENC );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE bool_t VALID_ENC_RANGE( const ENC_REF IENC ) { return BOOL_T(_VALID_ENC_RANGE( IENC )); }
static INLINE bool_t DEFINED_ENC( const ENC_REF IENC )     { return BOOL_T(_DEFINED_ENC( IENC ));     }
static INLINE bool_t ALLOCATED_ENC( const ENC_REF IENC )   { return BOOL_T(_ALLOCATED_ENC( IENC ));   }
static INLINE bool_t ACTIVE_ENC( const ENC_REF IENC )      { return BOOL_T(_ACTIVE_ENC( IENC ));      }
static INLINE bool_t WAITING_ENC( const ENC_REF IENC )     { return BOOL_T(_WAITING_ENC( IENC ));     }
static INLINE bool_t TERMINATED_ENC( const ENC_REF IENC )  { return BOOL_T(_TERMINATED_ENC( IENC ));  }

static INLINE size_t  GET_INDEX_PENC( const enc_t * PENC )  { return _GET_INDEX_PENC( PENC );  }
static INLINE ENC_REF GET_REF_PENC( const enc_t * PENC )    { return _GET_REF_PENC( PENC );    }
static INLINE bool_t  DEFINED_PENC( const enc_t * PENC )    { return BOOL_T(_DEFINED_PENC( PENC ));    }
static INLINE bool_t  VALID_ENC_PTR( const enc_t * PENC )   { return BOOL_T(_VALID_ENC_PTR( PENC ));   }
static INLINE bool_t  ALLOCATED_PENC( const enc_t * PENC )  { return BOOL_T(_ALLOCATED_PENC( PENC ));  }
static INLINE bool_t  ACTIVE_PENC( const enc_t * PENC )     { return BOOL_T(_ACTIVE_PENC( PENC ));     }
static INLINE bool_t  TERMINATED_PENC( const enc_t * PENC ) { return BOOL_T(_TERMINATED_PENC( PENC )); }

static INLINE bool_t INGAME_ENC_BASE( const ENC_REF IENC )  { return BOOL_T(_INGAME_ENC_BASE( IENC ));  }
static INLINE bool_t INGAME_PENC_BASE( const enc_t * PENC ) { return BOOL_T(_INGAME_PENC_BASE( PENC )); }

static INLINE bool_t INGAME_ENC( const ENC_REF IENC )       { return BOOL_T(_INGAME_ENC( IENC ));  }
static INLINE bool_t INGAME_PENC( const enc_t * PENC )      { return BOOL_T(_INGAME_PENC( PENC )); }
