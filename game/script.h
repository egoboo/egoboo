#pragma once

// These are for the AI script loading/parsing routines
extern int                     iNumAis;

#define PITNOSOUND          -256                    // Stop sound at bottom of pits...
#define MSGDISTANCE         2000                 // Range for SendMessageNear

#define MAXAI               129                     //
#define MAXCODE             1024                    // Number of lines in AICODES.TXT
#define MAXCODENAMESIZE     64                      //
#define AISMAXCOMPILESIZE   (MAXAI*MAXCODE)         // For parsing AI scripts

typedef enum script_opcode_e
{
  F_IfSpawned = 0, // 0                     // Scripted AI functions (v0.10)
  F_IfTimeOut, // 1
  F_IfAtWaypoint, // 2
  F_IfAtLastWaypoint, // 3
  F_IfAttacked, // 4
  F_IfBumped, // 5
  F_IfSignaled, // 6
  F_IfCalledForHelp, // 7
  F_SetContent, // 8
  F_IfKilled, // 9
  F_IfTargetKilled, // 10
  F_ClearWaypoints, // 11
  F_AddWaypoint, // 12
  F_FindPath, // 13
  F_Compass, // 14
  F_GetTargetArmorPrice, // 15
  F_SetTime, // 16
  F_GetContent, // 17
  F_JoinTargetTeam, // 18
  F_SetTargetToNearbyEnemy, // 19
  F_SetTargetToTargetLeftHand, // 20
  F_SetTargetToTargetRightHand, // 21
  F_SetTargetToWhoeverAttacked, // 22
  F_SetTargetToWhoeverBumped, // 23
  F_SetTargetToWhoeverCalledForHelp, // 24
  F_SetTargetToOldTarget, // 25
  F_SetTurnModeToVelocity, // 26
  F_SetTurnModeToWatch, // 27
  F_SetTurnModeToSpin, // 28
  F_SetBumpHeight, // 29
  F_IfTargetHasID, // 30
  F_IfTargetHasItemID, // 31
  F_IfTargetHoldingItemID, // 32
  F_IfTargetHasSkillID, // 33
  F_Else, // 34
  F_Run, // 35
  F_Walk, // 36
  F_Sneak, // 37
  F_DoAction, // 38
  F_KeepAction, // 39
  F_SignalTeam, // 40
  F_DropWeapons, // 41
  F_TargetDoAction, // 42
  F_OpenPassage, // 43
  F_ClosePassage, // 44
  F_IfPassageOpen, // 45
  F_GoPoof, // 46
  F_CostTargetItemID, // 47
  F_DoActionOverride, // 48
  F_IfHealed, // 49
  F_DisplayMessage, // 50
  F_CallForHelp, // 51
  F_AddIDSZ, // 52
  F_End, // 53
  F_SetState, // 54                         // Scripted AI functions (v0.20)
  F_GetState, // 55
  F_IfStateIs, // 56
  F_IfTargetCanOpenStuff, // 57             // Scripted AI functions (v0.30)
  F_IfGrabbed, // 58
  F_IfDropped, // 59
  F_SetTargetToWhoeverIsHolding, // 60
  F_DamageTarget, // 61
  F_IfXIsLessThanY, // 62
  F_SetWeatherTime, // 63                   // Scripted AI functions (v0.40)
  F_GetBumpHeight, // 64
  F_IfReaffirmed, // 65
  F_UnkeepAction, // 66
  F_IfTargetIsOnOtherTeam, // 67
  F_IfTargetIsOnHatedTeam, // 68         // Scripted AI functions (v0.50)
  F_PressLatchButton, // 69
  F_SetTargetToTargetOfLeader, // 70
  F_IfLeaderKilled, // 71
  F_BecomeLeader, // 72
  F_ChangeTargetArmor, // 73               // Scripted AI functions (v0.60)
  F_GiveMoneyToTarget, // 74
  F_DropKeys, // 75
  F_IfLeaderIsAlive, // 76
  F_IfTargetIsOldTarget, // 77
  F_SetTargetToLeader, // 78
  F_SpawnCharacter, // 79
  F_RespawnCharacter, // 80
  F_ChangeTile, // 81
  F_IfUsed, // 82
  F_DropMoney, // 83
  F_SetOldTarget, // 84
  F_DetachFromHolder, // 85
  F_IfTargetHasVulnerabilityID, // 86
  F_CleanUp, // 87
  F_IfCleanedUp, // 88
  F_IfSitting, // 89
  F_IfTargetIsHurt, // 90
  F_IfTargetIsAPlayer, // 91
  F_PlaySound, // 92
  F_SpawnParticle, // 93
  F_IfTargetIsAlive, // 94
  F_Stop, // 95
  F_DisaffirmCharacter, // 96
  F_ReaffirmCharacter, // 97
  F_IfTargetIsSelf, // 98
  F_IfTargetIsMale, // 99
  F_IfTargetIsFemale, // 100
  F_SetTargetToSelf, // 101           // Scripted AI functions (v0.70)
  F_SetTargetToRider, // 102
  F_GetAttackTurn, // 103
  F_GetDamageType, // 104
  F_BecomeSpell, // 105
  F_BecomeSpellbook, // 106
  F_IfScoredAHit, // 107
  F_IfDisaffirmed, // 108
  F_DecodeOrder, // 109
  F_SetTargetToWhoeverWasHit, // 110
  F_SetTargetToWideEnemy, // 111
  F_IfChanged, // 112
  F_IfInWater, // 113
  F_IfBored, // 114
  F_IfTooMuchBaggage, // 115
  F_IfGrogged, // 116
  F_IfDazed, // 117
  F_IfTargetHasSpecialID, // 118
  F_PressTargetLatchButton, // 119
  F_IfInvisible, // 120
  F_IfArmorIs, // 121
  F_GetTargetGrogTime, // 122
  F_GetTargetDazeTime, // 123
  F_SetDamageType, // 124
  F_SetWaterLevel, // 125
  F_EnchantTarget, // 126
  F_EnchantChild, // 127
  F_TeleportTarget, // 128
  F_GiveExperienceToTarget, // 129
  F_IncreaseAmmo, // 130
  F_UnkurseTarget, // 131
  F_GiveExperienceToTargetTeam, // 132
  F_IfUnarmed, // 133
  F_RestockTargetAmmoIDAll, // 134
  F_RestockTargetAmmoIDFirst, // 135
  F_FlashTarget, // 136
  F_SetRedShift, // 137
  F_SetGreenShift, // 138
  F_SetBlueShift, // 139
  F_SetLight, // 140
  F_SetAlpha, // 141
  F_IfHitFromBehind, // 142
  F_IfHitFromFront, // 143
  F_IfHitFromLeft, // 144
  F_IfHitFromRight, // 145
  F_IfTargetIsOnSameTeam, // 146
  F_KillTarget, // 147
  F_UndoEnchant, // 148
  F_GetWaterLevel, // 149
  F_CostTargetMana, // 150
  F_IfTargetHasAnyID, // 151
  F_SetBumpSize, // 152
  F_IfNotDropped, // 153
  F_IfYIsLessThanX, // 154
  F_SetFlyHeight, // 155
  F_IfBlocked, // 156
  F_IfTargetIsDefending, // 157
  F_IfTargetIsAttacking, // 158
  F_IfStateIs0, // 159
  F_IfStateIs1, // 160
  F_IfStateIs2, // 161
  F_IfStateIs3, // 162
  F_IfStateIs4, // 163
  F_IfStateIs5, // 164
  F_IfStateIs6, // 165
  F_IfStateIs7, // 166
  F_IfContentIs, // 167
  F_SetTurnModeToWatchTarget, // 168
  F_IfStateIsNot, // 169
  F_IfXIsEqualToY, // 170
  F_DisplayDebugMessage, // 171
  F_BlackTarget, // 172                       // Scripted AI functions (v0.80)
  F_DisplayMessageNear, // 173
  F_IfHitGround, // 174
  F_IfNameIsKnown, // 175
  F_IfUsageIsKnown, // 176
  F_IfHoldingItemID, // 177
  F_IfHoldingRangedWeapon, // 178
  F_IfHoldingMeleeWeapon, // 179
  F_IfHoldingShield, // 180
  F_IfKursed, // 181
  F_IfTargetIsKursed, // 182
  F_IfTargetIsDressedUp, // 183
  F_IfOverWater, // 184
  F_IfThrown, // 185
  F_MakeNameKnown, // 186
  F_MakeUsageKnown, // 187
  F_StopTargetMovement, // 188
  F_SetXY, // 189
  F_GetXY, // 190
  F_AddXY, // 191
  F_MakeAmmoKnown, // 192
  F_SpawnAttachedParticle, // 193
  F_SpawnExactParticle, // 194
  F_AccelerateTarget, // 195
  F_IfDistanceIsMoreThanTurn, // 196
  F_IfCrushed, // 197
  F_MakeCrushValid, // 198
  F_SetTargetToLowestTarget, // 199
  F_IfNotPutAway, // 200
  F_IfTakenOut, // 201
  F_IfAmmoOut, // 202
  F_PlaySoundLooped, // 203
  F_StopSoundLoop, // 204
  F_HealSelf, // 205
  F_Equip, // 206
  F_IfTargetHasItemIDEquipped, // 207
  F_SetOwnerToTarget, // 208
  F_SetTargetToOwner, // 209
  F_SetFrame, // 210
  F_BreakPassage, // 211
  F_SetReloadTime, // 212
  F_SetTargetToWideBlahID, // 213
  F_PoofTarget, // 214
  F_ChildDoActionOverride, // 215
  F_SpawnPoof, // 216
  F_SetSpeedPercent, // 217
  F_SetChildState, // 218
  F_SpawnAttachedSizedParticle, // 219
  F_ChangeArmor, // 220
  F_ShowTimer, // 221
  F_IfFacingTarget, // 222
  F_PlaySoundVolume, // 223
  F_SpawnAttachedFacedParticle, // 224
  F_IfStateIsOdd, // 225
  F_SetTargetToDistantEnemy, // 226
  F_Teleport, // 227
  F_GiveStrengthToTarget, // 228
  F_GiveWisdomToTarget, // 229
  F_GiveIntelligenceToTarget, // 230
  F_GiveDexterityToTarget, // 231
  F_GiveLifeToTarget, // 232
  F_GiveManaToTarget, // 233
  F_ShowMap, // 234
  F_ShowYouAreHere, // 235
  F_ShowBlipXY, // 236
  F_HealTarget, // 237
  F_PumpTarget, // 238
  F_CostAmmo, // 239
  F_MakeSimilarNamesKnown, // 240
  F_SpawnAttachedHolderParticle, // 241
  F_SetTargetReloadTime, // 242
  F_SetFogLevel, // 243
  F_GetFogLevel, // 244
  F_SetFogTAD, // 245
  F_SetFogBottomLevel, // 246
  F_GetFogBottomLevel, // 247
  F_CorrectActionForHand, // 248
  F_IfTargetIsMounted, // 249
  F_SparkleIcon, // 250
  F_UnsparkleIcon, // 251
  F_GetTileXY, // 252
  F_SetTileXY, // 253
  F_SetShadowSize, // 254
  F_SignalTarget, // 255
  F_SetTargetToWhoeverIsInPassage, // 256
  F_IfCharacterWasABook, // 257
  F_SetEnchantBoostValues, // 258              : Scripted AI functions (v0.90)
  F_SpawnCharacterXYZ, // 259
  F_SpawnExactCharacterXYZ, // 260
  F_ChangeTargetClass, // 261
  F_PlayFullSound, // 262
  F_SpawnExactChaseParticle, // 263
  F_EncodeOrder, // 264
  F_SignalSpecialID, // 265
  F_UnkurseTargetInventory, // 266
  F_IfTargetIsSneaking, // 267
  F_DropItems, // 268
  F_RespawnTarget, // 269
  F_TargetDoActionSetFrame, // 270
  F_IfTargetCanSeeInvisible, // 271
  F_SetTargetToNearestBlahID, // 272
  F_SetTargetToNearestEnemy, // 273
  F_SetTargetToNearestFriend, // 274
  F_SetTargetToNearestLifeform, // 275
  F_FlashPassage, // 276
  F_FindTileInPassage, // 277
  F_IfHeldInLeftSaddle, // 278
  F_NotAnItem, // 279
  F_SetChildAmmo, // 280
  F_IfHitVulnerable, // 281
  F_IfTargetIsFlying, // 282
  F_IdentifyTarget, // 283
  F_BeatModule, // 284
  F_EndModule, // 285
  F_DisableExport, // 286
  F_EnableExport, // 287
  F_GetTargetState, // 288
  F_SetSpeech, // 289
  F_SetMoveSpeech, // 290
  F_SetSecondMoveSpeech, // 291
  F_SetAttackSpeech, // 292
  F_SetAssistSpeech, // 293
  F_SetTerrainSpeech, // 294
  F_SetSelectSpeech, // 295
  F_ClearEndText, // 296
  F_AddEndText, // 297
  F_PlayMusic, // 298
  F_SetMusicPassage, // 299
  F_MakeCrushInvalid, // 300
  F_StopMusic, // 301
  F_FlashVariable, // 302
  F_AccelerateUp, // 303
  F_FlashVariableHeight, // 304
  F_SetDamageTime, // 305
  F_IfStateIs8, // 306
  F_IfStateIs9, // 307
  F_IfStateIs10, // 308
  F_IfStateIs11, // 309
  F_IfStateIs12, // 310
  F_IfStateIs13, // 311
  F_IfStateIs14, // 312
  F_IfStateIs15, // 313
  F_IfTargetIsAMount, // 314
  F_IfTargetIsAPlatform, // 315
  F_AddStat, // 316
  F_DisenchantTarget, // 317
  F_DisenchantAll, // 318
  F_SetVolumeNearestTeammate, // 319
  F_AddShopPassage, // 320
  F_TargetPayForArmor, // 321
  F_JoinEvilTeam, // 322
  F_JoinNullTeam, // 323
  F_JoinGoodTeam, // 324
  F_PitsKill, // 325
  F_SetTargetToPassageID, // 326
  F_MakeNameUnknown, // 327
  F_SpawnExactParticleEndSpawn, // 328
  F_SpawnPoofSpeedSpacingDamage, // 329
  F_GiveExperienceToGoodTeam,           // 330
  F_DoNothing,                          // 331 : Scripted AI functions (v0.95)
  F_DazeTarget,                         // 332
  F_GrogTarget,                         // 333
  F_IfEquipped,                         //
  F_DropTargetMoney,                    //
  F_GetTargetContent,                   //
  F_DropTargetKeys,                     //
  F_JoinTeam,                           //
  F_TargetJoinTeam,                     //
  F_ClearMusicPassage,                  //
  F_AddQuest,                           // Scripted AI functions (v1.00)
  F_BeatQuest,                          //
  F_IfTargetHasQuest,                   //
  F_SetQuestLevel, 					          	//
  F_IfTargetHasNotFullMana,		      		//
  F_IfDoingAction,						        	//
  F_IfOperatorIsLinux,				        	//
  F_IfTargetIsOwner,                    // Scripted AI functions (v1.05)
  F_SetCameraSwing,					          	//
  F_EnableRespawn,					          	//
  F_DisableRespawn,					          	//
  F_IfButtonPressed				          		//
} OPCODE;

