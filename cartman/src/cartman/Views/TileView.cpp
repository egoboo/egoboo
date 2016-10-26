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

#include "cartman/Views/TileView.hpp"
#include "cartman/cartman_gui.h"
#include "cartman/cartman_functions.h"
#include "cartman/cartman_gfx.h"
#include "cartman/cartman_typedef.h"
#include "cartman/cartman_math.h"

namespace Cartman {

void TileView::render(Gui::Window& window, float zoom_hrz, float zoom_vrt) {
    if (!window.on || !HAS_BITS(window.mode, WINMODE_TILE)) return;

    if (NULL == window.pmesh) window.pmesh = &mesh;

    glPushAttrib(GL_SCISSOR_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT);
    {
        beginRender(window, zoom_hrz, zoom_vrt);
        cartman_begin_ortho_camera_hrz(window, &cam, zoom_hrz, zoom_hrz);
        {
            std::vector<Index2D> indices;
            getTileRange(cam, *window.pmesh, indices);

            for (auto index : indices) {
                int fan = window.pmesh->get_ifan(index);
                if (!VALID_MPD_TILE_RANGE(fan)) continue;

                int x = index.x() * Info<int>::Grid::Size();
                int y = index.y() * Info<int>::Grid::Size();
                auto tx_tile = tile_at(window.pmesh, fan);
                if (!tx_tile) continue;

                draw_top_tile(x, y, fan, tx_tile, false, window.pmesh);
            }
        }
        cartman_end_ortho_camera();

        //for (cnt = 0; cnt < numlight; cnt++)
        //{
        //    draw_light(cnt, pwin);
        //}
    }
    glPopAttrib();
}

} // namespace Cartman
