#include "egolib/game/Graphics/Billboard.hpp"
#include "egolib/Entities/_Include.hpp"

namespace Ego {
namespace Graphics {

Billboard::Billboard(::Time::Ticks endTime, std::shared_ptr<Ego::Texture> texture, const float size) :
    _endTime(endTime),
    _position(), _offset(), _offset_add(),
    _size(size), _size_add(0.0f),
    _texture(texture), _object(),
    _tint(Colour3f::white(), 1.0f), _tint_add(0.0f, 0.0f, 0.0f, 0.0f)
{
    /* Intentionally empty. */
}

bool Billboard::update(::Time::Ticks now)
{
    if ((now >= _endTime) || (nullptr == _texture))
    {
        return false;
    }
    auto obj_ptr = _object.lock();
    if (!obj_ptr || obj_ptr->isTerminated() || obj_ptr->isBeingHeld() || obj_ptr->isInsideInventory())
    {
        return false;
    }

    // Determine where the new position should be.
    Vector3f vup;
    chr_getMatUp(obj_ptr.get(), vup);

    _size += _size_add;

    using namespace Ego::Math;
    _tint = Colour4f(constrain(_tint.get_r() + _tint_add[kX], 0.0f, 1.0f),
                     constrain(_tint.get_g() + _tint_add[kY], 0.0f, 1.0f),
                     constrain(_tint.get_b() + _tint_add[kZ], 0.0f, 1.0f),
                     constrain(_tint.get_a() + _tint_add[kW], 0.0f, 1.0f));

    /// @todo Why is this disabled. It should be there.
    //@note Zefz> because it looked bad in-game, only apply Z offset looks much better
    //_offset[kX] += _offset_add[kX];
    //_offset[kY] += _offset_add[kY];
    _offset.z() += _offset_add.z();

    // Automatically kill a billboard that is no longer useful.
    if (_tint.get_a() == 0.0f || _size <= 0.0f)
    {
        return false;
    }

    return true;
}

} // namespace Graphics
} // namespace Ego
