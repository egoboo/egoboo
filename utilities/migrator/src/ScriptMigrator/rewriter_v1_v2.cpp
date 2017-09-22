#include "ScriptMigrator/rewriter_v1_v2.hpp"
#include "ScriptMigrator/tree_v1.hpp"
#include "ScriptMigrator/tree_v2.hpp"

rewriter_v1_v2::rewriter_v1_v2(id::diagnostics *diagnostics) :
    m_diagnostics(diagnostics)
{
    if (!m_diagnostics)
    {
        throw id::null_error(__FILE__, __LINE__, "diagnostics");
    }
}

rewriter_v1_v2::~rewriter_v1_v2()
{}

std::shared_ptr<::word_v2> rewriter_v1_v2::word(const std::shared_ptr<::word_v1>& word_v1)
{
    switch (word_v1->id::category_element<word_kind_v1, word_kind_v1::unknown>::category())
    {
        case word_kind_v1::op:
        {
            auto word_v2 = std::make_shared<::word_v2>(word_kind_v2::op, word_v1->get_location());
            word_v2->set_text(word_v1->get_text());
            return word_v2;
        } break;
        case word_kind_v1::identifier:
        {
            if (word_v1->get_text() == "End")
            {
                return nullptr;
            }
            std::unordered_map<std::string,std::string> nullary =
            {
                // spawn
                { "selfspawnx", "self.spawn.x" },
                { "selfspawny", "self.spawn.y" },
                // turn
                { "targetturn", "target.turn" },
                { "selfturn", "self.turn" },
                { "leaderturn", "leader.turn" },
                // position (z for self)
                { "selfz", "self.z" },
                // target altitude
                { "targetaltitude", "target.altitude" },
                { "targetteam", "target.team" },
                { "targetmaxlife", "target.maxlife" },
                // position (x/y for target and self and leader)
                { "targetx", "target.x"},
                { "selfx", "self.x" },
                { "leaderx", "leader.x" },
                { "targety", "target.y" },
                { "selfy", "self.y" },
                { "leadery", "leader.y" },
                // speed (target and self)
                { "targetspeedx", "target.speed.x" },
                { "selfspeedx", "self.speed.x" },
                { "targetspeedy", "target.speed.y" },
                { "selfspeedy", "self.speed.y" },
                { "targetspeedz", "target.speed.z" },
                { "selfspeedz", "self.speed.z" },
                // strength (target and self)
                { "targetstr", "target.strength" },
                { "selfstr", "self.strength" },
                // intelligence (target and self)
                { "targetint", "target.intelligence" },
                { "selfint", "self.intelligence" },
                // wisdom (target and self)
                { "targetwis", "target.wisdom" },
                { "selfwis", "self.wisdom" },
                // dexterity (target and self)
                { "targetdex", "target.dexterity" },
                { "selfdex", "self.dexterity" },
                // life (target and self)
                { "targetlife", "target.life" },
                { "selflife", "self.life" },
                // mana (target and self)
                { "targetmana", "target.mana" },
                { "selfmana", "self.mana" },
                // level (target and self)
                { "targetlevel", "target.level" },
                { "selflevel", "self.level" },
                // local variables x, y, argument, turn, distance
                { "tmpx", "x" },
                { "tmpy", "y" },
                { "tmpturn", "turn" },
                { "tmpdistance", "distance" },
                { "tmpargument", "argument" },
                // Give*
                { "GiveDexterityToTarget", "GiveDexterityToTarget(argument)" },
                { "GiveExperienceToGoodTeam", "GiveExperienceToGoodTeam(argument, distance)" },
                { "GiveExperienceToTarget", "GiveExperienceToTarget(argument, distance) "},
                { "GiveExperienceToTargetTeam", "GiveExperienceToTargetTeam(argument, distance)" },
                { "GiveIntelligenceToTarget", "GiveIntelligenceToTarget(argument)" },
                { "GiveLifeToTarget", "GiveLifeToTarget(argument)" },
                { "GiveManaToTarget", "GiveManaToTarget(argument)" },
                { "GiveStrengthToTarget", "GiveStrengthToTarget(argument)" },
                { "GiveSkillToTarget", "GiveSkillToTarget(argument)" },
                /// Issue*
                { "IssueOrder", "IssueOrder(argument)" },
                // Waypoints & Compass
                { "ClearWaypoints", "ClearWaypoints()" },
                { "AddWaypoint", "AddWaypoint(x, y)" },
                { "Compass", "Compass(turn, distance)" },
                // If*
                { "IfArmorIs", "if (ArmorIs(argument))" },
                { "IfAttacked", "if (Attacked())" },
                { "IfBackstabbed", "if (Backstabbed())" },
                { "IfBored", "if (Bored())" },
                { "IfBumped", "if (Bumped())" },
                { "IfContentIs", "if (ContentIs(argument))" },
                { "IfDropped", "if (Dropped())" },
                { "IfGrabbed", "if (Grabbed())" },
                { "IfHealed", "if (Healed())" },
                { "IfHeld", "if (Held())" },
                { "IfHoldingMeleeWeapon", "if(HoldingMeleeWeapon())" },
                { "IfKilled", "if (Killed())" },
                { "IfHitGround", "if (HitGround())" },
                { "IfOrdered", "if (Ordered())" },
                { "IfUsed", "if (Used())" },
                // IfTarget*
                { "IfTargetHasSpecialID", "if (TargetHasSpecialID(argument))" },
                { "IfTargetKilled", "if(TargetKilled())" },
                { "IfTargetIsFemale", "if (TargetIsFemale())" },
                { "IfTargetIsOnHatedTeam", "if (TargetIsOnHatedTeam())" },
                { "IfTargetIsOnSameTeam", "if (TargetIsOnSameTeadm())" },
                { "IfTargetIsSelf", "if (TargetIsSelf())" },
                { "IfTargetIsAPlayer", "if (TargetIsPlayer())" },
                { "IfTargetIsMounted", "if (TargetIsMounted())" },
                // IfStateIs*
                { "IfStateIs0",  "if (GetState() == 0)" },
                { "IfStateIs1",  "if (GetState() == 1)" },
                { "IfStateIs2",  "if (GetState() == 2)" },
                { "IfStateIs3",  "if (GetState() == 3)" },
                { "IfStateIs4",  "if (GetState() == 4)" },
                { "IfStateIs5",  "if (GetState() == 5)" },
                { "IfStateIs6",  "if (GetState() == 6)" },
                { "IfStateIs7",  "if (GetState() == 7)" },
                { "IfStateIs8",  "if (GetState() == 8)" },
                { "IfStateIs9",  "if (GetState() == 9)" },
                { "IfStateIs10", "if (GetState() == 10)" },
                { "IfSpawned", "if (Spawned())" },
                { "IfPassageOpen", "if (PassageOpen(argument))" },
                { "IfTakenOut", "if (TakenOut())" },
                { "IfNotDropped", "if (not Dropped())" },
                { "IfXIsEqualToY", "if (x == y)" },
                { "IfXIsMoreThanY", "if (x > y)" },
                { "IfXIsLessThanY", "if (x < y)" },
                { "IfTimeOut", "if (TimeOut())" },
                { "IfStateIs", "if (GetState() == argument)" },
                // Change*
                { "ChangeArmor", "ChangeArmor(argument)" },
                { "ChangeTargetArmor", "ChangeTargetArmor(argument)" },
                { "ChangeTargetClass", "ChangeTargetClass(argument)" },
                // Drop*
                { "DropKeys", "DropKeys()" },
                { "DropItems", "DropItems()" },
                { "DropWeapons", "DropWeapons()" },
                { "DropMoney", "DropMoney()" },
                { "DropTargetMoney", "DropTargetMoney" },
                // Set*
                { "SetBumpHeight", "SetBumpHeight(argument)" },
                { "SetBumpSize", "SetBumpSize(argument)" },
                { "SetTime", "SetTime(argument)" },
                // SetTarget*
                { "SetTargetToLeftHand", "SetTargetToLeftHand()" },
                { "SetTargetToRightHand", "SetTargetToRightHand()" },
                { "SetTargetToNearbyEnemy", "SetTargetToNearbyEnemy()" },
                { "SetTargetToDistantEnemy", "SetTargetToDistantEnemy()" },
                { "SetTargetToOwner", "SetTargetToOwner()" },
                { "SetTargetToRider", "SetTargetToRider()" },
                { "SetTargetToSelf", "SetTargetToSelf()" },
                { "SetTargetToWhoeverAttacked", "SetTargetToWhoeverAttacked()" },
                { "SetTargetToWhoeverBumped", "SetTargetToWhoeverBumped()" },
                { "SetTargetToWhoeverIsHolding", "SetTargetToWhoeverIsHolding()" },
                { "SetTargetToWhoeverIsInPassage", "SetTargetToWhoeverIsInPassage()" },
                { "SetTargetToLowestTarget", "SetTargetToLowestTarget()" },
                // Spawn*
                { "SpawnAttachedParticle", "SpawnAttachedParticle(argument, distance)" },
                { "SpawnExactParticle", "SpawnExactParticle(argument, turn, x, y, distance)" },
                { "SpawnPoof", "SpawnPoof()" },
                { "SpawnExactCharacterXYZ", "SpawnExactCharacterXYZ(argument, x, y, distance, turn)" },

                { "SetTargetToOldTarget", "SetTargetToOldTarget()" },
                { "SetOldTarget", "SetOldTarget()" },

                { "SetState", "SetState(argument)" },
                // *Poof*
                { "PoofTarget", "PoofTarget()" },
                { "GoPoof", "GoPoof()" },
                //
                { "DoAction", "if (DoAction(argument))" },
                { "KeepAction", "KeepAction()" },
                { "Else", "else" },
                { "OpenPassage", "OpenPassage(argument)" },
                { "DoActionOverride", "DoActionOverride(argument)" },
                { "ClosePassage", "ClosePassage(argument)" },
                { "SendMessageNear", "SendMessageNear(argument)" },
                { "SetReloadTime", "SetReloadTime(argument)" },
                { "PlaySound", "PlaySound(argument)" },
                { "GetContent", "argument = content" },
                { "SetContent", "content = argument" },
                { "PlayFullSound", "PlayFullSound(argument)" },
                { "PlayMusic", "PlayMusic(argument, distance)" },

                { "ShowBlipXY", "ShowBlipXY(x, y, argument)" },
            };
            auto it = nullary.find(word_v1->get_text());
            if (it != nullary.cend())
            {
                auto word_v2 = std::make_shared<::word_v2>(word_kind_v2::identifier, word_v1->get_location());
                word_v2->set_text((*it).second);
                return word_v2;
            }
            else
            {
                m_diagnostics->report(word_v1->get_location(), id::severity::error, id::pass::semantical, "unknown/unhandled expression `" + word_v1->get_text() + "`");
                auto word_v2 = std::make_shared<::word_v2>(word_kind_v2::identifier, word_v1->get_location());
                word_v2->set_text(word_v1->get_text());
                return word_v2;
            }
        } break;
        case word_kind_v1::number:
        {
            auto word_v2 = std::make_shared<::word_v2>(word_kind_v2::number, word_v1->get_location());
            word_v2->set_text(word_v1->get_text());
            return word_v2;
        } break;
        case word_kind_v1::string:
        {
            auto word_v2 = std::make_shared<::word_v2>(word_kind_v2::string, word_v1->get_location());
            word_v2->set_text(word_v1->get_text());
            return word_v2;
        } break;
        case word_kind_v1::idsz:
        {
            auto word_v2 = std::make_shared<::word_v2>(word_kind_v2::identifier, word_v1->get_location());
            word_v2->set_text("MakeIdsz(\"" + word_v1->get_text() + "\")");
            return word_v2;
        } break;
        case word_kind_v1::unknown:
        default:
        {
            throw id::unhandled_switch_case_error(__FILE__, __LINE__);
        } break;
    };
}

