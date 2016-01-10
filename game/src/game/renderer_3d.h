//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file game/renderer_3d.h
/// @brief Routines for rendering 3d primitves

#pragma once

#include "egolib/egolib.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Forward declarations.
class Camera;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// some lines to be drawn in the display

namespace Ego {
struct LineSegment {
	Vector3f p, q;
	Ego::Math::Colour4f colour;
	int time;
};
}

struct LineSegmentList {
	static const size_t capacity = 100;
	Ego::LineSegment _elements[capacity];
	/// @brief Initialize the line list so that all lines are free.
	void init();
	/// @brief Get the index of a free line.
	/// @return the index of a free line if any, #LINE_COUNT otherwise
	size_t get_free();
	bool add(const Vector3f& p, const Vector3f& q, const int duration);
	void draw_all(Camera& camera);
};

extern LineSegmentList g_lineSegmentList;


//--------------------------------------------------------------------------------------------
// some points to be drawn in the display

namespace Ego {
struct Point {
	Vector3f p;
	Ego::Math::Colour4f colour;
	int time;
};
}


struct PointList {
	static const size_t capacity = 100;
	Ego::Point _elements[capacity];
	/// @brief Initialize the point list so that all points are free.
	void init();
	/// @brief Get the index of a free point.
	/// @return the index of a free point if any, #POINT_COUNT otherwise
	size_t get_free();
	bool add(const Vector3f& p, const int duration);
	void draw_all(Camera& camera);
};

extern PointList g_pointList;


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void gfx_begin_3d(Camera& camera);
void gfx_end_3d();

void render_oct_bb(const oct_bb_t &bv, bool drawSquare, bool drawDiamond, const Ego::Math::Colour4f& squareColour = Ego::Math::Colour4f(1, 0.5f, 1, 0.5f), const Ego::Math::Colour4f& diamondColour = Ego::Math::Colour4f(0.5f, 1, 1, 0.5f));
void render_aabb(const AABB3f& bv, const Ego::Math::Colour4f& colour);
