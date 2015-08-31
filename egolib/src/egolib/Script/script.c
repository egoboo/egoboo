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

/// @file  egolib/Script/script.c
/// @brief Implements the game's scripting language.
/// @details

#include "egolib/Script/script.h"
#include "game/script_compile.h"
#include "game/script_implementation.h"
#include "game/script_functions.h"
#include "egolib/AI/AStar.h"
#include "game/game.h"
#include "game/network.h"
#include "game/player.h"
#include "game/Entities/_Include.hpp"
#include "game/char.h"
#include "game/Core/GameEngine.hpp"
#include "game/Graphics/CameraSystem.hpp"
#include "game/Module/Module.hpp"

namespace Ego {
namespace Script {

Runtime::Runtime() 
	:
		_functionValueCodeToFunctionPointer
		{
			{ FIFSPAWNED, &scr_Spawned },
			{ FIFTIMEOUT, &scr_TimeOut },
			{ FIFATWAYPOINT, &scr_AtWaypoint },
			{ FIFATLASTWAYPOINT, &scr_AtLastWaypoint },
			{ FIFATTACKED, &scr_Attacked },
			{ FIFBUMPED, &scr_Bumped },
			{ FIFORDERED, &scr_Ordered },
			{ FIFCALLEDFORHELP, &scr_CalledForHelp },
			{ FSETCONTENT, &scr_set_Content },
			{ FIFKILLED, &scr_Killed },
			{ FIFTARGETKILLED, &scr_TargetKilled },
			{ FCLEARWAYPOINTS, &scr_ClearWaypoints },
			{ FADDWAYPOINT, &scr_AddWaypoint },
			{ FFINDPATH, &scr_FindPath },
			{ FCOMPASS, &scr_Compass },
			{ FGETTARGETARMORPRICE, &scr_get_TargetArmorPrice },
			{ FSETTIME, &scr_set_Time },
			{ FGETCONTENT, &scr_get_Content },
			{ FJOINTARGETTEAM, &scr_JoinTargetTeam },
			{ FSETTARGETTONEARBYENEMY, &scr_set_TargetToNearbyEnemy },
			{ FSETTARGETTOTARGETLEFTHAND, &scr_set_TargetToTargetLeftHand },
			{ FSETTARGETTOTARGETRIGHTHAND, &scr_set_TargetToTargetRightHand },
			{ FSETTARGETTOWHOEVERATTACKED, &scr_set_TargetToWhoeverAttacked },
			{ FSETTARGETTOWHOEVERBUMPED, &scr_set_TargetToWhoeverBumped },
			{ FSETTARGETTOWHOEVERCALLEDFORHELP, &scr_set_TargetToWhoeverCalledForHelp },
			{ FSETTARGETTOOLDTARGET, &scr_set_TargetToOldTarget },
			{ FSETTURNMODETOVELOCITY, &scr_set_TurnModeToVelocity },
			{ FSETTURNMODETOWATCH, &scr_set_TurnModeToWatch },
			{ FSETTURNMODETOSPIN, &scr_set_TurnModeToSpin },
			{ FSETBUMPHEIGHT, &scr_set_BumpHeight },
			{ FIFTARGETHASID, &scr_TargetHasID },
			{ FIFTARGETHASITEMID, &scr_TargetHasItemID },
			{ FIFTARGETHOLDINGITEMID, &scr_TargetHoldingItemID },
			{ FIFTARGETHASSKILLID, &scr_TargetHasSkillID },
			{ FELSE, &scr_Else },
			{ FRUN, &scr_Run },
			{ FWALK, &scr_Walk },
			{ FSNEAK, &scr_Sneak },
			{ FDOACTION, &scr_DoAction },
			{ FKEEPACTION, &scr_KeepAction },
			{ FISSUEORDER, &scr_IssueOrder },
			{ FDROPWEAPONS, &scr_DropWeapons },
			{ FTARGETDOACTION, &scr_TargetDoAction },
			{ FOPENPASSAGE, &scr_OpenPassage },
			{ FCLOSEPASSAGE, &scr_ClosePassage },
			{ FIFPASSAGEOPEN, &scr_PassageOpen },
			{ FGOPOOF, &scr_GoPoof },
			{ FCOSTTARGETITEMID, &scr_CostTargetItemID },
			{ FDOACTIONOVERRIDE, &scr_DoActionOverride },
			{ FIFHEALED, &scr_Healed },
			{ FSENDMESSAGE, &scr_SendPlayerMessage },
			{ FCALLFORHELP, &scr_CallForHelp },
			{ FADDIDSZ, &scr_AddIDSZ },
			{ FSETSTATE, &scr_set_State },
			{ FGETSTATE, &scr_get_State },
			{ FIFSTATEIS, &scr_StateIs },
			{ FIFTARGETCANOPENSTUFF, &scr_TargetCanOpenStuff },
			{ FIFGRABBED, &scr_Grabbed },
			{ FIFDROPPED, &scr_Dropped },
			{ FSETTARGETTOWHOEVERISHOLDING, &scr_set_TargetToWhoeverIsHolding },
			{ FDAMAGETARGET, &scr_DamageTarget },
			{ FIFXISLESSTHANY, &scr_XIsLessThanY },
			{ FSETWEATHERTIME, &scr_set_WeatherTime },
			{ FGETBUMPHEIGHT, &scr_get_BumpHeight },
			{ FIFREAFFIRMED, &scr_Reaffirmed },
			{ FUNKEEPACTION, &scr_UnkeepAction },
			{ FIFTARGETISONOTHERTEAM, &scr_TargetIsOnOtherTeam },
			{ FIFTARGETISONHATEDTEAM, &scr_TargetIsOnHatedTeam },
			{ FPRESSLATCHBUTTON, &scr_PressLatchButton },
			{ FSETTARGETTOTARGETOFLEADER, &scr_set_TargetToTargetOfLeader },
			{ FIFLEADERKILLED, &scr_LeaderKilled },
			{ FBECOMELEADER, &scr_BecomeLeader },
			{ FCHANGETARGETARMOR, &scr_ChangeTargetArmor },
			{ FGIVEMONEYTOTARGET, &scr_GiveMoneyToTarget },
			{ FDROPKEYS, &scr_DropKeys },
			{ FIFLEADERISALIVE, &scr_LeaderIsAlive },
			{ FIFTARGETISOLDTARGET, &scr_TargetIsOldTarget },
			{ FSETTARGETTOLEADER, &scr_set_TargetToLeader },
			{ FSPAWNCHARACTER, &scr_SpawnCharacter },
			{ FRESPAWNCHARACTER, &scr_RespawnCharacter },
			{ FCHANGETILE, &scr_ChangeTile },
			{ FIFUSED, &scr_Used },
			{ FDROPMONEY, &scr_DropMoney },
			{ FSETOLDTARGET, &scr_set_OldTarget },
			{ FDETACHFROMHOLDER, &scr_DetachFromHolder },
			{ FIFTARGETHASVULNERABILITYID, &scr_TargetHasVulnerabilityID },
			{ FCLEANUP, &scr_CleanUp },
			{ FIFCLEANEDUP, &scr_CleanedUp },
			{ FIFSITTING, &scr_Sitting },
			{ FIFTARGETISHURT, &scr_TargetIsHurt },
			{ FIFTARGETISAPLAYER, &scr_TargetIsAPlayer },
			{ FPLAYSOUND, &scr_PlaySound },
			{ FSPAWNPARTICLE, &scr_SpawnParticle },
			{ FIFTARGETISALIVE, &scr_TargetIsAlive },
			{ FSTOP, &scr_Stop },
			{ FDISAFFIRMCHARACTER, &scr_DisaffirmCharacter },
			{ FREAFFIRMCHARACTER, &scr_ReaffirmCharacter },
			{ FIFTARGETISSELF, &scr_TargetIsSelf },
			{ FIFTARGETISMALE, &scr_TargetIsMale },
			{ FIFTARGETISFEMALE, &scr_TargetIsFemale },
			{ FSETTARGETTOSELF, &scr_set_TargetToSelf },
			{ FSETTARGETTORIDER, &scr_set_TargetToRider },
			{ FGETATTACKTURN, &scr_get_AttackTurn },
			{ FGETDAMAGETYPE, &scr_get_DamageType },
			{ FBECOMESPELL, &scr_BecomeSpell },
			{ FBECOMESPELLBOOK, &scr_BecomeSpellbook },
			{ FIFSCOREDAHIT, &scr_ScoredAHit },
			{ FIFDISAFFIRMED, &scr_Disaffirmed },
			{ FTRANSLATEORDER, &scr_TranslateOrder },
			{ FSETTARGETTOWHOEVERWASHIT, &scr_set_TargetToWhoeverWasHit },
			{ FSETTARGETTOWIDEENEMY, &scr_set_TargetToWideEnemy },
			{ FIFCHANGED, &scr_Changed },
			{ FIFINWATER, &scr_InWater },
			{ FIFBORED, &scr_Bored },
			{ FIFTOOMUCHBAGGAGE, &scr_TooMuchBaggage },
			{ FIFGROGGED, &scr_Grogged },
			{ FIFDAZED, &scr_Dazed },
			{ FIFTARGETHASSPECIALID, &scr_TargetHasSpecialID },
			{ FPRESSTARGETLATCHBUTTON, &scr_PressTargetLatchButton },
			{ FIFINVISIBLE, &scr_Invisible },
			{ FIFARMORIS, &scr_ArmorIs },
			{ FGETTARGETGROGTIME, &scr_get_TargetGrogTime },
			{ FGETTARGETDAZETIME, &scr_get_TargetDazeTime },
			{ FSETDAMAGETYPE, &scr_set_DamageType },
			{ FSETWATERLEVEL, &scr_set_WaterLevel },
			{ FENCHANTTARGET, &scr_EnchantTarget },
			{ FENCHANTCHILD, &scr_EnchantChild },
			{ FTELEPORTTARGET, &scr_TeleportTarget },
			{ FGIVEEXPERIENCETOTARGET, &scr_add_TargetExperience },
			{ FINCREASEAMMO, &scr_IncreaseAmmo },
			{ FUNKURSETARGET, &scr_UnkurseTarget },
			{ FGIVEEXPERIENCETOTARGETTEAM, &scr_add_TargetTeamExperience },
			{ FIFUNARMED, &scr_Unarmed },
			{ FRESTOCKTARGETAMMOIDALL, &scr_RestockTargetAmmoIDAll },
			{ FRESTOCKTARGETAMMOIDFIRST, &scr_RestockTargetAmmoIDFirst },
			{ FFLASHTARGET, &scr_FlashTarget },
			{ FSETREDSHIFT, &scr_set_RedShift },
			{ FSETGREENSHIFT, &scr_set_GreenShift },
			{ FSETBLUESHIFT, &scr_set_BlueShift },
			{ FSETLIGHT, &scr_set_Light },
			{ FSETALPHA, &scr_set_Alpha },
			{ FIFHITFROMBEHIND, &scr_HitFromBehind },
			{ FIFHITFROMFRONT, &scr_HitFromFront },
			{ FIFHITFROMLEFT, &scr_HitFromLeft },
			{ FIFHITFROMRIGHT, &scr_HitFromRight },
			{ FIFTARGETISONSAMETEAM, &scr_TargetIsOnSameTeam },
			{ FKILLTARGET, &scr_KillTarget },
			{ FUNDOENCHANT, &scr_UndoEnchant },
			{ FGETWATERLEVEL, &scr_get_WaterLevel },
			{ FCOSTTARGETMANA, &scr_CostTargetMana },
			{ FIFTARGETHASANYID, &scr_TargetHasAnyID },
			{ FSETBUMPSIZE, &scr_set_BumpSize },
			{ FIFNOTDROPPED, &scr_NotDropped },
			{ FIFYISLESSTHANX, &scr_YIsLessThanX },
			{ FSETFLYHEIGHT, &scr_set_FlyHeight },
			{ FIFBLOCKED, &scr_Blocked },
			{ FIFTARGETISDEFENDING, &scr_TargetIsDefending },
			{ FIFTARGETISATTACKING, &scr_TargetIsAttacking },
			{ FIFSTATEIS0, &scr_StateIs0 },
			{ FIFSTATEIS1, &scr_StateIs1 },
			{ FIFSTATEIS2, &scr_StateIs2 },
			{ FIFSTATEIS3, &scr_StateIs3 },
			{ FIFSTATEIS4, &scr_StateIs4 },
			{ FIFSTATEIS5, &scr_StateIs5 },
			{ FIFSTATEIS6, &scr_StateIs6 },
			{ FIFSTATEIS7, &scr_StateIs7 },
			{ FIFCONTENTIS, &scr_ContentIs },
			{ FSETTURNMODETOWATCHTARGET, &scr_set_TurnModeToWatchTarget },
			{ FIFSTATEISNOT, &scr_StateIsNot },
			{ FIFXISEQUALTOY, &scr_XIsEqualToY },
			{ FDEBUGMESSAGE, &scr_DebugMessage },
			{ FBLACKTARGET, &scr_BlackTarget },
			{ FSENDMESSAGENEAR, &scr_SendMessageNear },
			{ FIFHITGROUND, &scr_HitGround },
			{ FIFNAMEISKNOWN, &scr_NameIsKnown },
			{ FIFUSAGEISKNOWN, &scr_UsageIsKnown },
			{ FIFHOLDINGITEMID, &scr_HoldingItemID },
			{ FIFHOLDINGRANGEDWEAPON, &scr_HoldingRangedWeapon },
			{ FIFHOLDINGMELEEWEAPON, &scr_HoldingMeleeWeapon },
			{ FIFHOLDINGSHIELD, &scr_HoldingShield },
			{ FIFKURSED, &scr_Kursed },
			{ FIFTARGETISKURSED, &scr_TargetIsKursed },
			{ FIFTARGETISDRESSEDUP, &scr_TargetIsDressedUp },
			{ FIFOVERWATER, &scr_OverWater },
			{ FIFTHROWN, &scr_Thrown },
			{ FMAKENAMEKNOWN, &scr_MakeNameKnown },
			{ FMAKEUSAGEKNOWN, &scr_MakeUsageKnown },
			{ FSTOPTARGETMOVEMENT, &scr_StopTargetMovement },
			{ FSETXY, &scr_set_XY },
			{ FGETXY, &scr_get_XY },
			{ FADDXY, &scr_AddXY },
			{ FMAKEAMMOKNOWN, &scr_MakeAmmoKnown },
			{ FSPAWNATTACHEDPARTICLE, &scr_SpawnAttachedParticle },
			{ FSPAWNEXACTPARTICLE, &scr_SpawnExactParticle },
			{ FACCELERATETARGET, &scr_AccelerateTarget },
			{ FIFDISTANCEISMORETHANTURN, &scr_distanceIsMoreThanTurn },
			{ FIFCRUSHED, &scr_Crushed },
			{ FMAKECRUSHVALID, &scr_MakeCrushValid },
			{ FSETTARGETTOLOWESTTARGET, &scr_set_TargetToLowestTarget },
			{ FIFNOTPUTAWAY, &scr_NotPutAway },
			{ FIFTAKENOUT, &scr_TakenOut },
			{ FIFAMMOOUT, &scr_AmmoOut },
			{ FPLAYSOUNDLOOPED, &scr_PlaySoundLooped },
			{ FSTOPSOUND, &scr_StopSound },
			{ FHEALSELF, &scr_HealSelf },
			{ FEQUIP, &scr_Equip },
			{ FIFTARGETHASITEMIDEQUIPPED, &scr_TargetHasItemIDEquipped },
			{ FSETOWNERTOTARGET, &scr_set_OwnerToTarget },
			{ FSETTARGETTOOWNER, &scr_set_TargetToOwner },
			{ FSETFRAME, &scr_set_Frame },
			{ FBREAKPASSAGE, &scr_BreakPassage },
			{ FSETRELOADTIME, &scr_set_ReloadTime },
			{ FSETTARGETTOWIDEBLAHID, &scr_set_TargetToWideBlahID },
			{ FPOOFTARGET, &scr_PoofTarget },
			{ FCHILDDOACTIONOVERRIDE, &scr_ChildDoActionOverride },
			{ FSPAWNPOOF, &scr_SpawnPoof },
			{ FSETSPEEDPERCENT, &scr_set_SpeedPercent },
			{ FSETCHILDSTATE, &scr_set_ChildState },
			{ FSPAWNATTACHEDSIZEDPARTICLE, &scr_SpawnAttachedSizedParticle },
			{ FCHANGEARMOR, &scr_ChangeArmor },
			{ FSHOWTIMER, &scr_ShowTimer },
			{ FIFFACINGTARGET, &scr_FacingTarget },
			{ FPLAYSOUNDVOLUME, &scr_PlaySoundVolume },
			{ FSPAWNATTACHEDFACEDPARTICLE, &scr_SpawnAttachedFacedParticle },
			{ FIFSTATEISODD, &scr_StateIsOdd },
			{ FSETTARGETTODISTANTENEMY, &scr_set_TargetToDistantEnemy },
			{ FTELEPORT, &scr_Teleport },
			{ FGIVESTRENGTHTOTARGET, &scr_add_TargetStrength },
			{ FGIVEINTELLECTTOTARGET, &scr_add_TargetIntelligence },
			{ FGIVEINTELLIGENCETOTARGET, &scr_add_TargetIntelligence },
			{ FGIVEDEXTERITYTOTARGET, &scr_add_TargetDexterity },
			{ FGIVELIFETOTARGET, &scr_add_TargetLife },
			{ FGIVEMANATOTARGET, &scr_add_TargetMana },
			{ FSHOWMAP, &scr_ShowMap },
			{ FSHOWYOUAREHERE, &scr_ShowYouAreHere },
			{ FSHOWBLIPXY, &scr_ShowBlipXY },
			{ FHEALTARGET, &scr_HealTarget },
			{ FPUMPTARGET, &scr_PumpTarget },
			{ FCOSTAMMO, &scr_CostAmmo },
			{ FMAKESIMILARNAMESKNOWN, &scr_MakeSimilarNamesKnown },
			{ FSPAWNATTACHEDHOLDERPARTICLE, &scr_SpawnAttachedHolderParticle },
			{ FSETTARGETRELOADTIME, &scr_set_TargetReloadTime },
			{ FSETFOGLEVEL, &scr_set_FogLevel },
			{ FGETFOGLEVEL, &scr_get_FogLevel },
			{ FSETFOGTAD, &scr_set_FogTAD },
			{ FSETFOGBOTTOMLEVEL, &scr_set_FogBottomLevel },
			{ FGETFOGBOTTOMLEVEL, &scr_get_FogBottomLevel },
			{ FCORRECTACTIONFORHAND, &scr_CorrectActionForHand },
			{ FIFTARGETISMOUNTED, &scr_TargetIsMounted },
			{ FSPARKLEICON, &scr_SparkleIcon },
			{ FUNSPARKLEICON, &scr_UnsparkleIcon },
			{ FGETTILEXY, &scr_get_TileXY },
			{ FSETTILEXY, &scr_set_TileXY },
			{ FSETSHADOWSIZE, &scr_set_ShadowSize },
			{ FORDERTARGET, &scr_OrderTarget },
			{ FSETTARGETTOWHOEVERISINPASSAGE, &scr_set_TargetToWhoeverIsInPassage },
			{ FIFCHARACTERWASABOOK, &scr_CharacterWasABook },
			{ FSETENCHANTBOOSTVALUES, &scr_set_EnchantBoostValues },
			{ FSPAWNCHARACTERXYZ, &scr_SpawnCharacterXYZ },
			{ FSPAWNEXACTCHARACTERXYZ, &scr_SpawnExactCharacterXYZ },
			{ FCHANGETARGETCLASS, &scr_ChangeTargetClass },
			{ FPLAYFULLSOUND, &scr_PlayFullSound },
			{ FSPAWNEXACTCHASEPARTICLE, &scr_SpawnExactChaseParticle },
			{ FCREATEORDER, &scr_CreateOrder },
			{ FORDERSPECIALID, &scr_OrderSpecialID },
			{ FUNKURSETARGETINVENTORY, &scr_UnkurseTargetInventory },
			{ FIFTARGETISSNEAKING, &scr_TargetIsSneaking },
			{ FDROPITEMS, &scr_DropItems },
			{ FRESPAWNTARGET, &scr_RespawnTarget },
			{ FTARGETDOACTIONSETFRAME, &scr_TargetDoActionSetFrame },
			{ FIFTARGETCANSEEINVISIBLE, &scr_TargetCanSeeInvisible },
			{ FSETTARGETTONEARESTBLAHID, &scr_set_TargetToNearestBlahID },
			{ FSETTARGETTONEARESTENEMY, &scr_set_TargetToNearestEnemy },
			{ FSETTARGETTONEARESTFRIEND, &scr_set_TargetToNearestFriend },
			{ FSETTARGETTONEARESTLIFEFORM, &scr_set_TargetToNearestLifeform },
			{ FFLASHPASSAGE, &scr_FlashPassage },
			{ FFINDTILEINPASSAGE, &scr_FindTileInPassage },
			{ FIFHELDINLEFTHAND, &scr_HeldInLeftHand },
			{ FNOTANITEM, &scr_NotAnItem },
			{ FSETCHILDAMMO, &scr_set_ChildAmmo },
			{ FIFHITVULNERABLE, &scr_HitVulnerable },
			{ FIFTARGETISFLYING, &scr_TargetIsFlying },
			{ FIDENTIFYTARGET, &scr_IdentifyTarget },
			{ FBEATMODULE, &scr_BeatModule },
			{ FENDMODULE, &scr_EndModule },
			{ FDISABLEEXPORT, &scr_DisableExport },
			{ FENABLEEXPORT, &scr_EnableExport },
			{ FGETTARGETSTATE, &scr_get_TargetState },
			{ FIFEQUIPPED, &scr_Equipped },
			{ FDROPTARGETMONEY, &scr_DropTargetMoney },
			{ FGETTARGETCONTENT, &scr_get_TargetContent },
			{ FDROPTARGETKEYS, &scr_DropTargetKeys },
			{ FJOINTEAM, &scr_JoinTeam },
			{ FTARGETJOINTEAM, &scr_TargetJoinTeam },
			{ FCLEARMUSICPASSAGE, &scr_ClearMusicPassage },
			{ FCLEARENDMESSAGE, &scr_ClearEndMessage },
			{ FADDENDMESSAGE, &scr_AddEndMessage },
			{ FPLAYMUSIC, &scr_PlayMusic },
			{ FSETMUSICPASSAGE, &scr_set_MusicPassage },
			{ FMAKECRUSHINVALID, &scr_MakeCrushInvalid },
			{ FSTOPMUSIC, &scr_StopMusic },
			{ FFLASHVARIABLE, &scr_FlashVariable },
			{ FACCELERATEUP, &scr_AccelerateUp },
			{ FFLASHVARIABLEHEIGHT, &scr_FlashVariableHeight },
			{ FSETDAMAGETIME, &scr_set_DamageTime },
			{ FIFSTATEIS8, &scr_StateIs8 },
			{ FIFSTATEIS9, &scr_StateIs9 },
			{ FIFSTATEIS10, &scr_StateIs10 },
			{ FIFSTATEIS11, &scr_StateIs11 },
			{ FIFSTATEIS12, &scr_StateIs12 },
			{ FIFSTATEIS13, &scr_StateIs13 },
			{ FIFSTATEIS14, &scr_StateIs14 },
			{ FIFSTATEIS15, &scr_StateIs15 },
			{ FIFTARGETISAMOUNT, &scr_TargetIsAMount },
			{ FIFTARGETISAPLATFORM, &scr_TargetIsAPlatform },
			{ FADDSTAT, &scr_AddStat },
			{ FDISENCHANTTARGET, &scr_DisenchantTarget },
			{ FDISENCHANTALL, &scr_DisenchantAll },
			{ FSETVOLUMENEARESTTEAMMATE, &scr_set_VolumeNearestTeammate },
			{ FADDSHOPPASSAGE, &scr_AddShopPassage },
			{ FTARGETPAYFORARMOR, &scr_TargetPayForArmor },
			{ FJOINEVILTEAM, &scr_JoinEvilTeam },
			{ FJOINNULLTEAM, &scr_JoinNullTeam },
			{ FJOINGOODTEAM, &scr_JoinGoodTeam },
			{ FPITSKILL, &scr_PitsKill },
			{ FSETTARGETTOPASSAGEID, &scr_set_TargetToPassageID },
			{ FMAKENAMEUNKNOWN, &scr_MakeNameUnknown },
			{ FSPAWNEXACTPARTICLEENDSPAWN, &scr_SpawnExactParticleEndSpawn },
			{ FSPAWNPOOFSPEEDSPACINGDAMAGE, &scr_SpawnPoofSpeedSpacingDamage },
			{ FGIVEEXPERIENCETOGOODTEAM, &scr_add_GoodTeamExperience },
			{ FDONOTHING, &scr_DoNothing },
			{ FGROGTARGET, &scr_GrogTarget },
			{ FDAZETARGET, &scr_DazeTarget },
			{ FENABLERESPAWN, &scr_EnableRespawn },
			{ FDISABLERESPAWN, &scr_DisableRespawn },
			{ FDISPELTARGETENCHANTID, &scr_DispelTargetEnchantID },
			{ FIFHOLDERBLOCKED, &scr_HolderBlocked },

			{ FIFTARGETHASNOTFULLMANA, &scr_TargetHasNotFullMana },
			{ FENABLELISTENSKILL, &scr_EnableListenSkill },
			{ FSETTARGETTOLASTITEMUSED, &scr_set_TargetToLastItemUsed },
			{ FFOLLOWLINK, &scr_FollowLink },
			{ FIFOPERATORISLINUX, &scr_OperatorIsLinux },
			{ FIFTARGETISAWEAPON, &scr_TargetIsAWeapon },
			{ FIFSOMEONEISSTEALING, &scr_SomeoneIsStealing },
			{ FIFTARGETISASPELL, &scr_TargetIsASpell },
			{ FIFBACKSTABBED, &scr_Backstabbed },
			{ FGETTARGETDAMAGETYPE, &scr_get_TargetDamageType },
			{ FADDQUEST, &scr_AddQuest },
			{ FBEATQUESTALLPLAYERS, &scr_BeatQuestAllPlayers },
			{ FIFTARGETHASQUEST, &scr_TargetHasQuest },
			{ FSETQUESTLEVEL, &scr_set_QuestLevel },
			{ FADDQUESTALLPLAYERS, &scr_AddQuestAllPlayers },
			{ FADDBLIPALLENEMIES, &scr_AddBlipAllEnemies },
			{ FPITSFALL, &scr_PitsFall },
			{ FIFTARGETISOWNER, &scr_TargetIsOwner },
			{ FEND, &scr_End },

			{ FSETSPEECH, &scr_set_Speech },
			{ FSETMOVESPEECH, &scr_set_MoveSpeech },
			{ FSETSECONDMOVESPEECH, &scr_set_SecondMoveSpeech },
			{ FSETATTACKSPEECH, &scr_set_AttackSpeech },
			{ FSETASSISTSPEECH, &scr_set_AssistSpeech },
			{ FSETTERRAINSPEECH, &scr_set_TerrainSpeech },
			{ FSETSELECTSPEECH, &scr_set_SelectSpeech },

			{ FTAKEPICTURE, &scr_TakePicture },
			{ FIFOPERATORISMACINTOSH, &scr_OperatorIsMacintosh },
			{ FIFMODULEHASIDSZ, &scr_ModuleHasIDSZ },
			{ FMORPHTOTARGET, &scr_MorphToTarget },
			{ FGIVEMANAFLOWTOTARGET, &scr_add_TargetManaFlow },
			{ FGIVEMANARETURNTOTARGET, &scr_add_TargetManaReturn },
			{ FSETMONEY, &scr_set_Money },
			{ FIFTARGETCANSEEKURSES, &scr_TargetCanSeeKurses },
			{ FSPAWNATTACHEDCHARACTER, &scr_SpawnAttachedCharacter },
			{ FKURSETARGET, &scr_KurseTarget },
			{ FSETCHILDCONTENT, &scr_set_ChildContent },
			{ FSETTARGETTOCHILD, &scr_set_TargetToChild },
			{ FSETDAMAGETHRESHOLD, &scr_set_DamageThreshold },
			{ FACCELERATETARGETUP, &scr_AccelerateTargetUp },
			{ FSETTARGETAMMO, &scr_set_TargetAmmo },
			{ FENABLEINVICTUS, &scr_EnableInvictus },
			{ FDISABLEINVICTUS, &scr_DisableInvictus },
			{ FTARGETDAMAGESELF, &scr_TargetDamageSelf },
			{ FSETTARGETSIZE, &scr_set_TargetSize },
			{ FIFTARGETISFACINGSELF, &scr_TargetIsFacingSelf },

			{ FDRAWBILLBOARD, &scr_DrawBillboard },
			{ FSETTARGETTOFIRSTBLAHINPASSAGE, &scr_set_TargetToBlahInPassage },
			{ FIFLEVELUP, &scr_LevelUp },
			{ FGIVESKILLTOTARGET, &scr_add_TargetSkill },
			{ FSETTARGETTONEARBYMELEEWEAPON, &scr_set_TargetToNearbyMeleeWeapon },

			{ FENABLESTEALTH, &scr_EnableStealth },
			{ FDISABLESTEALTH, &scr_DisableStealth },
			{ FIFSTEALTHED, &scr_Stealthed },
			{ FSETTARGETTODISTANTFRIEND, &scr_set_TargetToDistantFriend },
			{ FDISPLAYCHARGE, &scr_DisplayCharge },
		}
{
}

Runtime::~Runtime() {
}

} // namespace Script
} // namespace Ego

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A counter to measure the time of an invocation of a script function.
/// Its window size is 1 as the duration spend in the invocation is added to an histogram (see below).
static std::shared_ptr<Ego::Time::Clock<Ego::Time::ClockPolicy::NonRecursive>> g_scriptFunctionClock = nullptr;
/// @todo Data points (avg. runtimes) sorted into categories (script functions): This is a histogram
///       and can be implemented as such.
static int    _script_function_calls[SCRIPT_FUNCTIONS_COUNT];
static double _script_function_times[SCRIPT_FUNCTIONS_COUNT];

