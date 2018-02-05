#pragma once

#include "egolib/game/Core/GameEngine.hpp"
#include "egolib/game/GUI/Component.hpp"

namespace Ego {
namespace GUI {

class Image : public Component {
public:
    Image();
    Image(const std::string& filePath);
    Image(const DeferredTexture &image);

    //TODO: remove
    Image(std::shared_ptr<Texture> texture);

    virtual void draw(DrawingContext& drawingContext) override;

    void setImage(const std::string &filePath);

    void setTint(const Math::Colour4f &colour);

    int getTextureWidth();
    int getTextureHeight();

private:
    DeferredTexture _texture;
    std::shared_ptr<Texture> _image;
    Math::Colour4f _tint;
};

} // namespace GUI
} // namespace Ego
