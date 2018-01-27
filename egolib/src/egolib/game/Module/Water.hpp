#pragma once

#include "egolib/egolib.h"

// water constants
#define MAXWATERLAYER 2                                    ///< Maximum water layers
#define MAXWATERFRAME (1 << 10)                            ///< Maximum number of wave frames
#define WATERFRAMEAND (MAXWATERFRAME-1)
#define WATERPOINTS 4                                      ///< Points in a water fan

/// The data descibing the state of a water layer
struct water_instance_layer_t
{
    water_instance_layer_t();

    uint16_t _frame;        ///< Frame
    uint32_t _frame_add;    ///< Speed

    /**
     * @brief
     *  The base height of this water layer.
     */
    float _z;            ///< Base height of this water layer
    /**
     * @brief
     *  The peak amplitude of the waves of this water layer.
     */
    float _amp;

    Vector2f _dist;         ///< some indication of "how far away" the layer is if it is an overlay

    Vector2f _tx;           ///< Coordinates of texture

    float _light_dir;    ///< direct  reflectivity 0 - 1
    float _light_add;    ///< ambient reflectivity 0 - 1

    uint8_t _alpha;        ///< layer transparency

    Vector2f _tx_add;            ///< Texture movement

    /**
     * @brief
     *  Animate this water instance layer.
     */
    void update();

    /**
     * @brief
     *  Get the level of this water layer.
     * @return
     *  the level of this water layer
     * @remark
     *  The level of a water layer is its base height plus the peak amplitude of its waves i.e.
     *  <tt>z + amp</tt>.
     */
    float get_level() const;
};

//--------------------------------------------------------------------------------------------

/// The data descibing the water state
struct water_instance_t
{
    float _surface_level;          ///< Surface level for water striders
    float _douse_level;            ///< Surface level for torches
    bool _is_water;               ///< Is it water?  ( Or lava... )
    bool _overlay_req;
    bool _background_req;
    bool _light;                  ///< Is it light ( default is alpha )

    float _foregroundrepeat;
    float _backgroundrepeat;

    uint32_t _spek[256];              ///< Specular highlights

    int _layer_count;
    water_instance_layer_t _layers[MAXWATERLAYER];

    float  _layer_z_add[MAXWATERLAYER][MAXWATERFRAME][WATERPOINTS];

    /**
     * @brief
     *  Get the level of this water instance.
     * @return
     *  the level of this water instance.
     * @remark
     *  The level of a water instance is the maximum of the levels of its layers.
     */
    float get_level() const;

    /**
     * @brief
     *  Animate this water instance.
     */
    void update();
    void upload(const wawalite_water_t& source);
    void make(const wawalite_water_t& source);

    void set_douse_level(float level);
};

// implementing water layer data
bool upload_water_layer_data(water_instance_layer_t inst[], const wawalite_water_layer_t data[], const int layer_count);

