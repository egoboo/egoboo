#pragma once

#include "egolib/platform.h"

namespace Ego {
namespace Math {

/**
 * The type of an L colour space with floating-point components each within the range from 0 (inclusive) to 1 (inclusive).
 * A component value of 0 indicates minimal intensity of the component and 255 indicates maximal intensity of the component.
 */
struct Lf {
    /**
    * @brief The component type.
    */
    using ComponentType = float;

    /**
     * @brief Get if the colour space has RGB components.
     * @return @a true if the colour space has RGB components, @a false otherwise
     */
    static constexpr bool hasRgb() { return false; }

    /**
     * @brief Get if the colour space has an A component.
     * @return @a true if the colour space has an A component, @a false otherwise
     */
    static constexpr bool hasA() { return false; }

    /**
     * @brief Get if the colour space has a L component.
     * @return @a true if the colour space has an L component, @a false otherwise
     */
    static constexpr bool hasL() { return true; }

    /**
     * @brief Get the number of components of a colour in the colour space.
     * @return the number of components of a colour in the colour space
     */
    static constexpr size_t count() { return 1; }

    /**
     * @brief Get the minimum component value.
     * @return the minimum component value
     */
    static constexpr ComponentType min() { return 0.0f; }

    /**
     * @brief Get the maximum component value.
     *  @return the maximum component value
     */
    static constexpr ComponentType max() { return 1.0f; }
};

/**
* The type of an LA colour space with floating-point components each within the range from 0 (inclusive) to 1 (inclusive).
* A component value of 0 indicates minimal intensity of the component and 255 indicates maximal intensity of the component.
*/
struct LAf {
    /**
     * @brief The component type.
     */
    using ComponentType = float;

    /**
     * @brief Get if the colour space has RGB components.
     * @return @a true if the colour space has RGB components, @a false otherwise
     */
    static constexpr bool hasRgb() { return false; }

    /**
     * @brief Get if the colour space has an A component.
     * @return @a true if the colour space has an A component, @a false otherwise
     */
    static constexpr bool hasA() { return true; }

    /**
     * @brief Get if the colour space has a L component.
     * @return @a true if the colour space has an L component, @a false otherwise
     */
    static constexpr bool hasL() { return true; }

    /**
     * @brief Get the number of components of a colour in the colour space.
     * @return the number of components of a colour in the colour space
     */
    static constexpr size_t count() { return 2; }

    /**
     * @brief Get the minimum component value.
     * @return the minimum component value
     */
    static constexpr ComponentType min() { return 0.0f; }

    /**
     * @brief Get the maximum component value.
     *  @return the maximum component value
     */
    static constexpr ComponentType max() { return 1.0f; }
};

/**
 * The type of an L colour space with unsigned integer components each within the range from 0 (inclusive) to 1 (inclusive).
 * A component value of 0 indicates minimal intensity of the component and 255 indicates maximal intensity of the component.
 */
struct Lb {
    /**
     * @brief The component type.
     */
    using ComponentType = uint8_t;

    /**
     * @brief Get if the colour space has RGB components.
     * @return @a true if the colour space has RGB components, @a false otherwise
     */
    static constexpr bool hasRgb() { return false; }

    /**
     * @brief Get if the colour space has an A component.
     * @return @a true if the colour space has an A component, @a false otherwise
     */
    static constexpr bool hasA() { return false; }

    /**
     * @brief Get if the colour space has a L component.
     * @return @a true if the colour space has an L component, @a false otherwise
     */
    static constexpr bool hasL() { return true; }

    /**
     * @brief Get the number of components of a colour in the colour space.
     * @return the number of components of a colour in the colour space
     */
    static constexpr size_t count() { return 1; }

    /**
     * @brief Get the minimum component value.
     * @return the minimum component value
     */
    static constexpr ComponentType min() { return 0; }

    /**
     * @brief Get the maximum component value.
     *  @return the maximum component value
     */
    static constexpr ComponentType max() { return 255; }
};

/**
 * The type of an LA colour space with unsigned integer components each within the range from 0 (inclusive) to 1 (inclusive).
 * A component value of 0 indicates minimal intensity of the component and 255 indicates maximal intensity of the component.
 */
struct LAb {
    /**
     * @brief The component type.
     */
    using ComponentType = uint8_t;

