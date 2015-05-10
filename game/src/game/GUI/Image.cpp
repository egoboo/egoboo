#include "game/GUI/Image.hpp"

Image::Image() :
    _image(new oglx_texture_t())
{}

Image::Image(const std::string &filePath) : 
    _image(new oglx_texture_t())
{
    ego_texture_load_vfs(_image, filePath.c_str(), TRANSCOLOR);
}

Image::~Image()
{
    delete _image;
    _image = nullptr;
}

void Image::draw()
{
    _gameEngine->getUIManager()->drawImage(*_image, getX(), getY(), getWidth(), getHeight());
}

void Image::setImage(const std::string &filePath)
{
    // Unload any old image first
    _image->release();

    // Load new image
    ego_texture_load_vfs(_image, filePath.c_str(), TRANSCOLOR);
}
