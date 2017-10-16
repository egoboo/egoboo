#include "game/Module/Weather.hpp"
#include "game/Logic/Player.hpp"
#include "game/Module/Module.hpp"
#include "egolib/Entities/_Include.hpp"

void WeatherState::update()
{
    //Does this module have valid weather?
    if (time < 0 || part_gpip == LocalParticleProfileRef::Invalid) {
        return;
    }

    time--;
    if (0 == time)
    {
        time = timer_reset;

        // Find a valid player
        std::shared_ptr<Ego::Player> player = nullptr;
        if (!_currentModule->getPlayerList().empty()) {
            iplayer = (iplayer + 1) % _currentModule->getPlayerList().size();
            player = _currentModule->getPlayerList()[iplayer];
        }

        // Did we find one?
        if (player)
        {
            const std::shared_ptr<Object> pchr = player->getObject();
            if (pchr)
            {
                // Yes, so spawn nearby that character
                std::shared_ptr<Ego::Particle> particle = ParticleHandler::get().spawnGlobalParticle(pchr->getPosition(), ATK_FRONT, part_gpip, 0, over_water);
                if (particle)
                {
                    // Weather particles spawned at the edge of the map look ugly, so don't spawn them there
                    if (particle->getPosX() < EDGE || particle->getPosX() > _currentModule->getMeshPointer()->_tmem._edge_x - EDGE)
                    {
                        particle->requestTerminate();
                    }
                    else if (particle->getPosY() < EDGE || particle->getPosY() > _currentModule->getMeshPointer()->_tmem._edge_y - EDGE)
                    {
                        particle->requestTerminate();
                    }
                }
            }
        }
    }
}

void WeatherState::upload(const wawalite_weather_t& source)
{
    this->iplayer = 0;

    this->timer_reset = source.timer_reset;
    this->over_water = source.over_water;
    this->part_gpip = source.part_gpip;

    // Ensure an update.
    this->time = this->timer_reset;
}
