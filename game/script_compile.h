#pragma once

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

#include "egoboo_typedef.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// AI stuff
#define AISMAXLOADSIZE      (1024*1024)            ///< For parsing AI scripts
#define AISMAXCOMPILESIZE   (MAX_AI*4096/4)        ///< For parsing AI scripts
#define MAXLINESIZE         1024
#define MAX_OPCODE             1024                ///< Number of lines in AICODES.TXT
#define MAXCODENAMESIZE     64

#define FUNCTION_BIT 0x80000000
#define DATA_BITS    0x78000000
#define VALUE_BITS   0x07FFFFFF
#define END_VALUE    (FUNCTION_BIT | FEND)

#define GET_DATA_BITS(X) ( ( (X) >>   27 ) &  0x0F )
#define SET_DATA_BITS(X) ( ( (X) &  0x0F ) <<   27 )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The data describing where a script is in AisCompiled_buffer
struct s_script_storage_info
{
    STRING szName;
    Uint32 iStartPosition;
    Uint32 iEndPosition;
};
typedef struct s_script_storage_info script_storage_info_t;

DECLARE_STATIC_ARY_TYPE( AisStorageAry, script_storage_info_t, MAX_AI );
DECLARE_EXTERN_STATIC_ARY( AisStorageAry, AisStorage );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern int    AisCompiled_offset;
extern Uint32 AisCompiled_buffer[AISMAXCOMPILESIZE];

extern bool_t debug_scripts;
extern FILE * debug_script_file;

/// temporary data describing a single egoscript opcode
struct s_opcode_data
{
    Uint8  cType;
    Uint32 iValue;
    char   cName[MAXCODENAMESIZE];
};
typedef struct s_opcode_data opcode_data_t;

DECLARE_STATIC_ARY_TYPE( OpListAry, opcode_data_t, MAX_OPCODE );
DECLARE_EXTERN_STATIC_ARY( OpListAry, OpList );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A list of all possible egoscript functions
enum e_script_functions
{
    /// Scripted AI functions (v0.10)
    FIFSPAWNED = 0,                      // == 0
    FIFTIMEOUT,                          // == 1
    FIFATWAYPOINT,                       // == 2
    FIFATLASTWAYPOINT,                   // == 3
    FIFATTACKED,                         // == 4
    FIFBUMPED,                           // == 5
    FIFORDERED,                          // == 6
    FIFCALLEDFORHELP,                    // == 7
    FSETCONTENT,                         // == 8
    FIFKILLED,                           // == 9
    FIFTARGETKILLED,                     // == 10
    FCLEARWAYPOINTS,                     // == 11
    FADDWAYPOINT,                        // == 12
    FFINDPATH,                           // == 13
    FCOMPASS,                            // == 14
    FGETTARGETARMORPRICE,                // == 15
    FSETTIME,                            // == 16
    FGETCONTENT,                         // == 17
    FJOINTARGETTEAM,                     // == 18
    FSETTARGETTONEARBYENEMY,             // == 19
    FSETTARGETTOTARGETLEFTHAND,          // == 20
    FSETTARGETTOTARGETRIGHTHAND,         // == 21
    FSETTARGETTOWHOEVERATTACKED,         // == 22
    FSETTARGETTOWHOEVERBUMPED,           // == 23
    FSETTARGETTOWHOEVERCALLEDFORHELP,    // == 24
    FSETTARGETTOOLDTARGET,               // == 25
    FSETTURNMODETOVELOCITY,              // == 26
    FSETTURNMODETOWATCH,                 // == 27
    FSETTURNMODETOSPIN,                  // == 28
    FSETBUMPHEIGHT,                      // == 29
    FIFTARGETHASID,                      // == 30
    FIFTARGETHASITEMID,                  // == 31
    FIFTARGETHOLDINGITEMID,              // == 32
    FIFTARGETHASSKILLID,                 // == 33
    FELSE,                               // == 34
    FRUN,                                // == 35
    FWALK,                               // == 36
    FSNEAK,                              // == 37
    FDOACTION,                           // == 38
    FKEEPACTION,                         // == 39
    FISSUEORDER,                         // == 40
    FDROPWEAPONS,                        // == 41
    FTARGETDOACTION,                     // == 42
    FOPENPASSAGE,                        // == 43
    FCLOSEPASSAGE,                       // == 44
    FIFPASSAGEOPEN,                      // == 45
    FGOPOOF,                             // == 46
    FCOSTTARGETITEMID,                   // == 47
    FDOACTIONOVERRIDE,                   // == 48
    FIFHEALED,                           // == 49
    FSENDMESSAGE,                        // == 50
    FCALLFORHELP,                        // == 51
    FADDIDSZ,                            // == 52
    FEND,                                // == 53

