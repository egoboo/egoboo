#pragma once

#include "egolib/Math/_Include.hpp"

namespace Ego {
namespace Graphics {
	
/// @brief A viewport.
class Viewport
{
private:
    /// @brief The left side of the viewport in pixels.
    float m_leftPixels;

    /// @brief The top side of the viewport in pixels.
    float m_topPixels;

    /// @brief The width of the viewport in pixels.
    float m_widthPixels;

    /// @brief The height of the viewport in pixels.
    float m_heightPixels;

    /// @brief The clear colour of the viewport.
    Ego::Math::Colour4f m_clearColour;

    /// @brief The clear depth of the viewport.
    float m_clearDepth;

public:
	/// @brief Construct this viewport with default values.
	Viewport();
	
	/// @brief Destruct this viewport.
	virtual ~Viewport();

public:
	/// @brief Signal raised if the left side of the viewport in pixels changed.
    /// The first argument is the old left, the second argument is the new left.
	idlib::signal<void(float, float)> LeftPixelsChanged;

	/// @brief Get the left side of the viewport in pixels.
	/// @return the left side of the viewport in pixels
	/// @default Default is \f$0\f$.
	float getLeftPixels() const;
	
	/// @brief Set the left side of the viewport in pixels.
	/// @param leftPixels the left side of the viewport in pixels
	void setLeftPixels(float leftPixels);
	
public:
	/// @brief Signal raised if the top side of the viewport in pixels changed.
    /// The first argument is the old top, the second argument is the new top.
	idlib::signal<void(float, float)> TopPixelsChanged;

	/// @brief Get the top side of the viewport in pixels.
	/// @return the top side of the viewport in pixels
	/// @default is \f$0\f$.
	float getTopPixels() const;
	
	/// @brief Set the top side of the viewport in pixels.
	/// @param topPixels the top side of the viewport in pixels
	void setTopPixels(float topPixels);
	
public:
	/// @brief Signal raised if the width of the viewport in pixels changed.
    /// The first argument is the old width, the second argument is the new width.
	idlib::signal<void(float, float)> WidthPixelsChanged;

	/// @brief Get the width of the viewport in pixels.
	/// @return the width of the viewport in pixels
	/// @default Default is \f$800\f$.
	float getWidthPixels() const;
	
	/// @brief Set the width of the viewport in pixels.
	/// @param widthPixels the width of the viewport in pixels
	void setWidthPixels(float widthPixels);
	
public:
	/// @brief Signal raised if the height of the viewport in pixels changed.
    /// The first argument is the old height, the second argument is the new height.
	idlib::signal<void(float, float)> HeightPixelsChanged;

	/// @brief Get the height of the viewport in pixels.
	/// @return the height of the viewport in pixels
	/// @default Default is \f$600\f$.
	float getHeightPixels() const;
	
	/// @brief Set the height of the viewport in pixels
	/// @param heightPixels the height of the viewport in pixels
	void setHeightPixels(float heightPixels);

public:
    /// @brief Signal raised if the clear colour of the viewport changed.
    /// The first argument is the old clear colour, the second argument is the new clear colour.
    idlib::signal<void(Ego::Math::Colour4f, Ego::Math::Colour4f)> ClearColourChanged;

    /// @brief Get the clear colour of the viewport.
    /// @return the clear colour of the viewport
    /// @default Default is \f$(0, 0, 0, 0)\f$.
    const Ego::Math::Colour4f& getClearColour() const;

    /// @brief Set the clear colour of the viewport.
    /// @param clearColour the clear colour of the viewport
    void setClearColour(const Ego::Math::Colour4f& clearColour);

public:
    /// @brief Signal raised if the clear depth of the viewport changed.
    /// The first argument is the old clear depth, the second argument is the new clear depth.
    idlib::signal<void(float, float)> ClearDepthChanged;

    /// @brief Get the clear depth of the viewport.
    /// @return the clear depth of the viewport
    /// @default Default is \f$1\f$.
    float getClearDepth() const;

    /// @brief Set the clear depth of the viewport.
    /// @param clearColour the clear depth of the viewport
    void setClearDepth(float clearDepth);
};
	
} // namespace Graphics
} // namespace Ego
