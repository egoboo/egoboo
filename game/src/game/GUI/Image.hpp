#pragma once

#include "game/Core/GameEngine.hpp"
#include "game/GUI/GUIComponent.hpp"

class Image : public GUIComponent
{
public:
    Image();
    Image(const std::string &filePath);
    Image(oglx_texture_t *texture);
    ~Image();

    virtual void draw() override;

    void setImage(const std::string &filePath);

    inline int getTextureWidth() const { return _image->getSourceWidth(); }
    inline int getTextureHeight() const { return _image->getSourceHeight(); }

private:
    oglx_texture_t *_image;
    bool _freeOnDestroy;
};