    /// Scripted AI functions (v0.20)
    FSETSTATE,                           // == 54
    FGETSTATE,                           // == 55
    FIFSTATEIS,                          // == 56

    /// Scripted AI functions (v0.30)
    FIFTARGETCANOPENSTUFF,               // == 57
    FIFGRABBED,                          // == 58
    FIFDROPPED,                          // == 59
    FSETTARGETTOWHOEVERISHOLDING,        // == 60
    FDAMAGETARGET,                       // == 61
    FIFXISLESSTHANY,                     // == 62

    /// Scripted AI functions (v0.40)
    FSETWEATHERTIME,                     // == 63
    FGETBUMPHEIGHT,                      // == 64
    FIFREAFFIRMED,                       // == 65
    FUNKEEPACTION,                       // == 66
    FIFTARGETISONOTHERTEAM,              // == 67

    /// Scripted AI functions (v0.50)
    FIFTARGETISONHATEDTEAM,              // == 68
    FPRESSLATCHBUTTON,                   // == 69
    FSETTARGETTOTARGETOFLEADER,          // == 70
    FIFLEADERKILLED,                     // == 71
    FBECOMELEADER,                       // == 72

    /// Scripted AI functions (v0.60)
    FCHANGETARGETARMOR,                  // == 73
    FGIVEMONEYTOTARGET,                  // == 74
    FDROPKEYS,                           // == 75
    FIFLEADERISALIVE,                    // == 76
    FIFTARGETISOLDTARGET,                // == 77
    FSETTARGETTOLEADER,                  // == 78
    FSPAWNCHARACTER,                     // == 79
    FRESPAWNCHARACTER,                   // == 80
    FCHANGETILE,                         // == 81
    FIFUSED,                             // == 82
    FDROPMONEY,                          // == 83
    FSETOLDTARGET,                       // == 84
    FDETACHFROMHOLDER,                   // == 85
    FIFTARGETHASVULNERABILITYID,         // == 86
    FCLEANUP,                            // == 87
    FIFCLEANEDUP,                        // == 88
    FIFSITTING,                          // == 89
    FIFTARGETISHURT,                     // == 90
    FIFTARGETISAPLAYER,                  // == 91
    FPLAYSOUND,                          // == 92
    FSPAWNPARTICLE,                      // == 93
    FIFTARGETISALIVE,                    // == 94
    FSTOP,                               // == 95
    FDISAFFIRMCHARACTER,                 // == 96
    FREAFFIRMCHARACTER,                  // == 97
    FIFTARGETISSELF,                     // == 98
    FIFTARGETISMALE,                     // == 99
    FIFTARGETISFEMALE,                   // == 100

