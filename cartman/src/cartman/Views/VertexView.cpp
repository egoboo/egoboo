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
        // set the viewport transformation
        Ego::Renderer::get().setViewportRectangle(window.position.getX(),
                                                  sdl_scr.height - (window.position.getY() + window.surfacey), window.surfacex, window.surfacey);

        // clip the viewport
        Ego::Renderer::get().setScissorTestEnabled(true);
        Ego::Renderer::get().setScissorRectangle(window.position.getX(),
                                                 sdl_scr.height - (window.position.getY() + window.surfacey), window.surfacex, window.surfacey);

        cartman_begin_ortho_camera_hrz(window, &cam, zoom_hrz, zoom_hrz);
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

                    if (VALID_MPD_TILE_RANGE(fan)) {
                        draw_top_fan(mdata.win_select, fan, zoom_hrz, zoom_vrt);
                    }
                }
            }

            if (mdata.rect_draw) {
                float color[4];
                float x_min, x_max;
                float y_min, y_max;

                make_rgba(color, 255, 128 + (Clocks::timePassed<Time::Unit::Ticks, int>() & 128),
                          128 + (Clocks::timePassed<Time::Unit::Ticks, int>() & 128), 0);

                x_min = mdata.rect_x0;
                x_max = mdata.rect_x1;
                if (x_min > x_max) std::swap(x_max, x_min);

                y_min = mdata.rect_y0;
                y_max = mdata.rect_y1;
                if (y_min > y_max) std::swap(y_max, y_min);

                ogl_draw_box_xy(x_min, y_min, cam.z, x_max - x_min, y_max - y_min, color);
            }
        }
        cartman_end_ortho_camera();
    }
    glPopAttrib();
}

} // namespace Cartman
