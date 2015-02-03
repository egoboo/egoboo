#pragma once

#include "game/core/GameEngine.hpp"
#include "game/gui/GUIComponent.hpp"

class Image : public GUIComponent
{
public:
	Image(const std::string &filePath);
	~Image();

    virtual void draw() override;

    inline int getTextureWidth() const {return _image.imgW;}
    inline int getTextureHeight() const {return _image.imgH;}

    //Disable copying class
    Image(const Image& copy) = delete;
    Image& operator=(const Image&) = delete;

private:
    oglx_texture_t _image;
};
