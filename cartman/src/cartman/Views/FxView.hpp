#pragma once

#include "cartman/View.hpp"

namespace Cartman {

struct FxView : public View {
    void render(Gui::Window& window, float zoom_hrz, float zoom_vrt) override;
};

} // namespace Cartman
