#include "game/GUI/Image.hpp"

Image::Image() :
    _image()
{
    if (!oglx_texture_t::ctor(&_image))
    {
        throw std::runtime_error("unable to construct image texture");
    }
}

Image::Image(const std::string &filePath) : 
    _image()
{
    if (!oglx_texture_t::ctor(&_image))
    {
        throw std::runtime_error("unable to construct image texture");
    }
    ego_texture_load_vfs(&_image, filePath.c_str(), TRANSCOLOR);
}

Image::~Image()
{
    oglx_texture_t::dtor(&_image);
}

void Image::draw()
{
    _gameEngine->getUIManager()->drawImage(_image, getX(), getY(), getWidth(), getHeight());
}

void Image::setImage(const std::string &filePath)
{
    // Unload any old image first
    oglx_texture_t::release(&_image);

    // Load new image
    ego_texture_load_vfs(&_image, filePath.c_str(), TRANSCOLOR);
}
