#pragma once

namespace Ego {
namespace GUI {

struct DrawingContext {
    DrawingContext();
    DrawingContext(bool useDerived);
    ~DrawingContext();
    bool useDerived;
};

} // namespace GUI
} // namespace Ego
