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
#include "game/script_scanner.hpp"

token_t *token_t::ctor(token_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }

    // to be explicit
    self->iLine = 0;
    self->iIndex = MAX_OPCODE;
    self->iValue = 0;
    self->cType = '?';

    self->szWord_length = 0;
    self->szWord[0] = CSTR_END;

    return self;
}

void token_t::dtor(token_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }

    self->szWord[0] = CSTR_END;
    self->szWord_length = 0;

    self->cType = '?';
    self->iValue = 0;
    self->iIndex = MAX_OPCODE;
    self->iLine = 0;
}