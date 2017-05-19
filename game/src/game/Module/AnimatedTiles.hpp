#pragma once

#include "egolib/egolib.h"

/// The state of the animated tiles.
struct AnimatedTilesState {
    /// The state of a layer of the animated tiles.
    struct Layer {
        Layer();

        int    update_and;            ///< how often to update the tile

        uint16_t frame_and;             ///< how many images within the "tile set"?
        uint16_t base_and;              ///< animated "tile set"
        uint16_t frame_add;             ///< which image within the tile set?
        uint16_t frame_add_old;         ///< the frame offset, the last time it was updated
        uint32_t frame_update_old;
        /// @brief Iterate the state of the layer.
        void update();
    };
    std::array<Layer, 2> elements;
    void upload(const wawalite_animtile_t& source);
    /// @brief Iterate the state of the layers.
    void update();
};
