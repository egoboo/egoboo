#include "game/gui/Image.hpp"
#include "game/ui.h"

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
	oglx_texture_release(&_image);
}

void Image::draw()
{
	ui_drawImage(0, &_image, getX(), getY(), getWidth(), getHeight(), nullptr);
}

void Image::setImage(const std::string &filePath)
{
	//Unload any old image first
	oglx_texture_release(&_image);

	//Load new image
	ego_texture_load_vfs(&_image, filePath.c_str(), TRANSCOLOR);
}
