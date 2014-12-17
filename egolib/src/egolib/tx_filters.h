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
#pragma once

/**
 * @brief
 *	An enumeration of texture filtering methods.
 */
typedef enum e_tx_filters {
	
	TX_UNFILTERED,
    
	TX_LINEAR,
    
	TX_MIPMAP,
    
	TX_BILINEAR,
    
	TX_TRILINEAR_1,
    
	TX_TRILINEAR_2,
    
	TX_ANISOTROPIC,

	TX_FILTER_COUNT,

} e_tx_filters;