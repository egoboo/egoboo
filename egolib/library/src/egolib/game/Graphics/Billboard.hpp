#pragma once

#include "egolib/game/egoboo.h"

// Forward declarations.
class Camera;
namespace Ego { class Font; }

namespace Ego::Graphics {

/// @todo Supposed to be a generic billboard.
/// Currently, it merely shows a texture and allows for some flags for position, blending and motion.
struct Billboard
{
    enum Flags
    {
        None = EMPTY_BIT_FIELD,
        /// @brief Randomize the position of the billboard within one grid along all axes.
        RandomPosition = (1 << 0),
        /// @brief Randomize the velocity of the billboard along all axes.
        /// Enough to move it by two tiles within its lifetime.
        RandomVelocity = (1 << 1),
        /// @brief Make the billboard fade out over time.
        Fade = (1 << 2),
        /// @brief Make the tint fully satured over time.
        Burn = (1 << 3),
        /// @brief All of the above.
        All = FULL_BIT_FIELD,
    };

    using Colour3f = Ego::Colour3f;
    using Colour4f = Ego::Colour4f;

    /// @brief The point in time after which the billboard is expired.
    ::Time::Ticks _endTime;

    /// @brief The texture reference.
    std::shared_ptr<Ego::Texture> _texture;

    /// @brief The position of the bottom-missle of the box.
    Vector3f _position;

    /// @brief The object this billboard is attached to.
    std::weak_ptr<Object> _object;

    /// @brief The colour of the billboard.
    Colour4f _tint;

    /// @brief Additive over-time colour modifier.
    /// @remark Each time the billboard is updated, <tt>_tint += _tint_add</tt> is computed.
    Vector4f _tint_add;

    /// @brief An offset to the billboard's position.
    /// @remark The offset is given in world cordinates.
    Vector3f _offset;

    /// @brief Additive over-time offset modifier.
    /// @remark Each time the billboard is updated, <tt>_offset += _offset_add</tt> is computed.
    Vector3f _offset_add;

    float _size;
    float _size_add;

    Billboard(::Time::Ticks endTime, std::shared_ptr<Ego::Texture> texture, const float size);

    /// @brief Update this billboard.
    /// @param now the current time
    bool update(::Time::Ticks now);

};

} // namespace Ego::Graphics