    // Scripted AI functions (v0.70)
    FSETTARGETTOSELF,                    // == 101
    FSETTARGETTORIDER,                   // == 102
    FGETATTACKTURN,                      // == 103
    FGETDAMAGETYPE,                      // == 104
    FBECOMESPELL,                        // == 105
    FBECOMESPELLBOOK,                    // == 106
    FIFSCOREDAHIT,                       // == 107
    FIFDISAFFIRMED,                      // == 108
    FTRANSLATEORDER,                     // == 109
    FSETTARGETTOWHOEVERWASHIT,           // == 110
    FSETTARGETTOWIDEENEMY,               // == 111
    FIFCHANGED,                          // == 112
    FIFINWATER,                          // == 113
    FIFBORED,                            // == 114
    FIFTOOMUCHBAGGAGE,                   // == 115
    FIFGROGGED,                          // == 116
    FIFDAZED,                            // == 117
    FIFTARGETHASSPECIALID,               // == 118
    FPRESSTARGETLATCHBUTTON,             // == 119
    FIFINVISIBLE,                        // == 120
    FIFARMORIS,                          // == 121
    FGETTARGETGROGTIME,                  // == 122
    FGETTARGETDAZETIME,                  // == 123
    FSETDAMAGETYPE,                      // == 124
    FSETWATERLEVEL,                      // == 125
    FENCHANTTARGET,                      // == 126
    FENCHANTCHILD,                       // == 127
    FTELEPORTTARGET,                     // == 128
    FGIVEEXPERIENCETOTARGET,             // == 129
    FINCREASEAMMO,                       // == 130
    FUNKURSETARGET,                      // == 131
    FGIVEEXPERIENCETOTARGETTEAM,         // == 132
    FIFUNARMED,                          // == 133
    FRESTOCKTARGETAMMOIDALL,             // == 134
    FRESTOCKTARGETAMMOIDFIRST,           // == 135
    FFLASHTARGET,                        // == 136
    FSETREDSHIFT,                        // == 137
    FSETGREENSHIFT,                      // == 138
    FSETBLUESHIFT,                       // == 139
    FSETLIGHT,                           // == 140
    FSETALPHA,                           // == 141
    FIFHITFROMBEHIND,                    // == 142
    FIFHITFROMFRONT,                     // == 143
    FIFHITFROMLEFT,                      // == 144
    FIFHITFROMRIGHT,                     // == 145
    FIFTARGETISONSAMETEAM,               // == 146
    FKILLTARGET,                         // == 147
    FUNDOENCHANT,                        // == 148
    FGETWATERLEVEL,                      // == 149
    FCOSTTARGETMANA,                     // == 150
    FIFTARGETHASANYID,                   // == 151
    FSETBUMPSIZE,                        // == 152
    FIFNOTDROPPED,                       // == 153
    FIFYISLESSTHANX,                     // == 154
    FSETFLYHEIGHT,                       // == 155
    FIFBLOCKED,                          // == 156
    FIFTARGETISDEFENDING,                // == 157
    FIFTARGETISATTACKING,                // == 158
    FIFSTATEIS0,                         // == 159
    FIFSTATEIS1,                         // == 160
    FIFSTATEIS2,                         // == 161
    FIFSTATEIS3,                         // == 162
    FIFSTATEIS4,                         // == 163
    FIFSTATEIS5,                         // == 164
    FIFSTATEIS6,                         // == 165
    FIFSTATEIS7,                         // == 166
    FIFCONTENTIS,                        // == 167
    FSETTURNMODETOWATCHTARGET,           // == 168
    FIFSTATEISNOT,                       // == 169
    FIFXISEQUALTOY,                      // == 170
    FDEBUGMESSAGE,                       // == 171

