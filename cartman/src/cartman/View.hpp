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
    /// Render a view.
    /// @param window the window to render the view into
    /// qparam zoom_hrz, zoom_vrt the horizontal and vertical zoom
    virtual void render(Gui::Window& window, float zoom_hrz, float zoom_vrt) = 0;
};

} // namespace Cartman