typedef enum script_operation_e
{
  OP_ADD = 0,     // +
  OP_SUB,         // -
  OP_AND,         // &
  OP_SHR,         // >
  OP_SHL,         // <
  OP_MUL,         // *
  OP_DIV,         // /
  OP_MOD          // %
} OPERATION;

typedef enum script_variable_e
{
  VAR_TMP_X = 0,
  VAR_TMP_Y,
  VAR_TMP_DISTANCE,
  VAR_TMP_TURN,
  VAR_TMP_ARGUMENT,
  VAR_RAND,
  VAR_SELF_X,
  VAR_SELF_Y,
  VAR_SELF_TURN,
  VAR_SELF_COUNTER,
  VAR_SELF_ORDER,
  VAR_SELF_MORALE,
  VAR_SELF_LIFE,
  VAR_TARGET_X,
  VAR_TARGET_Y,
  VAR_TARGET_DISTANCE,
  VAR_TARGET_TURN,
  VAR_LEADER_X,
  VAR_LEADER_Y,
  VAR_LEADER_DISTANCE,
  VAR_LEADER_TURN,
  VAR_GOTO_X,
  VAR_GOTO_Y,
  VAR_GOTO_DISTANCE,
  VAR_TARGET_TURNTO,
  VAR_PASSAGE,
  VAR_WEIGHT,
  VAR_SELF_ALTITUDE,
  VAR_SELF_ID,
  VAR_SELF_HATEID,
  VAR_SELF_MANA,
  VAR_TARGET_STR,
  VAR_TARGET_WIS,
  VAR_TARGET_INT,
  VAR_TARGET_DEX,
  VAR_TARGET_LIFE,
  VAR_TARGET_MANA,
  VAR_TARGET_LEVEL,
  VAR_TARGET_SPEEDX,
  VAR_TARGET_SPEEDY,
  VAR_TARGET_SPEEDZ,
  VAR_SELF_SPAWNX,
  VAR_SELF_SPAWNY,
  VAR_SELF_STATE,
  VAR_SELF_STR,
  VAR_SELF_WIS,
  VAR_SELF_INT,
  VAR_SELF_DEX,
  VAR_SELF_MANAFLOW,
  VAR_TARGET_MANAFLOW,
  VAR_SELF_ATTACHED,
  VAR_SWINGTURN,
  VAR_XYDISTANCE,
  VAR_SELF_Z,
  VAR_TARGET_ALTITUDE,
  VAR_TARGET_Z,
  VAR_SELF_INDEX,
  VAR_OWNER_X,
  VAR_OWNER_Y,
  VAR_OWNER_TURN,
  VAR_OWNER_DISTANCE,
  VAR_OWNER_TURNTO,
  VAR_XYTURNTO,
  VAR_SELF_MONEY,
  VAR_SELF_ACCEL,
  VAR_TARGET_EXP,
  VAR_SELF_AMMO,
  VAR_TARGET_AMMO,
  VAR_TARGET_MONEY,
  VAR_TARGET_TURNAWAY,
  VAR_SELF_LEVEL,
  VAR_SPAWN_DISTANCE
} VARIABLE;

typedef struct script_global_values_t
{
  Uint16 oldtarget;
  Sint32 tmpx;
  Sint32 tmpy;
  Uint32 tmpturn;
  Sint32 tmpdistance;
  Sint32 tmpargument;
  Uint32 lastindent;
  Sint32 operationsum;
} SCRIPT_GLOBAL_VALUES;

extern SCRIPT_GLOBAL_VALUES scr_globals;