    /// Scripted AI functions (v0.80)
    FBLACKTARGET,                        // == 172
    FSENDMESSAGENEAR,                    // == 173
    FIFHITGROUND,                        // == 174
    FIFNAMEISKNOWN,                      // == 175
    FIFUSAGEISKNOWN,                     // == 176
    FIFHOLDINGITEMID,                    // == 177
    FIFHOLDINGRANGEDWEAPON,              // == 178
    FIFHOLDINGMELEEWEAPON,               // == 179
    FIFHOLDINGSHIELD,                    // == 180
    FIFKURSED,                           // == 181
    FIFTARGETISKURSED,                   // == 182
    FIFTARGETISDRESSEDUP,                // == 183
    FIFOVERWATER,                        // == 184
    FIFTHROWN,                           // == 185
    FMAKENAMEKNOWN,                      // == 186
    FMAKEUSAGEKNOWN,                     // == 187
    FSTOPTARGETMOVEMENT,                 // == 188
    FSETXY,                              // == 189
    FGETXY,                              // == 190
    FADDXY,                              // == 191
    FMAKEAMMOKNOWN,                      // == 192
    FSPAWNATTACHEDPARTICLE,              // == 193
    FSPAWNEXACTPARTICLE,                 // == 194
    FACCELERATETARGET,                   // == 195
    FIFDISTANCEISMORETHANTURN,           // == 196
    FIFCRUSHED,                          // == 197
    FMAKECRUSHVALID,                     // == 198
    FSETTARGETTOLOWESTTARGET,            // == 199
    FIFNOTPUTAWAY,                       // == 200
    FIFTAKENOUT,                         // == 201
    FIFAMMOOUT,                          // == 202
    FPLAYSOUNDLOOPED,                    // == 203
    FSTOPSOUND,                          // == 204
    FHEALSELF,                           // == 205
    FEQUIP,                              // == 206
    FIFTARGETHASITEMIDEQUIPPED,          // == 207
    FSETOWNERTOTARGET,                   // == 208
    FSETTARGETTOOWNER,                   // == 209
    FSETFRAME,                           // == 210
    FBREAKPASSAGE,                       // == 211
    FSETRELOADTIME,                      // == 212
    FSETTARGETTOWIDEBLAHID,              // == 213
    FPOOFTARGET,                         // == 214
    FCHILDDOACTIONOVERRIDE,              // == 215
    FSPAWNPOOF,                          // == 216
    FSETSPEEDPERCENT,                    // == 217
    FSETCHILDSTATE,                      // == 218
    FSPAWNATTACHEDSIZEDPARTICLE,         // == 219
    FCHANGEARMOR,                        // == 220
    FSHOWTIMER,                          // == 221
    FIFFACINGTARGET,                     // == 222
    FPLAYSOUNDVOLUME,                    // == 223
    FSPAWNATTACHEDFACEDPARTICLE,         // == 224
    FIFSTATEISODD,                       // == 225
    FSETTARGETTODISTANTENEMY,            // == 226
    FTELEPORT,                           // == 227
    FGIVESTRENGTHTOTARGET,               // == 228
    FGIVEWISDOMTOTARGET,                 // == 229
    FGIVEINTELLIGENCETOTARGET,           // == 230
    FGIVEDEXTERITYTOTARGET,              // == 231
    FGIVELIFETOTARGET,                   // == 232
    FGIVEMANATOTARGET,                   // == 233
    FSHOWMAP,                            // == 234
    FSHOWYOUAREHERE,                     // == 235
    FSHOWBLIPXY,                         // == 236
    FHEALTARGET,                         // == 237
    FPUMPTARGET,                         // == 238
    FCOSTAMMO,                           // == 239
    FMAKESIMILARNAMESKNOWN,              // == 240
    FSPAWNATTACHEDHOLDERPARTICLE,        // == 241
    FSETTARGETRELOADTIME,                // == 242
    FSETFOGLEVEL,                        // == 243
    FGETFOGLEVEL,                        // == 244
    FSETFOGTAD,                          // == 245
    FSETFOGBOTTOMLEVEL,                  // == 246
    FGETFOGBOTTOMLEVEL,                  // == 247
    FCORRECTACTIONFORHAND,               // == 248
    FIFTARGETISMOUNTED,                  // == 249
    FSPARKLEICON,                        // == 250
    FUNSPARKLEICON,                      // == 251
    FGETTILEXY,                          // == 252
    FSETTILEXY,                          // == 253
    FSETSHADOWSIZE,                      // == 254
    FORDERTARGET,                        // == 255
    FSETTARGETTOWHOEVERISINPASSAGE,      // == 256
    FIFCHARACTERWASABOOK,                // == 257

