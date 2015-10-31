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

#include <game/Module/Module.hpp>

class Collidable
{
public:
    Collidable() :
        _position(0.0f, 0.0f, 0.0f),
        _spawnPosition(0.0f, 0.0f, 0.0f),
        _safePosition(0.0f, 0.0f, 0.0f),
        _safeValid(false),
        _tile(TileIndex::Invalid),
        _block(BlockIndex::Invalid)
    {
        //ctor
    }

    /**
    * @return
    *   true if this Entity can collide with another Entity
    **/
    virtual bool canCollide() const = 0;

    /**
     * @return the current position of this object
     */
    inline const Vector3f& getPosition() const {
        return _position;
    }

    /**
     * @return 
     *  the initial position of this object
     */
    inline const Vector3f& getSpawnPosition() const {
        return _spawnPosition;
    }

    bool hasSafePosition() const {
        return _safeValid;
    }

    void setSafePosition(const Vector3f &pos) {
        _safePosition = pos;
        _safeValid = true;
    }

    /**
    * @brief Set current X, Y, Z position of this Object
    * @return true if the position of this object has changed
    **/
    inline bool setPosition(const float x, const float y, const float z) {
        return setPosition(Vector3f(x, y, z));
    }

    /**
    * @brief 
    *   Set current position of this Object
    * @return 
    *   true if the position of this object has changed
    **/
    bool setPosition(const Vector3f &pos) {
        EGO_DEBUG_VALIDATE(pos);

        //No change?
        if(pos == _position) {
            return false;
        }

        //Change our new position
        _position = pos;

        _tile = _currentModule->getMeshPointer()->get_grid(PointWorld(getPosX(), getPosY()));
        _block = _currentModule->getMeshPointer()->get_block(PointWorld(getPosX(), getPosY()));

        //Are we inside a wall now?
        Vector2f nrm;
        float pressure = 0.0f;
        BIT_FIELD hit_a_wall = hit_wall(nrm, &pressure, NULL);
        if (EMPTY_BIT_FIELD == hit_a_wall && 0.0f <= pressure)
        {
            //Nope, this is a safe position
            setSafePosition(getPosition());
        }

        return true;
    }

    /**
    * @brief
    *   Changes the first initial position of the Object. This is the
    *   location where it will respawn when it dies or the area a monster
    *   might patrol.
    **/
    void setSpawnPosition(const Vector3f &pos) {
        _spawnPosition = pos;
    }

    /**
     * @return the position of this object along the x-axis
     */
    inline float getPosX() const {
        return _position[kX];
    }

    /**
     * @return the position of this object along the y-axis
     */
    inline float getPosY() const {
        return _position[kY];
    }

    /**
     * @return the position of this object along the z-axis
     */
    inline float getPosZ() const {
        return _position[kZ];
    }

    /**
    * @return
    *   Our last "safe" location where we did not hit a wall
    *   If no safe location is valid, the spawn position will
    *   be returned instead.
    * @see
    *   hasSafeLocation()
    */
    const Vector3f& getSafePosition() const {
        if(!_safeValid) {
            return _spawnPosition;
        }
        return _safePosition;
    }

    /**
     * @brief Get the tile this object is currently on.
     * @return the tile index of the tile this object is on.
     * If the object is currently on no tile, TileIndex::Invalid is returned.
     */
    inline const TileIndex& getTile() const {
        return _tile;
    }

    /**
     * @brief Get the block this object is currently on.
     * @return the block index of the block this object is on.
     * If the object is currently on no block, BlockIndex::Invalid is returned.
     */
    inline const BlockIndex& getBlock() const {
        return _block;
    }

    /// @brief Return nonzero if the entity hit a wall that the entity is not allowed to cross.
    inline BIT_FIELD hit_wall(Vector2f& nrm, float *pressure, mesh_wall_data_t *data)
    {
        return hit_wall(getPosition(), nrm, pressure, data);
    }

    /// @brief Returns nonzero if the entity hit a wall that the entity is not allowed to cross.
    virtual BIT_FIELD hit_wall(const Vector3f& pos, Vector2f& nrm, float *pressure, mesh_wall_data_t *data) = 0;

    inline BIT_FIELD test_wall(mesh_wall_data_t *data)
    {
        return test_wall(getPosition(), data);
    }

    /// @brief Return nonzero if the entity hit a wall that the entity is not allowed to cross.
    virtual BIT_FIELD test_wall(const Vector3f& pos, mesh_wall_data_t *data) = 0;

protected:
    /**
    * @brief
    *  Current position in the world
    */
    Vector3f _position;

private:
    /**
    * @brief
    *  The initial/starting position.
    */
    Vector3f _spawnPosition;

    /**
    * @brief
    *  Our last "safe" location where we did not hit a wall
    */
    Vector3f _safePosition;
    bool _safeValid;

    /**
     * @brief
     *  The tile this object is on or TileIndex::Invalid if none.
     */
    TileIndex _tile;
    /**
     * @brief
     *  The block this object is on or BlockIndex::Invalid if none.
     */
    BlockIndex _block;
};