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
//*
//********************************************************************************************

#pragma once

#include "cartman/cartman_typedef.h"

namespace Cartman {

#define WINMODE_NOTHING 0   // Default window display mode.
#define WINMODE_TILE 1      //
#define WINMODE_VERTEX 2    //
#define WINMODE_SIDE 4      //
#define WINMODE_FX 8        //

/// The views a window may contain w.r.t. the mesh.
struct View {
    void beginRender(Gui::Window& window, float zoom_hrz, float zoom_vrt);
    /// Compute the range of tiles to be rendered.
    void getTileRange(camera_t& camera, cartman_mpd_t& mesh, int& startx, int& starty, int& endx, int& endy);
    void getTileRange(camera_t& camera, cartman_mpd_t& mesh, std::vector<Index2D>& indices);
    /// Render a view.
    /// @param window the window to render the view into
    /// qparam zoom_hrz, zoom_vrt the horizontal and vertical zoom
    virtual void render(Gui::Window& window, float zoom_hrz, float zoom_vrt) = 0;
};

} // namespace Cartman