    /// Scripted AI functions (v0.90)
    FSETENCHANTBOOSTVALUES,              // == 258
    FSPAWNCHARACTERXYZ,                  // == 259
    FSPAWNEXACTCHARACTERXYZ,             // == 260
    FCHANGETARGETCLASS,                  // == 261
    FPLAYFULLSOUND,                      // == 262
    FSPAWNEXACTCHASEPARTICLE,            // == 263
    FCREATEORDER,                        // == 264
    FORDERSPECIALID,                     // == 265
    FUNKURSETARGETINVENTORY,             // == 266
    FIFTARGETISSNEAKING,                 // == 267
    FDROPITEMS,                          // == 268
    FRESPAWNTARGET,                      // == 269
    FTARGETDOACTIONSETFRAME,             // == 270
    FIFTARGETCANSEEINVISIBLE,            // == 271
    FSETTARGETTONEARESTBLAHID,           // == 272
    FSETTARGETTONEARESTENEMY,            // == 273
    FSETTARGETTONEARESTFRIEND,           // == 274
    FSETTARGETTONEARESTLIFEFORM,         // == 275
    FFLASHPASSAGE,                       // == 276
    FFINDTILEINPASSAGE,                  // == 277
    FIFHELDINLEFTHAND,                   // == 278
    FNOTANITEM,                          // == 279
    FSETCHILDAMMO,                       // == 280
    FIFHITVULNERABLE,                    // == 281
    FIFTARGETISFLYING,                   // == 282
    FIDENTIFYTARGET,                     // == 283
    FBEATMODULE,                         // == 284
    FENDMODULE,                          // == 285
    FDISABLEEXPORT,                      // == 286
    FENABLEEXPORT,                       // == 287
    FGETTARGETSTATE,                     // == 288

    /// Redone in v 0.95
    FIFEQUIPPED,                         // == 289
    FDROPTARGETMONEY,                    // == 290
    FGETTARGETCONTENT,                   // == 291
    FDROPTARGETKEYS,                     // == 292
    FJOINTEAM,                           // == 293
    FTARGETJOINTEAM,                     // == 294

    /// Below is original code again
    FCLEARMUSICPASSAGE,                  // == 295
    FCLEARENDMESSAGE,                    // == 296
    FADDENDMESSAGE,                      // == 297
    FPLAYMUSIC,                          // == 298
    FSETMUSICPASSAGE,                    // == 299
    FMAKECRUSHINVALID,                   // == 300
    FSTOPMUSIC,                          // == 301
    FFLASHVARIABLE,                      // == 302
    FACCELERATEUP,                       // == 303
    FFLASHVARIABLEHEIGHT,                // == 304
    FSETDAMAGETIME,                      // == 305
    FIFSTATEIS8,                         // == 306
    FIFSTATEIS9,                         // == 307
    FIFSTATEIS10,                        // == 308
    FIFSTATEIS11,                        // == 309
    FIFSTATEIS12,                        // == 310
    FIFSTATEIS13,                        // == 311
    FIFSTATEIS14,                        // == 312
    FIFSTATEIS15,                        // == 313
    FIFTARGETISAMOUNT,                   // == 314
    FIFTARGETISAPLATFORM,                // == 315
    FADDSTAT,                            // == 316
    FDISENCHANTTARGET,                   // == 317
    FDISENCHANTALL,                      // == 318
    FSETVOLUMENEARESTTEAMMATE,           // == 319
    FADDSHOPPASSAGE,                     // == 320
    FTARGETPAYFORARMOR,                  // == 321
    FJOINEVILTEAM,                       // == 322
    FJOINNULLTEAM,                       // == 323
    FJOINGOODTEAM,                       // == 324
    FPITSKILL,                           // == 325
    FSETTARGETTOPASSAGEID,               // == 326
    FMAKENAMEUNKNOWN,                    // == 327
    FSPAWNEXACTPARTICLEENDSPAWN,         // == 328
    FSPAWNPOOFSPEEDSPACINGDAMAGE,        // == 329
    FGIVEEXPERIENCETOGOODTEAM,           // == 330

