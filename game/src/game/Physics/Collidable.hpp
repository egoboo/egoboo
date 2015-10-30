#pragma once

class Collidable
{
    /**
    * @return
    *   true if this Entity can collide with another Entity
    **/
    virtual bool canCollide() const = 0;
};
