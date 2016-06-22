#include "game/GUI/Image.hpp"

Image::Image() :
    _texture(""),
    _image(nullptr),
    _tint(Ego::Colour4f::white())
{

}

Image::Image(const std::string &filePath) : 
    _texture(filePath),
    _image(nullptr),
    _tint(Ego::Colour4f::white())
{
}

Image::Image(std::shared_ptr<Ego::Texture> texture) :
    _texture(""),
    _image(texture),
    _tint(Ego::Colour4f::white())
{

}

Image::Image(const Ego::DeferredTexture &image) :
    _texture(image),
    _image(nullptr),
    _tint(Ego::Colour4f::white())
{

}

void Image::draw()
{
    if(_image) {
        _gameEngine->getUIManager()->drawImage(_image, Point2f(getX(), getY()), Vector2f(getWidth(), getHeight()), _tint);
    }
    else if(_texture) {
        _gameEngine->getUIManager()->drawImage(_texture.get_ptr(), Point2f(getX(), getY()), Vector2f(getWidth(), getHeight()), _tint);
    }
}

void Image::setImage(const std::string &filePath)
{
    _texture.setTextureSource(filePath);
}

int Image::getTextureWidth() 
{ 
    if(_image) return _image->getSourceWidth(); 
    return _texture.get()->getSourceWidth();
}

int Image::getTextureHeight() 
{ 
    if(_image) return _image->getSourceHeight(); 
    return _texture.get()->getSourceHeight();
}

void Image::setTint(const Ego::Math::Colour4f &colour)
{
    _tint = colour;
}
