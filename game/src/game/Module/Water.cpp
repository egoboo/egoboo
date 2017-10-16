#include "game/Module/Water.hpp"
#include "game/graphic.h"

water_instance_layer_t::water_instance_layer_t() :
    _frame(0),
    _frame_add(0),
    _z(0.0f),
    _amp(0.0f),
    _dist(),
    _tx(),
    _light_dir(0.0f),
    _light_add(0.0f),
    _alpha(0),
    _tx_add()
{

}

float water_instance_layer_t::get_level() const
{
    return _z + _amp;
}

void water_instance_layer_t::update()
{
    _tx[SS] += _tx_add[SS];
    _tx[TT] += _tx_add[TT];

    if (_tx[SS] >  1.0f) _tx[SS] -= 1.0f;
    if (_tx[TT] >  1.0f) _tx[TT] -= 1.0f;
    if (_tx[SS] < -1.0f) _tx[SS] += 1.0f;
    if (_tx[TT] < -1.0f) _tx[TT] += 1.0f;

    _frame = (_frame + _frame_add) & WATERFRAMEAND;
}

void water_instance_t::make(const wawalite_water_t& source)
{
    /// @author ZZ
    /// @details This function sets up water movements

    /// @todo wawalite_water_t.layer_count should be an unsigned type.
    ///       layer should be the same type. 
    for (int layer = 0; layer < source.layer_count; ++layer)
    {
        _layers[layer]._tx[SS] = 0;
        _layers[layer]._tx[TT] = 0;

        for (size_t frame = 0; frame < (size_t)MAXWATERFRAME; ++frame)
        {
            // Do first mode
            for (size_t point = 0; point < (size_t)WATERPOINTS; ++point)
            {
                using namespace Ego::Math;
                float temp = (frame * id::two_pi<float>() / MAXWATERFRAME)
                    + (id::two_pi<float>() * point / WATERPOINTS) + (id::pi_over<float, 2>() * layer / MAXWATERLAYER);
                temp = std::sin(temp);
                _layer_z_add[layer][frame][point] = temp * source.layer[layer].amp;
            }
        }
    }

    // Calculate specular highlights
    for (size_t i = 0; i < 256; ++i)
    {
        uint8_t spek = 0;
        if (i > source.spek_start)
        {
            float temp = i - source.spek_start;
            temp = temp / (256 - source.spek_start);
            temp = temp * temp;
            spek = temp * source.spek_level;
        }

        /// @note claforte@> Probably need to replace this with a
        ///           GL_DEBUG(glColor4f)(spek/256.0f, spek/256.0f, spek/256.0f, 1.0f) call:
        if (!gfx.gouraudShading_enable)
            _spek[i] = 0;
        else
            _spek[i] = spek;
    }
}

void water_instance_t::upload(const wawalite_water_t& source)
{
    // upload the data
    _surface_level = source.surface_level;
    _douse_level = source.douse_level;

    _is_water = source.is_water;
    _overlay_req = source.overlay_req;
    _background_req = source.background_req;

    _light = source.light;

    _foregroundrepeat = source.foregroundrepeat;
    _backgroundrepeat = source.backgroundrepeat;

    // upload the layer data
    _layer_count = source.layer_count;
    upload_water_layer_data(_layers, source.layer, source.layer_count);

    make(source);

    // Allow slow machines to ignore the fancy stuff
    if (!egoboo_config_t::get().graphic_twoLayerWater_enable.getValue() && _layer_count > 1)
    {
        int iTmp = source.layer[0].light_add;
        iTmp = (source.layer[1].light_add * iTmp * id::fraction<float, 1, 255>()) + iTmp;
        if (iTmp > 255) iTmp = 255;

        _layer_count = 1;
        _layers[0]._light_add = iTmp * id::fraction<float, 1, 255>();
    }
}

void water_instance_t::update() {
    for (size_t i = 0; i < (size_t)MAXWATERLAYER; ++i) {
        _layers[i].update();
    }
}

void water_instance_t::set_douse_level(float level)
{
    // get the level difference
    float dlevel = level - _douse_level;

    // update all special values
    _surface_level += dlevel;
    _douse_level += dlevel;

    // update the gfx height of the water
    for (size_t i = 0; i < (size_t)MAXWATERLAYER; ++i) {
        _layers[i]._z += dlevel;
    }
}

float water_instance_t::get_level() const
{
    float level = _layers[0].get_level();

    if (egoboo_config_t::get().graphic_twoLayerWater_enable.getValue())
    {
        for (size_t i = 1; i < (size_t)MAXWATERLAYER; ++i)
        {
            level = std::max(level, _layers[i].get_level());
        }
    }

    return level;
}

bool upload_water_layer_data(water_instance_layer_t inst[], const wawalite_water_layer_t data[], const int layer_count)
{
    if (nullptr == inst || 0 == layer_count) return false;

    for (int layer = 0; layer < layer_count; layer++)
    {
        //Reset to default
        inst[layer] = water_instance_layer_t();
    }

    // set the frame
    for (int layer = 0; layer < layer_count; layer++)
    {
        inst[layer]._frame = Random::next<uint16_t>(WATERFRAMEAND);
    }

    if (nullptr != data)
    {
        for (int layer = 0; layer < layer_count; layer++)
        {
            const wawalite_water_layer_t * pwawa = data + layer;
            water_instance_layer_t       * player = inst + layer;

            player->_z = pwawa->z;
            player->_amp = pwawa->amp;

            player->_dist = pwawa->dist;

            player->_light_dir = pwawa->light_dir / 63.0f;
            player->_light_add = pwawa->light_add / 63.0f;

            player->_tx_add = pwawa->tx_add;

            player->_alpha = pwawa->alpha;

            player->_frame_add = pwawa->frame_add;
        }
    }

    return true;
}
