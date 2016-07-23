#include "game/GUI/Image.hpp"
#include "game/GUI/Material.hpp"

namespace Ego {
namespace GUI {

Image::Image() :
    _texture(""),
    _image(nullptr),
    _tint(Colour4f::white()) {

}

Image::Image(const std::string &filePath) :
    _texture(filePath),
    _image(nullptr),
    _tint(Colour4f::white()) {}

Image::Image(std::shared_ptr<Texture> texture) :
    _texture(""),
    _image(texture),
    _tint(Colour4f::white()) {

}

Image::Image(const DeferredTexture &image) :
    _texture(image),
    _image(nullptr),
    _tint(Colour4f::white()) {

}

void Image::draw(DrawingContext& drawingContext) {
    if (_image) {
        auto material = std::make_shared<const Material>(_image, _tint, true);
        _gameEngine->getUIManager()->drawImage(Point2f(getX(), getY()), Vector2f(getWidth(), getHeight()), material);
    } else if (_texture) {
        auto material = std::make_shared<const Material>(_texture.get_ptr(), _tint, true);
        _gameEngine->getUIManager()->drawImage(Point2f(getX(), getY()), Vector2f(getWidth(), getHeight()), material);
    }
}

void Image::setImage(const std::string &filePath) {
    _texture.setTextureSource(filePath);
}

int Image::getTextureWidth() {
    if (_image) return _image->getSourceWidth();
    return _texture.get()->getSourceWidth();
}

int Image::getTextureHeight() {
    if (_image) return _image->getSourceHeight();
    return _texture.get()->getSourceHeight();
}

void Image::setTint(const Math::Colour4f &colour) {
    _tint = colour;
}

} // namespace GUI
} // namespace Ego
