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

/**
 * @file   IdLib/IdLib.hpp
 * @brief  Master include file for IdLib.
 * @author Michael Heilmann
 */

#pragma once

#define IDLIB_PRIVATE 1
#include "IdLib/Target.hpp"
#include "IdLib/Platform.hpp"
#include "IdLib/NonCopyable.hpp"

// Exceptions.
#include "IdLib/Exception.hpp"
#include "IdLib/EnvironmentErrorException.hpp"
#include "IdLib/AssertionFailedException.hpp"
#include "IdLib/UnhandledSwitchCaseException.hpp"
#include "IdLib/RuntimeErrorException.hpp"
#include "IdLib/InvalidArgumentException.hpp"
#include "IdLib/OutOfBoundsException.hpp"

// DSL utilities.
#include "IdLib/Location.hpp"

// DSL exceptions.
#include "IdLib/AbstractLexicalErrorException.hpp"
#include "IdLib/LexicalErrorException.hpp"
#include "IdLib/AbstractSyntacticalErrorException.hpp"
#include "IdLib/SyntacticalErrorException.hpp"

/// Define __ID_CURRENT_FILE__, __ID_CURRENT_LINE__ and __ID_CURRENT_FUNCTION__.
/// Those constants will either be properly defined or not at all.
#include "IdLib/CurrentFunction.inline"

#undef IDLIB_PRIVATE