    /**
     * @brief Get if the colour space has RGB components.
     * @return @a true if the colour space has RGB components, @a false otherwise
     */
    static constexpr bool hasRgb() { return false; }

    /**
     * @brief Get if the colour space has an A component.
     * @return @a true if the colour space has an A component, @a false otherwise
     */
    static constexpr bool hasA() { return true; }

    /**
     * @brief Get if the colour space has a L component.
     * @return @a true if the colour space has an L component, @a false otherwise
     */
    static constexpr bool hasL() { return true; }

    /**
     * @brief Get the number of components of a colour in the colour space.
     * @return the number of components of a colour in the colour space
     */
    static constexpr size_t count() { return 2; }

    /**
     * @brief Get the minimum component value.
     * @return the minimum component value
     */
    static constexpr ComponentType min() { return 0; }

    /**
     * @brief Get the maximum component value.
     *  @return the maximum component value
     */
    static constexpr ComponentType max() { return 255; }
};

/**
 * The type of an RGB colour space with floating-point components each within the range from 0 (inclusive) to 1 (inclusive).
 * A component value of 0 indicates minimal intensity of the component and 1 indicates maximal intensity of the component.
 */
struct RGBf {
    /**
     * @brief The component type.
     */
    using ComponentType = float;
    
    /**
     * @brief Get if the colour space has RGB components.
     * @return @a true if the colour space has RGB components, @a false otherwise 
     */
    static constexpr bool hasRgb() { return true; }
    
    /**
     * @brief Get if the colour space has an A component.
     * @return @a true if the colour space has an A component, @a false otherwise
     */
    static constexpr bool hasA() { return false; }

    /**
     * @brief Get if the colour space has a L component.
     * @return @a true if the colour space has an L component, @a false otherwise
     */
    static constexpr bool hasL() { return false; }

    /**
     * @brief Get the number of components of a colour in the colour space.
     * @return the number of components of a colour in the colour space
     */
    static constexpr size_t count() { return 3; }
    
    /**
     * @brief Get the minimum component value.
     * @return the minimum component value
     */
    static constexpr ComponentType min() { return 0.0f; }

    /**
     * @brief Get the maximum component value.
     *  @return the maximum component value
     */
    static constexpr ComponentType max() { return 1.0f; }

};

/**
 * The type of an RGBA colour space with floating-point components each within the range from 0 (inclusive) to 1 (inclusive).
 * A component value of 0 indicates minimal intensity of the component and 1 indicates maximal intensity of the component.
 */
struct RGBAf {
    /**
     * @brief The component type.
     */
    using ComponentType = float;

    /**
     * @brief Get if the colour space has RGB components.
     * @return @a true if the colour space has RGB components, @a false otherwise
     */
    static constexpr bool hasRgb() { return true; }

    /**
     * @brief Get if the colour space has an A component.
     * @return @a true if the colour space has an A component, @a false otherwise
     */
    static constexpr bool hasA() { return true; }

    /**
     * @brief Get if the colour space has an L component.
     * @return @a true if the colour space has an L component, @a false otherwise
     */
    static constexpr bool hasL() { return false; }

    /**
     * @brief Get the number of components of a colour in the colour space.
     * @return the number of components of a colour in the colour space
     */
    static constexpr size_t count() { return 4; }

    /**
     * @brief Get the minimum component value.
     * @return the minimum component value
     */
    static constexpr ComponentType min() { return 0.0f; }

    /**
     * @brief Get the maximum component value.
     *  @return the maximum component value
     */
    static constexpr ComponentType max() { return 1.0f; }

};

/**
 * The type of an RGB colour space with unsigned integer components each within the range from 0 (inclusive) to 255 (inclusive).
 * A component value of 0 indicates minimal intensity of the component and 255 indicates maximal intensity of the component.
 */
struct RGBb {
    /**
     * @brief The component type.
     */
    using ComponentType = uint8_t;

