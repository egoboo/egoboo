#pragma once

#include "game/Core/GameEngine.hpp"
#include "game/GUI/GUIComponent.hpp"

class Image : public GUIComponent
{
public:
    Image();
    Image(const std::string &filePath);
    Image(oglx_texture_t *texture);

    virtual void draw() override;

    void setImage(const std::string &filePath);

    int getTextureWidth();
    int getTextureHeight();

private:
    Ego::DeferredOpenGLTexture _texture;
    oglx_texture_t *_image;
};
