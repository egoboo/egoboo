#include "game/GUI/Image.hpp"

Image::Image() :
	_image()
{
	//default ctor
}

Image::Image(const std::string &filePath) : 
	_image()
{
	ego_texture_load_vfs(&_image, filePath.c_str(), TRANSCOLOR);
}

Image::~Image()
{
	oglx_texture_t::release(&_image);
}

void Image::draw()
{
	_gameEngine->getUIManager()->drawImage(_image, getX(), getY(), getWidth(), getHeight());
}

void Image::setImage(const std::string &filePath)
{
	//Unload any old image first
	oglx_texture_t::release(&_image);

	//Load new image
	ego_texture_load_vfs(&_image, filePath.c_str(), TRANSCOLOR);
}
