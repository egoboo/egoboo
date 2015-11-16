#pragma once

namespace Grid {
template <typename Type>
struct Info {
private:
    _Type _width;
    _Type _height;
public:
    Info(_Type width, _Type height)
        : _width(width), _height(height) {}
    Info(const Info& other)
        : _width(other._width), _height(other._height) {}
    _Type getWidth() const {
        return _width;
    }
    _Type getHeight() const {
        return _height;
    }
};
} // namespace Grid
