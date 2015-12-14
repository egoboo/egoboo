#include "cartman/Views/SideView.hpp"
#include "cartman/cartman_gui.h"
#include "cartman/cartman_functions.h"
#include "cartman/cartman_gfx.h"
#include "cartman/cartman_typedef.h"
#include "cartman/cartman_math.h"
#include "cartman/cartman_input.h"
#include "cartman/Clocks.h"

namespace Cartman {

void SideView::render(Window& window, float zoom_hrz, float zoom_vrt) {
    if (!window.on || !HAS_BITS(window.mode, WINMODE_SIDE)) return;

    if (NULL == window.pmesh) window.pmesh = &mesh;

    glPushAttrib(GL_SCISSOR_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT);
    {
        // set the viewport transformation
        Ego::Renderer::get().setViewportRectangle(window.x, sdl_scr.y - (window.y + window.surfacey), window.surfacex, window.surfacey);

        // clip the viewport
        Ego::Renderer::get().setScissorTestEnabled(true);
        Ego::Renderer::get().setScissorRectangle(window.x, sdl_scr.y - (window.y + window.surfacey), window.surfacex, window.surfacey);

        cartman_begin_ortho_camera_vrt(window, &cam, zoom_hrz, zoom_vrt * 2.0f);
        {
            int mapxstt = std::floor((cam.x - cam.w * 0.5f) / Info<float>::Grid::Size()) - 1.0f;
            int mapystt = std::floor((cam.y - cam.h * 0.5f) / Info<float>::Grid::Size()) - 1.0f;

            int mapxend = std::ceil((cam.x + cam.w * 0.5f) / Info<float>::Grid::Size()) + 1;
            int mapyend = std::ceil((cam.y + cam.h * 0.5f) / Info<float>::Grid::Size()) + 1;

            for (int mapy = mapystt; mapy <= mapyend; mapy++) {
                if (mapy < 0 || mapy >= window.pmesh->info.getTileCountY()) continue;

                for (int mapx = mapxstt; mapx <= mapxend; mapx++) {
                    if (mapx < 0 || mapx >= window.pmesh->info.getTileCountX()) continue;

                    int fan = window.pmesh->get_ifan(mapx, mapy);
                    if (!VALID_MPD_TILE_RANGE(fan)) continue;

                    draw_side_fan(mdata.win_select, fan, zoom_hrz, zoom_vrt);
                }
            }

            if (mdata.rect_draw) {
                float color[4];
                float x_min, x_max;
                float z_min, z_max;

                OGL_MAKE_COLOR_4(color, 0x3F, 16 + (Clocks::timePassed<Time::Unit::Ticks,int>() & 15),
                                              16 + (Clocks::timePassed<Time::Unit::Ticks,int>() & 15), 0);

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
