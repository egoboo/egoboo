#include "egolib/game/Module/AnimatedTiles.hpp"
#include "egolib/game/game.h"

AnimatedTilesState::Layer::Layer() :
    update_and(0),
    frame_and(0),
    base_and(0),
    frame_add(0),
    frame_add_old(0),
    frame_update_old(0)
{}

void AnimatedTilesState::upload(const wawalite_animtile_t& source)
{
    elements.fill(Layer());

    uint32_t frame_and = source.frame_and;
    for (auto& element : elements)
    {
        element.update_and = source.update_and;
        element.frame_and = frame_and;
        element.base_and = ~element.frame_and;
        element.frame_add = 0;
        frame_and = (frame_and << 1) | 1;
    }
}

void AnimatedTilesState::Layer::update()
{
    // skip it if there were no updates
    if (frame_update_old == update_wld) return;

    // save the old frame_add when we update to detect changes
    frame_add_old = frame_add;

    // cycle through all frames since the last time
    for (uint32_t tnc = frame_update_old + 1; tnc <= update_wld; tnc++)
    {
        if (0 == (tnc & update_and))
        {
            frame_add = (frame_add + 1) & frame_and;
        }
    }

    // save the frame update
    frame_update_old = update_wld;
}

void AnimatedTilesState::update()
{
    for (auto& element : elements)
    {
        element.update();
    }
}
