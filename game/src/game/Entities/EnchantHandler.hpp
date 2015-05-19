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

/// @file  game/Entities/EnchantHandler.hpp
/// @brief Manager of enchantment entities.

#pragma once
#if !defined(GAME_ENTITIES_PRIVATE) || GAME_ENTITIES_PRIVATE != 1
#error(do not include directly, include `game/Entities/_Include.hpp` instead)
#endif


#include "game/egoboo_typedef.h"
#include "game/egoboo_object.h"
#include "game/LockableList.hpp"
#include "game/Entities/Enchant.hpp"

//--------------------------------------------------------------------------------------------
// looping macros
//--------------------------------------------------------------------------------------------

// Macros automate looping through the EncList. This hides code which defers the creation and deletion of
// objects until the loop terminates, so tha the length of the list will not change during the loop.
#define ENC_BEGIN_LOOP_ACTIVE(IT, PENC) \
    { \
        int IT##_internal; \
        int enc_loop_start_depth = EnchantHandler::get().getLockCount(); \
        EnchantHandler::get().lock(); \
        for(IT##_internal=0;IT##_internal<EnchantHandler::get().getUsedCount();IT##_internal++) \
        { \
            ENC_REF IT; \
            enc_t * PENC = NULL; \
            IT = (ENC_REF)EnchantHandler::get().used_ref[IT##_internal]; \
            if(!ACTIVE_ENC(IT)) continue; \
            PENC =  EnchantHandler::get().get_ptr(IT);

#define ENC_END_LOOP() \
        } \
        EnchantHandler::get().unlock(); \
        EGOBOO_ASSERT(enc_loop_start_depth == EnchantHandler::get().getLockCount()); \
        EnchantHandler::get().maybeRunDeferred(); \
    }

//--------------------------------------------------------------------------------------------
// external variables
//--------------------------------------------------------------------------------------------

struct EnchantHandler : public _LockableList < enc_t, ENC_REF, INVALID_ENC_REF, ENCHANTS_MAX, BSP_LEAF_ENC>
{
    EnchantHandler() :
        _LockableList()
    {
    }

public:
    static EnchantHandler& get();
    ENC_REF allocate(const ENC_REF override);
    void update_used();
    static ENC_REF spawn_one_enchant(const CHR_REF owner, const CHR_REF target, const CHR_REF spawner, const ENC_REF enc_override, const PRO_REF modeloptional);
};

//--------------------------------------------------------------------------------------------
// testing functions
//--------------------------------------------------------------------------------------------
bool VALID_ENC_RANGE(const ENC_REF ref);
bool DEFINED_ENC(const ENC_REF ref);
bool ALLOCATED_ENC(const ENC_REF ref);
bool ACTIVE_ENC(const ENC_REF ref);
bool WAITING_ENC(const ENC_REF ref);
bool TERMINATED_ENC(const ENC_REF ref);
ENC_REF GET_REF_PENC(const enc_t *ptr);
bool DEFINED_PENC(const enc_t *ptr);
bool ALLOCATED_PENC(const enc_t *ptr);
bool ACTIVE_PENC(const enc_t *ptr);
bool WAITIN_PENC(const enc_t *ptr);
bool TERMINATED_PENC(const enc_t *ptr);
bool INGAME_ENC_BASE(const ENC_REF ref);
bool INGAME_PENC_BASE(const enc_t *ptr);
bool INGAME_ENC(const ENC_REF ref);
bool INGAME_PENC(const enc_t *ptr);
