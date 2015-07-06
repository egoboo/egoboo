#include "game/GUI/Image.hpp"

Image::Image() :
    _texture(""),
    _image(nullptr)
{

}

Image::Image(const std::string &filePath) : 
    _texture(filePath),
    _image(nullptr)
{
}

Image::Image(oglx_texture_t *texture) :
    _texture(""),
    _image(texture)
{

}

void Image::draw()
{
    if(_image) {
        _gameEngine->getUIManager()->drawImage(*_image, getX(), getY(), getWidth(), getHeight());
    }
    else {
        _gameEngine->getUIManager()->drawImage(_texture.get(), getX(), getY(), getWidth(), getHeight());        
    }
}

void Image::setImage(const std::string &filePath)
{
    _texture.setTextureSource(filePath);
}

int Image::getTextureWidth() 
{ 
    if(_image) return _image->getSourceWidth(); 
    return _texture.get().getSourceWidth();
}

int Image::getTextureHeight() 
{ 
    if(_image) return _image->getSourceHeight(); 
    return _texture.get().getSourceHeight();
}