static PRO_REF script_error_model = INVALID_PRO_REF;
static const char * script_error_classname = "UNKNOWN";

static bool _scripting_system_initialized = false;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void scripting_system_begin()
{
	if (!_scripting_system_initialized) {
		Ego::Script::Runtime::initialize();
		g_scriptFunctionClock = std::make_shared<Ego::Time::Clock<Ego::Time::ClockPolicy::NonRecursive>>("script function clock", 1);
		for (size_t i = 0; i < SCRIPT_FUNCTIONS_COUNT; ++i) {
			_script_function_calls[i] = 0;
			_script_function_times[i] = 0.0F;
		}
		_scripting_system_initialized = true;
	}
}

void scripting_system_end()
{
    if (_scripting_system_initialized) {
		vfs_FILE *target = vfs_openAppend("/debug/script_function_timing.txt");
		if (nullptr != target) {
            for (size_t i = 0; i < SCRIPT_FUNCTIONS_COUNT; ++i) {
                if (_script_function_calls[i] > 0) {
					vfs_printf(target, "function == %d\tname == \"%s\"\tcalls == %d\ttime == %lf\n",
						       static_cast<int>(i), script_function_names[i], _script_function_calls[i], _script_function_times[i]);
                }
            }
            vfs_close(target);
        }
		g_scriptFunctionClock = nullptr;
		Ego::Script::Runtime::uninitialize();
        _scripting_system_initialized = false;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void scr_run_chr_script(Object *pchr) {

	// Make sure that this module is initialized.
	scripting_system_begin();

	// Do not run scripts of terminated entities.
	if (pchr->isTerminated()) {
		return;
	}
	ai_state_t& aiState = pchr->ai;
	script_info_t& script = pchr->getProfile()->getAIScript();

	// Has the time for this character to die come and gone?
	if (aiState.poof_time >= 0 && aiState.poof_time <= (Sint32)update_wld) {
		return;
	}

	// Grab the "changed" value from the last time the script was run.
	if (aiState.changed) {
		SET_BIT(aiState.alert, ALERTIF_CHANGED);
		aiState.changed = false;
	}

	Ego::Time::ClockScope<Ego::Time::ClockPolicy::NonRecursive> scope(*aiState._clock);

	// debug a certain script
	// debug_scripts = ( 385 == pself->index && 76 == pchr->profile_ref );

	// target_old is set to the target every time the script is run
	aiState.target_old = aiState.target;

	// Make life easier
	script_error_classname = "UNKNOWN";
	script_error_model = pchr->getProfileID();
	if (script_error_model < INVALID_PRO_REF)
	{
		script_error_classname = ProfileSystem::get().getProfile(script_error_model)->getClassName().c_str();
	}

	if (debug_scripts && debug_script_file) {
		vfs_FILE * scr_file = debug_script_file;

		vfs_printf(scr_file, "\n\n--------\n%s\n", script._name.c_str());
		vfs_printf(scr_file, "%d - %s\n", REF_TO_INT(script_error_model), script_error_classname);

		// who are we related to?
		vfs_printf(scr_file, "\tindex  == %d\n", REF_TO_INT(aiState.index));
		vfs_printf(scr_file, "\ttarget == %d\n", REF_TO_INT(aiState.target));
		vfs_printf(scr_file, "\towner  == %d\n", REF_TO_INT(aiState.owner));
		vfs_printf(scr_file, "\tchild  == %d\n", REF_TO_INT(aiState.child));

		// some local storage
		vfs_printf(scr_file, "\talert     == %x\n", aiState.alert);
		vfs_printf(scr_file, "\tstate     == %d\n", aiState.state);
		vfs_printf(scr_file, "\tcontent   == %d\n", aiState.content);
		vfs_printf(scr_file, "\ttimer     == %d\n", aiState.timer);
		vfs_printf(scr_file, "\tupdate_wld == %d\n", update_wld);

		// ai memory from the last event
		vfs_printf(scr_file, "\tbumplast       == %d\n", REF_TO_INT(aiState.bumplast));
		vfs_printf(scr_file, "\tattacklast     == %d\n", REF_TO_INT(aiState.attacklast));
		vfs_printf(scr_file, "\thitlast        == %d\n", REF_TO_INT(aiState.hitlast));
		vfs_printf(scr_file, "\tdirectionlast  == %d\n", aiState.directionlast);
		vfs_printf(scr_file, "\tdamagetypelast == %d\n", aiState.damagetypelast);
		vfs_printf(scr_file, "\tlastitemused   == %d\n", REF_TO_INT(aiState.lastitemused));
		vfs_printf(scr_file, "\ttarget_old     == %d\n", REF_TO_INT(aiState.target_old));

		// message handling
		vfs_printf(scr_file, "\torder == %d\n", aiState.order_value);
		vfs_printf(scr_file, "\tcounter == %d\n", aiState.order_counter);

		// waypoints
		vfs_printf(scr_file, "\twp_tail == %d\n", aiState.wp_lst.tail);
		vfs_printf(scr_file, "\twp_head == %d\n\n", aiState.wp_lst.head);
	}

	// Clear the button latches.
	if (!VALID_PLA(pchr->is_which_player)) {
		RESET_BIT_FIELD(pchr->latch.b);
	}

	// Reset the target if it can't be seen.
	if (aiState.target != aiState.index) {
		const std::shared_ptr<Object> &target = _currentModule->getObjectHandler()[aiState.target];
		if (target && !pchr->canSeeObject(target)) {
			aiState.target = aiState.index;
		}
	}

	// Reset the script state.
	script_state_t my_state;

	// Reset the ai.
	aiState.terminate = false;
	script.indent = 0;

	// Run the AI Script.
	script.set_pos(0);
	while (!aiState.terminate && script.get_pos() < script._instructions.getLength()) {
		// This is used by the Else function
		// it only keeps track of functions.
		script.indent_last = script.indent;
		script.indent = script._instructions[script.get_pos()].getDataBits();

		// Was it a function.
		if (script._instructions[script.get_pos()].isInv()) {
			if (!script_state_t::run_function_call(my_state, aiState, script)) {
				break;
			}
		}
		else {
			if (!script_state_t::run_operation(my_state, aiState, script)) {
				break;
			}
		}
	}

	// Set latches
	if (!VALID_PLA(pchr->is_which_player)) {
		float latch2;

		ai_state_t::ensure_wp(aiState);

		if (pchr->isMount() && _currentModule->getObjectHandler().exists(pchr->holdingwhich[SLOT_LEFT])) {
			// Mount
			pchr->latch.x = _currentModule->getObjectHandler().get(pchr->holdingwhich[SLOT_LEFT])->latch.x;
			pchr->latch.y = _currentModule->getObjectHandler().get(pchr->holdingwhich[SLOT_LEFT])->latch.y;
		}
		else if (aiState.wp_valid) {
			// Normal AI
			pchr->latch.x = (aiState.wp[kX] - pchr->getPosX()) / (GRID_ISIZE << 1);
			pchr->latch.y = (aiState.wp[kY] - pchr->getPosY()) / (GRID_ISIZE << 1);
		}
		else {
			// AI, but no valid waypoints
			pchr->latch.x = 0;
			pchr->latch.y = 0;
		}

		latch2 = pchr->latch.x * pchr->latch.x + pchr->latch.y * pchr->latch.y;
		if (latch2 > 1.0f) {
			float scale = 1.0f / std::sqrt(latch2);
			pchr->latch.x *= scale;
			pchr->latch.y *= scale;
		}
	}

	// Clear alerts for next time around
	RESET_BIT_FIELD(aiState.alert);
}
void scr_run_chr_script( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function lets one character do AI stuff

    // Make sure that this module is initialized.
    scripting_system_begin();

	if (!_currentModule->getObjectHandler().exists(character))  {
		return;
	}
	Object *pchr = _currentModule->getObjectHandler().get(character);
	return scr_run_chr_script(pchr);
}

//--------------------------------------------------------------------------------------------
bool script_state_t::run_function_call( script_state_t& state, ai_state_t& aiState, script_info_t& script )
{
    Uint8  functionreturn;

    // check for valid execution pointer
    if ( script.get_pos() >= script._instructions.getLength() ) return false;

    // Run the function
	functionreturn = script_state_t::run_function(state, aiState, script);

    // move the execution pointer to the jump code
    script.increment_pos();
    if ( functionreturn )
    {
        // move the execution pointer to the next opcode
		script.increment_pos();
    }
    else
    {
        // use the jump code to jump to the right location
        size_t new_index = script._instructions[script.get_pos()]._value;

        // make sure the value is valid
        EGOBOO_ASSERT( new_index <= script._instructions.getLength() );

        // actually do the jump
		script.set_pos(new_index);
    }

    return true;
}

//--------------------------------------------------------------------------------------------
/// @todo Merge with caller.
bool script_state_t::run_operation( script_state_t& state, ai_state_t& aiState, script_info_t& script )
{
    const char * variable;
    Uint32 var_value, operand_count, i;


    // check for valid execution pointer
    if ( script.get_pos() >= script._instructions.getLength() ) return false;

    var_value = script._instructions[script.get_pos()] & Instruction::VALUEBITS;

    // debug stuff
    variable = "UNKNOWN";
    if ( debug_scripts && debug_script_file )
    {

        for ( i = 0; i < script.indent; i++ ) { vfs_printf( debug_script_file, "  " ); }

        for ( i = 0; i < MAX_OPCODE; i++ )
        {
            if ( Token::Type::Variable == OpList.ary[i]._type && var_value == OpList.ary[i].iValue )
            {
                variable = OpList.ary[i].cName;
                break;
            };
        }

        vfs_printf( debug_script_file, "%s = ", variable );
    }

    // Get the number of operands
	script.increment_pos();
    operand_count = script._instructions[script.get_pos()]._value;

    // Now run the operation
    state.operationsum = 0;
    for ( i = 0; i < operand_count && script.get_pos() < script._instructions.getLength(); ++i )
    {
		script.increment_pos();
		script_state_t::run_operand(state, aiState, &script);
    }
    if ( debug_scripts && debug_script_file )
    {
        vfs_printf( debug_script_file, " == %d \n", state.operationsum );
    }

    // Save the results in the register that called the arithmetic
    script_state_t::set_operand( state, var_value );

    // go to the next opcode
	script.increment_pos();

    return true;
}

//--------------------------------------------------------------------------------------------
Uint8 script_state_t::run_function(script_state_t& self, ai_state_t& aiState, script_info_t& script)
{
    /// @author BB
    /// @details This is about half-way to what is needed for Lua integration

    // Mask out the indentation
    Uint32 valuecode = script._instructions[script.get_pos()] & Instruction::VALUEBITS;

    // Assume that the function will pass, as most do
    Uint8 returncode = true;
    if ( MAX_OPCODE == valuecode )
    {
        log_message( "SCRIPT ERROR: scr_run_function() - model == %d, class name == \"%s\" - Unknown opcode found!\n", REF_TO_INT( script_error_model ), script_error_classname );
        return false;
    }

    // debug stuff
    if ( debug_scripts && debug_script_file )
    {
        Uint32 i;

        for ( i = 0; i < script.indent; i++ ) { vfs_printf( debug_script_file,  "  " ); }

        for ( i = 0; i < MAX_OPCODE; i++ )
        {
            if ( Token::Type::Function == OpList.ary[i]._type && valuecode == OpList.ary[i].iValue )
            {
                vfs_printf( debug_script_file,  "%s\n", OpList.ary[i].cName );
                break;
            };
        }
    }

    if ( valuecode > SCRIPT_FUNCTIONS_COUNT )
    {
    }
    else
    {
		{ 
			Ego::Time::ClockScope<Ego::Time::ClockPolicy::NonRecursive> scope(*g_scriptFunctionClock);
			auto result = Ego::Script::Runtime::get()._functionValueCodeToFunctionPointer.find(valuecode);
			if (Ego::Script::Runtime::get()._functionValueCodeToFunctionPointer.cend() == result) {
				log_message("SCRIPT ERROR: scr_run_function() - ai script \"%s\" - unhandled script function %d\n", script._name.c_str(), valuecode);
				returncode = false;
			} else {
				returncode = result->second(self, aiState);
			}
        }

        _script_function_calls[valuecode] += 1;
        _script_function_times[valuecode] += g_scriptFunctionClock->lst();
    }

    return returncode;
}

//--------------------------------------------------------------------------------------------
void script_state_t::set_operand( script_state_t& state, Uint8 variable )
{
    /// @author ZZ
    /// @details This function sets one of the tmp* values for scripted AI
    switch ( variable )
    {
        case VARTMPX:
            state.x = state.operationsum;
            break;

        case VARTMPY:
            state.y = state.operationsum;
            break;

        case VARTMPDISTANCE:
            state.distance = state.operationsum;
            break;

        case VARTMPTURN:
            state.turn = state.operationsum;
            break;

        case VARTMPARGUMENT:
            state.argument = state.operationsum;
            break;

        default:
            log_warning( "scr_set_operand() - cannot assign a number to index %d\n", variable );
            break;
    }
}

//--------------------------------------------------------------------------------------------
void script_state_t::run_operand( script_state_t& state, ai_state_t& aiState, script_info_t * pscript )
{
    /// @author ZZ
    /// @details This function does the scripted arithmetic in OPERATOR, OPERAND pscriptrs

    const char * varname, * op;

    STRING buffer = EMPTY_CSTR;
    Uint8  variable;
    Uint8  operation;

    int32_t iTmp;

    Object * pchr = NULL, * ptarget = NULL, * powner = NULL;

	if (!_currentModule->getObjectHandler().exists(aiState.index)) return;
	pchr = _currentModule->getObjectHandler().get(aiState.index);

	if (_currentModule->getObjectHandler().exists(aiState.target))
    {
		ptarget = _currentModule->getObjectHandler().get(aiState.target);
    }

	if (_currentModule->getObjectHandler().exists(aiState.owner))
    {
		powner = _currentModule->getObjectHandler().get(aiState.owner);
    }

    // get the operator
    iTmp      = 0;
    varname   = buffer;
    operation = pscript->_instructions[pscript->get_pos()].getDataBits();
    if ( pscript->_instructions[pscript->get_pos()].isLdc() )
    {
        // Get the working opcode from a constant, constants are all but high 5 bits
        iTmp = pscript->_instructions[pscript->get_pos()] & Instruction::VALUEBITS;
        if ( debug_scripts ) snprintf( buffer, SDL_arraysize( buffer ), "%d", iTmp );
    }
    else
    {
        // Get the variable opcode from a register
        variable = pscript->_instructions[pscript->get_pos()] & Instruction::VALUEBITS;

        switch ( variable )
        {
            case VARTMPX:
                varname = "TMPX";
                iTmp = state.x;
                break;

            case VARTMPY:
                varname = "TMPY";
                iTmp = state.y;
                break;

            case VARTMPDISTANCE:
                varname = "TMPDISTANCE";
                iTmp = state.distance;
                break;

            case VARTMPTURN:
                varname = "TMPTURN";
                iTmp = state.turn;
                break;

            case VARTMPARGUMENT:
                varname = "TMPARGUMENT";
                iTmp = state.argument;
                break;

            case VARRAND:
                varname = "RAND";
                iTmp = Random::next(std::numeric_limits<uint16_t>::max());
                break;

            case VARSELFX:
                varname = "SELFX";
                iTmp = pchr->getPosX();
                break;

            case VARSELFY:
                varname = "SELFY";
                iTmp = pchr->getPosY();
                break;

            case VARSELFTURN:
                varname = "SELFTURN";
                iTmp = pchr->ori.facing_z;
                break;

            case VARSELFCOUNTER:
                varname = "SELFCOUNTER";
				iTmp = aiState.order_counter;
                break;

            case VARSELFORDER:
                varname = "SELFORDER";
				iTmp = aiState.order_value;
                break;

            case VARSELFMORALE:
                varname = "SELFMORALE";
                iTmp = _currentModule->getTeamList()[pchr->team_base].getMorale();
                break;

            case VARSELFLIFE:
                varname = "SELFLIFE";
                iTmp = FLOAT_TO_FP8(pchr->getLife());
                break;

            case VARTARGETX:
                varname = "TARGETX";
                iTmp = ( nullptr == ptarget ) ? 0 : ptarget->getPosX();
                break;

            case VARTARGETY:
                varname = "TARGETY";
                iTmp = ( nullptr == ptarget ) ? 0 : ptarget->getPosY();
                break;

            case VARTARGETDISTANCE:
                varname = "TARGETDISTANCE";
                if ( nullptr == ptarget )
                {
                    iTmp = 0x7FFFFFFF;
                }
                else
                {
                    iTmp = std::abs(ptarget->getPosX() - pchr->getPosX())
                         + std::abs(ptarget->getPosY() - pchr->getPosY());
                }
                break;

            case VARTARGETTURN:
                varname = "TARGETTURN";
                iTmp = ( nullptr == ptarget ) ? 0 : ptarget->ori.facing_z;
                break;

            case VARLEADERX:
            {
                varname = "LEADERX";
                iTmp = pchr->getPosX();
                std::shared_ptr<Object> leader = _currentModule->getTeamList()[pchr->team].getLeader();
                if ( leader )
                    iTmp = leader->getPosX();
                break;
            }

            case VARLEADERY:
            {
                varname = "LEADERY";
                iTmp = pchr->getPosY();
                std::shared_ptr<Object> leader = _currentModule->getTeamList()[pchr->team].getLeader();
                if ( leader )
                    iTmp = leader->getPosY();

                break; 
            }

            case VARLEADERDISTANCE:
                {
                    varname = "LEADERDISTANCE";

                    std::shared_ptr<Object> pleader = _currentModule->getTeamList()[pchr->team].getLeader();
                    if ( !pleader )
                    {
                        iTmp = 0x7FFFFFFF;
                    }
                    else
                    {
                        iTmp = std::abs(pleader->getPosX() - pchr->getPosX())
                             + std::abs(pleader->getPosY() - pchr->getPosY());
                    }
                }
                break;

            case VARLEADERTURN:
                varname = "LEADERTURN";
                iTmp = pchr->ori.facing_z;
                if ( _currentModule->getTeamList()[pchr->team].getLeader() )
                    iTmp = _currentModule->getTeamList()[pchr->team].getLeader()->ori.facing_z;

                break;

            case VARGOTOX:
                varname = "GOTOX";

				ai_state_t::ensure_wp(aiState);

				if (!aiState.wp_valid)
                {
                    iTmp = pchr->getPosX();
                }
                else
                {
					iTmp = aiState.wp[kX];
                }
                break;

            case VARGOTOY:
                varname = "GOTOY";

				ai_state_t::ensure_wp(aiState);

				if (!aiState.wp_valid)
                {
                    iTmp = pchr->getPosY();
                }
                else
                {
					iTmp = aiState.wp[kY];
                }
                break;

            case VARGOTODISTANCE:
                varname = "GOTODISTANCE";

				ai_state_t::ensure_wp(aiState);

				if (!aiState.wp_valid)
                {
                    iTmp = 0x7FFFFFFF;
                }
                else
                {
					iTmp = std::abs(aiState.wp[kX] - pchr->getPosX())
						 + std::abs(aiState.wp[kY] - pchr->getPosY());
                }
                break;

            case VARTARGETTURNTO:
                varname = "TARGETTURNTO";
                if ( NULL == ptarget )
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = vec_to_facing( ptarget->getPosX() - pchr->getPosX() , ptarget->getPosY() - pchr->getPosY() );
                    iTmp = CLIP_TO_16BITS( iTmp );
                }
                break;

            case VARPASSAGE:
                varname = "PASSAGE";
				iTmp = aiState.passage;
                break;

            case VARWEIGHT:
                varname = "WEIGHT";
                iTmp = pchr->holdingweight;
                break;

            case VARSELFALTITUDE:
                varname = "SELFALTITUDE";
                iTmp = pchr->getPosZ() - pchr->enviro.floor_level;
                break;

            case VARSELFID:
                varname = "SELFID";
				iTmp = chr_get_idsz(aiState.index, IDSZ_TYPE);
                break;

            case VARSELFHATEID:
                varname = "SELFHATEID";
				iTmp = chr_get_idsz(aiState.index, IDSZ_HATE);
                break;

            case VARSELFMANA:
                varname = "SELFMANA";
                iTmp = FLOAT_TO_FP8(pchr->getMana());
                if ( pchr->getAttribute(Ego::Attribute::CHANNEL_LIFE) )  iTmp += FLOAT_TO_FP8(pchr->getLife());

                break;

            case VARTARGETSTR:
                varname = "TARGETSTR";
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::MIGHT));
                break;

            case VARTARGETINT:
                varname = "TARGETINT";
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::INTELLECT));
                break;

            case VARTARGETDEX:
                varname = "TARGETDEX";
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::AGILITY));
                break;

            case VARTARGETLIFE:
                varname = "TARGETLIFE";
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getLife());
                break;

            case VARTARGETMANA:
                varname = "TARGETMANA";
                if ( NULL == ptarget )
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = FLOAT_TO_FP8(ptarget->getMana());
                    if ( ptarget->getAttribute(Ego::Attribute::CHANNEL_LIFE) ) iTmp += FLOAT_TO_FP8(ptarget->getLife());
                }

                break;

            case VARTARGETLEVEL:
                varname = "TARGETLEVEL";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->experiencelevel;
                break;

            case VARTARGETSPEEDX:
                varname = "TARGETSPEEDX";
                iTmp = ( NULL == ptarget ) ? 0 : std::abs(ptarget->vel[kX]);
                break;

            case VARTARGETSPEEDY:
                varname = "TARGETSPEEDY";
                iTmp = ( NULL == ptarget ) ? 0 : std::abs(ptarget->vel[kY]);
                break;

            case VARTARGETSPEEDZ:
                varname = "TARGETSPEEDZ";
                iTmp = ( NULL == ptarget ) ? 0 : std::abs(ptarget->vel[kZ]);
                break;

            case VARSELFSPAWNX:
                varname = "SELFSPAWNX";
                iTmp = pchr->pos_stt[kX];
                break;

            case VARSELFSPAWNY:
                varname = "SELFSPAWNY";
                iTmp = pchr->pos_stt[kY];
                break;

            case VARSELFSTATE:
                varname = "SELFSTATE";
				iTmp = aiState.state;
                break;

            case VARSELFCONTENT:
                varname = "SELFCONTENT";
				iTmp = aiState.content;
                break;

            case VARSELFSTR:
                varname = "SELFSTR";
                iTmp = FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::MIGHT));
                break;

            case VARSELFINT:
                varname = "SELFINT";
                iTmp = FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::INTELLECT));
                break;

            case VARSELFDEX:
                varname = "SELFDEX";
                iTmp = FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::AGILITY));
                break;

            case VARSELFMANAFLOW:
                varname = "SELFMANAFLOW";
                iTmp = FLOAT_TO_FP8(pchr->getAttribute(Ego::Attribute::SPELL_POWER));
                break;

            case VARTARGETMANAFLOW:
                varname = "TARGETMANAFLOW";
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::SPELL_POWER));
                break;

            case VARSELFATTACHED:
                varname = "SELFATTACHED";
				iTmp = number_of_attached_particles(aiState.index);
                break;

            case VARSWINGTURN:
                varname = "SWINGTURN";
                {
					std::shared_ptr<Camera> camera = CameraSystem::get()->getCameraByChrID(aiState.index);

                    iTmp = 0;
                    if ( camera )
                    {
                        iTmp = camera->getSwing() << 2;
                    }
                }
                break;

            case VARXYDISTANCE:
                varname = "XYDISTANCE";
                iTmp = std::sqrt( state.x * state.x + state.y * state.y );
                break;

            case VARSELFZ:
                varname = "SELFZ";
                iTmp = pchr->getPosZ();
                break;

            case VARTARGETALTITUDE:
                varname = "TARGETALTITUDE";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->getPosZ() - ptarget->enviro.floor_level;
                break;

            case VARTARGETZ:
                varname = "TARGETZ";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->getPosZ();
                break;

            case VARSELFINDEX:
                varname = "SELFINDEX";
				iTmp = REF_TO_INT(aiState.index);
                break;

            case VAROWNERX:
                varname = "OWNERX";
                iTmp = ( NULL == powner ) ? 0 : powner->getPosX();
                break;

            case VAROWNERY:
                varname = "OWNERY";
                iTmp = ( NULL == powner ) ? 0 : powner->getPosY();
                break;

            case VAROWNERTURN:
                varname = "OWNERTURN";
                iTmp = ( NULL == powner ) ? 0 : powner->ori.facing_z;
                break;

            case VAROWNERDISTANCE:
                varname = "OWNERDISTANCE";
                if ( NULL == powner )
                {
                    iTmp = 0x7FFFFFFF;
                }
                else
                {
                    iTmp = std::abs(powner->getPosX() - pchr->getPosX())
                         + std::abs(powner->getPosY() - pchr->getPosY());
                }
                break;

            case VAROWNERTURNTO:
                varname = "OWNERTURNTO";
                if ( NULL == powner )
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = vec_to_facing( powner->getPosX() - pchr->getPosX() , powner->getPosY() - pchr->getPosY() );
                    iTmp = CLIP_TO_16BITS( iTmp );
                }
                break;

            case VARXYTURNTO:
                varname = "XYTURNTO";
                iTmp = vec_to_facing( state.x - pchr->getPosX() , state.y - pchr->getPosY() );
                iTmp = CLIP_TO_16BITS( iTmp );
                break;

            case VARSELFMONEY:
                varname = "SELFMONEY";
                iTmp = pchr->money;
                break;

            case VARSELFACCEL:
                varname = "SELFACCEL";
                iTmp = ( pchr->getAttribute(Ego::Attribute::ACCELERATION) * 100.0f );
                break;

            case VARTARGETEXP:
                varname = "TARGETEXP";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->experience;
                break;

            case VARSELFAMMO:
                varname = "SELFAMMO";
                iTmp = pchr->ammo;
                break;

            case VARTARGETAMMO:
                varname = "TARGETAMMO";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->ammo;
                break;

            case VARTARGETMONEY:
                varname = "TARGETMONEY";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->money;
                break;

            case VARTARGETTURNAWAY:
                varname = "TARGETTURNAWAY";
                if ( NULL == ptarget )
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = vec_to_facing( ptarget->getPosX() - pchr->getPosX() , ptarget->getPosY() - pchr->getPosY() );
                    iTmp = CLIP_TO_16BITS( iTmp );
                }
                break;

            case VARSELFLEVEL:
                varname = "SELFLEVEL";
                iTmp = pchr->experiencelevel;
                break;

            case VARTARGETRELOADTIME:
                varname = "TARGETRELOADTIME";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->reload_timer;
                break;

            case VARSPAWNDISTANCE:
                varname = "SPAWNDISTANCE";
                iTmp = std::abs( pchr->pos_stt[kX] - pchr->getPosX() )
                     + std::abs( pchr->pos_stt[kY] - pchr->getPosY() );
                break;

            case VARTARGETMAXLIFE:
                varname = "TARGETMAXLIFE";
                iTmp = ( NULL == ptarget ) ? 0 : FLOAT_TO_FP8(ptarget->getAttribute(Ego::Attribute::MAX_LIFE));
                break;

            case VARTARGETTEAM:
                varname = "TARGETTEAM";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->team;
                //iTmp = REF_TO_INT( chr_get_iteam( pself->target ) );
                break;

            case VARTARGETARMOR:
                varname = "TARGETARMOR";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->skin;
                break;

            case VARDIFFICULTY:
                varname = "DIFFICULTY";
                iTmp = static_cast<uint32_t>(egoboo_config_t::get().game_difficulty.getValue());
                break;

            case VARTIMEHOURS:
                varname = "TIMEHOURS";
                iTmp = Ego::Time::LocalTime().getHours();
                break;

            case VARTIMEMINUTES:
                varname = "TIMEMINUTES";
                iTmp = Ego::Time::LocalTime().getMinutes();
                break;

            case VARTIMESECONDS:
                varname = "TIMESECONDS";
                iTmp = Ego::Time::LocalTime().getSeconds();
                break;

            case VARDATEMONTH:
                varname = "DATEMONTH";
                iTmp = Ego::Time::LocalTime().getMonth() + 1; /// @todo The addition of +1 should be removed and
				                                              /// the whole Ego::Time::LocalTime class should be
				                                              /// made available via EgoScript. However, EgoScript
				                                              /// is not yet ready for that ... not yet.
                break;

            case VARDATEDAY:
                varname = "DATEDAY";
                iTmp = Ego::Time::LocalTime().getDayOfMonth();
                break;

            default:
                log_message( "SCRIPT ERROR: scr_run_operand() - model == %d, class name == \"%s\" - Unknown variable found!\n", REF_TO_INT( script_error_model ), script_error_classname );
                break;
        }
    }

    // Now do the math
    op = "UNKNOWN";
    switch ( operation )
    {
        case OPADD:
            op = "ADD";
            state.operationsum += iTmp;
            break;

        case OPSUB:
            op = "SUB";
            state.operationsum -= iTmp;
            break;

        case OPAND:
            op = "AND";
            state.operationsum &= iTmp;
            break;

        case OPSHR:
            op = "SHR";
            state.operationsum >>= iTmp;
            break;

        case OPSHL:
            op = "SHL";
            state.operationsum <<= iTmp;
            break;

        case OPMUL:
            op = "MUL";
            state.operationsum *= iTmp;
            break;

        case OPDIV:
            op = "DIV";
            if ( iTmp != 0 )
            {
                state.operationsum = static_cast<float>(state.operationsum) / iTmp;
            }
            else
            {
                log_message( "SCRIPT ERROR: scr_run_operand() - model == %d, class name == \"%s\" - Cannot divide by zero!\n", REF_TO_INT( script_error_model ), script_error_classname );
            }
            break;

        case OPMOD:
            op = "MOD";
            if ( iTmp != 0 )
            {
                state.operationsum %= iTmp;
            }
            else
            {
                log_message( "SCRIPT ERROR: scr_run_operand() - model == %d, class name == \"%s\" - Cannot modulo by zero!\n", REF_TO_INT( script_error_model ), script_error_classname );
            }
            break;

        default:
            log_message( "SCRIPT ERROR: scr_run_operand() - model == %d, class name == \"%s\" - unknown op\n", REF_TO_INT( script_error_model ), script_error_classname );
            break;
    }

    if ( debug_scripts && debug_script_file )
    {
        vfs_printf( debug_script_file, "%s %s(%d) ", op, varname, iTmp );
    }
}