    /// Scripted AI functions (v0.95)
    FDONOTHING,                          // == 331
    FGROGTARGET,                         // == 332
    FDAZETARGET,                         // == 333
    FENABLERESPAWN,                      // == 334
    FDISABLERESPAWN,                     // == 335

    /// Redone in v 1.10
    FDISPELTARGETENCHANTID,              // == 336
    FIFHOLDERBLOCKED,                    // == 337
    FGETSKILLLEVEL,                      // == 338
    FIFTARGETHASNOTFULLMANA,             // == 339
    FENABLELISTENSKILL,                  // == 340
    FSETTARGETTOLASTITEMUSED,            // == 341
    FFOLLOWLINK,                         // == 342  Scripted AI functions (v1.00)
    FIFOPERATORISLINUX,                  // == 343
    FIFTARGETISAWEAPON,                  // == 344
    FIFSOMEONEISSTEALING,                // == 345
    FIFTARGETISASPELL,                   // == 346
    FIFBACKSTABBED,                      // == 347
    FGETTARGETDAMAGETYPE,                // == 348
    FADDQUEST,                           // == 349
    FBEATQUESTALLPLAYERS,                // == 350
    FIFTARGETHASQUEST,                   // == 351
    FSETQUESTLEVEL,                      // == 352
    FADDQUESTALLPLAYERS,                 // == 353
    FADDBLIPALLENEMIES,                  // == 354
    FPITSFALL,                           // == 355
    FIFTARGETISOWNER,                    // == 356

    /// adding in the "speech" thing so the script can define its "ouch" sound, for instance
    FSETSPEECH,                  // == 357
    FSETMOVESPEECH,              // == 358
    FSETSECONDMOVESPEECH,        // == 359
    FSETATTACKSPEECH,            // == 360
    FSETASSISTSPEECH,            // == 361
    FSETTERRAINSPEECH,           // == 362
    FSETSELECTSPEECH,            // == 363

    /// Scripted AI functions (v1.10)
    FTAKEPICTURE,                       // == 364
    FIFOPERATORISMACINTOSH,             // == 365
    FIFMODULEHASIDSZ,                   // == 366
    FMORPHTOTARGET,                     // == 367
    FGIVEMANAFLOWTOTARGET,              // == 368
    FGIVEMANARETURNTOTARGET,            // == 369
    FSETMONEY,                          // == 370
    FIFTARGETCANSEEKURSES,              // == 371
    FSPAWNATTACHEDCHARACTER,            // == 372
    FKURSETARGET,                       // == 373
    FSETCHILDCONTENT,                   // == 374
    FSETTARGETTOCHILD,                  // == 375
    FSETDAMAGETHRESHOLD,                // == 376
    FACCELERATETARGETUP,                // == 377
    FSETTARGETAMMO,                     // == 378
    FENABLEINVICTUS,                    // == 379
    FDISABLEINVICTUS,                   // == 380
    FTARGETDAMAGESELF,                  // == 381
    FSETTARGETSIZE,                     // == 382
    FIFTARGETISFACINGSELF,              // == 383
    FDRAWBILLBOARD,                     // == 384
    FSETTARGETTOFIRSTBLAHINPASSAGE,     // == 385

    FIFLEVELUP,                         // == 386
    FGIVESKILLTOTARGET,                 // == 387

    SCRIPT_FUNCTIONS_COUNT
};

