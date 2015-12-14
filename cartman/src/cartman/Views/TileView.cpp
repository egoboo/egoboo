#include "cartman/Views/TileView.hpp"
#include "cartman/cartman_gui.h"
#include "cartman/cartman_functions.h"
#include "cartman/cartman_gfx.h"
#include "cartman/cartman_typedef.h"
#include "cartman/cartman_math.h"

namespace Cartman {

void TileView::render(Window& window, float zoom_hrz, float zoom_vrt) {
    if (!window.on || !HAS_BITS(window.mode, WINMODE_TILE)) return;

    if (NULL == window.pmesh) window.pmesh = &mesh;

    glPushAttrib(GL_SCISSOR_BIT | GL_VIEWPORT_BIT | GL_ENABLE_BIT);
    {
        // set the viewport transformation
        Ego::Renderer::get().setViewportRectangle(window.x, sdl_scr.y - (window.y + window.surfacey), window.surfacex, window.surfacey);

        // clip the viewport
        Ego::Renderer::get().setScissorTestEnabled(true);
        Ego::Renderer::get().setScissorRectangle(window.x, sdl_scr.y - (window.y + window.surfacey), window.surfacex, window.surfacey);

        cartman_begin_ortho_camera_hrz(window, &cam, zoom_hrz, zoom_hrz);
        {
            int mapxstt = std::floor((cam.x - cam.w  * 0.5f) / Info<float>::Grid::Size()) - 1.0f;
            int mapystt = std::floor((cam.y - cam.h  * 0.5f) / Info<float>::Grid::Size()) - 1.0f;

            int mapxend = std::ceil((cam.x + cam.w  * 0.5f) / Info<float>::Grid::Size()) + 1.0f;
            int mapyend = std::ceil((cam.y + cam.h  * 0.5f) / Info<float>::Grid::Size()) + 1.0f;

            for (int mapy = mapystt; mapy <= mapyend; mapy++) {
                if (mapy < 0 || mapy >= window.pmesh->info.getTileCountY()) continue;
                int y = mapy * Info<int>::Grid::Size();

                for (int mapx = mapxstt; mapx <= mapxend; mapx++) {
                    if (mapx < 0 || mapx >= window.pmesh->info.getTileCountX()) continue;
                    int x = mapx * Info<int>::Grid::Size();

                    int fan = window.pmesh->get_ifan(mapx, mapy);

                    Ego::Texture * tx_tile = NULL;
                    if (VALID_MPD_TILE_RANGE(fan)) {
                        tx_tile = tile_at(window.pmesh, fan);
                    }

                    if (NULL != tx_tile) {
                        draw_top_tile(x, y, fan, tx_tile, false, window.pmesh);
                    }
                }
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
