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

#include "egolib/AI/LineOfSight.hpp"
#include "egolib/Mesh/Info.hpp"
#include "game/mesh.h"

bool line_of_sight_info_t::blocked(line_of_sight_info_t& self, std::shared_ptr<const ego_mesh_t> mesh) {
    bool mesh_hit = with_mesh(self, mesh);
    //if (mesh_hit) {
    //    self.x1 = (self.collide_x + 0.5f) * Info<float>::Grid::Size();
    //    self.y1 = (self.collide_y + 0.5f) * Info<float>::Grid::Size();
    //}
    //bool chr_hit = with_characters(self);
    return mesh_hit /*|| chr_hit*/;
}

bool line_of_sight_info_t::with_mesh(line_of_sight_info_t& self, std::shared_ptr<const ego_mesh_t> mesh) {
    int Dx, Dy;
    int ix, ix_stt, ix_end;
    int iy, iy_stt, iy_end;

    int Dbig, Dsmall;
    int ibig, ibig_stt, ibig_end;
    int ismall, ismall_stt, ismall_end;
    int dbig, dsmall;
    int TwoDsmall, TwoDsmallMinusTwoDbig, TwoDsmallMinusDbig;

    bool steep;

    //is there any point of these calculations?
    if (EMPTY_BIT_FIELD == self.stopped_by) return false;

    ix_stt = std::floor(self.x0 / Info<float>::Grid::Size()); /// @todo We have a projection function for that.
    ix_end = std::floor(self.x1 / Info<float>::Grid::Size());

    iy_stt = std::floor(self.y0 / Info<float>::Grid::Size()); /// @todo We have a projection function for that.
    iy_end = std::floor(self.y1 / Info<float>::Grid::Size());

    Dx = self.x1 - self.x0;
    Dy = self.y1 - self.y0;

    steep = (std::abs(Dy) >= std::abs(Dx));

    // determine which are the big and small values
    if (steep)
    {
        ibig_stt = iy_stt;
        ibig_end = iy_end;

        ismall_stt = ix_stt;
        ismall_end = ix_end;
    }
    else
    {
        ibig_stt = ix_stt;
        ibig_end = ix_end;

        ismall_stt = iy_stt;
        ismall_end = iy_end;
    }

    // set up the big loop variables
    dbig = 1;
    Dbig = ibig_end - ibig_stt;
    if (Dbig < 0)
    {
        dbig = -1;
        Dbig = -Dbig;
        ibig_end--;
    }
    else
    {
        ibig_end++;
    }

    // set up the small loop variables
    dsmall = 1;
    Dsmall = ismall_end - ismall_stt;
    if (Dsmall < 0)
    {
        dsmall = -1;
        Dsmall = -Dsmall;
    }

    // pre-compute some common values
    TwoDsmall = 2 * Dsmall;
    TwoDsmallMinusTwoDbig = TwoDsmall - 2 * Dbig;
    TwoDsmallMinusDbig = TwoDsmall - Dbig;

    Index1D fan_last = Index1D::Invalid;
    for (ibig = ibig_stt, ismall = ismall_stt; ibig != ibig_end; ibig += dbig)
    {
        if (steep)
        {
            ix = ismall;
            iy = ibig;
        }
        else
        {
            ix = ibig;
            iy = ismall;
        }

        // check to see if the "ray" collides with the mesh
        Index1D fan = mesh->getTileIndex(Index2D(ix, iy));
        if (Index1D::Invalid != fan && fan != fan_last)
        {
            uint32_t collide_fx = mesh->test_fx(fan, self.stopped_by);
            // collide the ray with the mesh

            if (EMPTY_BIT_FIELD != collide_fx)
            {
                self.collide_x = ix;
                self.collide_y = iy;
                self.collide_fx = collide_fx;

                return true;
            }

            fan_last = fan;
        }

        // go to the next step
        if (TwoDsmallMinusDbig > 0)
        {
            TwoDsmallMinusDbig += TwoDsmallMinusTwoDbig;
            ismall += dsmall;
        }
        else
        {
            TwoDsmallMinusDbig += TwoDsmall;
        }
    }

    return false;
}

bool line_of_sight_info_t::with_characters(line_of_sight_info_t& self) {
    // TODO: Do line/character intersection.
    return false;
}