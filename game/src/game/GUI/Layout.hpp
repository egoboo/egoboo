#pragma once

// Include directives and forward declarations.
#include "egolib/egolib.h"
namespace Ego {
namespace GUI {
class Component;
} // GUI
} // namespace Ego

namespace Ego {
namespace GUI {

#define EGO_GUI_WITH_ALIGNMENT (0)

#if defined(EGO_GUI_WITH_ALIGNMENT) && 1 == EGO_GUI_WITH_ALIGNMENT
/// Horizontal alignment.
enum class HorizontalAlignment {
    Left, Center, Right,
};
#endif

#if defined(EGO_GUI_WITH_ALIGNMENT) && 1 == EGO_GUI_WITH_ALIGNMENT
/// Vertical alignment.
enum class VerticalAlignment {
    Top, Center, Bottom,
};
#endif

/// Computes the maximum size \f$(w_max,h_max)\f$ of the components.
/// The component c_0 is assigned the position \f$P_0 = P\f$.
/// The component \f$c_i\f$ is assigned the position \f$P_i\f$
/// where
/// \f$P_i = P_{i-1} + (P_{i-1,x}, (h_max + \delta_y)\cdot i)\f$
/// if
/// \f$P_{i-1} + (0,h_max) \leq l\f$
/// and
/// \f$P_i = (P_x + (hw_max + \delta_x)\cdot i, P_{i-1,y})\f$
/// otherwise.
/// @see Ego::GUI::LayoutRows
struct LayoutColumns {
private:
    Point2f leftTop;
    float bottom;
    /// \f$\delta_x\f$
    float horizontalGap;
    /// \f$\delta_y\f$.
    float verticalGap;
#if defined(EGO_GUI_WITH_ALIGNMENT) && 1 == EGO_GUI_WITH_ALIGNMENT
    /// The horizotal alignment of components in their cell.
    HorizontalAlignment horizontalAlignment;
    /// The vertical alignment of components in their cell.
    VerticalAlignment verticalAlignment;
#endif
public:
    LayoutColumns();
    LayoutColumns(const Point2f& leftTop, float bottom, float horizontalGap, float verticalGap);
    void operator()(const std::vector<std::shared_ptr<Component>>& components) const;
};

/// Computes the maximum size \f$(w_max, h_max)\f$ of the components.
/// The component \f$c_0\f$ is assigned the position \f$P_0 = P\f$.
/// The component \f$c_i\f$ is assigned the position \f$P_i\f$
/// where
/// \f$P_i = P_{i-1} + ((w_max + \delta_x)\cdot i, P_{i-1,y})\f$
/// if
/// \f$P_{i-1} + (w_max,0) \leq right\f$
/// and
/// \f$P_i = (P_x,P_{i-1,y} + (w_max + \delta_y)\cdot i)\f$
/// otherwise.
/// @see Ego::GUI::LayoutColumns
struct LayoutRows {
private:
    Point2f leftTop;
    float right;
    /// \f$\delta_x\f$.
    float horizontalGap;
    /// \f$\delta_y\f$.
    float verticalGap;
#if defined(EGO_GUI_WITH_ALIGNMENT) && 1 == EGO_GUI_WITH_ALIGNMENT
    /// The horizontal alignment of components in their cell.
    HorizontalAlignment horizontalAlignment;
    /// The vertical alignment of components in their cell.
    VerticalAlignment verticalAlignment;
#endif
public:
    LayoutRows();
    LayoutRows(const Point2f& leftTop, float right, float horizontalGap, float verticalGap);
    void operator()(const std::vector<std::shared_ptr<Component>>& components) const;
};

} // namespace GUI
} // namespace Ego
