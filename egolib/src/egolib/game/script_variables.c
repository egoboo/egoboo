#include "egolib/game/script_variables.h"

#include "egolib/Entities/_Include.hpp"
#include "egolib/Script/script.h"
#include "egolib/game/game.h"
#include "egolib/game/Graphics/CameraSystem.hpp"

int32_t load_VARTMPX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return scriptState.x;
}

int32_t load_VARTMPY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return scriptState.y;
}

int32_t load_VARTMPDISTANCE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return scriptState.distance;
}

int32_t load_VARTMPTURN(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return scriptState.turn;
}

int32_t load_VARTMPARGUMENT(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return scriptState.argument;
}

int32_t load_VARRAND(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return Random::next(std::numeric_limits<uint16_t>::max());
}

int32_t load_VARSELFX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return pobject->getPosX();
}

int32_t load_VARSELFY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return pobject->getPosY();
}

int32_t load_VARSELFTURN(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return uint16_t(pobject->ori.facing_z);
}

int32_t load_VARSELFCOUNTER(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return aiState.order_counter;
}

int32_t load_VARSELFORDER(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return aiState.order_value;
}

int32_t load_VARSELFMORALE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return _currentModule->getTeamList()[pobject->team_base].getMorale();
}

int32_t load_VARSELFLIFE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return FLOAT_TO_FP8(pobject->getLife());
}

int32_t load_VARTARGETX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : ptarget->getPosX();
}

int32_t load_VARTARGETY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : ptarget->getPosY();
}

int32_t load_VARTARGETDISTANCE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    if (nullptr == ptarget)
    {
        return 0x7FFFFFFF;
    }
    else
    {
        return std::abs(ptarget->getPosX() - pobject->getPosX())
            + std::abs(ptarget->getPosY() - pobject->getPosY());
    }
}

int32_t load_VARTARGETTURN(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : uint16_t(ptarget->ori.facing_z);
}

int32_t load_VARLEADERX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    if (pleader)
    {
        return pleader->getPosX();
    }
    else
    {
        return pobject->getPosX();
    }
}

int32_t load_VARLEADERY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    if (pleader)
    {
        return pleader->getPosY();
    }
    else
    {
        return pobject->getPosY();
    }
}

int32_t load_VARLEADERDISTANCE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    if (!pleader)
    {
        return 0x7FFFFFFF;
    }
    else
    {
        return std::abs(pleader->getPosX() - pobject->getPosX())
            + std::abs(pleader->getPosY() - pobject->getPosY());
    }
}

int32_t load_VARLEADERTURN(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    if (pleader)
    {
        return uint16_t(pleader->ori.facing_z);
    }
    else
    {
        return uint16_t(pobject->ori.facing_z);
    }
}

int32_t load_VARGOTOX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    ai_state_t::ensure_wp(aiState);

    if (!aiState.wp_valid)
    {
        return pobject->getPosX();
    }
    else
    {
        return aiState.wp[kX];
    }
}

int32_t load_VARGOTOY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    ai_state_t::ensure_wp(aiState);

    if (!aiState.wp_valid)
    {
        return pobject->getPosY();
    }
    else
    {
        return aiState.wp[kY];
    }
}

int32_t load_VARGOTODISTANCE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    ai_state_t::ensure_wp(aiState);

    if (!aiState.wp_valid)
    {
        return 0x7FFFFFFF;
    }
    else
    {
        return std::abs(aiState.wp[kX] - pobject->getPosX())
            + std::abs(aiState.wp[kY] - pobject->getPosY());
    }
}

int32_t load_VARTARGETTURNTO(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    if (nullptr == ptarget)
    {
        return 0;
    }
    else
    {
        int32_t temporary = FACING_T(vec_to_facing(ptarget->getPosX() - pobject->getPosX(), ptarget->getPosY() - pobject->getPosY()));
        return Ego::Math::clipBits<16>(temporary);
    }
}

int32_t load_VARPASSAGE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return aiState.passage;
}

int32_t load_VARWEIGHT(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return pobject->holdingweight;
}

int32_t load_VARSELFALTITUDE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return pobject->getPosZ() - pobject->getObjectPhysics().getGroundElevation();
}

int32_t load_VARSELFID(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return pobject->getProfile()->getIDSZ(IDSZ_TYPE).toUint32();
}

int32_t load_VARSELFHATEID(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return pobject->getProfile()->getIDSZ(IDSZ_HATE).toUint32();
}

int32_t load_VARSELFMANA(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    int32_t temporary = FLOAT_TO_FP8(pobject->getMana());
    if (pobject->getAttribute(Ego::Attribute::CHANNEL_LIFE))
    {
        temporary += FLOAT_TO_FP8(pobject->getLife());
    }
    return temporary;
}

int32_t load_VARTARGETSTR(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::MIGHT));
}

int32_t load_VARTARGETINT(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::INTELLECT));
}

int32_t load_VARTARGETDEX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::AGILITY));
}

int32_t load_VARTARGETLIFE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : FLOAT_TO_FP8(ptarget->getLife());
}

int32_t load_VARTARGETMANA(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    if (nullptr == ptarget)
    {
        return 0;
    }
    else
    {
        int32_t temporary = FLOAT_TO_FP8(ptarget->getMana());
        if (ptarget->getAttribute(Ego::Attribute::CHANNEL_LIFE))
        {
            temporary += FLOAT_TO_FP8(ptarget->getLife());
        }
        return temporary;
    }
}

int32_t load_VARTARGETSPEEDX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : std::abs(ptarget->getVelocity().x());
}

int32_t load_VARTARGETSPEEDY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : std::abs(ptarget->getVelocity().y());
}

