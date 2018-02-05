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

#include "cartman/View.hpp"
#include "cartman/cartman_gui.h"
#include "cartman/cartman_gfx.h"

namespace Cartman {
void View::beginRender(Cartman::Gui::Window& window, float zoom_hrz, float zoom_vrt) {
    auto& renderer = Ego::Renderer::get();
    // Compute left, bottom, width, height in pixels.
    int left = window.position.x();
    int bottom = Ego::GraphicsSystem::get().window->getDrawableSize().y() - (window.position.y() + window.size.y());
    int width = window.size.x();
    int height = window.size.y();
    // Set viewport.
    renderer.setViewportRectangle(left, bottom, width, height);
    // Enable scissor tests.
    Ego::Renderer::get().setScissorTestEnabled(true);
    // Set scissor rectangle.
    Ego::Renderer::get().setScissorRectangle(left, bottom , width, height);
}

void View::getTileRange(camera_t& camera, cartman_mpd_t& mesh, std::vector<Index2D>& indices) {
    int mapxstt, mapystt, mapxend, mapyend;
    getTileRange(camera, mesh, mapxstt, mapystt, mapxend, mapyend);
    // Loop over the tails and get index pairs.
    for (int mapy = mapystt; mapy <= mapyend; mapy++) {
        if (mapy < 0 || mapy >= mesh.info.getTileCountY()) continue;
        for (int mapx = mapxstt; mapx <= mapxend; mapx++) {
            if (mapx < 0 || mapx >= mesh.info.getTileCountX()) continue;
            indices.push_back(Index2D(mapx, mapy));
        }
    }
}

void View::getTileRange(camera_t& camera, cartman_mpd_t& mesh, int& startx, int& starty, int& endx, int& endy) {
    // half-width/half-size to the left divided by the grid size, rounding down the nearest integer.
    // -1.0f is subtract for unknown reasons.
    startx = std::floor((camera.x - camera.w  * 0.5f) / Info<float>::Grid::Size()) - 1.0f;
    starty = std::floor((camera.y - camera.h  * 0.5f) / Info<float>::Grid::Size()) - 1.0f;

    // half-width/half-size to the right divided by the grid size, rounding up to the nearest integer.
    // +1.0f is added for unknown reasons.
    endx = std::ceil((camera.x + camera.w  * 0.5f) / Info<float>::Grid::Size()) + 1.0f;
    endy = std::ceil((camera.y + camera.h  * 0.5f) / Info<float>::Grid::Size()) + 1.0f;
}
}