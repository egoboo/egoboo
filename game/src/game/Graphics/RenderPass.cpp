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

/// @file game/Graphics/RenderPass.cpp
/// @brief Abstract implementation render passes
/// @author Michael Heilmann

#include "game/Graphics/RenderPass.hpp"

namespace Ego {
namespace Graphics {

RenderPass::RenderPass(const std::string& name) :
    clock(name, 512)
{}

RenderPass::~RenderPass()
{}

void RenderPass::run(::Camera& camera, const TileList& tileList, const EntityList& entityList)
{
    ClockScope<ClockPolicy::NonRecursive> clockScope(clock);
    OpenGL::Utilities::isError();
    doRun(camera, tileList, entityList);
    OpenGL::Utilities::isError();
}

} // namespace Graphics
} // namespace Ego
