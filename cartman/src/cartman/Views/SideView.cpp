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

#include "cartman/Views/SideView.hpp"
#include "cartman/cartman_gui.h"
#include "cartman/cartman_functions.h"
#include "cartman/cartman_gfx.h"
#include "cartman/cartman_typedef.h"
#include "cartman/cartman_math.h"
#include "cartman/cartman_input.h"
#include "cartman/Clocks.h"

namespace Cartman {

void SideView::render(Gui::Window& window, float zoom_hrz, float zoom_vrt) {
    if (!window.on || !HAS_BITS(window.mode, WINMODE_SIDE)) return;

    if (NULL == window.pmesh) window.pmesh = &mesh;

    glPushAttrib(GL_SCISSOR_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT);
    {
        beginRender(window, zoom_hrz, zoom_vrt);
        cartman_begin_ortho_camera_vrt(window, &cam, zoom_hrz, zoom_vrt * 2.0f);
        {
            std::vector<Index2D> indices;
            getTileRange(cam, *window.pmesh, indices);

            for (auto index : indices) {
                int fan = window.pmesh->get_ifan(index);
                if (!VALID_MPD_TILE_RANGE(fan)) continue;

                draw_side_fan(mdata.win_select, fan, zoom_hrz, zoom_vrt);
            }

            if (mdata.rect_draw) {
                Ego::Math::Colour4f color;
                float x_min, x_max;
                float z_min, z_max;

                color = make_rgba(255, 128 + (Clocks::timePassed<Time::Unit::Ticks, int>() & 128),
                                       128 + (Clocks::timePassed<Time::Unit::Ticks, int>() & 128), 64);

                x_min = mdata.rect_x0;
                x_max = mdata.rect_x1;
                if (x_min > x_max) std::swap(x_max, x_min);

                z_min = mdata.rect_z0;
                z_max = mdata.rect_z1;
                if (z_min > z_max) std::swap(z_max, z_min);

                ogl_draw_box_xz(x_min, cam.y, z_min, x_max - x_min, z_max - z_min, color);
            }
        }
        cartman_end_ortho_camera();
    }
    glPopAttrib();
}

} // namespace Cartman