extern const char * script_function_names[SCRIPT_FUNCTIONS_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A list of all possible egoscript operators
enum e_script_operators
{
    OPADD = 0,            ///< +
    OPSUB,                ///< -
    OPAND,                ///< &
    OPSHR,                ///< >
    OPSHL,                ///< <
    OPMUL,                ///< *
    OPDIV,                ///< /
    OPMOD                 ///< %
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A list of all possible egoscript pre-defined variables
enum e_script_variables
{
    VARTMPX = 0,         // == 0
    VARTMPY,             // == 1
    VARTMPDISTANCE,      // == 2
    VARTMPTURN,          // == 3
    VARTMPARGUMENT,      // == 4
    VARRAND,             // == 5
    VARSELFX,            // == 6
    VARSELFY,            // == 7
    VARSELFTURN,         // == 8
    VARSELFCOUNTER,      // == 9
    VARSELFORDER,        // == 10
    VARSELFMORALE,       // == 11
    VARSELFLIFE,         // == 12
    VARTARGETX,          // == 13
    VARTARGETY,          // == 14
    VARTARGETDISTANCE,   // == 15
    VARTARGETTURN,       // == 16
    VARLEADERX,          // == 17
    VARLEADERY,          // == 18
    VARLEADERDISTANCE,   // == 19
    VARLEADERTURN,       // == 20
    VARGOTOX,            // == 21
    VARGOTOY,            // == 22
    VARGOTODISTANCE,     // == 23
    VARTARGETTURNTO,     // == 24
    VARPASSAGE,          // == 25
    VARWEIGHT,           // == 26
    VARSELFALTITUDE,     // == 27
    VARSELFID,           // == 28
    VARSELFHATEID,       // == 29
    VARSELFMANA,         // == 30
    VARTARGETSTR,        // == 31
    VARTARGETWIS,        // == 32
    VARTARGETINT,        // == 33
    VARTARGETDEX,        // == 34
    VARTARGETLIFE,       // == 35
    VARTARGETMANA,       // == 36
    VARTARGETLEVEL,      // == 37
    VARTARGETSPEEDX,     // == 38
    VARTARGETSPEEDY,     // == 39
    VARTARGETSPEEDZ,     // == 40
    VARSELFSPAWNX,       // == 41
    VARSELFSPAWNY,       // == 42
    VARSELFSTATE,        // == 43
    VARSELFSTR,          // == 44
    VARSELFWIS,          // == 45
    VARSELFINT,          // == 46
    VARSELFDEX,          // == 47
    VARSELFMANAFLOW,     // == 48
    VARTARGETMANAFLOW,   // == 49
    VARSELFATTACHED,     // == 50
    VARSWINGTURN,        // == 51
    VARXYDISTANCE,       // == 52
    VARSELFZ,            // == 53
    VARTARGETALTITUDE,   // == 54
    VARTARGETZ,          // == 55
    VARSELFINDEX,        // == 56
    VAROWNERX,           // == 57
    VAROWNERY,           // == 58
    VAROWNERTURN,        // == 59
    VAROWNERDISTANCE,    // == 60
    VAROWNERTURNTO,      // == 61
    VARXYTURNTO,         // == 62
    VARSELFMONEY,        // == 63
    VARSELFACCEL,        // == 64
    VARTARGETEXP,        // == 65
    VARSELFAMMO,         // == 66
    VARTARGETAMMO,       // == 67
    VARTARGETMONEY,      // == 68
    VARTARGETTURNAWAY,   // == 69
    VARSELFLEVEL,        // == 70
    VARTARGETRELOADTIME, // == 71
    VARSELFCONTENT,      // == 72
    VARSPAWNDISTANCE,    // == 73
    VARTARGETMAXLIFE,    // == 74
    VARTARGETTEAM,       // == 75
    VARTARGETARMOR,      // == 76
    VARDIFFICULTY,       // == 77
    VARTIMEHOURS,        // == 78
    VARTIMEMINUTES,      // == 79
    VARTIMESECONDS,      // == 80
    VARDATEMONTH,        // == 81
    VARDATEDAY           // == 82
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern bool_t parseerror;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// function prototypes

int  load_ai_script_vfs( const char *loadname );

void init_all_ai_scripts();
void release_all_ai_scripts();

void script_compiler_init();