    /**
     * @brief Get if the colour space has RGB components.
     * @return @a true if the colour space has RGB components, @a false otherwise
     */
    static constexpr bool hasRgb() { return true; }

    /**
     * @brief Get if the colour space has an A component.
     * @return @a true if the colour space has an A component, @a false otherwise
     */
    static constexpr bool hasA() { return false; }

    /**
     * @brief Get if the colour space has an L component.
     * @return @a true if the colour space has an L component, @a false otherwise
     */
    static constexpr bool hasL() { return false; }

    /**
     * @brief Get the number of components of a colour in the colour space.
     * @return the number of components of a colour in the colour space
     */
    static constexpr size_t count() { return 3; }

    /**
     * @brief Get the minimum component value.
     * @return the minimum component value
     */
    static constexpr ComponentType min() { return 0; }

    /**
     * @brief Get the maximum component value.
     *  @return the maximum component value
     */
    static constexpr ComponentType max() { return 255; }
};

/**
 * The type of an RGBA colour space with unsigned integer components each within the range from 0 (inclusive) to 255 (inclusive).
 * A component value of 0 indicates minimal intensity of the component and 255 indicates maximal intensity of the component.
 */
struct RGBAb {
    /**
     * @brief The component type.
     */
    using ComponentType = uint8_t;

    /**
     * @brief Get if the colour space has RGB components.
     * @return @a true if the colour space has RGB components, @a false otherwise
     */
    static constexpr bool hasRgb() { return true; }

    /**
     * @brief Get if the colour space has an A component.
     * @return @a true if the colour space has an A component, @a false otherwise
     */
    static constexpr bool hasA() { return true; }

    /**
     * @brief Get if the colour space has an L component.
     * @return @a true if the colour space has an L component, @a false otherwise
     */
    static constexpr bool hasL() { return false; }

    /**
     * @brief Get the number of components in the colour space.
     * @return the number of components of a colour in the colour space
     */
    static constexpr size_t count() { return 4; }

    /**
     * @brief Get the minimum component value.
     * @return the minimum component value
     */
    static constexpr ComponentType min() { return 0; }

    /**
     * @brief Get the maximum component value.
     *  @return the maximum component value
     */
    static constexpr ComponentType max() { return 255; }
};

/**
 * @brief Get the type of the opaque color space of a specified color space.
 * @remark The opaque color space consists of all opaque colors of the specified color space.
 */
template <typename _ColourSpaceType>
struct _Opaque;

template <>
struct _Opaque<RGBb> { using Type = RGBb; };
template <>
struct _Opaque<RGBf> { using Type = RGBf; };
template <>
struct _Opaque<Lb> { using Type = Lb; };
template <>
struct _Opaque<Lf> { using Type = Lf; };

template <>
struct _Opaque<RGBAb> { using Type = RGBb; };
template <>
struct _Opaque<RGBAf> { using Type = RGBf; };
template <>
struct _Opaque<LAb> { using Type = Lb; };
template <>
struct _Opaque<LAf> { using Type = Lf; };

template <typename _ColourSpaceType>
using Opaque = typename _Opaque<_ColourSpaceType>::Type;

namespace Internal {

template <typename _ColourSpaceType>
struct IsRgb {
    static constexpr bool value = _ColourSpaceType::hasRgb()
                               && !_ColourSpaceType::hasL()
                               && !_ColourSpaceType::hasA();
};

template <typename _ColourSpaceType>
struct IsRgba {
    static constexpr bool value = _ColourSpaceType::hasRgb()
                               && !_ColourSpaceType::hasL()
                               && _ColourSpaceType::hasA();
};

template <typename _ColourSpaceType>
struct IsL {
    static constexpr bool value = !_ColourSpaceType::hasRgb()
                               && _ColourSpaceType::hasL()
                               && !_ColourSpaceType::hasA();
};

template <typename _ColourSpaceType>
struct IsLA {
    static constexpr bool value = !_ColourSpaceType::hasRgb()
                               && _ColourSpaceType::hasL()
                               && _ColourSpaceType::hasA();
};

} // namespace Internal

} // namespace Math
} // namespace Ego
