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

#pragma once

#include "game/graphics/Camera.hpp"

class ExtendedCamera : public Camera
{
public:
    ExtendedCamera(const CameraOptions &options);
    ~ExtendedCamera();

    /**
    * Initialization that has to be after object construction
    **/
    void initialize(int renderList, int doList);

    inline const ego_frect_t& getScreen() const { return _screen; }
    void setScreen(float xmin, float ymin, float xmax, float ymax);

    inline int getLastFrame() const {return _lastFrame;}
    inline int getRenderList() const {return _renderList;}
    inline int getDoList() const {return _doList;}
    inline const std::forward_list<CHR_REF>& getTrackList() const {return _trackList;}

    /**
    * @brief Makes this camera track the specified target
    **/
    void addTrackTarget(const CHR_REF target);

    void reset(const ego_mesh_t * pmesh);

    void update(const ego_mesh_t * pmesh);

    void resetTarget( const ego_mesh_t * pmesh );

    void setLastFrame(int frame) {_lastFrame = frame;};

private:
    void updateProjectionExtended();

private:
    std::forward_list<CHR_REF> _trackList;
    ego_frect_t         _screen;

    int                 _lastFrame;
    int                 _renderList;
    int                 _doList;
};
