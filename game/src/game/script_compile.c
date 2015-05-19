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

/// @file game/script_compile.c
/// @brief Implementation of the script compiler
/// @details

#include "game/script_compile.h"
#include "game/game.h"
#include "game/renderer_2d.h"
#include "game/egoboo.h"

struct aicode_t
{
    /// The type.
    uint8_t _type;
    /// The value of th constant.
    uint32_t _value;
    /// The name.
    const char *_name;
};

static const aicode_t AICODES[] =
{
    { 'F', 0, "IfSpawned" },
    { 'F', 1, "IfTimeOut" },
    { 'F', 2, "IfAtWaypoint" },
    { 'F', 3, "IfAtLastWaypoint" },
    { 'F', 3, "IfPutAway" },
    { 'F', 4, "IfAttacked" },
    { 'F', 5, "IfBumped" },
    { 'F', 6, "IfOrdered" },
    { 'F', 7, "IfCalledForHelp" },
    { 'F', 8, "SetContent" },
    { 'F', 9, "IfKilled" },
    { 'F', 10, "IfTargetKilled" },
    { 'F', 11, "ClearWaypoints" },
    { 'F', 12, "AddWaypoint" },
    { 'F', 13, "FindPath" },
    { 'F', 14, "Compass" },
    { 'F', 15, "GetTargetArmorPrice" },
    { 'F', 16, "SetTime" },
    { 'F', 17, "GetContent" },
    { 'F', 18, "JoinTargetTeam" },
    { 'F', 19, "SetTargetToNearbyEnemy" },
    { 'F', 20, "SetTargetToTargetLeftHand" },
    { 'F', 21, "SetTargetToTargetRightHand" },
    { 'F', 22, "SetTargetToWhoeverAttacked" },
    { 'F', 22, "SetTargetToWhoeverHealed" },
    { 'F', 23, "SetTargetToWhoeverBumped" },
    { 'F', 24, "SetTargetToWhoeverCalledForHelp" },
    { 'F', 25, "SetTargetToOldTarget" },
    { 'F', 26, "SetTurnModeToVelocity" },
    { 'F', 27, "SetTurnModeToWatch" },
    { 'F', 28, "SetTurnModeToSpin" },
    { 'F', 29, "SetBumpHeight" },
    { 'F', 30, "IfTargetHasID" },
    { 'F', 31, "IfTargetHasItemID" },
    { 'F', 32, "IfTargetHoldingItemID" },
    { 'F', 33, "IfTargetHasSkillID" },
    { 'F', 34, "Else" },
    { 'F', 35, "Run" },
    { 'F', 36, "Walk" },
    { 'F', 37, "Sneak" },
    { 'F', 38, "DoAction" },
    { 'F', 39, "KeepAction" },
    { 'F', 40, "IssueOrder" },
    { 'F', 41, "DropWeapons" },
    { 'F', 42, "TargetDoAction" },
    { 'F', 43, "OpenPassage" },
    { 'F', 44, "ClosePassage" },
    { 'F', 45, "IfPassageOpen" },
    { 'F', 46, "GoPoof" },
    { 'F', 47, "CostTargetItemID" },
    { 'F', 48, "DoActionOverride" },
    { 'F', 49, "IfHealed" },
    { 'F', 50, "SendMessage" },
    { 'F', 51, "CallForHelp" },
    { 'F', 52, "AddIDSZ" },
    { 'F', 53, "End" },
    { 'F', 54, "SetState" },
    { 'F', 55, "GetState" },
    { 'F', 56, "IfStateIs" },
    { 'F', 57, "IfTargetCanOpenStuff" },
    { 'F', 58, "IfGrabbed" },
    { 'F', 58, "IfMounted" },
    { 'F', 59, "IfDropped" },
    { 'F', 59, "IfDismounted" },
    { 'F', 60, "SetTargetToWhoeverIsHolding" },
    { 'F', 61, "DamageTarget" },
    { 'F', 62, "IfXIsLessThanY" },
    { 'F', 62, "IfYIsMoreThanX" },
    { 'F', 63, "SetWeatherTime" },
    { 'F', 64, "GetBumpHeight" },
    { 'F', 65, "IfReaffirmed" },
    { 'F', 66, "UnkeepAction" },
    { 'F', 67, "IfTargetIsOnOtherTeam" },
    { 'F', 68, "IfTargetIsOnHatedTeam" },
    { 'F', 69, "PressLatchButton" },
    { 'F', 70, "SetTargetToTargetOfLeader" },
    { 'F', 71, "IfLeaderKilled" },
    { 'F', 72, "BecomeLeader" },
    { 'F', 73, "ChangeTargetArmor" },
    { 'F', 74, "GiveMoneyToTarget" },
    { 'F', 75, "DropKeys" },
    { 'F', 76, "IfLeaderIsAlive" },
    { 'F', 77, "IfTargetIsOldTarget" },
    { 'F', 78, "SetTargetToLeader" },
    { 'F', 79, "SpawnCharacter" },
    { 'F', 80, "RespawnCharacter" },
    { 'F', 81, "ChangeTile" },
    { 'F', 82, "IfUsed" },
    { 'F', 83, "DropMoney" },
    { 'F', 84, "SetOldTarget" },
    { 'F', 85, "DetachFromHolder" },
    { 'F', 86, "IfTargetHasVulnerabilityID" },
    { 'F', 87, "CleanUp" },
    { 'F', 88, "IfCleanedUp" },
    { 'F', 89, "IfSitting" },
    { 'F', 89, "IfHeld" },
    { 'F', 90, "IfTargetIsHurt" },
    { 'F', 91, "IfTargetIsAPlayer" },
    { 'F', 92, "PlaySound" },
    { 'F', 93, "SpawnParticle" },
    { 'F', 94, "IfTargetIsAlive" },
    { 'F', 95, "Stop" },
    { 'F', 96, "DisaffirmCharacter" },
    { 'F', 97, "ReaffirmCharacter" },
    { 'F', 98, "IfTargetIsSelf" },
    { 'F', 99, "IfTargetIsMale" },
    { 'F', 100, "IfTargetIsFemale" },
    { 'F', 101, "SetTargetToSelf" },
    { 'F', 102, "SetTargetToRider" },
    { 'F', 103, "GetAttackTurn" },
    { 'F', 104, "GetDamageType" },
    { 'F', 105, "BecomeSpell" },
    { 'F', 106, "BecomeSpellbook" },
    { 'F', 107, "IfScoredAHit" },
    { 'F', 108, "IfDisaffirmed" },
    { 'F', 109, "TranslateOrder" },
    { 'F', 110, "SetTargetToWhoeverWasHit" },
    { 'F', 111, "SetTargetToWideEnemy" },
    { 'F', 112, "IfChanged" },
    { 'F', 113, "IfInWater" },
    { 'F', 114, "IfBored" },
    { 'F', 115, "IfTooMuchBaggage" },
    { 'F', 116, "IfGrogged" },
    { 'F', 117, "IfDazed" },
    { 'F', 118, "IfTargetHasSpecialID" },
    { 'F', 119, "PressTargetLatchButton" },
    { 'F', 120, "IfInvisible" },
    { 'F', 121, "IfArmorIs" },
    { 'F', 122, "GetTargetGrogTime" },
    { 'F', 123, "GetTargetDazeTime" },
    { 'F', 124, "SetDamageType" },
    { 'F', 125, "SetWaterLevel" },
    { 'F', 126, "EnchantTarget" },
    { 'F', 127, "EnchantChild" },
    { 'F', 128, "TeleportTarget" },
    { 'F', 129, "GiveExperienceToTarget" },
    { 'F', 130, "IncreaseAmmo" },
    { 'F', 131, "UnkurseTarget" },
    { 'F', 132, "GiveExperienceToTargetTeam" },
    { 'F', 133, "IfUnarmed" },
    { 'F', 134, "RestockTargetAmmoIDAll" },
    { 'F', 135, "RestockTargetAmmoIDFirst" },
    { 'F', 136, "FlashTarget" },
    { 'F', 137, "SetRedShift" },
    { 'F', 138, "SetGreenShift" },
    { 'F', 139, "SetBlueShift" },
    { 'F', 140, "SetLight" },
    { 'F', 141, "SetAlpha" },
    { 'F', 142, "IfHitFromBehind" },
    { 'F', 143, "IfHitFromFront" },
    { 'F', 144, "IfHitFromLeft" },
    { 'F', 145, "IfHitFromRight" },
    { 'F', 146, "IfTargetIsOnSameTeam" },
    { 'F', 147, "KillTarget" },
    { 'F', 148, "UndoEnchant" },
    { 'F', 149, "GetWaterLevel" },
    { 'F', 150, "CostTargetMana" },
    { 'F', 151, "IfTargetHasAnyID" },
    { 'F', 152, "SetBumpSize" },
    { 'F', 153, "IfNotDropped" },
    { 'F', 154, "IfYIsLessThanX" },
    { 'F', 154, "IfXIsMoreThanY" },
    { 'F', 155, "SetFlyHeight" },
    { 'F', 156, "IfBlocked" },
    { 'F', 157, "IfTargetIsDefending" },
    { 'F', 158, "IfTargetIsAttacking" },
    { 'F', 159, "IfStateIs0" },
    { 'F', 159, "IfStateIsParry" },
    { 'F', 160, "IfStateIs1" },
    { 'F', 160, "IfStateIsWander" },
    { 'F', 161, "IfStateIs2" },
    { 'F', 161, "IfStateIsGuard" },
    { 'F', 162, "IfStateIs3" },
    { 'F', 162, "IfStateIsFollow" },
    { 'F', 163, "IfStateIs4" },
    { 'F', 163, "IfStateIsSurround" },
    { 'F', 164, "IfStateIs5" },
    { 'F', 164, "IfStateIsRetreat" },
    { 'F', 165, "IfStateIs6" },
    { 'F', 165, "IfStateIsCharge" },
    { 'F', 166, "IfStateIs7" },
    { 'F', 166, "IfStateIsCombat" },
    { 'F', 167, "IfContentIs" },
    { 'F', 168, "SetTurnModeToWatchTarget" },
    { 'F', 169, "IfStateIsNot" },
    { 'F', 170, "IfXIsEqualToY" },
    { 'F', 170, "IfYIsEqualToX" },
    { 'F', 171, "DebugMessage" },
    { 'F', 172, "BlackTarget" },
    { 'F', 173, "SendMessageNear" },
    { 'F', 174, "IfHitGround" },
    { 'F', 175, "IfNameIsKnown" },
    { 'F', 176, "IfUsageIsKnown" },
    { 'F', 177, "IfHoldingItemID" },
    { 'F', 178, "IfHoldingRangedWeapon" },
    { 'F', 179, "IfHoldingMeleeWeapon" },
    { 'F', 180, "IfHoldingShield" },
    { 'F', 181, "IfKursed" },
    { 'F', 182, "IfTargetIsKursed" },
    { 'F', 183, "IfTargetIsDressedUp" },
    { 'F', 184, "IfOverWater" },
    { 'F', 185, "IfThrown" },
    { 'F', 186, "MakeNameKnown" },
    { 'F', 187, "MakeUsageKnown" },
    { 'F', 188, "StopTargetMovement" },
    { 'F', 189, "SetXY" },
    { 'F', 190, "GetXY" },
    { 'F', 191, "AddXY" },
    { 'F', 192, "MakeAmmoKnown" },
    { 'F', 193, "SpawnAttachedParticle" },
    { 'F', 194, "SpawnExactParticle" },
    { 'F', 195, "AccelerateTarget" },
    { 'F', 196, "IfDistanceIsMoreThanTurn" },
    { 'F', 197, "IfCrushed" },
    { 'F', 198, "MakeCrushValid" },
    { 'F', 199, "SetTargetToLowestTarget" },
    { 'F', 200, "IfNotPutAway" },
    { 'F', 200, "IfNotTakenOut" },
    { 'F', 201, "IfTakenOut" },
    { 'F', 202, "IfAmmoOut" },
    { 'F', 203, "PlaySoundLooped" },
    { 'F', 204, "StopSound" },
    { 'F', 205, "HealSelf" },
    { 'F', 206, "Equip" },
    { 'F', 207, "IfTargetHasItemIDEquipped" },
    { 'F', 208, "SetOwnerToTarget" },
    { 'F', 209, "SetTargetToOwner" },
    { 'F', 210, "SetFrame" },
    { 'F', 211, "BreakPassage" },
    { 'F', 212, "SetReloadTime" },
    { 'F', 213, "SetTargetToWideBlahID" },
    { 'F', 214, "PoofTarget" },
    { 'F', 215, "ChildDoActionOverride" },
    { 'F', 216, "SpawnPoof" },
    { 'F', 217, "SetSpeedPercent" },
    { 'F', 218, "SetChildState" },
    { 'F', 219, "SpawnAttachedSizedParticle" },
    { 'F', 220, "ChangeArmor" },
    { 'F', 221, "ShowTimer" },
    { 'F', 222, "IfFacingTarget" },
    { 'F', 223, "PlaySoundVolume" },
    { 'F', 224, "SpawnAttachedFacedParticle" },
    { 'F', 225, "IfStateIsOdd" },
    { 'F', 226, "SetTargetToDistantEnemy" },
    { 'F', 227, "Teleport" },
    { 'F', 228, "GiveStrengthToTarget" },
    { 'F', 229, "GiveWisdomToTarget" },
    { 'F', 230, "GiveIntelligenceToTarget" },
    { 'F', 231, "GiveDexterityToTarget" },
    { 'F', 232, "GiveLifeToTarget" },
    { 'F', 233, "GiveManaToTarget" },
    { 'F', 234, "ShowMap" },
    { 'F', 235, "ShowYouAreHere" },
    { 'F', 236, "ShowBlipXY" },
    { 'F', 237, "HealTarget" },
    { 'F', 238, "PumpTarget" },
    { 'F', 239, "CostAmmo" },
    { 'F', 240, "MakeSimilarNamesKnown" },
    { 'F', 241, "SpawnAttachedHolderParticle" },
    { 'F', 242, "SetTargetReloadTime" },
    { 'F', 243, "SetFogLevel" },
    { 'F', 244, "GetFogLevel" },
    { 'F', 245, "SetFogTAD" },
    { 'F', 246, "SetFogBottomLevel" },
    { 'F', 247, "GetFogBottomLevel" },
    { 'F', 248, "CorrectActionForHand" },
    { 'F', 249, "IfTargetIsMounted" },
    { 'F', 250, "SparkleIcon" },
    { 'F', 251, "UnsparkleIcon" },
    { 'F', 252, "GetTileXY" },
    { 'F', 253, "SetTileXY" },
    { 'F', 254, "SetShadowSize" },
    { 'F', 255, "OrderTarget" },
    { 'F', 256, "SetTargetToWhoeverIsInPassage" },
    { 'F', 257, "IfCharacterWasABook" },
    { 'F', 258, "SetEnchantBoostValues" },
    { 'F', 259, "SpawnCharacterXYZ" },
    { 'F', 260, "SpawnExactCharacterXYZ" },
    { 'F', 261, "ChangeTargetClass" },
    { 'F', 262, "PlayFullSound" },
    { 'F', 263, "SpawnExactChaseParticle" },
    { 'F', 264, "CreateOrder" },
    { 'F', 265, "OrderSpecialID" },
    { 'F', 266, "UnkurseTargetInventory" },
    { 'F', 267, "IfTargetIsSneaking" },
    { 'F', 268, "DropItems" },
    { 'F', 269, "RespawnTarget" },
    { 'F', 270, "TargetDoActionSetFrame" },
    { 'F', 271, "IfTargetCanSeeInvisible" },
    { 'F', 272, "SetTargetToNearestBlahID" },
    { 'F', 273, "SetTargetToNearestEnemy" },
    { 'F', 274, "SetTargetToNearestFriend" },
    { 'F', 275, "SetTargetToNearestLifeform" },
    { 'F', 276, "FlashPassage" },
    { 'F', 277, "FindTileInPassage" },
    { 'F', 278, "IfHeldInLeftHand" },
    { 'F', 279, "NotAnItem" },
    { 'F', 280, "SetChildAmmo" },
    { 'F', 281, "IfHitVulnerable" },
    { 'F', 282, "IfTargetIsFlying" },
    { 'F', 283, "IdentifyTarget" },
    { 'F', 284, "BeatModule" },
    { 'F', 285, "EndModule" },
    { 'F', 286, "DisableExport" },
    { 'F', 287, "EnableExport" },
    { 'F', 288, "GetTargetState" },
    { 'F', 289, "IfEquipped" },
    { 'F', 290, "DropTargetMoney" },
    { 'F', 291, "GetTargetContent" },
    { 'F', 292, "DropTargetKeys" },
    { 'F', 293, "JoinTeam" },
    { 'F', 294, "TargetJoinTeam" },
    { 'F', 295, "ClearMusicPassage" },
    { 'F', 296, "ClearEndMessage" },
    { 'F', 297, "AddEndMessage" },
    { 'F', 298, "PlayMusic" },
    { 'F', 299, "SetMusicPassage" },
    { 'F', 300, "MakeCrushInvalid" },
    { 'F', 301, "StopMusic" },
    { 'F', 302, "FlashVariable" },
    { 'F', 303, "AccelerateUp" },
    { 'F', 304, "FlashVariableHeight" },
    { 'F', 305, "SetDamageTime" },
    { 'F', 306, "IfStateIs8" },
    { 'F', 307, "IfStateIs9" },
    { 'F', 308, "IfStateIs10" },
    { 'F', 309, "IfStateIs11" },
    { 'F', 310, "IfStateIs12" },
    { 'F', 311, "IfStateIs13" },
    { 'F', 312, "IfStateIs14" },
    { 'F', 313, "IfStateIs15" },
    { 'F', 314, "IfTargetIsAMount" },
    { 'F', 315, "IfTargetIsAPlatform" },
    { 'F', 316, "AddStat" },
    { 'F', 317, "DisenchantTarget" },
    { 'F', 318, "DisenchantAll" },
    { 'F', 319, "SetVolumeNearestTeammate" },
    { 'F', 320, "AddShopPassage" },
    { 'F', 321, "TargetPayForArmor" },
    { 'F', 322, "JoinEvilTeam" },
    { 'F', 323, "JoinNullTeam" },
    { 'F', 324, "JoinGoodTeam" },
    { 'F', 325, "PitsKill" },
    { 'F', 326, "SetTargetToPassageID" },
    { 'F', 327, "MakeNameUnknown" },
    { 'F', 328, "SpawnExactParticleEndSpawn" },
    { 'F', 329, "SpawnPoofSpeedSpacingDamage" },
    { 'F', 330, "GiveExperienceToGoodTeam" },
    { 'F', 331, "DoNothing" },
    { 'F', 332, "GrogTarget" },
    { 'F', 333, "DazeTarget" },
    { 'F', 334, "EnableRespawn" },
    { 'F', 335, "DisableRespawn" },
    { 'F', 336, "DispelTargetEnchantID" },
    { 'F', 337, "IfHolderBlocked" },
    { 'F', 338, "GetTargetShieldProfiency" },
    { 'F', 339, "IfTargetHasNotFullMana" },
    { 'F', 340, "EnableListenSkill" },
    { 'F', 341, "SetTargetToLastItemUsed" },
    { 'F', 342, "FollowLink" },
    { 'F', 343, "IfOperatorIsLinux" },
    { 'F', 344, "IfTargetIsAWeapon" },
    { 'F', 345, "IfSomeoneIsStealing" },
    { 'F', 346, "IfTargetIsASpell" },
    { 'F', 347, "IfBackstabbed" },
    { 'F', 348, "GetTargetDamageType" },
    { 'F', 349, "AddTargetQuest" },
    { 'F', 350, "BeatQuestAllPlayers" },
    { 'F', 351, "IfTargetHasQuest" },
    { 'F', 352, "SetTargetQuestLevel" },
    { 'F', 353, "AddQuestAllPlayers" },
    { 'F', 354, "AddBlipAllEnemies" },
    { 'F', 355, "PitsFall" },
    { 'F', 356, "IfTargetIsOwner" },
    { 'F', 357, "SetSpeech" },
    { 'F', 364, "TakePicture" },
    { 'F', 365, "IfOperatorIsMacintosh" },
    { 'F', 366, "IfModuleHasIDSZ" },
    { 'F', 367, "MorphToTarget" },
    { 'F', 368, "GiveManaFlowToTarget" },
    { 'F', 369, "GiveManaReturnToTarget" },
    { 'F', 370, "SetMoney" },
    { 'F', 371, "IfTargetCanSeeKurses" },
    { 'F', 372, "SpawnAttachedCharacter" },
    { 'F', 373, "KurseTarget" },
    { 'F', 374, "SetChildContent" },
    { 'F', 375, "SetTargetToChild" },
    { 'F', 376, "SetDamageThreshold" },
    { 'F', 377, "AccelerateTargetUp" },
    { 'F', 378, "SetTargetAmmo" },
    { 'F', 379, "EnableInvictus" },
    { 'F', 380, "DisableInvictus" },
    { 'F', 381, "TargetDamageSelf" },
    { 'F', 382, "SetTargetSize" },
    { 'F', 383, "IfTargetIsFacingSelf" },
    { 'F', 384, "DrawBillboard" },
    { 'F', 385, "SetTargetToBlahInPassage" },
    { 'F', 386, "IfLevelUp" },
    { 'F', 387, "GiveSkillToTarget" },
    { 'F', 388, "SetTargetToNearbyMeleeWeapon" },
    { 'C', 1, "BLAHDEAD" },
    { 'C', 2, "BLAHENEMIES" },
    { 'C', 4, "BLAHFRIENDS" },
    { 'C', 8, "BLAHITEMS" },
    { 'C', 16, "BLAHINVERTID" },
    { 'C', 32, "BLAHPLAYERS" },
    { 'C', 64, "BLAHSKILL" },
    { 'C', 128, "BLAHQUEST" },
    { 'C', 0, "STATEPARRY" },
    { 'C', 1, "STATEWANDER" },
    { 'C', 2, "STATEGUARD" },
    { 'C', 3, "STATEFOLLOW" },
    { 'C', 4, "STATESURROUND" },
    { 'C', 5, "STATERETREAT" },
    { 'C', 6, "STATECHARGE" },
    { 'C', 7, "STATECOMBAT" },
    { 'C', 4, "GRIPONLY" },
    { 'C', 4, "GRIPLEFT" },
    { 'C', 8, "GRIPRIGHT" },
    { 'C', 0, "SPAWNORIGIN" },
    { 'C', 1, "SPAWNLAST" },
    { 'C', 0, "LATCHLEFT" },
    { 'C', 1, "LATCHRIGHT" },
    { 'C', 2, "LATCHJUMP" },
    { 'C', 3, "LATCHALTLEFT" },
    { 'C', 4, "LATCHALTRIGHT" },
    { 'C', 5, "LATCHPACKLEFT" },
    { 'C', 6, "LATCHPACKRIGHT" },
    { 'C', 0, "DAMAGESLASH" },
    { 'C', 1, "DAMAGECRUSH" },
    { 'C', 2, "DAMAGEPOKE" },
    { 'C', 3, "DAMAGEHOLY" },
    { 'C', 4, "DAMAGEEVIL" },
    { 'C', 5, "DAMAGEFIRE" },
    { 'C', 6, "DAMAGEICE" },
    { 'C', 7, "DAMAGEZAP" },
    { 'C', 0, "ACTIONDA" },
    { 'C', 1, "ACTIONDB" },
    { 'C', 2, "ACTIONDC" },
    { 'C', 3, "ACTIONDD" },
    { 'C', 4, "ACTIONUA" },
    { 'C', 5, "ACTIONUB" },
    { 'C', 6, "ACTIONUC" },
    { 'C', 7, "ACTIONUD" },
    { 'C', 8, "ACTIONTA" },
    { 'C', 9, "ACTIONTB" },
    { 'C', 10, "ACTIONTC" },
    { 'C', 11, "ACTIONTD" },
    { 'C', 12, "ACTIONCA" },
    { 'C', 13, "ACTIONCB" },
    { 'C', 14, "ACTIONCC" },
    { 'C', 15, "ACTIONCD" },
    { 'C', 16, "ACTIONSA" },
    { 'C', 17, "ACTIONSB" },
    { 'C', 18, "ACTIONSC" },
    { 'C', 19, "ACTIONSD" },
    { 'C', 20, "ACTIONBA" },
    { 'C', 21, "ACTIONBB" },
    { 'C', 22, "ACTIONBC" },
    { 'C', 23, "ACTIONBD" },
    { 'C', 24, "ACTIONLA" },
    { 'C', 25, "ACTIONLB" },
    { 'C', 26, "ACTIONLC" },
    { 'C', 27, "ACTIONLD" },
    { 'C', 28, "ACTIONXA" },
    { 'C', 29, "ACTIONXB" },
    { 'C', 30, "ACTIONXC" },
    { 'C', 31, "ACTIONXD" },
    { 'C', 32, "ACTIONFA" },
    { 'C', 33, "ACTIONFB" },
    { 'C', 34, "ACTIONFC" },
    { 'C', 35, "ACTIONFD" },
    { 'C', 36, "ACTIONPA" },
    { 'C', 37, "ACTIONPB" },
    { 'C', 38, "ACTIONPC" },
    { 'C', 39, "ACTIONPD" },
    { 'C', 40, "ACTIONEA" },
    { 'C', 41, "ACTIONEB" },
    { 'C', 42, "ACTIONRA" },
    { 'C', 43, "ACTIONZA" },
    { 'C', 44, "ACTIONZB" },
    { 'C', 45, "ACTIONZC" },
    { 'C', 46, "ACTIONZD" },
    { 'C', 47, "ACTIONWA" },
    { 'C', 48, "ACTIONWB" },
    { 'C', 49, "ACTIONWC" },
    { 'C', 50, "ACTIONWD" },
    { 'C', 51, "ACTIONJA" },
    { 'C', 52, "ACTIONJB" },
    { 'C', 53, "ACTIONJC" },
    { 'C', 54, "ACTIONHA" },
    { 'C', 55, "ACTIONHB" },
    { 'C', 56, "ACTIONHC" },
    { 'C', 57, "ACTIONHD" },
    { 'C', 58, "ACTIONKA" },
    { 'C', 59, "ACTIONKB" },
    { 'C', 60, "ACTIONKC" },
    { 'C', 61, "ACTIONKD" },
    { 'C', 62, "ACTIONMA" },
    { 'C', 63, "ACTIONMB" },
    { 'C', 64, "ACTIONMC" },
    { 'C', 65, "ACTIONMD" },
    { 'C', 66, "ACTIONME" },
    { 'C', 67, "ACTIONMF" },
    { 'C', 68, "ACTIONMG" },
    { 'C', 69, "ACTIONMH" },
    { 'C', 70, "ACTIONMI" },
    { 'C', 71, "ACTIONMJ" },
    { 'C', 072, "ACTIONMK" },
    { 'C', 073, "ACTIONML" },
    { 'C', 074, "ACTIONMM" },
    { 'C', 75, "ACTIONMN" },
    { 'C', 0, "EXPSECRET" },
    { 'C', 1, "EXPQUEST" },
    { 'C', 2, "EXPDARE" },
    { 'C', 3, "EXPKILL" },
    { 'C', 4, "EXPMURDER" },
    { 'C', 5, "EXPREVENGE" },
    { 'C', 6, "EXPTEAMWORK" },
    { 'C', 7, "EXPROLEPLAY" },
    { 'C', 0, "MESSAGEDEATH" },
    { 'C', 1, "MESSAGEHATE" },
    { 'C', 2, "MESSAGEOUCH" },
    { 'C', 3, "MESSAGEFRAG" },
    { 'C', 4, "MESSAGEACCIDENT" },
    { 'C', 5, "MESSAGECOSTUME" },
    { 'C', 0, "ORDERMOVE" },
    { 'C', 1, "ORDERATTACK" },
    { 'C', 2, "ORDERASSIST" },
    { 'C', 3, "ORDERSTAND" },
    { 'C', 4, "ORDERTERRAIN" },
    { 'C', 0, "WHITE" },
    { 'C', 1, "RED" },
    { 'C', 2, "YELLOW" },
    { 'C', 3, "GREEN" },
    { 'C', 4, "BLUE" },
    { 'C', 5, "PURPLE" },
    { 'C', 1, "FXNOREFLECT" },
    { 'C', 2, "FXDRAWREFLECT" },
    { 'C', 4, "FXANIM" },
    { 'C', 8, "FXWATER" },
    { 'C', 16, "FXBARRIER" },
    { 'C', 32, "FXIMPASS" },
    { 'C', 64, "FXDAMAGE" },
    { 'C', 128, "FXSLIPPY" },
    { 'C', 0, "TEAMA" },
    { 'C', 1, "TEAMB" },
    { 'C', 2, "TEAMC" },
    { 'C', 3, "TEAMD" },
    { 'C', 4, "TEAME" },
    { 'C', 5, "TEAMF" },
    { 'C', 6, "TEAMG" },
    { 'C', 7, "TEAMH" },
    { 'C', 8, "TEAMI" },
    { 'C', 9, "TEAMJ" },
    { 'C', 10, "TEAMK" },
    { 'C', 11, "TEAML" },
    { 'C', 12, "TEAMM" },
    { 'C', 13, "TEAMN" },
    { 'C', 14, "TEAMO" },
    { 'C', 15, "TEAMP" },
    { 'C', 16, "TEAMQ" },
    { 'C', 17, "TEAMR" },
    { 'C', 18, "TEAMS" },
    { 'C', 19, "TEAMT" },
    { 'C', 20, "TEAMU" },
    { 'C', 21, "TEAMV" },
    { 'C', 22, "TEAMW" },
    { 'C', 23, "TEAMX" },
    { 'C', 24, "TEAMY" },
    { 'C', 25, "TEAMZ" },
    { 'C', 1, "INVENTORY" },
    { 'C', 2, "LEFT" },
    { 'C', 3, "RIGHT" },
    { 'C', 0, "EASY" },
    { 'C', 1, "NORMAL" },
    { 'C', 2, "HARD" },
    { 'V', 0, "tmpx" },
    { 'V', 1, "tmpy" },
    { 'V', 2, "tmpdist" },
    { 'V', 2, "tmpdistance" },
    { 'V', 3, "tmpturn" },
    { 'V', 4, "tmpargument" },
    { 'V', 5, "rand" },
    { 'V', 6, "selfx" },
    { 'V', 7, "selfy" },
    { 'V', 8, "selfturn" },
    { 'V', 9, "selfcounter" },
    { 'V', 10, "selforder" },
    { 'V', 11, "selfmorale" },
    { 'V', 12, "selflife" },
    { 'V', 13, "targetx" },
    { 'V', 14, "targety" },
    { 'V', 15, "targetdistance" },
    { 'V', 16, "targetturn" },
    { 'V', 17, "leaderx" },
    { 'V', 18, "leadery" },
    { 'V', 19, "leaderdistance" },
    { 'V', 20, "leaderturn" },
    { 'V', 21, "gotox" },
    { 'V', 22, "gotoy" },
    { 'V', 23, "gotodistance" },
    { 'V', 24, "targetturnto" },
    { 'V', 25, "passage" },
    { 'V', 26, "weight" },
    { 'V', 27, "selfaltitude" },
    { 'V', 28, "selfid" },
    { 'V', 29, "selfhateid" },
    { 'V', 30, "selfmana" },
    { 'V', 31, "targetstr" },
    { 'V', 32, "targetwis" },
    { 'V', 33, "targetint" },
    { 'V', 34, "targetdex" },
    { 'V', 35, "targetlife" },
    { 'V', 36, "targetmana" },
    { 'V', 37, "targetlevel" },
    { 'V', 38, "targetspeedx" },
    { 'V', 39, "targetspeedy" },
    { 'V', 40, "targetspeedz" },
    { 'V', 41, "selfspawnx" },
    { 'V', 42, "selfspawny" },
    { 'V', 43, "selfstate" },
    { 'V', 44, "selfstr" },
    { 'V', 45, "selfwis" },
    { 'V', 46, "selfint" },
    { 'V', 47, "selfdex" },
    { 'V', 48, "selfmanaflow" },
    { 'V', 49, "targetmanaflow" },
    { 'V', 50, "selfattached" },
    { 'V', 51, "swingturn" },
    { 'V', 52, "xydistance" },
    { 'V', 53, "selfz" },
    { 'V', 54, "targetaltitude" },
    { 'V', 55, "targetz" },
    { 'V', 56, "selfindex" },
    { 'V', 57, "ownerx" },
    { 'V', 58, "ownery" },
    { 'V', 59, "ownerturn" },
    { 'V', 60, "ownerdistance" },
    { 'V', 61, "ownerturnto" },
    { 'V', 62, "xyturnto" },
    { 'V', 63, "selfmoney" },
    { 'V', 64, "selfaccel" },
    { 'V', 65, "targetexp" },
    { 'V', 66, "selfammo" },
    { 'V', 67, "targetammo" },
    { 'V', 68, "targetmoney" },
    { 'V', 69, "targetturnfrom" },
    { 'V', 70, "selflevel" },
    { 'V', 71, "targetreloadtime" },
    { 'V', 72, "selfcontent" },
    { 'V', 73, "spawndistance" },
    { 'V', 74, "targetmaxlife" },
    { 'V', 75, "targetteam" },
    { 'V', 76, "targetarmor" },
    { 'V', 77, "difficulty" },
    { 'V', 78, "timehours" },
    { 'V', 79, "timeminutes" },
    { 'V', 80, "timeseconds" },
    { 'V', 81, "datemonth" },
    { 'V', 82, "dateday" },
    { 'O', 0, "+" },
    { 'O', 1, "-" },
    { 'O', 2, "&" },
    { 'O', 3, ">" },
    { 'O', 4, "<" },
    { 'O', 5, "*" },
    { 'O', 6, "/" },
    { 'O', 7, "%" },
};