//--------------------------------------------------------------------------------------------

bool script_info_t::increment_pos() {
	if (_position >= _instructions.getLength()) {
		return false;
	}
	_position++;
    return true;
}

size_t script_info_t::get_pos() const {
	return _position;
}

bool script_info_t::set_pos(size_t position) {
	if (position >= _instructions.getLength()) {
		return false;
	}
	_position = position;
    return true;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool ai_state_t::get_wp( ai_state_t& self )
{
    // try to load up the top waypoint

    if ( !_currentModule->getObjectHandler().exists( self.index ) ) return false;

    self.wp_valid = waypoint_list_peek( self.wp_lst, self.wp );

    return true;
}

//--------------------------------------------------------------------------------------------
bool ai_state_t::ensure_wp(ai_state_t& self)
{
    // is the current waypoint is not valid, try to load up the top waypoint

	if (!_currentModule->getObjectHandler().exists(self.index)) {
		return false;
	}
	if (self.wp_valid) {
		return true;
	}
    return ai_state_t::get_wp(self);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void set_alerts( const CHR_REF character )
{
    /// @author ZZ
    /// @details This function polls some alert conditions

    // invalid characters do not think
	if (!_currentModule->getObjectHandler().exists(character)) {
		return;
	}
	Object *pchr = _currentModule->getObjectHandler().get(character);
	ai_state_t& aiState = pchr->ai;

	if (waypoint_list_empty(aiState.wp_lst)) {
		return;
	}

    // let's let mounts get alert updates...
    // imagine a mount, like a racecar, that needs to make sure that it follows X
    // waypoints around a track or something

    // mounts do not get alerts
    // if ( _currentModule->getObjectHandler().exists(pchr->attachedto) ) return;

    // is the current waypoint is not valid, try to load up the top waypoint
    ai_state_t::ensure_wp(aiState);

    bool at_waypoint = false;
	if (aiState.wp_valid)
    {
		at_waypoint = (std::abs(pchr->getPosX() - aiState.wp[kX]) < WAYTHRESH) &&
			          (std::abs(pchr->getPosY() - aiState.wp[kY]) < WAYTHRESH);
    }

    if ( at_waypoint )
    {
		SET_BIT(aiState.alert, ALERTIF_ATWAYPOINT);

		if (waypoint_list_finished(aiState.wp_lst))
        {
            // we are now at the last waypoint
            // if the object can be alerted to last waypoint, do it
            // this test needs to be done because the ALERTIF_ATLASTWAYPOINT
            // doubles for "at last waypoint" and "not put away"
            if ( !pchr->getProfile()->isEquipment() )
            {
				SET_BIT(aiState.alert, ALERTIF_ATLASTWAYPOINT);
            }

            // !!!!restart the waypoint list, do not clear them!!!!
			waypoint_list_reset(aiState.wp_lst);

            // load the top waypoint
			ai_state_t::get_wp(aiState);
        }
		else if (waypoint_list_advance(aiState.wp_lst))
        {
            // load the top waypoint
			ai_state_t::get_wp(aiState);
        }
    }
}

//--------------------------------------------------------------------------------------------
void issue_order( const CHR_REF character, Uint32 value )
{
    /// @author ZZ
    /// @details This function issues an value for help to all teammates
    int counter = 0;

    const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[character];

    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        if ( object->isTerminated() ) continue;

        if ( object->getTeam() == pchr->getTeam() )
        {
            ai_state_t::add_order(object->ai, value, counter);
            counter++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void issue_special_order( Uint32 value, IDSZ idsz )
{
    /// @author ZZ
    /// @details This function issues an order to all characters with the a matching special IDSZ
    int counter = 0;

    for(const std::shared_ptr<Object> &object : _currentModule->getObjectHandler().iterator())
    {
        if ( object->isTerminated() ) continue;

        if ( idsz == object->getProfile()->getIDSZ(IDSZ_SPECIAL) )
        {
            ai_state_t::add_order(object->ai, value, counter);
            counter++;
        }
    }
}

//--------------------------------------------------------------------------------------------

ai_state_t::ai_state_t() {
	_clock = std::make_shared<Ego::Time::Clock<Ego::Time::ClockPolicy::NonRecursive>>("", 8);
	poof_time = -1;
	changed = false;
	terminate = false;

	// who are we related to?
	index = INVALID_CHR_REF;
	target = INVALID_CHR_REF;
	owner = INVALID_CHR_REF;
	child = INVALID_CHR_REF;

	// some local storage
	alert = 0;
	state = 0;
	content = 0;
	passage = 0;
	timer = 0;
	for (size_t i = 0; i < STOR_COUNT; ++i) {
		x[i] = 0;
		y[i] = 0;
	}

	// ai memory from the last event
	bumplast = INVALID_CHR_REF;
	bumplast_time = 0;

	attacklast = INVALID_CHR_REF;
	hitlast = INVALID_CHR_REF;
	directionlast = 0;
	damagetypelast = DamageType::DAMAGE_DIRECT;
	lastitemused = INVALID_CHR_REF;
	target_old = INVALID_CHR_REF;

	// message handling
	order_value = 0;
	order_counter = 0;

	// waypoints
	wp_valid = false;
	wp_lst.head = wp_lst.tail = 0;
	astar_timer = 0;
}

ai_state_t::~ai_state_t() {
	_clock = nullptr;
}

void ai_state_t::reset(ai_state_t& self)
{
	self._clock->reinit();

	self.poof_time = -1;
	self.changed = false;
	self.terminate = false;

	// who are we related to?
	self.index = INVALID_CHR_REF;
	self.target = INVALID_CHR_REF;
	self.owner = INVALID_CHR_REF;
	self.child = INVALID_CHR_REF;

	// some local storage
	self.alert = 0;         ///< Alerts for AI script
	self.state = 0;
	self.content = 0;
	self.passage = 0;
	self.timer = 0;
	for (size_t i = 0; i < STOR_COUNT; ++i) {
		self.x[i] = 0;
		self.y[i] = 0;
	}
    self.maxSpeed = 1.0f;

	// ai memory from the last event
	self.bumplast = INVALID_CHR_REF;
	self.bumplast_time = 0;

	self.attacklast = INVALID_CHR_REF;
	self.hitlast = INVALID_CHR_REF;
	self.directionlast = 0;
	self.damagetypelast = DamageType::DAMAGE_DIRECT;
	self.lastitemused = INVALID_CHR_REF;
	self.target_old = INVALID_CHR_REF;

	// message handling
	self.order_value = 0;
	self.order_counter = 0;

	// waypoints
	self.wp_valid = false;
	self.wp_lst.head = self.wp_lst.tail = 0;
	self.astar_timer = 0;
}

bool ai_state_t::add_order(ai_state_t& self, Uint32 value, Uint16 counter)
{
    // this function is only truely valid if there is no other order
	bool retval = HAS_NO_BITS(self.alert, ALERTIF_ORDERED);

	SET_BIT(self.alert, ALERTIF_ORDERED);
    self.order_value   = value;
    self.order_counter = counter;

    return retval;
}

bool ai_state_t::set_changed(ai_state_t& self)
{
    /// @author BB
    /// @details do something tricky here

    bool retval = false;

	if (HAS_NO_BITS(self.alert, ALERTIF_CHANGED))
    {
		SET_BIT(self.alert, ALERTIF_CHANGED);
        retval = true;
    }

	if (!self.changed)
    {
		self.changed = true;
        retval = true;
    }

    return retval;
}

bool ai_state_t::set_bumplast(ai_state_t& self, const CHR_REF ichr)
{
    /// @author BB
    /// @details bumping into a chest can initiate whole loads of update messages.
    ///     Try to throttle the rate that new "bump" messages can be passed to the ai

	if (!_currentModule->getObjectHandler().exists(ichr)) {
		return false;
	}

    // 5 bumps per second?
	if (self.bumplast != ichr || update_wld > self.bumplast_time + GameEngine::GAME_TARGET_UPS / 5) {
		self.bumplast_time = update_wld;
		SET_BIT(self.alert, ALERTIF_BUMPED);
    }
	self.bumplast = ichr;

    return true;
}

void ai_state_t::spawn(ai_state_t& self, const CHR_REF index, const PRO_REF iobj, Uint16 rank)
{
    const std::shared_ptr<Object> &pchr = _currentModule->getObjectHandler()[index];
	ai_state_t::reset(self);

	if (!pchr) {
		return;
	}

	self.index = index;
	self.alert = ALERTIF_SPAWNED;
	self.state = pchr->getProfile()->getStateOverride();
	self.content = pchr->getProfile()->getContentOverride();
	self.passage = 0;
	self.target = index;
	self.owner = index;
	self.child = index;
	self.target_old = index;
    self.maxSpeed = 1.0f;

	self.bumplast = index;
	self.hitlast = index;

	self.order_counter = rank;
	self.order_value = 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
script_state_t::script_state_t()
	: x(0), y(0), turn(0), distance(0),
	  argument(0), operationsum(0)
{
}

script_state_t::script_state_t(const script_state_t& other)
	: x(other.x), y(other.y), turn(other.turn), distance(other.distance),
	  argument(other.argument), operationsum(other.operationsum)
{
}
