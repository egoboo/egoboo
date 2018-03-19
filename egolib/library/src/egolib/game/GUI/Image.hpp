#pragma once

#include "egolib/game/GUI/Component.hpp"
#include "egolib/Renderer/DeferredTexture.hpp"

namespace Ego::GUI {

class Image : public Component {
public:
    Image();
    Image(const std::string& filePath);
    Image(const DeferredTexture &image);

    //TODO: remove
    Image(std::shared_ptr<Texture> texture);

    virtual void draw(DrawingContext& drawingContext) override;

    void setImage(const std::string &filePath);

    void setTint(const Colour4f &colour);

    int getTextureWidth();
    int getTextureHeight();

private:
    DeferredTexture _texture;
    std::shared_ptr<Texture> _image;
    Colour4f _tint;
};

} // namespace Ego::GUI