static bool load_ai_codes_vfs();

parser_state_t *parser_state_t::ctor(parser_state_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }

    BLANK_STRUCT_PTR(self);

    // Construct the token.
    token_t::ctor(&(self->token));

    load_ai_codes_vfs();
    debug_script_file = vfs_openWrite("/debug/script_debug.txt");

    // just to be explicit
    self->error = false;

    return self;
}

void parser_state_t::dtor(parser_state_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }

    BLANK_STRUCT_PTR(self);

    // Destruct the token.
    token_t::dtor(&(self->token));

    vfs_close(debug_script_file);
    debug_script_file = nullptr;

    // just to be explicit
    self->error = false;
}

bool parser_state_t::get_error(parser_state_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    return self->error;
}

void parser_state_t::clear_error(parser_state_t *self)
{
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    self->error = false;
}

/// the pointer to the singleton
parser_state_t * parser_state_t::_singleton = nullptr;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static script_info_t default_ai_script;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

StaticArray<opcode_data_t, MAX_OPCODE> OpList;

bool debug_scripts = false;
vfs_FILE *debug_script_file = NULL;

const char *script_function_names[SCRIPT_FUNCTIONS_COUNT] =
{
    /// Scripted AI functions (v0.10)
    "FIFSPAWNED",                          // == 0
    "FIFTIMEOUT",                          // == 1
    "FIFATWAYPOINT",                       // == 2
    "FIFATLASTWAYPOINT",                   // == 3
    "FIFATTACKED",                         // == 4
    "FIFBUMPED",                           // == 5
    "FIFORDERED",                          // == 6
    "FIFCALLEDFORHELP",                    // == 7
    "FSETCONTENT",                         // == 8
    "FIFKILLED",                           // == 9
    "FIFTARGETKILLED",                     // == 10
    "FCLEARWAYPOINTS",                     // == 11
    "FADDWAYPOINT",                        // == 12
    "FFINDPATH",                           // == 13
    "FCOMPASS",                            // == 14
    "FGETTARGETARMORPRICE",                // == 15
    "FSETTIME",                            // == 16
    "FGETCONTENT",                         // == 17
    "FJOINTARGETTEAM",                     // == 18
    "FSETTARGETTONEARBYENEMY",             // == 19
    "FSETTARGETTOTARGETLEFTHAND",          // == 20
    "FSETTARGETTOTARGETRIGHTHAND",         // == 21
    "FSETTARGETTOWHOEVERATTACKED",         // == 22
    "FSETTARGETTOWHOEVERBUMPED",           // == 23
    "FSETTARGETTOWHOEVERCALLEDFORHELP",    // == 24
    "FSETTARGETTOOLDTARGET",               // == 25
    "FSETTURNMODETOVELOCITY",              // == 26
    "FSETTURNMODETOWATCH",                 // == 27
    "FSETTURNMODETOSPIN",                  // == 28
    "FSETBUMPHEIGHT",                      // == 29
    "FIFTARGETHASID",                      // == 30
    "FIFTARGETHASITEMID",                  // == 31
    "FIFTARGETHOLDINGITEMID",              // == 32
    "FIFTARGETHASSKILLID",                 // == 33
    "FELSE",                               // == 34
    "FRUN",                                // == 35
    "FWALK",                               // == 36
    "FSNEAK",                              // == 37
    "FDOACTION",                           // == 38
    "FKEEPACTION",                         // == 39
    "FISSUEORDER",                         // == 40
    "FDROPWEAPONS",                        // == 41
    "FTARGETDOACTION",                     // == 42
    "FOPENPASSAGE",                        // == 43
    "FCLOSEPASSAGE",                       // == 44
    "FIFPASSAGEOPEN",                      // == 45
    "FGOPOOF",                             // == 46
    "FCOSTTARGETITEMID",                   // == 47
    "FDOACTIONOVERRIDE",                   // == 48
    "FIFHEALED",                           // == 49
    "FSENDMESSAGE",                        // == 50
    "FCALLFORHELP",                        // == 51
    "FADDIDSZ",                            // == 52
    "FEND",                                // == 53

    /// Scripted AI functions (v0.20)
    "FSETSTATE",                           // == 54
    "FGETSTATE",                           // == 55
    "FIFSTATEIS",                          // == 56

    /// Scripted AI functions (v0.30)
    "FIFTARGETCANOPENSTUFF",               // == 57
    "FIFGRABBED",                          // == 58
    "FIFDROPPED",                          // == 59
    "FSETTARGETTOWHOEVERISHOLDING",        // == 60
    "FDAMAGETARGET",                       // == 61
    "FIFXISLESSTHANY",                     // == 62

    /// Scripted AI functions (v0.40)
    "FSETWEATHERTIME",                     // == 63
    "FGETBUMPHEIGHT",                      // == 64
    "FIFREAFFIRMED",                       // == 65
    "FUNKEEPACTION",                       // == 66
    "FIFTARGETISONOTHERTEAM",              // == 67

    /// Scripted AI functions (v0.50)
    "FIFTARGETISONHATEDTEAM",              // == 68
    "FPRESSLATCHBUTTON",                   // == 69
    "FSETTARGETTOTARGETOFLEADER",          // == 70
    "FIFLEADERKILLED",                     // == 71
    "FBECOMELEADER",                       // == 72

    /// Scripted AI functions (v0.60)
    "FCHANGETARGETARMOR",                  // == 73
    "FGIVEMONEYTOTARGET",                  // == 74
    "FDROPKEYS",                           // == 75
    "FIFLEADERISALIVE",                    // == 76
    "FIFTARGETISOLDTARGET",                // == 77
    "FSETTARGETTOLEADER",                  // == 78
    "FSPAWNCHARACTER",                     // == 79
    "FRESPAWNCHARACTER",                   // == 80
    "FCHANGETILE",                         // == 81
    "FIFUSED",                             // == 82
    "FDROPMONEY",                          // == 83
    "FSETOLDTARGET",                       // == 84
    "FDETACHFROMHOLDER",                   // == 85
    "FIFTARGETHASVULNERABILITYID",         // == 86
    "FCLEANUP",                            // == 87
    "FIFCLEANEDUP",                        // == 88
    "FIFSITTING",                          // == 89
    "FIFTARGETISHURT",                     // == 90
    "FIFTARGETISAPLAYER",                  // == 91
    "FPLAYSOUND",                          // == 92
    "FSPAWNPARTICLE",                      // == 93
    "FIFTARGETISALIVE",                    // == 94
    "FSTOP",                               // == 95
    "FDISAFFIRMCHARACTER",                 // == 96
    "FREAFFIRMCHARACTER",                  // == 97
    "FIFTARGETISSELF",                     // == 98
    "FIFTARGETISMALE",                     // == 99
    "FIFTARGETISFEMALE",                   // == 100

    // Scripted AI functions (v0.70)
    "FSETTARGETTOSELF",                    // == 101
    "FSETTARGETTORIDER",                   // == 102
    "FGETATTACKTURN",                      // == 103
    "FGETDAMAGETYPE",                      // == 104
    "FBECOMESPELL",                        // == 105
    "FBECOMESPELLBOOK",                    // == 106
    "FIFSCOREDAHIT",                       // == 107
    "FIFDISAFFIRMED",                      // == 108
    "FTRANSLATEORDER",                     // == 109
    "FSETTARGETTOWHOEVERWASHIT",           // == 110
    "FSETTARGETTOWIDEENEMY",               // == 111
    "FIFCHANGED",                          // == 112
    "FIFINWATER",                          // == 113
    "FIFBORED",                            // == 114
    "FIFTOOMUCHBAGGAGE",                   // == 115
    "FIFGROGGED",                          // == 116
    "FIFDAZED",                            // == 117
    "FIFTARGETHASSPECIALID",               // == 118
    "FPRESSTARGETLATCHBUTTON",             // == 119
    "FIFINVISIBLE",                        // == 120
    "FIFARMORIS",                          // == 121
    "FGETTARGETGROGTIME",                  // == 122
    "FGETTARGETDAZETIME",                  // == 123
    "FSETDAMAGETYPE",                      // == 124
    "FSETWATERLEVEL",                      // == 125
    "FENCHANTTARGET",                      // == 126
    "FENCHANTCHILD",                       // == 127
    "FTELEPORTTARGET",                     // == 128
    "FGIVEEXPERIENCETOTARGET",             // == 129
    "FINCREASEAMMO",                       // == 130
    "FUNKURSETARGET",                      // == 131
    "FGIVEEXPERIENCETOTARGETTEAM",         // == 132
    "FIFUNARMED",                          // == 133
    "FRESTOCKTARGETAMMOIDALL",             // == 134
    "FRESTOCKTARGETAMMOIDFIRST",           // == 135
    "FFLASHTARGET",                        // == 136
    "FSETREDSHIFT",                        // == 137
    "FSETGREENSHIFT",                      // == 138
    "FSETBLUESHIFT",                       // == 139
    "FSETLIGHT",                           // == 140
    "FSETALPHA",                           // == 141
    "FIFHITFROMBEHIND",                    // == 142
    "FIFHITFROMFRONT",                     // == 143
    "FIFHITFROMLEFT",                      // == 144
    "FIFHITFROMRIGHT",                     // == 145
    "FIFTARGETISONSAMETEAM",               // == 146
    "FKILLTARGET",                         // == 147
    "FUNDOENCHANT",                        // == 148
    "FGETWATERLEVEL",                      // == 149
    "FCOSTTARGETMANA",                     // == 150
    "FIFTARGETHASANYID",                   // == 151
    "FSETBUMPSIZE",                        // == 152
    "FIFNOTDROPPED",                       // == 153
    "FIFYISLESSTHANX",                     // == 154
    "FSETFLYHEIGHT",                       // == 155
    "FIFBLOCKED",                          // == 156
    "FIFTARGETISDEFENDING",                // == 157
    "FIFTARGETISATTACKING",                // == 158
    "FIFSTATEIS0",                         // == 159
    "FIFSTATEIS1",                         // == 160
    "FIFSTATEIS2",                         // == 161
    "FIFSTATEIS3",                         // == 162
    "FIFSTATEIS4",                         // == 163
    "FIFSTATEIS5",                         // == 164
    "FIFSTATEIS6",                         // == 165
    "FIFSTATEIS7",                         // == 166
    "FIFCONTENTIS",                        // == 167
    "FSETTURNMODETOWATCHTARGET",           // == 168
    "FIFSTATEISNOT",                       // == 169
    "FIFXISEQUALTOY",                      // == 170
    "FDEBUGMESSAGE",                       // == 171

    /// Scripted AI functions (v0.80)
    "FBLACKTARGET",                        // == 172
    "FSENDMESSAGENEAR",                    // == 173
    "FIFHITGROUND",                        // == 174
    "FIFNAMEISKNOWN",                      // == 175
    "FIFUSAGEISKNOWN",                     // == 176
    "FIFHOLDINGITEMID",                    // == 177
    "FIFHOLDINGRANGEDWEAPON",              // == 178
    "FIFHOLDINGMELEEWEAPON",               // == 179
    "FIFHOLDINGSHIELD",                    // == 180
    "FIFKURSED",                           // == 181
    "FIFTARGETISKURSED",                   // == 182
    "FIFTARGETISDRESSEDUP",                // == 183
    "FIFOVERWATER",                        // == 184
    "FIFTHROWN",                           // == 185
    "FMAKENAMEKNOWN",                      // == 186
    "FMAKEUSAGEKNOWN",                     // == 187
    "FSTOPTARGETMOVEMENT",                 // == 188
    "FSETXY",                              // == 189
    "FGETXY",                              // == 190
    "FADDXY",                              // == 191
    "FMAKEAMMOKNOWN",                      // == 192
    "FSPAWNATTACHEDPARTICLE",              // == 193
    "FSPAWNEXACTPARTICLE",                 // == 194
    "FACCELERATETARGET",                   // == 195
    "FIFDISTANCEISMORETHANTURN",           // == 196
    "FIFCRUSHED",                          // == 197
    "FMAKECRUSHVALID",                     // == 198
    "FSETTARGETTOLOWESTTARGET",            // == 199
    "FIFNOTPUTAWAY",                       // == 200
    "FIFTAKENOUT",                         // == 201
    "FIFAMMOOUT",                          // == 202
    "FPLAYSOUNDLOOPED",                    // == 203
    "FSTOPSOUND",                          // == 204
    "FHEALSELF",                           // == 205
    "FEQUIP",                              // == 206
    "FIFTARGETHASITEMIDEQUIPPED",          // == 207
    "FSETOWNERTOTARGET",                   // == 208
    "FSETTARGETTOOWNER",                   // == 209
    "FSETFRAME",                           // == 210
    "FBREAKPASSAGE",                       // == 211
    "FSETRELOADTIME",                      // == 212
    "FSETTARGETTOWIDEBLAHID",              // == 213
    "FPOOFTARGET",                         // == 214
    "FCHILDDOACTIONOVERRIDE",              // == 215
    "FSPAWNPOOF",                          // == 216
    "FSETSPEEDPERCENT",                    // == 217
    "FSETCHILDSTATE",                      // == 218
    "FSPAWNATTACHEDSIZEDPARTICLE",         // == 219
    "FCHANGEARMOR",                        // == 220
    "FSHOWTIMER",                          // == 221
    "FIFFACINGTARGET",                     // == 222
    "FPLAYSOUNDVOLUME",                    // == 223
    "FSPAWNATTACHEDFACEDPARTICLE",         // == 224
    "FIFSTATEISODD",                       // == 225
    "FSETTARGETTODISTANTENEMY",            // == 226
    "FTELEPORT",                           // == 227
    "FGIVESTRENGTHTOTARGET",               // == 228
    "FGIVEWISDOMTOTARGET",                 // == 229
    "FGIVEINTELLIGENCETOTARGET",           // == 230
    "FGIVEDEXTERITYTOTARGET",              // == 231
    "FGIVELIFETOTARGET",                   // == 232
    "FGIVEMANATOTARGET",                   // == 233
    "FSHOWMAP",                            // == 234
    "FSHOWYOUAREHERE",                     // == 235
    "FSHOWBLIPXY",                         // == 236
    "FHEALTARGET",                         // == 237
    "FPUMPTARGET",                         // == 238
    "FCOSTAMMO",                           // == 239
    "FMAKESIMILARNAMESKNOWN",              // == 240
    "FSPAWNATTACHEDHOLDERPARTICLE",        // == 241
    "FSETTARGETRELOADTIME",                // == 242
    "FSETFOGLEVEL",                        // == 243
    "FGETFOGLEVEL",                        // == 244
    "FSETFOGTAD",                          // == 245
    "FSETFOGBOTTOMLEVEL",                  // == 246
    "FGETFOGBOTTOMLEVEL",                  // == 247
    "FCORRECTACTIONFORHAND",               // == 248
    "FIFTARGETISMOUNTED",                  // == 249
    "FSPARKLEICON",                        // == 250
    "FUNSPARKLEICON",                      // == 251
    "FGETTILEXY",                          // == 252
    "FSETTILEXY",                          // == 253
    "FSETSHADOWSIZE",                      // == 254
    "FORDERTARGET",                        // == 255
    "FSETTARGETTOWHOEVERISINPASSAGE",      // == 256
    "FIFCHARACTERWASABOOK",                // == 257

    /// Scripted AI functions (v0.90)
    "FSETENCHANTBOOSTVALUES",              // == 258
    "FSPAWNCHARACTERXYZ",                  // == 259
    "FSPAWNEXACTCHARACTERXYZ",             // == 260
    "FCHANGETARGETCLASS",                  // == 261
    "FPLAYFULLSOUND",                      // == 262
    "FSPAWNEXACTCHASEPARTICLE",            // == 263
    "FCREATEORDER",                        // == 264
    "FORDERSPECIALID",                     // == 265
    "FUNKURSETARGETINVENTORY",             // == 266
    "FIFTARGETISSNEAKING",                 // == 267
    "FDROPITEMS",                          // == 268
    "FRESPAWNTARGET",                      // == 269
    "FTARGETDOACTIONSETFRAME",             // == 270
    "FIFTARGETCANSEEINVISIBLE",            // == 271
    "FSETTARGETTONEARESTBLAHID",           // == 272
    "FSETTARGETTONEARESTENEMY",            // == 273
    "FSETTARGETTONEARESTFRIEND",           // == 274
    "FSETTARGETTONEARESTLIFEFORM",         // == 275
    "FFLASHPASSAGE",                       // == 276
    "FFINDTILEINPASSAGE",                  // == 277
    "FIFHELDINLEFTHAND",                   // == 278
    "FNOTANITEM",                          // == 279
    "FSETCHILDAMMO",                       // == 280
    "FIFHITVULNERABLE",                    // == 281
    "FIFTARGETISFLYING",                   // == 282
    "FIDENTIFYTARGET",                     // == 283
    "FBEATMODULE",                         // == 284
    "FENDMODULE",                          // == 285
    "FDISABLEEXPORT",                      // == 286
    "FENABLEEXPORT",                       // == 287
    "FGETTARGETSTATE",                     // == 288

    /// Redone in v 0.95
    "FIFEQUIPPED",                         // == 289
    "FDROPTARGETMONEY",                    // == 290
    "FGETTARGETCONTENT",                   // == 291
    "FDROPTARGETKEYS",                     // == 292
    "FJOINTEAM",                           // == 293
    "FTARGETJOINTEAM",                     // == 294

    /// Below is original code again
    "FCLEARMUSICPASSAGE",                  // == 295
    "FCLEARENDMESSAGE",                    // == 296
    "FADDENDMESSAGE",                      // == 297
    "FPLAYMUSIC",                          // == 298
    "FSETMUSICPASSAGE",                    // == 299
    "FMAKECRUSHINVALID",                   // == 300
    "FSTOPMUSIC",                          // == 301
    "FFLASHVARIABLE",                      // == 302
    "FACCELERATEUP",                       // == 303
    "FFLASHVARIABLEHEIGHT",                // == 304
    "FSETDAMAGETIME",                      // == 305
    "FIFSTATEIS8",                         // == 306
    "FIFSTATEIS9",                         // == 307
    "FIFSTATEIS10",                        // == 308
    "FIFSTATEIS11",                        // == 309
    "FIFSTATEIS12",                        // == 310
    "FIFSTATEIS13",                        // == 311
    "FIFSTATEIS14",                        // == 312
    "FIFSTATEIS15",                        // == 313
    "FIFTARGETISAMOUNT",                   // == 314
    "FIFTARGETISAPLATFORM",                // == 315
    "FADDSTAT",                            // == 316
    "FDISENCHANTTARGET",                   // == 317
    "FDISENCHANTALL",                      // == 318
    "FSETVOLUMENEARESTTEAMMATE",           // == 319
    "FADDSHOPPASSAGE",                     // == 320
    "FTARGETPAYFORARMOR",                  // == 321
    "FJOINEVILTEAM",                       // == 322
    "FJOINNULLTEAM",                       // == 323
    "FJOINGOODTEAM",                       // == 324
    "FPITSKILL",                           // == 325
    "FSETTARGETTOPASSAGEID",               // == 326
    "FMAKENAMEUNKNOWN",                    // == 327
    "FSPAWNEXACTPARTICLEENDSPAWN",         // == 328
    "FSPAWNPOOFSPEEDSPACINGDAMAGE",        // == 329
    "FGIVEEXPERIENCETOGOODTEAM",           // == 330

    /// Scripted AI functions (v0.95)
    "FDONOTHING",                          // == 331
    "FGROGTARGET",                         // == 332
    "FDAZETARGET",                         // == 333
    "FENABLERESPAWN",                      // == 334
    "FDISABLERESPAWN",                     // == 335

    /// Redone in v 1.10
    "FDISPELTARGETENCHANTID",              // == 336
    "FIFHOLDERBLOCKED",                    // == 337
    "FGETSKILLLEVEL",                      // == 338
    "FIFTARGETHASNOTFULLMANA",             // == 339
    "FENABLELISTENSKILL",                  // == 340
    "FSETTARGETTOLASTITEMUSED",            // == 341
    "FFOLLOWLINK",                         // == 342  Scripted AI functions (v1.00)
    "FIFOPERATORISLINUX",                  // == 343
    "FIFTARGETISAWEAPON",                  // == 344
    "FIFSOMEONEISSTEALING",                // == 345
    "FIFTARGETISASPELL",                   // == 346
    "FIFBACKSTABBED",                      // == 347
    "FGETTARGETDAMAGETYPE",                // == 348
    "FADDQUEST",                           // == 349
    "FBEATQUESTALLPLAYERS",                // == 350
    "FIFTARGETHASQUEST",                   // == 351
    "FSETQUESTLEVEL",                      // == 352
    "FADDQUESTALLPLAYERS",                 // == 353
    "FADDBLIPALLENEMIES",                  // == 354
    "FPITSFALL",                           // == 355
    "FIFTARGETISOWNER",                    // == 356

    /// adding in the "speech" thing so the script can define its "ouch" sound, for instance
    "FSETSPEECH",                  // == 357
    "FSETMOVESPEECH",              // == 358
    "FSETSECONDMOVESPEECH",        // == 359
    "FSETATTACKSPEECH",            // == 360
    "FSETASSISTSPEECH",            // == 361
    "FSETTERRAINSPEECH",           // == 362
    "FSETSELECTSPEECH",            // == 363

    /// Scripted AI functions (v1.10)
    "FTAKEPICTURE",                // == 364
    "FIFOPERATORISMACINTOSH",      // == 365
    "FIFMODULEHASIDSZ",            // == 366
    "FMORPHTOTARGET",              // == 367
    "FGIVEMANAFLOWTOTARGET",       // == 368
    "FGIVEMANARETURNTOTARGET",     // == 369
    "FSETMONEY",                   // == 370
    "FIFTARGETCANSEEKURSES",       // == 371
    "FSPAWNATTACHEDCHARACTER",     // == 372
    "FKURSETARGET",                // == 373
    "FSETCHILDCONTENT",            // == 374
    "FSETTARGETTOCHILD",           // == 375
    "FSETDAMAGETHRESHOLD",         // == 376
    "FACCELERATETARGETUP",         // == 377
    "FSETTARGETAMMO",              // == 378
    "FENABLEINVICTUS",             // == 379
    "FDISABLEINVICTUS",            // == 380
    "FTARGETDAMAGESELF",           // == 381
    "FSETTARGETSIZE",              // == 382
    "FIFTARGETISFACINGSELF",       // == 383

    // Scripted AI functions (v1.20)
    "FDRAWBILLBOARD",                // == 384
    "FSETTARGETTOBLAHINPASSAGE"      // == 385
    "FIFLEVELUP",                    // == 386
    "FGIVESKILLTOTARGET",            // == 387
    "FSETTARGETTONEARBYMELEEWEAPON", // == 388
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

//Private functions

static size_t       surround_space( size_t position, char buffer[], size_t buffer_size, const size_t buffer_max );
static size_t       insert_space( size_t position, char buffer[], size_t buffer_length, const size_t buffer_max );
static size_t       fix_operators( char buffer[], size_t buffer_size, const size_t buffer_max );

static size_t       load_one_line( parser_state_t * ps, size_t read, script_info_t *pscript );
static int          get_indentation( parser_state_t * ps, script_info_t *pscript );
static size_t       parse_token( parser_state_t * ps, token_t * ptok, ObjectProfile *ppro, script_info_t *pscript, size_t read );
static void         emit_opcode( token_t * ptok, const BIT_FIELD highbits, script_info_t *pscript );
static void         parse_line_by_line( parser_state_t * ps, ObjectProfile *ppro, script_info_t *pscript );
static Uint32       jump_goto( int index, int index_end, script_info_t *pscript );
static void         parse_jumps( script_info_t *pscript );
static egolib_rv    ai_script_upload_default( script_info_t *pscript );



// functions for debugging the scripts
#if (DEBUG_SCRIPT_LEVEL > 2) && defined(_DEBUG)
    static void print_token();
    static void print_line();
#else
    #define print_token()
    #define print_line()
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

parser_state_t *parser_state_t::initialize()
{
    if (!_singleton)
    {
        _singleton = (parser_state_t *)malloc(sizeof(parser_state_t));
        if (!_singleton)
        {
            return nullptr;
        }
        if (!parser_state_t::ctor(_singleton))
        {
            free(_singleton);
            _singleton = nullptr;
            return nullptr;
        }
    }
    return _singleton;
}

void parser_state_t::uninitialize()
{
    if (_singleton)
    {
        parser_state_t::dtor(_singleton);
        free(_singleton);
        _singleton = nullptr;
    }
}

parser_state_t *parser_state_t::get()
{
    return _singleton;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
size_t insert_space( size_t position, char buffer[], size_t buffer_length, const size_t buffer_max )
{
    /// @author ZZ
    /// @details This function adds a space into the load line if there isn't one there already

    char cTmp, cSwap;

    // fail if the new length will be too long
    if (buffer_length >= buffer_max)
    {
        return std::min( buffer_length, buffer_max );
    }

    if (!Ego::isspace(buffer[position]))
    {
        // we are definitely going to add one character to the length
        buffer_length++;

        // save the old value
        cTmp = buffer[position];

        // insert a space
        buffer[position] = ' ';
        position++;

        // bubble the values up
        while ( position < buffer_length )
        {
            // save the old one
            cSwap = buffer[position];

            // insert the saves one
            buffer[position] = cTmp;

            // save the new one
            cTmp = cSwap;

            position++;
        }

        buffer[buffer_length] = CSTR_END;
    }

    return buffer_length;
}

//--------------------------------------------------------------------------------------------
size_t load_one_line( parser_state_t * ps, size_t read, script_info_t *pscript )
{
    /// @author ZZ
    /// @details This function loads a line into the line buffer

    int stillgoing, foundtext;
    char cTmp;
    bool tabs_warning_needed, inside_string;

    // Parse to start to maintain indentation
    ps->line_buffer[0] = CSTR_END;
    ps->line_buffer_count = 0;

    stillgoing = true;
    inside_string = false;

    // try to trap all end of line conditions so we can properly count the lines
    tabs_warning_needed = false;
    while ( read < ps->load_buffer_count )
    {
        cTmp = ps->load_buffer[read];

        if ( ASCII_LINEFEED_CHAR == cTmp && C_CARRIAGE_RETURN_CHAR == ps->load_buffer[read+1] )
        {
            ps->line_buffer_count = 0;
            ps->line_buffer[0] = CSTR_END;
            return read + 2;
        }

        if ( C_CARRIAGE_RETURN_CHAR == cTmp && ASCII_LINEFEED_CHAR == ps->load_buffer[read+1] )
        {
            ps->line_buffer_count = 0;
            ps->line_buffer[0] = CSTR_END;
            return read + 2;
        }

        if ( ASCII_LINEFEED_CHAR == cTmp || C_CARRIAGE_RETURN_CHAR == cTmp )
        {
            ps->line_buffer_count = 0;
            ps->line_buffer[0] = CSTR_END;
            return read + 1;
        }

        if ( C_TAB_CHAR == cTmp )
        {
            tabs_warning_needed = true;
            cTmp = ' ';
        }

        if (!Ego::isspace(cTmp))
        {
            break;
        }

        ps->line_buffer[ps->line_buffer_count] = ' ';
        ps->line_buffer[ps->line_buffer_count+1] = CSTR_END;

        read++;
        ps->line_buffer_count++;
    }

    // Parse to comment or end of line
    foundtext = false;
    inside_string = false;
    while ( read < ps->load_buffer_count )
    {
        cTmp = ps->load_buffer[read];

        // we reached endline
        if ( C_CARRIAGE_RETURN_CHAR == cTmp || ASCII_LINEFEED_CHAR == cTmp )
        {
            break;
        }

        // we reached a comment
        if ( '/' == cTmp && '/' == ps->load_buffer[read + 1] )
        {
            break;
        }

        // Double apostrophe indicates where the string begins and ends
        if ( C_DOUBLE_QUOTE_CHAR == cTmp )
        {
            inside_string = !inside_string;
        }

        if ( inside_string )
        {
            if ( '\t' == cTmp )
            {
                // convert tab characters to the '~' symbol
                cTmp = '~';
            }
            else if (Ego::isspace(cTmp) || Ego::iscntrl(cTmp))
            {
                // all whitespace and control characters are converted to '_'
                cTmp = '_';
            }
        }
        else if (Ego::iscntrl(cTmp))
        {
            // Convert control characters to whitespace
            cTmp = ' ';
        }

        // convert whitespace characters to
        if ( !isspace(( unsigned )cTmp ) || inside_string )
        {
            foundtext = true;

            ps->line_buffer[ps->line_buffer_count] = cTmp;
            ps->line_buffer_count++;
        }

        read++;
    }

    if ( !foundtext )
    {
        ps->line_buffer_count = 0;
    }

    // terminate the line buffer properly
    ps->line_buffer[ps->line_buffer_count] = CSTR_END;

    if ( ps->line_buffer_count > 0  && tabs_warning_needed )
    {
        log_message( "SCRIPT ERROR: %s() - Tab character used to define spacing will cause an error \"%s\"(%d) - \n    \"%s\"\n", __FUNCTION__, pscript->name, ps->token.iLine, ps->line_buffer );
    }

    // scan to the beginning of the next line
    while ( read < ps->load_buffer_count )
    {
        if ( ASCII_LINEFEED_CHAR == ps->load_buffer[read] && C_CARRIAGE_RETURN_CHAR == ps->load_buffer[read+1] )
        {
            read += 2;
            break;
        }
        else if ( C_CARRIAGE_RETURN_CHAR == ps->load_buffer[read] && ASCII_LINEFEED_CHAR == ps->load_buffer[read+1] )
        {
            read += 2;
            break;
        }
        else if ( CSTR_END == ps->load_buffer[read] || ASCII_LINEFEED_CHAR == ps->load_buffer[read] || C_CARRIAGE_RETURN_CHAR == ps->load_buffer[read] )
        {
            read += 1;
            break;
        }

        read++;
    }

    return read;
}

//--------------------------------------------------------------------------------------------
size_t surround_space( size_t position, char buffer[], size_t buffer_size, const size_t buffer_max )
{
    buffer_size = insert_space( position + 1, buffer, buffer_size, buffer_max );

    if ( position >= 0 )
    {
        buffer_size = insert_space( position, buffer, buffer_size, buffer_max );
    }

    return buffer_size;
}

//--------------------------------------------------------------------------------------------
int get_indentation( parser_state_t * ps, script_info_t *pscript )
{
    /// @author ZZ
    /// @details This function returns the number of starting spaces in a line

    int cnt;
    char cTmp;

    cnt = 0;
    cTmp = ps->line_buffer[cnt];
    while (Ego::isspace(cTmp))
    {
        cnt++;
        cTmp = ps->line_buffer[cnt];
    }
    if ( HAS_SOME_BITS( cnt, 1 ) )
    {
        log_message( "SCRIPT ERROR: %s() - Invalid indentation \"%s\"(%d) - \"%s\"\n", __FUNCTION__, pscript->name, ps->token.iLine, ps->line_buffer );
        ps->error = true;
    }

    cnt >>= 1;
    if ( cnt > 15 )
    {
        log_message( "SCRIPT ERROR: %s() - Too many levels of indentation \"%s\"(%d) - \"%s\"\n", __FUNCTION__, pscript->name, ps->token.iLine, ps->line_buffer );
        ps->error = true;
        cnt = 15;
    }

    return cnt;
}

//--------------------------------------------------------------------------------------------
size_t fix_operators( char buffer[], size_t buffer_size, const size_t buffer_max )
{
    /// @author ZZ
    /// @details This function puts spaces around operators to seperate words better

    size_t cnt;
    char   cTmp;
    bool inside_string = false;

    cnt = 0;
    while ( cnt < buffer_size )
    {
        cTmp = buffer[cnt];
        if ( C_DOUBLE_QUOTE_CHAR == cTmp )
        {
            inside_string = !inside_string;
        }

        //Don't fix operator symbols inside a string
        if ( !inside_string )
        {
            if ( '+' == cTmp || '-' == cTmp || '/' == cTmp || '*' == cTmp ||
                 '%' == cTmp || '>' == cTmp || '<' == cTmp || '&' == cTmp ||
                 '=' == cTmp )
            {
                buffer_size = surround_space( cnt, buffer, buffer_size, buffer_max );
                cnt++;
            }
        }

        cnt++;
    }

    return buffer_size;
}

//--------------------------------------------------------------------------------------------
size_t parse_token(parser_state_t *self, token_t *ptok, ObjectProfile *ppro, script_info_t *pscript, size_t read)
{
    /// @author ZZ
    /// @details This function tells what code is being indexed by read, it
    ///    will return the next spot to read from and stick the code number
    ///    in ptok->iIndex

    int cnt;
    char cTmp;
    IDSZ idsz;
    bool parsed = false;

    size_t       szWord_length_max = 0;

    // check for bad pointers
    if (!self)
    {
        throw std::invalid_argument("nullptr == self");
    }
    if (!ptok || NULL == pscript ) return AISMAXLOADSIZE;

    // figure out what the max word length actually is
    szWord_length_max = SDL_arraysize( ptok->szWord );

    // Reset the token
    token_t::ctor( ptok );

    // Check bounds
    if ( read >= self->line_buffer_count )
    {
        return self->line_buffer_count;
    }

    // nothing is parsed yet
    parsed = false;

    // Skip any initial spaces
    cTmp = self->line_buffer[read];
    while (Ego::isspace(cTmp) && read < self->line_buffer_count)
    {
        read++;
        cTmp = self->line_buffer[read];
    }

    // break if there was nothing here
    if ( read >= self->line_buffer_count )
    {
        goto parse_token_end;
    }

    // initialize the word
    ptok->szWord_length = 0;
    ptok->szWord[0] = CSTR_END;

    // handle the special case of a string constant
    if ( C_DOUBLE_QUOTE_CHAR == cTmp )
    {
        do
        {
            // begin the copy
            ptok->szWord[ptok->szWord_length] = cTmp;
            ptok->szWord_length++;

            read++;
            cTmp = self->line_buffer[read];

            // Break out if we find the end of the string
            // Strings of the form "Here lies \"The Sandwich King\"" are not supported
        }
        while ( CSTR_END != cTmp && C_DOUBLE_QUOTE_CHAR != cTmp && ptok->szWord_length < szWord_length_max && read < self->line_buffer_count );

        if ( C_DOUBLE_QUOTE_CHAR == cTmp )
        {
            // skip the ending qoutation mark
            read++;
            cTmp = self->line_buffer[read];

            ptok->szWord[ptok->szWord_length] = CSTR_END;
            ptok->szWord_length++;
        }
        else
        {
            log_message( "SCRIPT ERROR: %s() - The string in line %d is too long\n.", __FUNCTION__, ptok->iLine );
        }
    }
    else
    {
        // Load the the word into the ptok->szWord buffer
        ptok->szWord_length = 0;
        ptok->szWord[0] = CSTR_END;

        while (!Ego::isspace(cTmp) && CSTR_END != cTmp && ptok->szWord_length < szWord_length_max && read < self->line_buffer_count )
        {
            ptok->szWord[ptok->szWord_length] = cTmp;
            ptok->szWord_length++;

            read++;
            cTmp = self->line_buffer[read];
        }

        if ( ptok->szWord_length < szWord_length_max )
        {
            ptok->szWord[ptok->szWord_length] = CSTR_END;
        }
    }

    // ensure that the string is terminated
    ptok->szWord[szWord_length_max-1] = CSTR_END;

    // Check for numeric constant
    if (!parsed && Ego::isdigit(ptok->szWord[0]))
    {
        sscanf( ptok->szWord, "%d", &ptok->iValue );
        ptok->cType  = 'C';
        ptok->iIndex = MAX_OPCODE;

        // move on to the next thing
        parsed = true;
    }

    // Check for IDSZ constant
    if ( !parsed && ( '[' == ptok->szWord[0] ) )
    {
        idsz = MAKE_IDSZ( ptok->szWord[1], ptok->szWord[2], ptok->szWord[3], ptok->szWord[4] );

        ptok->iValue = idsz;
        ptok->cType  = 'C';
        ptok->iIndex = MAX_OPCODE;

        // move on to the next thing
        parsed = true;
    }

    if ( !parsed && ( 0 == strcmp( ptok->szWord, "=" ) ) )
    {
        ptok->iValue = -1;
        ptok->cType  = 'O';
        ptok->iIndex = MAX_OPCODE;

        // move on to the next thing
        parsed = true;
    }

    // convert the string token to a new token type
    if ( !parsed && ( C_DOUBLE_QUOTE_CHAR == ptok->szWord[0] ) )
    {
        char * str = ptok->szWord + 1;

        if ( CSTR_END == ptok->szWord[1] || C_DOUBLE_QUOTE_CHAR == ptok->szWord[1] )
        {
            // some kind of empty string

            log_message( "SCRIPT ERROR: %s() - The string in line %d is empty\n.", __FUNCTION__, ptok->iLine );

            // some kind of error
            parsed = true;
        }
        else if ( '#' == str[0] )
        {
            //remove the reference symbol to figure out the actual folder name we are looking for
            std:string obj_name = str + 1;

            // Invalid profile as default
            ptok->iValue = INVALID_PRO_REF;

            // Convert reference to slot number
            for (const auto &element : ProfileSystem::get().getLoadedProfiles())
            {
                const std::shared_ptr<ObjectProfile> &profile = element.second;
                if(profile == nullptr) continue;


                //is this the object we are looking for?
                if (Ego::isSuffix(profile->getPathname(), obj_name))
                {
                    ptok->iValue = profile->getSlotNumber();
                    break;
                }
            }

            // Do we need to load the object?
            if (!ProfileSystem::get().isValidProfileID((PRO_REF)ptok->iValue))
            {
                std::string loadname = "mp_objects/" + obj_name;

                //find first free slot number
                for (PRO_REF ipro = MAX_IMPORT_PER_PLAYER * 4; ipro < INVALID_PRO_REF; ipro++ )
                {
                    //skip loaded profiles
                    if (ProfileSystem::get().isValidProfileID(ipro)) continue;

                    //found a free slot
                    ptok->iValue = ProfileSystem::get().loadOneProfile(loadname, REF_TO_INT(ipro));
                    if (ptok->iValue == ipro) break;
                }
            }

            // Failed to load object!
            if (!ProfileSystem::get().isValidProfileID((PRO_REF)ptok->iValue))
            {
                log_message( "SCRIPT ERROR: %s() - Failed to load object: %s through an AI script. %s (line %d)\n", __FUNCTION__, ptok->szWord, pscript->name, ptok->iLine );
            }

            ptok->cType = 'C';
            ptok->iIndex = MAX_OPCODE;

            parsed = true;
        }
        else
        {
            // a normal string
            // if this is a new string, add this message to the avalible messages of the object
            ptok->iValue = ppro->addMessage(str, true);

            ptok->cType = 'C';
            ptok->iIndex = MAX_OPCODE;

            parsed = true;
        }
    }

    // is it a constant, opcode, or value?
    if ( !parsed )
    {
        for ( cnt = 0; cnt < OpList.count; cnt++ )
        {
            if ( 0 == strncmp( ptok->szWord, OpList.ary[cnt].cName, MAXCODENAMESIZE ) )
            {
                ptok->iValue = OpList.ary[cnt].iValue;
                ptok->cType  = OpList.ary[cnt].cType;
                ptok->iIndex = cnt;

                // move on to the next thing
                parsed = true;

                break;
            }
        }
    }

    // We couldn't figure out what this is, throw out an error code
    if ( !parsed )
    {
        log_message( "SCRIPT ERROR: %s() - \"%s\"(%d) - unknown opcode \"%s\"\n", __FUNCTION__, pscript->name, ptok->iLine, ptok->szWord );

        // put the token in an error state
        ptok->iValue = -1;
        ptok->cType  = '?';
        ptok->iIndex = MAX_OPCODE;

        self->error = true;
    }

parse_token_end:

    print_token();
    return read;
}

//--------------------------------------------------------------------------------------------
void emit_opcode( token_t * ptok, const BIT_FIELD highbits, script_info_t *pscript )
{
    BIT_FIELD loc_highbits = highbits;

    if ( NULL == ptok )
    {
        log_message( "SCRIPT ERROR: %s() - emit_opcode() - Invalid token.\n", __FUNCTION__ );
        return;
    }

    // detect a constant value
    if ( 'C' == ptok->cType || 'F' == ptok->cType )
    {
        SET_BIT( loc_highbits, FUNCTION_BIT );
    }

    // emit the opcode
    if ( pscript->length < MAXAICOMPILESIZE )
    {
        pscript->data[pscript->length] = loc_highbits | ptok->iValue;
        pscript->length++;
    }
    else
    {
        log_message( "SCRIPT ERROR: %s() - emit_opcode() - Script index larger than array\n", __FUNCTION__ );
    }

}

//--------------------------------------------------------------------------------------------
void parse_line_by_line( parser_state_t * ps, ObjectProfile *ppro, script_info_t *pscript )
{
    /// @author ZF
    /// @details This parses an AI script line by line

    size_t read;
    Uint32 highbits;
    size_t parseposition;

    read = 0;
    for ( ps->token.iLine = 0; read < ps->load_buffer_count; ps->token.iLine++ )
    {
        read = load_one_line( ps, read, pscript );
        if ( 0 == ps->line_buffer_count ) continue;

#if (DEBUG_SCRIPT_LEVEL > 2) && defined(_DEBUG)
        print_line();
#endif

        ps->line_buffer_count = fix_operators( ps->line_buffer, ps->line_buffer_count, SDL_arraysize( ps->line_buffer ) );
        parseposition = 0;

        //------------------------------
        // grab the first opcode

        highbits = SET_DATA_BITS( get_indentation( ps, pscript ) );
        parseposition = parse_token( ps, &( ps->token ), ppro, pscript, parseposition );
        if ( 'F' == ps->token.cType )
        {
            if ( FEND == ps->token.iValue && 0 == highbits )
            {
                // stop processing the lines, since we're finished
                break;
            }

            //------------------------------
            // the code type is a function

            // save the opcode
            emit_opcode( &( ps->token ), highbits, pscript );

            // leave a space for the control code
            ps->token.iValue = 0;
            emit_opcode( &( ps->token ), 0, pscript );

        }
        else if ( 'V' == ps->token.cType )
        {
            //------------------------------
            // the code type is a math operation

            int operand_index;
            int operands = 0;

            // save in the value's opcode
            emit_opcode( &( ps->token ), highbits, pscript );

            // save a position for the operand count
            ps->token.iValue = 0;
            operand_index = pscript->length;    //AisCompiled_offset;
            emit_opcode( &( ps->token ), 0, pscript );

            // handle the "="
            highbits = 0;
            parseposition = parse_token( ps, &( ps->token ), ppro, pscript, parseposition );  // EQUALS
            if ( 'O' != ps->token.cType || 0 != strcmp( ps->token.szWord, "=" ) )
            {
                log_message( "SCRIPT ERROR: %s() - Invalid equation \"%s\"(%d) - \"%s\"\n", __FUNCTION__, pscript->name, ps->token.iLine, ps->line_buffer );
            }

            //------------------------------
            // grab the next opcode

            parseposition = parse_token( ps, &( ps->token ), ppro, pscript, parseposition );
            if ( 'V' == ps->token.cType || 'C' == ps->token.cType )
            {
                // this is a value or a constant
                emit_opcode( &( ps->token ), 0, pscript );
                operands++;

                parseposition = parse_token( ps, &( ps->token ), ppro, pscript, parseposition );
            }
            else if ( 'O' != ps->token.cType )
            {
                // this is a function or an unknown value. do not break the script.
                log_message( "SCRIPT ERROR: %s() - Invalid operand \"%s\"(%d) - \"%s\"\n", __FUNCTION__, pscript->name, ps->token.iLine, ps->token.szWord );

                emit_opcode( &( ps->token ), 0, pscript );
                operands++;

                parseposition = parse_token( ps, &( ps->token ), ppro, pscript, parseposition );
            }

            // expects a OPERATOR VALUE OPERATOR VALUE OPERATOR VALUE pattern
            while ( parseposition < ps->line_buffer_count )
            {
                // the current token should be an operator
                if ( 'O' != ps->token.cType )
                {
                    // problem with the loop
                    log_message( "SCRIPT ERROR: %s() - Expected an operator \"%s\"(%d) - \"%s\"\n", __FUNCTION__, pscript->name, ps->token.iLine, ps->line_buffer );
                    break;
                }

                // the highbits are the operator's value
                highbits = SET_DATA_BITS( ps->token.iValue );

                // VALUE
                parseposition = parse_token( ps, &( ps->token ), ppro, pscript, parseposition );
                if ( 'C' != ps->token.cType && 'V' != ps->token.cType )
                {
                    // not having a constant or a value here breaks the function. stop processing
                    log_message( "SCRIPT ERROR: %s() - Invalid operand \"%s\"(%d) - \"%s\"\n", __FUNCTION__, pscript->name, ps->token.iLine, ps->token.szWord );
                    break;
                }

                emit_opcode( &( ps->token ), highbits, pscript );
                operands++;

                // OPERATOR
                parseposition = parse_token( ps, &( ps->token ), ppro, pscript, parseposition );
            }
            pscript->data[operand_index] = operands;
        }
        else if ( 'C' == ps->token.cType )
        {
            log_message( "SCRIPT ERROR: %s() - Invalid constant \"%s\"(%d) - \"%s\"\n", __FUNCTION__, pscript->name, ps->token.iLine, ps->token.szWord );
        }
        else if ( '?' == ps->token.cType )
        {
            // unknown opcode, do not process this line
            log_message( "SCRIPT ERROR: %s() - Invalid operand \"%s\"(%d) - \"%s\"\n", __FUNCTION__, pscript->name, ps->token.iLine, ps->token.szWord );
        }
        else
        {
            log_message( "SCRIPT ERROR: %s() - Compiler is broken \"%s\"(%d) - \"%s\"\n", __FUNCTION__, pscript->name, ps->token.iLine, ps->token.szWord );
            break;
        }
    }

    ps->token.iValue = FEND;
    ps->token.cType  = 'F';
    emit_opcode( &( ps->token ), 0, pscript );
    ps->token.iValue = pscript->length + 1;
    emit_opcode( &( ps->token ), 0, pscript );
}

//--------------------------------------------------------------------------------------------
Uint32 jump_goto( int index, int index_end, script_info_t *pscript )
{
    /// @author ZZ
    /// @details This function figures out where to jump to on a fail based on the
    ///    starting location and the following code.  The starting location
    ///    should always be a function code with indentation

    Uint32 value;
    int targetindent, indent;

    value = pscript->data[index]; /*AisCompiled_buffer[index];*/  index += 2;
    targetindent = GET_DATA_BITS( value );
    indent = 100;

    while ( indent > targetindent && index < index_end )
    {
        value = pscript->data[index]; //AisCompiled_buffer[index];
        indent = GET_DATA_BITS( value );
        if ( indent > targetindent )
        {
            // Was it a function
            if ( HAS_SOME_BITS( value, FUNCTION_BIT ) )
            {
                // Each function needs a jump
                index++;
                index++;
            }
            else
            {
                // Operations cover each operand
                index++;
                value = pscript->data[index]; //AisCompiled_buffer[index];
                index++;
                index += ( value & 255 );
            }
        }
    }

    return std::min( index, index_end );
}

//--------------------------------------------------------------------------------------------
void parse_jumps( script_info_t *pscript )
{
    /// @author ZZ
    /// @details This function sets up the fail jumps for the down and dirty code

    Uint32 index, index_end;
    Uint32 value, iTmp;

    index     = 0;                    //AisStorage.ary[ainumber].iStartPosition;
    index_end = pscript->length;   //AisStorage.ary[ainumber].iEndPosition;

    value = pscript->data[index];             //AisCompiled_buffer[index];
    while ( index < index_end )
    {
        value = pscript->data[index];         //AisCompiled_buffer[index];

        // Was it a function
        if ( HAS_SOME_BITS( value, FUNCTION_BIT ) )
        {
            // Each function needs a jump
            iTmp = jump_goto( index, index_end, pscript );
            index++;
            pscript->data[index] = iTmp;              //AisCompiled_buffer[index] = iTmp;
            index++;
        }
        else
        {
            // Operations cover each operand
            index++;
            iTmp = pscript->data[index];              //AisCompiled_buffer[index];
            index++;
            index += CLIP_TO_08BITS( iTmp );
        }
    }
}

//--------------------------------------------------------------------------------------------
bool load_ai_codes_vfs()
{
    /// @author ZZ
    /// @details This function loads all of the function and variable names

    OpList.count = 0;
    for (size_t i = 0, n = sizeof(AICODES) / sizeof(aicode_t); i < n; ++i)
    {
        strncpy(OpList.ary[OpList.count].cName, AICODES[i]._name, SDL_arraysize(OpList.ary[OpList.count].cName));
        OpList.ary[OpList.count].cType = AICODES[i]._type;
        OpList.ary[OpList.count].iValue = AICODES[i]._value;
        OpList.count++;
    }
    return true;
}

//--------------------------------------------------------------------------------------------
egolib_rv ai_script_upload_default( script_info_t * pscript )
{
    /// @author ZF
    /// @details This loads the default AI script into a character profile ai
    //              It's not optimal since it duplicates the AI script data with memcpy

    if ( NULL == pscript ) return rv_error;

    strncpy( pscript->name, default_ai_script.name, sizeof( STRING ) );
    memcpy( pscript->data, default_ai_script.data, sizeof( pscript->data ) );

    pscript->indent = 0;
    pscript->indent_last = 0;

    pscript->length = default_ai_script.length;
    pscript->position = 0;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egolib_rv load_ai_script_vfs( parser_state_t * ps, const char *loadname, ObjectProfile *ppro, script_info_t *pscript )
{
    /// @author ZZ
    /// @details This function loads a script to memory

    vfs_FILE* fileread;
    size_t file_size;

    //Handle default AI
    if ( NULL == pscript ) pscript = &( default_ai_script );

    ps->line_count = 0;
    fileread = vfs_openRead( loadname );

    // No such file
    if ( NULL == fileread )
    {
        log_message( "SCRIPT ERROR: %s() - I am missing a AI script (%s)\n", __FUNCTION__, loadname );
        log_message( "              Using the default AI script instead (\"mp_data/script.txt\")\n" );

        ai_script_upload_default( pscript );
        return rv_fail;
    }

    // load the file
    file_size = vfs_fileLength( fileread );
    ps->load_buffer_count = ( int )vfs_read( ps->load_buffer, 1, file_size, fileread );
    vfs_close( fileread );

    // if the file is empty, use the default script
    if ( 0 == ps->load_buffer_count )
    {
        log_message( "SCRIPT ERROR: %s() - Script file is empty. \"%s\"\n", __FUNCTION__, loadname );

        ai_script_upload_default( pscript );
        return rv_fail;
    }

    // save the filename for error logging
    strncpy( pscript->name, loadname, sizeof( STRING ) );

    // we have parsed nothing yet
    pscript->length = 0;

    // parse/compile the scripts
    parse_line_by_line( ps, ppro, pscript );

    // determine the correct jumps
    parse_jumps( pscript );

    return rv_success;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#if (DEBUG_SCRIPT_LEVEL > 2) && defined(_DEBUG)
void print_token( token_t * ptok )
{
    if ( NULL != ptok )
    {
        printf( "------------\n", globalparsename, ptok->iLine );
        printf( "\tToken.iIndex == %d\n", ptok->iIndex );
        printf( "\tToken.iValue == %d\n", ptok->iValue );
        printf( "\tToken.cType  == \'%c\'\n", ptok->cType );
        printf( "\tToken.szWord  == \"%s\"\n", szWord );
    }
}

#endif

//--------------------------------------------------------------------------------------------
#if (DEBUG_SCRIPT_LEVEL > 2) && defined(_DEBUG)
void print_line( parser_state_t * ps )
{
    int i;
    char cTmp;

    printf( "\n===========\n\tfile == \"%s\"\n\tline == %d\n", globalparsename, ps->token.iLine );

    printf( "\tline == \"" );

    for ( i = 0; i < ps->line_buffer_count; i++ )
    {
        cTmp = ps->line_buffer[i];
        if ( isprint( cTmp ) )
        {
            printf( "%c", cTmp );
        }
        else
        {
            printf( "\\%03d", cTmp );
        }
    };

    printf( "\", length == %d\n", ps->line_buffer_count );
}

#endif
