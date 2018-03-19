#pragma once

namespace Ego::GUI {

struct DrawingContext {
    DrawingContext();
    DrawingContext(bool useDerived);
    ~DrawingContext();
    bool useDerived;
};

} // namespace Ego::GUI