std::shared_ptr<::line_v2> rewriter_v1_v2::line(const std::shared_ptr<::line_v1>& line_v1)
{
    auto line_v2 = std::make_shared<::line_v2>(line_v1->get_location());
    for (const auto& word_v1 : line_v1->get_words())
    {
        auto word_v2 = word(word_v1);
        if (word_v2)
        {
            word_v2->set_parent(line_v2);
            line_v2->get_words().push_back(word_v2);
        }
    }
    return line_v2;
}

std::shared_ptr<::empty_line_v2> rewriter_v1_v2::empty_line(const std::shared_ptr<::empty_line_v1>& empty_line_v1)
{
    auto empty_line_v2 = std::make_shared<::empty_line_v2>(empty_line_v1->get_location());
    return empty_line_v2;
}

std::shared_ptr<::block_v2> rewriter_v1_v2::block(const std::shared_ptr<::block_v1>& block_v1)
{
    auto block_v2 = std::make_shared<::block_v2>(block_v1->get_location());
    auto statement_list_v2 = this->statement_list(block_v1->get_statement_list());
    block_v2->set_statement_list(statement_list_v2);
    return block_v2;
}

std::shared_ptr<::statement_v2> rewriter_v1_v2::statement(const std::shared_ptr<::statement_v1>& statement_v1)
{
    switch (statement_v1->id::category_element<statement_kind_v1, statement_kind_v1::unknown>::category())
    {
        case statement_kind_v1::block:
        {
            auto block_v2 = block(std::dynamic_pointer_cast<::block_v1>(statement_v1));            
            return block_v2;
        }
        break;
        case statement_kind_v1::empty_line:
        {
            auto empty_line_v2 = empty_line(std::dynamic_pointer_cast<::empty_line_v1>(statement_v1));
            return empty_line_v2;
        }
        break;
        case statement_kind_v1::line:
        {
            auto line_v2 = line(std::dynamic_pointer_cast<::line_v1>(statement_v1));
            return line_v2;
        }
        break;
        case statement_kind_v1::statement_list:
        {
            auto statement_list_v2 = statement_list(std::dynamic_pointer_cast<::statement_list_v1>(statement_v1));
            return statement_list_v2;
        }
        break;
        case statement_kind_v1::unknown:
        default:
        {
            throw id::unhandled_switch_case_error(__FILE__, __LINE__);
        }
        break;
    };
}

std::shared_ptr<::statement_list_v2> rewriter_v1_v2::statement_list(const std::shared_ptr<::statement_list_v1>& statement_list_v1)
{
    auto statement_list_v2 = std::make_shared<::statement_list_v2>(statement_list_v1->get_location());
    for (const auto& statement_v1 : statement_list_v1->get_statements())
    {
        auto statement_v2 = this->statement(statement_v1);
        statement_list_v2->get_statements().push_back(statement_v2);
    }
    return statement_list_v2;
}

std::shared_ptr<::program_v2> rewriter_v1_v2::program(const std::shared_ptr<::program_v1>& program_v1)
{
    auto program_v2 = std::make_shared<::program_v2>(program_v1->get_location());
    auto statement_list_v2 = this->statement_list(program_v1->get_statement_list());
    program_v2->set_statement_list(statement_list_v2);
    return program_v2;
}

std::shared_ptr<::program_v2> rewriter_v1_v2::run(const std::shared_ptr<::program_v1>& program_v1)
{
    return this->program(program_v1);
}
