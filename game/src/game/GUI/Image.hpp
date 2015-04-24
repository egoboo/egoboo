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

    inline int getTextureWidth() const { return oglx_texture_t::getSourceWidth(&_image); }
    inline int getTextureHeight() const { return oglx_texture_t::getSourceHeight(&_image); }

private:
    oglx_texture_t _image;
};