int32_t load_VARTARGETSPEEDZ(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : std::abs(ptarget->getVelocity().z());
}

int32_t load_VARSELFSPAWNX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return pobject->getSpawnPosition()[kX];
}

int32_t load_VARSELFSPAWNY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return pobject->getSpawnPosition()[kY];
}

int32_t load_VARSELFSTATE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return aiState.state;
}

int32_t load_VARSELFCONTENT(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return aiState.content;
}

int32_t load_VARSELFSTR(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return FLOAT_TO_FP8(pobject->getAttribute(Ego::Attribute::MIGHT));
}

int32_t load_VARSELFINT(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return FLOAT_TO_FP8(pobject->getAttribute(Ego::Attribute::INTELLECT));
}

int32_t load_VARSELFDEX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return FLOAT_TO_FP8(pobject->getAttribute(Ego::Attribute::AGILITY));
}

int32_t load_VARSELFMANAFLOW(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return FLOAT_TO_FP8(pobject->getAttribute(Ego::Attribute::SPELL_POWER));
}

int32_t load_VARTARGETMANAFLOW(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::SPELL_POWER));
}

int32_t load_VARSELFATTACHED(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return number_of_attached_particles(aiState.getSelf());
}

int32_t load_VARTARGETLEVEL(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : ptarget->experiencelevel;
}

int32_t load_VARTARGETZ(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : ptarget->getPosZ();
}

int32_t load_VARSELFINDEX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return aiState.getSelf().get();
}

int32_t load_VAROWNERX(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == powner) ? 0 : powner->getPosX();
}

int32_t load_VAROWNERY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == powner) ? 0 : powner->getPosY();
}

int32_t load_VAROWNERTURN(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == powner) ? 0 : uint16_t(powner->ori.facing_z);
}

int32_t load_VAROWNERDISTANCE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    if (nullptr == powner)
    {
        return 0x7FFFFFFF;
    }
    else
    {
        return std::abs(powner->getPosX() - pobject->getPosX())
            + std::abs(powner->getPosY() - pobject->getPosY());
    }
}

int32_t load_VAROWNERTURNTO(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    if (nullptr == powner)
    {
        return 0;
    }
    else
    {
        int32_t temporary = FACING_T(vec_to_facing(powner->getPosX() - pobject->getPosX(), powner->getPosY() - pobject->getPosY()));
        return Ego::Math::clipBits<16>(temporary);
    }
}

int32_t load_VARXYTURNTO(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    int32_t temporary = FACING_T(vec_to_facing(scriptState.x - pobject->getPosX(), scriptState.y - pobject->getPosY()));
    return Ego::Math::clipBits<16>(temporary);
}

int32_t load_VARSELFMONEY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return pobject->getMoney();
}

int32_t load_VARSELFACCEL(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (pobject->getAttribute(Ego::Attribute::ACCELERATION) * 100.0f);
}

int32_t load_VARTARGETEXP(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : ptarget->experience;
}

int32_t load_VARSELFAMMO(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return pobject->ammo;
}

int32_t load_VARTARGETAMMO(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : ptarget->ammo;
}

int32_t load_VARTARGETMONEY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : ptarget->getMoney();
}

int32_t load_VARTARGETTURNAWAY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    if (nullptr == ptarget)
    {
        return 0;
    }
    else
    {
        int32_t temporary = FACING_T(vec_to_facing(ptarget->getPosX() - pobject->getPosX(), ptarget->getPosY() - pobject->getPosY()));
        return Ego::Math::clipBits<16>(temporary);
    }
}

int32_t load_VARSELFLEVEL(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return pobject->experiencelevel;
}

int32_t load_VARTARGETRELOADTIME(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : ptarget->reload_timer;
}

int32_t load_VARSPAWNDISTANCE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return std::abs(pobject->getSpawnPosition()[kX] - pobject->getPosX())
        + std::abs(pobject->getSpawnPosition()[kY] - pobject->getPosY());
}

int32_t load_VARTARGETMAXLIFE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::MAX_LIFE));
}

int32_t load_VARTARGETTEAM(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : ptarget->team;
}

int32_t load_VARTARGETARMOR(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : ptarget->skin;
}

int32_t load_VARDIFFICULTY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return static_cast<uint32_t>(egoboo_config_t::get().game_difficulty.getValue());
}

int32_t load_VARTIMEHOURS(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return Ego::Time::LocalTime().getHours();
}

int32_t load_VARTIMEMINUTES(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return Ego::Time::LocalTime().getMinutes();
}

int32_t load_VARTIMESECONDS(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return Ego::Time::LocalTime().getSeconds();
}

int32_t load_VARDATEMONTH(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return Ego::Time::LocalTime().getMonth() + 1; /// @todo The addition of +1 should be removed and
                                                  /// the whole Ego::Time::LocalTime class should be
                                                  /// made available via EgoScript. However, EgoScript
                                                  /// is not yet ready for that ... not yet.
}

int32_t load_VARDATEDAY(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return Ego::Time::LocalTime().getDayOfMonth();
}

int32_t load_VARSWINGTURN(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    auto camera = CameraSystem::get().getCamera(aiState.getSelf());
    return nullptr != camera ? camera->getSwing() << 2 : 0;
}

int32_t load_VARXYDISTANCE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return std::sqrt(scriptState.x * scriptState.x + scriptState.y * scriptState.y);
}

int32_t load_VARSELFZ(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return pobject->getPosZ();
}

int32_t load_VARTARGETALTITUDE(script_state_t& scriptState, ai_state_t& aiState, Object *pobject, Object *ptarget, Object *powner, Object *pleader)
{
    return (nullptr == ptarget) ? 0 : ptarget->getPosZ() - ptarget->getObjectPhysics().getGroundElevation();
}
