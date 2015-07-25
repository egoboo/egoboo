#pragma once

#include "game/Core/GameEngine.hpp"
#include "game/GUI/GUIComponent.hpp"

class Image : public GUIComponent
{
public:
    Image();
    Image(const std::string &filePath);
    Image(const Ego::DeferredOpenGLTexture &image);

    //TODO: remove
    Image(oglx_texture_t *texture);

    virtual void draw() override;

    void setImage(const std::string &filePath);

    void setTint(const Ego::Math::Colour4f &colour);

    int getTextureWidth();
    int getTextureHeight();

private:
    Ego::DeferredOpenGLTexture _texture;
    oglx_texture_t *_image;
    Ego::Math::Colour4f _tint;
};
