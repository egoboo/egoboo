#pragma once

#include "game/Core/GameEngine.hpp"
#include "game/GUI/GUIComponent.hpp"

class Image : public GUIComponent
{
public:
	Image();
	Image(const std::string &filePath);
	~Image();

    virtual void draw() override;

    void setImage(const std::string &filePath);

    inline int getTextureWidth() const {return _image.imgW;}
    inline int getTextureHeight() const {return _image.imgH;}

    //Disable copying class
    Image(const Image& copy) = delete;
    Image& operator=(const Image&) = delete;

private:
    oglx_texture_t _image;
};
