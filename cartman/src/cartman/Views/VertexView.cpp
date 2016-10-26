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

#include "cartman/Views/VertexView.hpp"
#include "cartman/cartman_gui.h"
#include "cartman/cartman_functions.h"
#include "cartman/cartman_gfx.h"
#include "cartman/cartman_typedef.h"
#include "cartman/cartman_math.h"
#include "cartman/cartman_input.h"
#include "cartman/Clocks.h"

namespace Cartman {

void VertexView::render(Gui::Window& window, float zoom_hrz, float zoom_vrt) {
    if (!window.on || !HAS_BITS(window.mode, WINMODE_VERTEX)) return;

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

                draw_top_fan(mdata.win_select, fan, zoom_hrz, zoom_vrt);
            }

            if (mdata.rect_draw) {
                auto color = make_rgba(255, 128 + (Clocks::timePassed<Time::Unit::Ticks, int>() & 128),
                                            128 + (Clocks::timePassed<Time::Unit::Ticks, int>() & 128), 64);

                float x_min = mdata.rect_x0;
                float x_max = mdata.rect_x1;
                if (x_min > x_max) std::swap(x_max, x_min);

                float y_min = mdata.rect_y0;
                float y_max = mdata.rect_y1;
                if (y_min > y_max) std::swap(y_max, y_min);

                ogl_draw_box_xy(x_min, y_min, cam.z, x_max - x_min, y_max - y_min, color);
            }
        }
        cartman_end_ortho_camera();
    }
    glPopAttrib();
}

} // namespace Cartman
