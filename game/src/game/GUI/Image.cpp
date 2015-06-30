#include "game/GUI/Image.hpp"

Image::Image() :
    _image(new oglx_texture_t()),
    _freeOnDestroy(true)
{

}

Image::Image(const std::string &filePath) : 
    _image(new oglx_texture_t()),
    _freeOnDestroy(true)
{
    ego_texture_load_vfs(_image, filePath.c_str(), TRANSCOLOR);
}

Image::Image(oglx_texture_t *texture) :
    _image(texture),
    _freeOnDestroy(false)
{

}


Image::~Image()
{
    if(_freeOnDestroy) {
        delete _image;
        _image = nullptr;        
    }
}

void Image::draw()
{
    _gameEngine->getUIManager()->drawImage(*_image, getX(), getY(), getWidth(), getHeight());
}

void Image::setImage(const std::string &filePath)
{
    // Unload any old image first
    if(_freeOnDestroy) {
        _image->release();
    }
    _freeOnDestroy = false;

    // Load new image
    ego_texture_load_vfs(_image, filePath.c_str(), TRANSCOLOR);
}
