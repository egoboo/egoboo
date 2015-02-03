#include "game/gui/Image.hpp"
#include "game/ui.h"

Image::Image(const std::string &filePath) :
	_image()
{
	ego_texture_load_vfs(&_image, filePath.c_str(), INVALID_KEY);
}

Image::~Image()
{
	oglx_texture_Release(&_image);
}

void Image::draw()
{
	ui_drawImage(0, &_image, getX(), getY(), getWidth(), getHeight(), nullptr);
}
