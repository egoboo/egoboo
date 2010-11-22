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

/// @file script_compile.c
/// @brief Implementation of the script compiler
/// @details

#include "script_compile.h"

#include "log.h"

#include "egoboo_vfs.h"
#include "egoboo_setup.h"
#include "egoboo_strutil.h"
#include "egoboo_fileutil.h"
#include "egoboo_math.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------

/// The description of a single pre-defined egoscript token
struct s_token
{
    int    iLine;
    int    iIndex;
    int    iValue;
    char   cType;
    char   cWord[MAXCODENAMESIZE];
};
typedef struct s_token token_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static int     iNumLine;

static int     iLineSize;
static char    cLineBuffer[MAXLINESIZE] = EMPTY_CSTR;

static int     iLoadSize;
static Uint8   cLoadBuffer[AISMAXLOADSIZE] = EMPTY_CSTR;

static token_t Token;

static const char * globalparsename = NULL;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INSTANTIATE_STATIC_ARY( OpListAry, OpList );
INSTANTIATE_STATIC_ARY( AisStorageAry, AisStorage );

int    AisCompiled_offset = 0;
Uint32 AisCompiled_buffer[AISMAXCOMPILESIZE];

bool_t debug_scripts     = bfalse;
FILE * debug_script_file = NULL;

bool_t parseerror  = bfalse;

const char * script_function_names[SCRIPT_FUNCTIONS_COUNT] =
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
    "FDRAWBILLBOARD",              // == 384
    "FSETTARGETTOBLAHINPASSAGE"    // == 385
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void   insert_space( int position );
static int    load_one_line( int read );
static void   surround_space( int position );
static int    get_indentation();
static void   fix_operators();
static int    parse_token( int read );
static void   emit_opcode( BIT_FIELD highbits );
static void   parse_line_by_line();
static Uint32 jump_goto( int index, int index_end );
static void   parse_jumps( int ainumber );
static int    ai_goto_colon( int read );
static void   get_code( int read );

static void load_ai_codes_vfs( const char* loadname );

// functions for debugging the scripts
#if (DEBUG_SCRIPT_LEVEL > 2) && defined(_DEBUG)
static void print_token();
static void print_line();
#else
#   define print_token()
#   define print_line()
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void script_compiler_init()
{
    /// @details BB@> initalize the sctipt compiling module

    // necessary for loading up the ai script
    init_all_ai_scripts();

    load_ai_codes_vfs( "mp_data/aicodes.txt" );

    debug_script_file = fopen( vfs_resolveWriteFilename( "/debug/script_debug.txt" ), "wt" );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void insert_space( int position )
{
    /// @details ZZ@> This function adds a space into the load line if there isn't one
    ///    there already

    char cTmp, cSwap;
    if ( !isspace( cLineBuffer[position] ) )
    {
        cTmp = cLineBuffer[position];
        cLineBuffer[position] = ' ';
        position++;
        iLineSize++;

        while ( position < iLineSize )
        {
            cSwap = cLineBuffer[position];
            cLineBuffer[position] = cTmp;
            cTmp = cSwap;
            position++;
        }

        cLineBuffer[position] = CSTR_END; // or cTmp as cTmp == CSTR_END
    }
}

//--------------------------------------------------------------------------------------------
int load_one_line( int read )
{
    /// @details ZZ@> This function loads a line into the line buffer

    int stillgoing, foundtext;
    char cTmp;
    bool_t tabs_warning_needed;

    // Parse to start to maintain indentation
    cLineBuffer[0] = CSTR_END;
    iLineSize = 0;
    stillgoing = btrue;

    // try to trap all end of line conditions so we can properly count the lines
    tabs_warning_needed = bfalse;
    while ( read < iLoadSize )
    {
        cTmp = cLoadBuffer[read];

        if ( ASCII_LINEFEED_CHAR == cTmp && C_CARRIAGE_RETURN_CHAR == cLoadBuffer[read+1] )
        {
            iLineSize = 0;
            cLineBuffer[0] = CSTR_END;
            return read + 2;
        }

        if ( C_CARRIAGE_RETURN_CHAR == cTmp && ASCII_LINEFEED_CHAR == cLoadBuffer[read+1] )
        {
            iLineSize = 0;
            cLineBuffer[0] = CSTR_END;
            return read + 2;
        }

        if ( ASCII_LINEFEED_CHAR == cTmp || C_CARRIAGE_RETURN_CHAR == cTmp )
        {
            iLineSize = 0;
            cLineBuffer[0] = CSTR_END;
            return read + 1;
        }

        if ( C_TAB_CHAR == cTmp )
        {
            tabs_warning_needed = btrue;
            cTmp = ' ';
        }

        if ( !isspace( cTmp ) )
        {
            break;
        }

        cLineBuffer[iLineSize] = ' ';
        cLineBuffer[iLineSize+1] = CSTR_END;

        read++;
        iLineSize++;
    }

    // Parse to comment or end of line
    foundtext = bfalse;
    while ( read < iLoadSize )
    {
        cTmp = cLoadBuffer[read];
        if ( C_CARRIAGE_RETURN_CHAR == cTmp || ASCII_LINEFEED_CHAR == cTmp )
        {
            break;
        }

        if ( '/' == cTmp && '/' == cLoadBuffer[read + 1] )
        {
            break;
        }

        read++;

        if ( iscntrl( cTmp ) )
        {
            cTmp = ' ';
        }

        if ( !isspace( cTmp ) )
        {
            foundtext = btrue;

            cLineBuffer[iLineSize]   = cTmp;
            cLineBuffer[iLineSize+1] = CSTR_END;
            iLineSize++;
        }
    }
    if ( !foundtext )
    {
        iLineSize = 0;
    }

    // terminate the line buffer properly
    cLineBuffer[iLineSize] = CSTR_END;

    if ( iLineSize > 0  && tabs_warning_needed )
    {
        log_warning( "Tab character used to define spacing will cause an error \"%s\"(%d) - \n    \"%s\"\n", globalparsename, Token.iLine, cLineBuffer );
    }

    // Parse to end of line
    while ( read < iLoadSize )
    {
        if ( ASCII_LINEFEED_CHAR == cLoadBuffer[read] && C_CARRIAGE_RETURN_CHAR == cLoadBuffer[read+1] )
        {
            read += 2;
            break;
        }
        else if ( C_CARRIAGE_RETURN_CHAR == cLoadBuffer[read] && ASCII_LINEFEED_CHAR == cLoadBuffer[read+1] )
        {
            read += 2;
            break;
        }
        else if ( CSTR_END == cLoadBuffer[read] || ASCII_LINEFEED_CHAR == cLoadBuffer[read] || C_CARRIAGE_RETURN_CHAR == cLoadBuffer[read] )
        {
            read += 1;
            break;
        }

        read++;
    }

    return read;
}

//--------------------------------------------------------------------------------------------
void surround_space( int position )
{
    insert_space( position + 1 );
    if ( position > 0 )
    {
        if ( !isspace( cLineBuffer[position-1] ) )
        {
            insert_space( position );
        }
    }
}

//--------------------------------------------------------------------------------------------
int get_indentation()
{
    /// @details ZZ@> This function returns the number of starting spaces in a line

    int cnt;
    char cTmp;

    cnt = 0;
    cTmp = cLineBuffer[cnt];
    while ( isspace( cTmp ) )
    {
        cnt++;
        cTmp = cLineBuffer[cnt];
    }
    if ( HAS_SOME_BITS( cnt, 1 ) )
    {
        log_warning( "Invalid indentation \"%s\"(%d) - \"%s\"\n", globalparsename, Token.iLine, cLineBuffer );
        parseerror = btrue;
    }

    cnt >>= 1;
    if ( cnt > 15 )
    {
        log_warning( "Too many levels of indentation \"%s\"(%d) - \"%s\"\n", globalparsename, Token.iLine, cLineBuffer );
        parseerror = btrue;
        cnt = 15;
    }

    return cnt;
}

//--------------------------------------------------------------------------------------------
void fix_operators()
{
    /// @details ZZ@> This function puts spaces around operators to seperate words
    ///    better

    int cnt;
    char cTmp;

    cnt = 0;

    while ( cnt < iLineSize )
    {
        cTmp = cLineBuffer[cnt];
        if ( '+' == cTmp || '-' == cTmp || '/' == cTmp || '*' == cTmp ||
             '%' == cTmp || '>' == cTmp || '<' == cTmp || '&' == cTmp ||
             '=' == cTmp )
        {
            surround_space( cnt );
            cnt++;
        }

        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
int parse_token( int read )
{
    /// @details ZZ@> This function tells what code is being indexed by read, it
    ///    will return the next spot to read from and stick the code number
    ///    in Token.iIndex

    int cnt, wordsize;
    char cTmp;
    IDSZ idsz;

    // Reset the token
    Token.iIndex   = MAX_OPCODE;
    Token.iValue   = 0;
    Token.cType    = '?';
    Token.cWord[0] = CSTR_END;

    // Check bounds
    if ( read >= iLineSize )
        return iLineSize;

    // Skip spaces
    cTmp = cLineBuffer[read];
    while ( isspace( cTmp ) && read < iLineSize )
    {
        read++;
        cTmp = cLineBuffer[read];
    }
    if ( read >= iLineSize )  { print_token(); return read; }

    // Load the word into the other buffer
    wordsize = 0;
    while ( !isspace( cTmp ) && CSTR_END != cTmp )
    {
        Token.cWord[wordsize] = cTmp;  wordsize++;
        read++;
        cTmp = cLineBuffer[read];
    }
    Token.cWord[wordsize] = CSTR_END;

    // Check for numeric constant
    if ( 0 != isdigit( Token.cWord[0] ) )
    {
        sscanf( Token.cWord, "%d", &Token.iValue );
        Token.cType  = 'C';
        Token.iIndex = MAX_OPCODE;
        { print_token();  return read; }
    }

    // Check for IDSZ constant
    if ( '[' == Token.cWord[0] )
    {
        idsz = MAKE_IDSZ( Token.cWord[1], Token.cWord[2], Token.cWord[3], Token.cWord[4] );

        Token.iValue = idsz;
        Token.cType  = 'C';
        Token.iIndex = MAX_OPCODE;

        { print_token();  return read; }
    }

    for ( cnt = 0; cnt < OpList.count; cnt++ )
    {
        if ( 0 == strncmp( Token.cWord, OpList.ary[cnt].cName, MAXCODENAMESIZE ) )
        {
            break;
        }
    }
    if ( cnt < OpList.count )
    {
        Token.iValue = OpList.ary[cnt].iValue;
        Token.cType  = OpList.ary[cnt].cType;
        Token.iIndex = cnt;
    }
    else if ( 0 == strcmp( Token.cWord, "=" ) )
    {
        Token.iValue = -1;
        Token.cType  = 'O';
        Token.iIndex = MAX_OPCODE;
    }
    else
    {
        // Throw out an error code if we're loggin' 'em
        log_message( "SCRIPT ERROR: \"%s\"(%d) - unknown opcode \"%s\"\n", globalparsename, Token.iLine, Token.cWord );

        Token.iValue = -1;
        Token.cType  = '?';
        Token.iIndex = MAX_OPCODE;

        parseerror = btrue;
    }

    { print_token();  return read; }
}

//--------------------------------------------------------------------------------------------
void emit_opcode( BIT_FIELD highbits )
{
    // detect a constant value
    if ( 'C' == Token.cType || 'F' == Token.cType )
    {
        SET_BIT( highbits, FUNCTION_BIT );
    }
    if ( AisCompiled_offset < AISMAXCOMPILESIZE )
    {
        AisCompiled_buffer[AisCompiled_offset] = highbits | Token.iValue;
        AisCompiled_offset++;
    }
    else
    {
        log_warning( "Script index larger than array\n" );
    }
}

//--------------------------------------------------------------------------------------------
void parse_line_by_line()
{
    /// @details ZZ@> This function removes comments and endline codes, replacing
    ///    them with a 0

    int read;
    Uint32 highbits;
    int parseposition;

    read = 0;
    for ( Token.iLine = 0; read < iLoadSize; Token.iLine++ )
    {
        read = load_one_line( read );
        if ( 0 == iLineSize ) continue;

#if (DEBUG_SCRIPT_LEVEL > 2) && defined(_DEBUG)
        print_line();
#endif

        fix_operators();
        parseposition = 0;

        //------------------------------
        // grab the first opcode

        highbits = SET_DATA_BITS( get_indentation() );
        parseposition = parse_token( parseposition );
        if ( 'F' == Token.cType )
        {
            if ( FEND == Token.iValue && 0 == highbits )
            {
                // stop processing the lines, since we're finished
                break;
            }

            //------------------------------
            // the code type is a function

            // save the opcode
            emit_opcode( highbits );

            // leave a space for the control code
            Token.iValue = 0;
            emit_opcode( 0 );

        }
        else if ( 'V' == Token.cType )
        {
            //------------------------------
            // the code type is a math operation

            int operand_index;
            int operands = 0;

            // save in the value's opcode
            emit_opcode( highbits );

            // save a position for the operand count
            Token.iValue = 0;
            operand_index = AisCompiled_offset;
            emit_opcode( 0 );

            // handle the "="
            highbits = 0;
            parseposition = parse_token( parseposition );  // EQUALS
            if ( 'O' != Token.cType || 0 != strcmp( Token.cWord, "=" ) )
            {
                log_warning( "Invalid equation \"%s\"(%d) - \"%s\"\n", globalparsename, Token.iLine, cLineBuffer );
            }

            //------------------------------
            // grab the next opcode

            parseposition = parse_token( parseposition );
            if ( 'V' == Token.cType || 'C' == Token.cType )
            {
                // this is a value or a constant
                emit_opcode( 0 );
                operands++;

                parseposition = parse_token( parseposition );
            }
            else if ( 'O' != Token.cType )
            {
                // this is a function or an unknown value. do not break the script.
                log_warning( "Invalid operand \"%s\"(%d) - \"%s\"\n", globalparsename, Token.iLine, Token.cWord );

                emit_opcode( 0 );
                operands++;

                parseposition = parse_token( parseposition );
            }

            // expects a OPERATOR VALUE OPERATOR VALUE OPERATOR VALUE pattern
            while ( parseposition < iLineSize )
            {
                // the current token should be an operator
                if ( 'O' != Token.cType )
                {
                    // problem with the loop
                    log_warning( "Expected an operator \"%s\"(%d) - \"%s\"\n", globalparsename, Token.iLine, cLineBuffer );
                    break;
                }

                // the highbits are the operator's value
                highbits = SET_DATA_BITS( Token.iValue );

                // VALUE
                parseposition = parse_token( parseposition );
                if ( 'C' != Token.cType && 'V' != Token.cType )
                {
                    // not having a constant or a value here breaks the function. stop processing
                    log_warning( "Invalid operand \"%s\"(%d) - \"%s\"\n", globalparsename, Token.iLine, Token.cWord );
                    break;
                }

                emit_opcode( highbits );
                operands++;

                // OPERATOR
                parseposition = parse_token( parseposition );
            }

            AisCompiled_buffer[operand_index] = operands;  // Number of operands
        }
        else if ( 'C' == Token.cType )
        {
            log_warning( "Invalid constant \"%s\"(%d) - \"%s\"\n", globalparsename, Token.iLine, Token.cWord );
        }
        else if ( '?' == Token.cType )
        {
            // unknown opcode, do not process this line
            log_warning( "Invalid operand \"%s\"(%d) - \"%s\"\n", globalparsename, Token.iLine, Token.cWord );
        }
        else
        {
            log_warning( "Compiler is broken \"%s\"(%d) - \"%s\"\n", globalparsename, Token.iLine, Token.cWord );
            break;
        }
    }

    Token.iValue = FEND;
    Token.cType  = 'F';
    emit_opcode( 0 );
    Token.iValue = AisCompiled_offset + 1;
    emit_opcode( 0 );
}

//--------------------------------------------------------------------------------------------
Uint32 jump_goto( int index, int index_end )
{
    /// @details ZZ@> This function figures out where to jump to on a fail based on the
    ///    starting location and the following code.  The starting location
    ///    should always be a function code with indentation

    Uint32 value;
    int targetindent, indent;

    value = AisCompiled_buffer[index];  index += 2;
    targetindent = GET_DATA_BITS( value );
    indent = 100;

    while ( indent > targetindent && index < index_end )
    {
        value = AisCompiled_buffer[index];
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
                value = AisCompiled_buffer[index];
                index++;
                index += ( value & 255 );
            }
        }
    }

    return MIN( index, index_end );
}

//--------------------------------------------------------------------------------------------
void parse_jumps( int ainumber )
{
    /// @details ZZ@> This function sets up the fail jumps for the down and dirty code

    int index, index_end;
    Uint32 value, iTmp;

    index     = AisStorage.ary[ainumber].iStartPosition;
    index_end = AisStorage.ary[ainumber].iEndPosition;

    value = AisCompiled_buffer[index];
    while ( index < index_end )
    {
        value = AisCompiled_buffer[index];

        // Was it a function
        if ( HAS_SOME_BITS( value, FUNCTION_BIT ) )
        {
            // Each function needs a jump
            iTmp = jump_goto( index, index_end );
            index++;
            AisCompiled_buffer[index] = iTmp;
            index++;
        }
        else
        {
            // Operations cover each operand
            index++;
            iTmp = AisCompiled_buffer[index];
            index++;
            index += CLIP_TO_08BITS( iTmp );
        }
    }
}

//--------------------------------------------------------------------------------------------
int ai_goto_colon( int read )
{
    /// @details ZZ@> This function goes to spot after the next colon

    char cTmp;

    cTmp = cLoadBuffer[read];

    while ( ':' != cTmp && read < iLoadSize )
    {
        read++;  cTmp = cLoadBuffer[read];
    }
    if ( read < iLoadSize )  read++;

    return read;
}

//--------------------------------------------------------------------------------------------
void get_code( int read )
{
    /// @details ZZ@> This function gets code names and other goodies

    char cTmp;
    int iTmp;
    STRING sTmp;

    sscanf(( char* )( cLoadBuffer + read ), "%c%d%255s", &cTmp, &iTmp, sTmp );

    strncpy( OpList.ary[OpList.count].cName, sTmp, SDL_arraysize( OpList.ary[OpList.count].cName ) );
    OpList.ary[OpList.count].cType  = toupper( cTmp );
    OpList.ary[OpList.count].iValue = iTmp;
}

//--------------------------------------------------------------------------------------------
void load_ai_codes_vfs( const char* loadname )
{
    /// @details ZZ@> This function loads all of the function and variable names

    vfs_FILE* fileread;
    int read;

    OpList.count = 0;
    fileread = vfs_openRead( loadname );
    if ( fileread )
    {
        iLoadSize = ( int )vfs_read( cLoadBuffer, 1, AISMAXLOADSIZE, fileread );
        read = 0;
        read = ai_goto_colon( read );

        while ( read != iLoadSize )
        {
            get_code( read );
            OpList.count++;
            read = ai_goto_colon( read );
        }

        vfs_close( fileread );
    }
}

//--------------------------------------------------------------------------------------------
int load_ai_script_vfs( const char *loadname )
{
    /// @details ZZ@> This function loads a script to memory and
    ///    returns the index of the script, returns -1 if it fails

    vfs_FILE* fileread;
    int retval = -1;

    iNumLine = 0;
    fileread = vfs_openRead( loadname );

    // No such file
    if ( NULL == fileread )
    {
        log_debug( "I am missing a AI script (%s)\n", loadname );
        log_message( "       Using the default AI script instead (\"mp_data/script.txt\")\n" );
        return 0;           //0 == default AI script
    }
    if ( AisStorage.count >= MAX_AI )
    {
        log_warning( "Too many script files. Cannot load file \"%s\"\n", loadname );
        return retval;
    }

    // load the file
    iLoadSize = ( int )vfs_read( cLoadBuffer, 1, AISMAXLOADSIZE, fileread );
    vfs_close( fileread );

    // if the file is empty, use the default script
    if ( 0 == iLoadSize )
    {
        log_warning( "Script file is empty. \"%s\"\n", loadname );
        return retval;
    }

    // save the filename for error logging
    strncpy( AisStorage.ary[AisStorage.count].szName, loadname, sizeof( STRING ) );
    globalparsename = loadname;

    // initialize the start and end position
    AisStorage.ary[AisStorage.count].iStartPosition = AisCompiled_offset;
    AisStorage.ary[AisStorage.count].iEndPosition   = AisCompiled_offset;

    // parse/compile the scripts
    // parse_null_terminate_comments();
    parse_line_by_line();

    // set the end position of the script
    AisStorage.ary[AisStorage.count].iEndPosition = AisCompiled_offset;

    // determine the correct jumps
    parse_jumps( AisStorage.count );

    // get the ai script index
    retval = AisStorage.count;

    // increase the ai script index
    AisStorage.count++;

    return retval;
}

//--------------------------------------------------------------------------------------------
void release_all_ai_scripts()
{
    /// @details ZZ@> This function resets the ai script "pointers"

    AisCompiled_offset = 0;
    AisStorage.count = 0;
}

//--------------------------------------------------------------------------------------------
void init_all_ai_scripts()
{
    /// @details ZZ@> This function initializes the ai script "pointers"

    AisCompiled_offset = 0;
    AisStorage.count = 0;
}

//--------------------------------------------------------------------------------------------
#if (DEBUG_SCRIPT_LEVEL > 2) && defined(_DEBUG)
void print_token()
{
    printf( "------------\n", globalparsename, Token.iLine );
    printf( "\tToken.iIndex == %d\n", Token.iIndex );
    printf( "\tToken.iValue == %d\n", Token.iValue );
    printf( "\tToken.cType  == \'%c\'\n", Token.cType );
    printf( "\tToken.cWord  == \"%s\"\n", Token.cWord );
}
#endif

//--------------------------------------------------------------------------------------------
#if (DEBUG_SCRIPT_LEVEL > 2) && defined(_DEBUG)
void print_line()
{
    int i;
    char cTmp;

    printf( "\n===========\n\tfile == \"%s\"\n\tline == %d\n", globalparsename, Token.iLine );

    printf( "\tline == \"" );

    for ( i = 0; i < iLineSize; i++ )
    {
        cTmp = cLineBuffer[i];
        if ( isprint( cTmp ) )
        {
            printf( "%c", cTmp );
        }
        else
        {
            printf( "\\%03d", cTmp );
        }
    };

    printf( "\", length == %d\n", iLineSize );
}
#endif

/** Preparation for eliminating aicodes.txt except for introducing aliases
DEFINE_FUNCTION( FIFSPAWNED,                       "IfSpawned"    )                          // == 0
DEFINE_FUNCTION( FIFTIMEOUT,                       "IfTimeOut"    )                              // == 1
DEFINE_FUNCTION( FIFATWAYPOINT,                    "IfAtWaypoint"    )                           // == 2
DEFINE_FUNCTION( FIFATLASTWAYPOINT,                "IfAtLastWaypoint"    )                       // == 3
DEFINE_FUNCTION( FIFATTACKED,                      "IfAttacked"    )                             // == 4
DEFINE_FUNCTION( FIFBUMPED,                        "IfBumped"    )                               // == 5
DEFINE_FUNCTION( FIFORDERED,                       "IfOrdered"    )                              // == 6
DEFINE_FUNCTION( FIFCALLEDFORHELP,                 "IfCalledForHelp"    )                        // == 7
DEFINE_FUNCTION( FSETCONTENT,                      "SetContent"    )                             // == 8
DEFINE_FUNCTION( FIFKILLED,                        "IfKilled"    )                               // == 9
DEFINE_FUNCTION( FIFTARGETKILLED,                  "IfTargetKilled"    )                         // == 10
DEFINE_FUNCTION( FCLEARWAYPOINTS,                  "ClearWaypoints"    )                         // == 11
DEFINE_FUNCTION( FADDWAYPOINT,                     "AddWaypoint"    )                            // == 12
DEFINE_FUNCTION( FFINDPATH,                        "FindPath"    )                               // == 13
DEFINE_FUNCTION( FCOMPASS,                         "Compass"    )                                // == 14
DEFINE_FUNCTION( FGETTARGETARMORPRICE,             "GetTargetArmorPrice"    )                    // == 15
DEFINE_FUNCTION( FSETTIME,                         "SetTime"    )                                // == 16
DEFINE_FUNCTION( FGETCONTENT,                      "GetContent"    )                             // == 17
DEFINE_FUNCTION( FJOINTARGETTEAM,                  "JoinTargetTeam"    )                         // == 18
DEFINE_FUNCTION( FSETTARGETTONEARBYENEMY,          "SetTargetToNearbyEnemy"    )                 // == 19
DEFINE_FUNCTION( FSETTARGETTOTARGETLEFTHAND,       "SetTargetToTargetLeftHand"    )              // == 20
DEFINE_FUNCTION( FSETTARGETTOTARGETRIGHTHAND,      "SetTargetToTargetRightHand"    )             // == 21
DEFINE_FUNCTION( FSETTARGETTOWHOEVERATTACKED,      "SetTargetToWhoeverAttacked"    )             // == 22
DEFINE_FUNCTION( FSETTARGETTOWHOEVERBUMPED,        "SetTargetToWhoeverBumped"    )               // == 23
DEFINE_FUNCTION( FSETTARGETTOWHOEVERCALLEDFORHELP, "SetTargetToWhoeverCalledForHelp"    )        // == 24
DEFINE_FUNCTION( FSETTARGETTOOLDTARGET,            "SetTargetToOldTarget"    )                   // == 25
DEFINE_FUNCTION( FSETTURNMODETOVELOCITY,           "SetTurnModeToVelocity"    )                  // == 26
DEFINE_FUNCTION( FSETTURNMODETOWATCH,              "SetTurnModeToWatch"    )                     // == 27
DEFINE_FUNCTION( FSETTURNMODETOSPIN,               "SetTurnModeToSpin"    )                      // == 28
DEFINE_FUNCTION( FSETBUMPHEIGHT,                   "SetBumpHeight"    )                          // == 29
DEFINE_FUNCTION( FIFTARGETHASID,                   "IfTargetHasID"    )                          // == 30
DEFINE_FUNCTION( FIFTARGETHASITEMID,               "IfTargetHasItemID"    )                      // == 31
DEFINE_FUNCTION( FIFTARGETHOLDINGITEMID,           "IfTargetHoldingItemID"    )                  // == 32
DEFINE_FUNCTION( FIFTARGETHASSKILLID,              "IfTargetHasSkillID"    )                     // == 33
DEFINE_FUNCTION( FELSE,                            "Else"    )                                   // == 34
DEFINE_FUNCTION( FRUN,                             "Run"    )                                    // == 35
DEFINE_FUNCTION( FWALK,                            "Walk"    )                                   // == 36
DEFINE_FUNCTION( FSNEAK,                           "Sneak"    )                                  // == 37
DEFINE_FUNCTION( FDOACTION,                        "DoAction"    )                               // == 38
DEFINE_FUNCTION( FKEEPACTION,                      "KeepAction"    )                             // == 39
DEFINE_FUNCTION( FISSUEORDER,                      "IssueOrder"    )                             // == 40
DEFINE_FUNCTION( FDROPWEAPONS,                     "DropWeapons"    )                            // == 41
DEFINE_FUNCTION( FTARGETDOACTION,                  "TargetDoAction"    )                         // == 42
DEFINE_FUNCTION( FOPENPASSAGE,                     "OpenPassage"    )                            // == 43
DEFINE_FUNCTION( FCLOSEPASSAGE,                    "ClosePassage"    )                           // == 44
DEFINE_FUNCTION( FIFPASSAGEOPEN,                   "IfPassageOpen"    )                          // == 45
DEFINE_FUNCTION( FGOPOOF,                          "GoPoof"    )                                 // == 46
DEFINE_FUNCTION( FCOSTTARGETITEMID,                "CostTargetItemID"    )                       // == 47
DEFINE_FUNCTION( FDOACTIONOVERRIDE,                "DoActionOverride"    )                       // == 48
DEFINE_FUNCTION( FIFHEALED,                        "IfHealed"    )                               // == 49
DEFINE_FUNCTION( FSENDMESSAGE,                     "SendMessage"    )                            // == 50
DEFINE_FUNCTION( FCALLFORHELP,                     "CallForHelp"    )                            // == 51
DEFINE_FUNCTION( FADDIDSZ,                         "AddIDSZ"    )                                // == 52
DEFINE_FUNCTION( FEND,                             "End"    )                                    // == 53
DEFINE_FUNCTION( FSETSTATE,                        "SetState"    )                               // == 54
DEFINE_FUNCTION( FGETSTATE,                        "GetState"    )                               // == 55
DEFINE_FUNCTION( FIFSTATEIS,                       "IfStateIs"    )                              // == 56
DEFINE_FUNCTION( FIFTARGETCANOPENSTUFF,            "IfTargetCanOpenStuff"    )                   // == 57
DEFINE_FUNCTION( FIFGRABBED,                       "IfGrabbed"    )                              // == 58
DEFINE_FUNCTION( FIFDROPPED,                       "IfDropped"    )                              // == 59
DEFINE_FUNCTION( FSETTARGETTOWHOEVERISHOLDING,     "SetTargetToWhoeverIsHolding"    )            // == 60
DEFINE_FUNCTION( FDAMAGETARGET,                    "DamageTarget"    )                           // == 61
DEFINE_FUNCTION( FIFXISLESSTHANY,                  "IfXIsLessThanY"    )                         // == 62
DEFINE_FUNCTION( FSETWEATHERTIME,                  "SetWeatherTime"    )                         // == 63
DEFINE_FUNCTION( FGETBUMPHEIGHT,                   "GetBumpHeight"    )                          // == 64
DEFINE_FUNCTION( FIFREAFFIRMED,                    "IfReaffirmed"    )                           // == 65
DEFINE_FUNCTION( FUNKEEPACTION,                    "UnkeepAction"    )                           // == 66
DEFINE_FUNCTION( FIFTARGETISONOTHERTEAM,           "IfTargetIsOnOtherTeam"    )                  // == 67
DEFINE_FUNCTION( FIFTARGETISONHATEDTEAM,           "IfTargetIsOnHatedTeam"    )                  // == 68
DEFINE_FUNCTION( FPRESSLATCHBUTTON,                "PressLatchButton"    )                       // == 69
DEFINE_FUNCTION( FSETTARGETTOTARGETOFLEADER,       "SetTargetToTargetOfLeader"    )              // == 70
DEFINE_FUNCTION( FIFLEADERKILLED,                  "IfLeaderKilled"    )                         // == 71
DEFINE_FUNCTION( FBECOMELEADER,                    "BecomeLeader"    )                           // == 72
DEFINE_FUNCTION( FCHANGETARGETARMOR,               "ChangeTargetArmor"    )                      // == 73
DEFINE_FUNCTION( FGIVEMONEYTOTARGET,               "GiveMoneyToTarget"    )                      // == 74
DEFINE_FUNCTION( FDROPKEYS,                        "DropKeys"    )                               // == 75
DEFINE_FUNCTION( FIFLEADERISALIVE,                 "IfLeaderIsAlive"    )                        // == 76
DEFINE_FUNCTION( FIFTARGETISOLDTARGET,             "IfTargetIsOldTarget"    )                    // == 77
DEFINE_FUNCTION( FSETTARGETTOLEADER,               "SetTargetToLeader"    )                      // == 78
DEFINE_FUNCTION( FSPAWNCHARACTER,                  "SpawnCharacter"    )                         // == 79
DEFINE_FUNCTION( FRESPAWNCHARACTER,                "RespawnCharacter"    )                       // == 80
DEFINE_FUNCTION( FCHANGETILE,                      "ChangeTile"    )                             // == 81
DEFINE_FUNCTION( FIFUSED,                          "IfUsed"    )                                 // == 82
DEFINE_FUNCTION( FDROPMONEY,                       "DropMoney"    )                              // == 83
DEFINE_FUNCTION( FSETOLDTARGET,                    "SetOldTarget"    )                           // == 84
DEFINE_FUNCTION( FDETACHFROMHOLDER,                "DetachFromHolder"    )                       // == 85
DEFINE_FUNCTION( FIFTARGETHASVULNERABILITYID,      "IfTargetHasVulnerabilityID"    )             // == 86
DEFINE_FUNCTION( FCLEANUP,                         "CleanUp"    )                                // == 87
DEFINE_FUNCTION( FIFCLEANEDUP,                     "IfCleanedUp"    )                            // == 88
DEFINE_FUNCTION( FIFSITTING,                       "IfSitting"    )                              // == 89
DEFINE_FUNCTION( FIFTARGETISHURT,                  "IfTargetIsHurt"    )                         // == 90
DEFINE_FUNCTION( FIFTARGETISAPLAYER,               "IfTargetIsAPlayer"    )                      // == 91
DEFINE_FUNCTION( FPLAYSOUND,                       "PlaySound"    )                              // == 92
DEFINE_FUNCTION( FSPAWNPARTICLE,                   "SpawnParticle"    )                          // == 93
DEFINE_FUNCTION( FIFTARGETISALIVE,                 "IfTargetIsAlive"    )                        // == 94
DEFINE_FUNCTION( FSTOP,                            "Stop"    )                                   // == 95
DEFINE_FUNCTION( FDISAFFIRMCHARACTER,              "DisaffirmCharacter"    )                     // == 96
DEFINE_FUNCTION( FREAFFIRMCHARACTER,               "ReaffirmCharacter"    )                      // == 97
DEFINE_FUNCTION( FIFTARGETISSELF,                  "IfTargetIsSelf"    )                         // == 98
DEFINE_FUNCTION( FIFTARGETISMALE,                  "IfTargetIsMale"    )                         // == 99
DEFINE_FUNCTION( FIFTARGETISFEMALE,                "IfTargetIsFemale"    )                       // == 100
DEFINE_FUNCTION( FSETTARGETTOSELF,                 "SetTargetToSelf"    )                        // == 101
DEFINE_FUNCTION( FSETTARGETTORIDER,                "SetTargetToRider"    )                       // == 102
DEFINE_FUNCTION( FGETATTACKTURN,                   "GetAttackTurn"    )                          // == 103
DEFINE_FUNCTION( FGETDAMAGETYPE,                   "GetDamageType"    )                          // == 104
DEFINE_FUNCTION( FBECOMESPELL,                    "BecomeSpell"    )                            // == 105
DEFINE_FUNCTION( FBECOMESPELLBOOK,                "BecomeSpellbook"    )                        // == 106
DEFINE_FUNCTION( FIFSCOREDAHIT,                   "IfScoredAHit"    )                           // == 107
DEFINE_FUNCTION( FIFDISAFFIRMED,                  "IfDisaffirmed"    )                          // == 108
DEFINE_FUNCTION( FTRANSLATEORDER,                 "TranslateOrder"    )                         // == 109
DEFINE_FUNCTION( FSETTARGETTOWHOEVERWASHIT,       "SetTargetToWhoeverWasHit"    )               // == 110
DEFINE_FUNCTION( FSETTARGETTOWIDEENEMY,           "SetTargetToWideEnemy"    )                   // == 111
DEFINE_FUNCTION( FIFCHANGED,                      "IfChanged"    )                              // == 112
DEFINE_FUNCTION( FIFINWATER,                      "IfInWater"    )                              // == 113
DEFINE_FUNCTION( FIFBORED,                        "IfBored"    )                                // == 114
DEFINE_FUNCTION( FIFTOOMUCHBAGGAGE,               "IfTooMuchBaggage"    )                       // == 115
DEFINE_FUNCTION( FIFGROGGED,                      "IfGrogged"    )                              // == 116
DEFINE_FUNCTION( FIFDAZED,                        "IfDazed"    )                                // == 117
DEFINE_FUNCTION( FIFTARGETHASSPECIALID,           "IfTargetHasSpecialID"    )                   // == 118
DEFINE_FUNCTION( FPRESSTARGETLATCHBUTTON,         "PressTargetLatchButton"    )                 // == 119
DEFINE_FUNCTION( FIFINVISIBLE,                    "IfInvisible"    )                            // == 120
DEFINE_FUNCTION( FIFARMORIS,    "IfArmorIs"    )                              // == 121
DEFINE_FUNCTION( FGETTARGETGROGTIME,    "GetTargetGrogTime"    )                      // == 122
DEFINE_FUNCTION( FGETTARGETDAZETIME,    "GetTargetDazeTime"    )                      // == 123
DEFINE_FUNCTION( FSETDAMAGETYPE,    "SetDamageType"    )                          // == 124
DEFINE_FUNCTION( FSETWATERLEVEL,    "SetWaterLevel"    )                          // == 125
DEFINE_FUNCTION( FENCHANTTARGET,    "EnchantTarget"    )                          // == 126
DEFINE_FUNCTION( FENCHANTCHILD,    "EnchantChild"    )                           // == 127
DEFINE_FUNCTION( FTELEPORTTARGET,    "TeleportTarget"    )                         // == 128
DEFINE_FUNCTION( FGIVEEXPERIENCETOTARGET,    "GiveExperienceToTarget"    )                 // == 129
DEFINE_FUNCTION( FINCREASEAMMO,    "IncreaseAmmo"    )                           // == 130
DEFINE_FUNCTION( FUNKURSETARGET,    "UnkurseTarget"    )                          // == 131
DEFINE_FUNCTION( FGIVEEXPERIENCETOTARGETTEAM,    "GiveExperienceToTargetTeam"    )             // == 132
DEFINE_FUNCTION( FIFUNARMED,    "IfUnarmed"    )                              // == 133
DEFINE_FUNCTION( FRESTOCKTARGETAMMOIDALL,    "RestockTargetAmmoIDAll"    )                 // == 134
DEFINE_FUNCTION( FRESTOCKTARGETAMMOIDFIRST,    "RestockTargetAmmoIDFirst"    )               // == 135
DEFINE_FUNCTION( FFLASHTARGET,    "FlashTarget"    )                            // == 136
DEFINE_FUNCTION( FSETREDSHIFT,    "SetRedShift"    )                            // == 137
DEFINE_FUNCTION( FSETGREENSHIFT,    "SetGreenShift"    )                          // == 138
DEFINE_FUNCTION( FSETBLUESHIFT,    "SetBlueShift"    )                           // == 139
DEFINE_FUNCTION( FSETLIGHT,    "SetLight"    )                               // == 140
DEFINE_FUNCTION( FSETALPHA,    "SetAlpha"    )                               // == 141
DEFINE_FUNCTION( FIFHITFROMBEHIND,    "IfHitFromBehind"    )                        // == 142
DEFINE_FUNCTION( FIFHITFROMFRONT,    "IfHitFromFront"    )                         // == 143
DEFINE_FUNCTION( FIFHITFROMLEFT,    "IfHitFromLeft"    )                          // == 144
DEFINE_FUNCTION( FIFHITFROMRIGHT,    "IfHitFromRight"    )                         // == 145
DEFINE_FUNCTION( FIFTARGETISONSAMETEAM,    "IfTargetIsOnSameTeam"    )                   // == 146
DEFINE_FUNCTION( FKILLTARGET,    "KillTarget"    )                             // == 147
DEFINE_FUNCTION( FUNDOENCHANT,    "UndoEnchant"    )                            // == 148
DEFINE_FUNCTION( FGETWATERLEVEL,    "GetWaterLevel"    )                          // == 149
DEFINE_FUNCTION( FCOSTTARGETMANA,    "CostTargetMana"    )                         // == 150
DEFINE_FUNCTION( FIFTARGETHASANYID,    "IfTargetHasAnyID"    )                       // == 151
DEFINE_FUNCTION( FSETBUMPSIZE,    "SetBumpSize"    )                            // == 152
DEFINE_FUNCTION( FIFNOTDROPPED,    "IfNotDropped"    )                           // == 153
DEFINE_FUNCTION( FIFYISLESSTHANX,    "IfYIsLessThanX"    )                         // == 154
DEFINE_FUNCTION( FSETFLYHEIGHT,    "SetFlyHeight"    )                           // == 155
DEFINE_FUNCTION( FIFBLOCKED,    "IfBlocked"    )                              // == 156
DEFINE_FUNCTION( FIFTARGETISDEFENDING,    "IfTargetIsDefending"    )                    // == 157
DEFINE_FUNCTION( FIFTARGETISATTACKING,    "IfTargetIsAttacking"    )                    // == 158
DEFINE_FUNCTION( FIFSTATEIS0,    "IfStateIs0"    )                             // == 159
DEFINE_FUNCTION( FIFSTATEIS1,    "IfStateIs1"    )                             // == 160
DEFINE_FUNCTION( FIFSTATEIS2,    "IfStateIs2"    )                             // == 161
DEFINE_FUNCTION( FIFSTATEIS3,    "IfStateIs3"    )                             // == 162
DEFINE_FUNCTION( FIFSTATEIS4,    "IfStateIs4"    )                             // == 163
DEFINE_FUNCTION( FIFSTATEIS5,    "IfStateIs5"    )                             // == 164
DEFINE_FUNCTION( FIFSTATEIS6,    "IfStateIs6"    )                             // == 165
DEFINE_FUNCTION( FIFSTATEIS7,    "IfStateIs7"    )                             // == 166
DEFINE_FUNCTION( FIFCONTENTIS,    "IfContentIs"    )                            // == 167
DEFINE_FUNCTION( FSETTURNMODETOWATCHTARGET,    "SetTurnModeToWatchTarget"    )               // == 168
DEFINE_FUNCTION( FIFSTATEISNOT,    "IfStateIsNot"    )                           // == 169
DEFINE_FUNCTION( FIFXISEQUALTOY,    "IfXIsEqualToY"    )                          // == 170
DEFINE_FUNCTION( FDEBUGMESSAGE,    "DebugMessage"    )                           // == 171
DEFINE_FUNCTION( FBLACKTARGET,    "BlackTarget"    )                            // == 172
DEFINE_FUNCTION( FSENDMESSAGENEAR,    "SendMessageNear"    )                        // == 173
DEFINE_FUNCTION( FIFHITGROUND,    "IfHitGround"    )                            // == 174
DEFINE_FUNCTION( FIFNAMEISKNOWN,    "IfNameIsKnown"    )                          // == 175
DEFINE_FUNCTION( FIFUSAGEISKNOWN,    "IfUsageIsKnown"    )                         // == 176
DEFINE_FUNCTION( FIFHOLDINGITEMID,    "IfHoldingItemID"    )                        // == 177
DEFINE_FUNCTION( FIFHOLDINGRANGEDWEAPON,    "IfHoldingRangedWeapon"    )                  // == 178
DEFINE_FUNCTION( FIFHOLDINGMELEEWEAPON,    "IfHoldingMeleeWeapon"    )                   // == 179
DEFINE_FUNCTION( FIFHOLDINGSHIELD,    "IfHoldingShield"    )                        // == 180
DEFINE_FUNCTION( FIFKURSED,    "IfKursed"    )                               // == 181
DEFINE_FUNCTION( FIFTARGETISKURSED,    "IfTargetIsKursed"    )                       // == 182
DEFINE_FUNCTION( FIFTARGETISDRESSEDUP,    "IfTargetIsDressedUp"    )                    // == 183
DEFINE_FUNCTION( FIFOVERWATER,    "IfOverWater"    )                            // == 184
DEFINE_FUNCTION( FIFTHROWN,    "IfThrown"    )                               // == 185
DEFINE_FUNCTION( FMAKENAMEKNOWN,    "MakeNameKnown"    )                          // == 186
DEFINE_FUNCTION( FMAKEUSAGEKNOWN,    "MakeUsageKnown"    )                         // == 187
DEFINE_FUNCTION( FSTOPTARGETMOVEMENT,    "StopTargetMovement"    )                     // == 188
DEFINE_FUNCTION( FSETXY,    "SetXY"    )                                  // == 189
DEFINE_FUNCTION( FGETXY,    "GetXY"    )                                  // == 190
DEFINE_FUNCTION( FADDXY,    "AddXY"    )                                  // == 191
DEFINE_FUNCTION( FMAKEAMMOKNOWN,    "MakeAmmoKnown"    )                          // == 192
DEFINE_FUNCTION( FSPAWNATTACHEDPARTICLE,    "SpawnAttachedParticle"    )                  // == 193
DEFINE_FUNCTION( FSPAWNEXACTPARTICLE,    "SpawnExactParticle"    )                     // == 194
DEFINE_FUNCTION( FACCELERATETARGET,    "AccelerateTarget"    )                       // == 195
DEFINE_FUNCTION( FIFDISTANCEISMORETHANTURN,    "IfDistanceIsMoreThanTurn"    )               // == 196
DEFINE_FUNCTION( FIFCRUSHED,    "IfCrushed"    )                              // == 197
DEFINE_FUNCTION( FMAKECRUSHVALID,    "MakeCrushValid"    )                         // == 198
DEFINE_FUNCTION( FSETTARGETTOLOWESTTARGET,    "SetTargetToLowestTarget"    )                // == 199
DEFINE_FUNCTION( FIFNOTPUTAWAY,    "IfNotPutAway"    )                           // == 200
DEFINE_FUNCTION( FIFTAKENOUT,    "IfTakenOut"    )                             // == 201
DEFINE_FUNCTION( FIFAMMOOUT,    "IfAmmoOut"    )                              // == 202
DEFINE_FUNCTION( FPLAYSOUNDLOOPED,    "PlaySoundLooped"    )                        // == 203
DEFINE_FUNCTION( FSTOPSOUND,    "StopSound"    )                              // == 204
DEFINE_FUNCTION( FHEALSELF,    "HealSelf"    )                               // == 205
DEFINE_FUNCTION( FEQUIP,    "Equip"    )                                  // == 206
DEFINE_FUNCTION( FIFTARGETHASITEMIDEQUIPPED,    "IfTargetHasItemIDEquipped"    )              // == 207
DEFINE_FUNCTION( FSETOWNERTOTARGET,    "SetOwnerToTarget"    )                       // == 208
DEFINE_FUNCTION( FSETTARGETTOOWNER,    "SetTargetToOwner"    )                       // == 209
DEFINE_FUNCTION( FSETFRAME,    "SetFrame"    )                               // == 210
DEFINE_FUNCTION( FBREAKPASSAGE,    "BreakPassage"    )                           // == 211
DEFINE_FUNCTION( FSETRELOADTIME,    "SetReloadTime"    )                          // == 212
DEFINE_FUNCTION( FSETTARGETTOWIDEBLAHID,    "SetTargetToWideBlahID"    )                  // == 213
DEFINE_FUNCTION( FPOOFTARGET,    "PoofTarget"    )                             // == 214
DEFINE_FUNCTION( FCHILDDOACTIONOVERRIDE,    "ChildDoActionOverride"    )                  // == 215
DEFINE_FUNCTION( FSPAWNPOOF,    "SpawnPoof"    )                              // == 216
DEFINE_FUNCTION( FSETSPEEDPERCENT,    "SetSpeedPercent"    )                        // == 217
DEFINE_FUNCTION( FSETCHILDSTATE,    "SetChildState"    )                          // == 218
DEFINE_FUNCTION( FSPAWNATTACHEDSIZEDPARTICLE,    "SpawnAttachedSizedParticle"    )             // == 219
DEFINE_FUNCTION( FCHANGEARMOR,    "ChangeArmor"    )                            // == 220
DEFINE_FUNCTION( FSHOWTIMER,    "ShowTimer"    )                              // == 221
DEFINE_FUNCTION( FIFFACINGTARGET,    "IfFacingTarget"    )                         // == 222
DEFINE_FUNCTION( FPLAYSOUNDVOLUME,    "PlaySoundVolume"    )                        // == 223
DEFINE_FUNCTION( FSPAWNATTACHEDFACEDPARTICLE,    "SpawnAttachedFacedParticle"    )             // == 224
DEFINE_FUNCTION( FIFSTATEISODD,    "IfStateIsOdd"    )                           // == 225
DEFINE_FUNCTION( FSETTARGETTODISTANTENEMY,    "SetTargetToDistantEnemy"    )                // == 226
DEFINE_FUNCTION( FTELEPORT,    "Teleport"    )                               // == 227
DEFINE_FUNCTION( FGIVESTRENGTHTOTARGET,    "GiveStrengthToTarget"    )                   // == 228
DEFINE_FUNCTION( FGIVEWISDOMTOTARGET,    "GiveWisdomToTarget"    )                     // == 229
DEFINE_FUNCTION( FGIVEINTELLIGENCETOTARGET,    "GiveIntelligenceToTarget"    )               // == 230
DEFINE_FUNCTION( FGIVEDEXTERITYTOTARGET,    "GiveDexterityToTarget"    )                  // == 231
DEFINE_FUNCTION( FGIVELIFETOTARGET,    "GiveLifeToTarget"    )                       // == 232
DEFINE_FUNCTION( FGIVEMANATOTARGET,    "GiveManaToTarget"    )                       // == 233
DEFINE_FUNCTION( FSHOWMAP,    "ShowMap"    )                                // == 234
DEFINE_FUNCTION( FSHOWYOUAREHERE,    "ShowYouAreHere"    )                         // == 235
DEFINE_FUNCTION( FSHOWBLIPXY,    "ShowBlipXY"    )                             // == 236
DEFINE_FUNCTION( FHEALTARGET,    "HealTarget"    )                             // == 237
DEFINE_FUNCTION( FPUMPTARGET,    "PumpTarget"    )                             // == 238
DEFINE_FUNCTION( FCOSTAMMO,    "CostAmmo"    )                               // == 239
DEFINE_FUNCTION( FMAKESIMILARNAMESKNOWN,    "MakeSimilarNamesKnown"    )                  // == 240
DEFINE_FUNCTION( FSPAWNATTACHEDHOLDERPARTICLE,    "SpawnAttachedHolderParticle"    )            // == 241
DEFINE_FUNCTION( FSETTARGETRELOADTIME,    "SetTargetReloadTime"    )                    // == 242
DEFINE_FUNCTION( FSETFOGLEVEL,    "SetFogLevel"    )                            // == 243
DEFINE_FUNCTION( FGETFOGLEVEL,    "GetFogLevel"    )                            // == 244
DEFINE_FUNCTION( FSETFOGTAD,    "SetFogTAD"    )                              // == 245
DEFINE_FUNCTION( FSETFOGBOTTOMLEVEL,    "SetFogBottomLevel"    )                      // == 246
DEFINE_FUNCTION( FGETFOGBOTTOMLEVEL,    "GetFogBottomLevel"    )                      // == 247
DEFINE_FUNCTION( FCORRECTACTIONFORHAND,    "CorrectActionForHand"    )                   // == 248
DEFINE_FUNCTION( FIFTARGETISMOUNTED,    "IfTargetIsMounted"    )                      // == 249
DEFINE_FUNCTION( FSPARKLEICON,    "SparkleIcon"    )                            // == 250
DEFINE_FUNCTION( FUNSPARKLEICON,    "UnsparkleIcon"    )                          // == 251
DEFINE_FUNCTION( FGETTILEXY,    "GetTileXY"    )                              // == 252
DEFINE_FUNCTION( FSETTILEXY,    "SetTileXY"    )                              // == 253
DEFINE_FUNCTION( FSETSHADOWSIZE,    "SetShadowSize"    )                          // == 254
DEFINE_FUNCTION( FORDERTARGET,    "OrderTarget"    )                            // == 255
DEFINE_FUNCTION( FSETTARGETTOWHOEVERISINPASSAGE,    "SetTargetToWhoeverIsInPassage"    )          // == 256
DEFINE_FUNCTION( FIFCHARACTERWASABOOK,    "IfCharacterWasABook"    )                    // == 257
DEFINE_FUNCTION( FSETENCHANTBOOSTVALUES,    "SetEnchantBoostValues"    )                  // == 258
DEFINE_FUNCTION( FSPAWNCHARACTERXYZ,    "SpawnCharacterXYZ"    )                      // == 259
DEFINE_FUNCTION( FSPAWNEXACTCHARACTERXYZ,    "SpawnExactCharacterXYZ"    )                 // == 260
DEFINE_FUNCTION( FCHANGETARGETCLASS,    "ChangeTargetClass"    )                      // == 261
DEFINE_FUNCTION( FPLAYFULLSOUND,    "PlayFullSound"    )                          // == 262
DEFINE_FUNCTION( FSPAWNEXACTCHASEPARTICLE,    "SpawnExactChaseParticle"    )                // == 263
DEFINE_FUNCTION( FCREATEORDER,    "CreateOrder"    )                            // == 264
DEFINE_FUNCTION( FORDERSPECIALID,    "OrderSpecialID"    )                         // == 265
DEFINE_FUNCTION( FUNKURSETARGETINVENTORY,    "UnkurseTargetInventory"    )                 // == 266
DEFINE_FUNCTION( FIFTARGETISSNEAKING,    "IfTargetIsSneaking"    )                     // == 267
DEFINE_FUNCTION( FDROPITEMS,    "DropItems"    )                              // == 268
DEFINE_FUNCTION( FRESPAWNTARGET,    "RespawnTarget"    )                          // == 269
DEFINE_FUNCTION( FTARGETDOACTIONSETFRAME,    "TargetDoActionSetFrame"    )                 // == 270
DEFINE_FUNCTION( FIFTARGETCANSEEINVISIBLE,    "IfTargetCanSeeInvisible"    )                // == 271
DEFINE_FUNCTION( FSETTARGETTONEARESTBLAHID,    "SetTargetToNearestBlahID"    )               // == 272
DEFINE_FUNCTION( FSETTARGETTONEARESTENEMY,    "SetTargetToNearestEnemy"    )                // == 273
DEFINE_FUNCTION( FSETTARGETTONEARESTFRIEND,    "SetTargetToNearestFriend"    )               // == 274
DEFINE_FUNCTION( FSETTARGETTONEARESTLIFEFORM,    "SetTargetToNearestLifeform"    )             // == 275
DEFINE_FUNCTION( FFLASHPASSAGE,    "FlashPassage"    )                           // == 276
DEFINE_FUNCTION( FFINDTILEINPASSAGE,    "FindTileInPassage"    )                      // == 277
DEFINE_FUNCTION( FIFHELDINLEFTHAND,    "IfHeldInLeftHand"    )                       // == 278
DEFINE_FUNCTION( FNOTANITEM,    "NotAnItem"    )                              // == 279
DEFINE_FUNCTION( FSETCHILDAMMO,    "SetChildAmmo"    )                           // == 280
DEFINE_FUNCTION( FIFHITVULNERABLE,    "IfHitVulnerable"    )                        // == 281
DEFINE_FUNCTION( FIFTARGETISFLYING,    "IfTargetIsFlying"    )                       // == 282
DEFINE_FUNCTION( FIDENTIFYTARGET,    "IdentifyTarget"    )                         // == 283
DEFINE_FUNCTION( FBEATMODULE,    "BeatModule"    )                             // == 284
DEFINE_FUNCTION( FENDMODULE,    "EndModule"    )                              // == 285
DEFINE_FUNCTION( FDISABLEEXPORT,    "DisableExport"    )                          // == 286
DEFINE_FUNCTION( FENABLEEXPORT,    "EnableExport"    )                           // == 287
DEFINE_FUNCTION( FGETTARGETSTATE,    "GetTargetState"    )                         // == 288
DEFINE_FUNCTION( FIFEQUIPPED,    "IfEquipped"    )                             // == 289
DEFINE_FUNCTION( FDROPTARGETMONEY,    "DropTargetMoney"    )                        // == 290
DEFINE_FUNCTION( FGETTARGETCONTENT,    "GetTargetContent"    )                       // == 291
DEFINE_FUNCTION( FDROPTARGETKEYS,    "DropTargetKeys"    )                         // == 292
DEFINE_FUNCTION( FJOINTEAM,    "JoinTeam"    )                               // == 293
DEFINE_FUNCTION( FTARGETJOINTEAM,    "TargetJoinTeam"    )                         // == 294
DEFINE_FUNCTION( FCLEARMUSICPASSAGE,    "ClearMusicPassage"    )                      // == 295
DEFINE_FUNCTION( FCLEARENDMESSAGE,    "ClearEndMessage"    )                        // == 296
DEFINE_FUNCTION( FADDENDMESSAGE,    "AddEndMessage"    )                          // == 297
DEFINE_FUNCTION( FPLAYMUSIC,    "PlayMusic"    )                              // == 298
DEFINE_FUNCTION( FSETMUSICPASSAGE,    "SetMusicPassage"    )                        // == 299
DEFINE_FUNCTION( FMAKECRUSHINVALID,    "MakeCrushInvalid"    )                       // == 300
DEFINE_FUNCTION( FSTOPMUSIC,    "StopMusic"    )                              // == 301
DEFINE_FUNCTION( FFLASHVARIABLE,    "FlashVariable"    )                          // == 302
DEFINE_FUNCTION( FACCELERATEUP,    "AccelerateUp"    )                           // == 303
DEFINE_FUNCTION( FFLASHVARIABLEHEIGHT,    "FlashVariableHeight"    )                    // == 304
DEFINE_FUNCTION( FSETDAMAGETIME,    "SetDamageTime"    )                          // == 305
DEFINE_FUNCTION( FIFSTATEIS8,    "IfStateIs8"    )                             // == 306
DEFINE_FUNCTION( FIFSTATEIS9,    "IfStateIs9"    )                             // == 307
DEFINE_FUNCTION( FIFSTATEIS10,    "IfStateIs10"    )                            // == 308
DEFINE_FUNCTION( FIFSTATEIS11,    "IfStateIs11"    )                            // == 309
DEFINE_FUNCTION( FIFSTATEIS12,    "IfStateIs12"    )                            // == 310
DEFINE_FUNCTION( FIFSTATEIS13,    "IfStateIs13"    )                            // == 311
DEFINE_FUNCTION( FIFSTATEIS14,    "IfStateIs14"    )                            // == 312
DEFINE_FUNCTION( FIFSTATEIS15,    "IfStateIs15"    )                            // == 313
DEFINE_FUNCTION( FIFTARGETISAMOUNT,    "IfTargetIsAMount"    )                       // == 314
DEFINE_FUNCTION( FIFTARGETISAPLATFORM,    "IfTargetIsAPlatform"    )                    // == 315
DEFINE_FUNCTION( FADDSTAT,    "AddStat"    )                                // == 316
DEFINE_FUNCTION( FDISENCHANTTARGET,    "DisenchantTarget"    )                       // == 317
DEFINE_FUNCTION( FDISENCHANTALL,    "DisenchantAll"    )                          // == 318
DEFINE_FUNCTION( FSETVOLUMENEARESTTEAMMATE,    "SetVolumeNearestTeammate"    )               // == 319
DEFINE_FUNCTION( FADDSHOPPASSAGE,    "AddShopPassage"    )                         // == 320
DEFINE_FUNCTION( FTARGETPAYFORARMOR,    "TargetPayForArmor"    )                      // == 321
DEFINE_FUNCTION( FJOINEVILTEAM,    "JoinEvilTeam"    )                           // == 322
DEFINE_FUNCTION( FJOINNULLTEAM,    "JoinNullTeam"    )                           // == 323
DEFINE_FUNCTION( FJOINGOODTEAM,    "JoinGoodTeam"    )                           // == 324
DEFINE_FUNCTION( FPITSKILL,    "PitsKill"    )                               // == 325
DEFINE_FUNCTION( FSETTARGETTOPASSAGEID,    "SetTargetToPassageID"    )                   // == 326
DEFINE_FUNCTION( FMAKENAMEUNKNOWN,    "MakeNameUnknown"    )                        // == 327
DEFINE_FUNCTION( FSPAWNEXACTPARTICLEENDSPAWN,    "SpawnExactParticleEndSpawn"    )             // == 328
DEFINE_FUNCTION( FSPAWNPOOFSPEEDSPACINGDAMAGE,    "SpawnPoofSpeedSpacingDamage"    )            // == 329
DEFINE_FUNCTION( FGIVEEXPERIENCETOGOODTEAM,    "GiveExperienceToGoodTeam"    )               // == 330
DEFINE_FUNCTION( FDONOTHING,    "DoNothing"    )                              // == 331
DEFINE_FUNCTION( FGROGTARGET,    "DazeTarget"    )                             // == 332
DEFINE_FUNCTION( FDAZETARGET,    "GrogTarget"    )                             // == 333
DEFINE_FUNCTION( FENABLERESPAWN,    "EnableRespawn"    )                          // == 334
DEFINE_FUNCTION( FDISABLERESPAWN,    "DisableRespawn"    )                         // == 335
DEFINE_FUNCTION( FDISPELTARGETENCHANTID,    "DispelTargetEnchantID"    )                  // == 336
DEFINE_FUNCTION( FIFHOLDERBLOCKED,    "IfHolderBlocked"    )                        // == 337
DEFINE_FUNCTION( FGETSKILLLEVEL,    "GetTargetShieldProfiency"    )                          // == 338
DEFINE_FUNCTION( FIFTARGETHASNOTFULLMANA,    "IfTargetHasNotFullMana"    )                 // == 339
DEFINE_FUNCTION( FENABLELISTENSKILL,    "EnableListenSkill"    )                      // == 340
DEFINE_FUNCTION( FSETTARGETTOLASTITEMUSED,    "SetTargetToLastItemUsed"    )                // == 341
DEFINE_FUNCTION( FFOLLOWLINK,    "FollowLink"    )                             // == 342  Scripted AI functions ( v1.00)
DEFINE_FUNCTION( FIFOPERATORISLINUX,    "IfOperatorIsLinux"    )                      // == 343
DEFINE_FUNCTION( FIFTARGETISAWEAPON,    "IfTargetIsAWeapon"    )                      // == 344
DEFINE_FUNCTION( FIFSOMEONEISSTEALING,    "IfSomeoneIsStealing"    )                    // == 345
DEFINE_FUNCTION( FIFTARGETISASPELL,    "IfTargetIsASpell"    )                       // == 346
DEFINE_FUNCTION( FIFBACKSTABBED,    "IfBackstabbed"    )                          // == 347
DEFINE_FUNCTION( FGETTARGETDAMAGETYPE,    "GetTargetDamageType"    )                    // == 348
DEFINE_FUNCTION( FADDQUEST,    "AddTargetQuest"    )                               // == 349
DEFINE_FUNCTION( FBEATQUESTALLPLAYERS,    "BeatQuestAllPlayers"    )                    // == 350
DEFINE_FUNCTION( FIFTARGETHASQUEST,    "IfTargetHasQuest"    )                       // == 351
DEFINE_FUNCTION( FSETQUESTLEVEL,    "SetTargetQuestLevel"    )                          // == 352
DEFINE_FUNCTION( FADDQUESTALLPLAYERS,    "AddQuestAllPlayers"    )                     // == 353
DEFINE_FUNCTION( FADDBLIPALLENEMIES,    "AddBlipAllEnemies"    )                      // == 354
DEFINE_FUNCTION( FPITSFALL,    "PitsFall"    )                               // == 355
DEFINE_FUNCTION( FIFTARGETISOWNER,    "IfTargetIsOwner"    )                        // == 356
DEFINE_FUNCTION( FSETSPEECH,    "SetSpeech"    )                      // == 357
DEFINE_FUNCTION( FSETMOVESPEECH,        "FSETMOVESPEECH"    )                  // == 358
DEFINE_FUNCTION( FSETSECONDMOVESPEECH,        "FSETSECONDMOVESPEECH"    )            // == 359
DEFINE_FUNCTION( FSETATTACKSPEECH,        "FSETATTACKSPEECH"    )                // == 360
DEFINE_FUNCTION( FSETASSISTSPEECH,        "FSETASSISTSPEECH"    )                // == 361
DEFINE_FUNCTION( FSETTERRAINSPEECH,        "FSETTERRAINSPEECH"    )               // == 362
DEFINE_FUNCTION( FSETSELECTSPEECH,        "FSETSELECTSPEECH"    )                // == 363
DEFINE_FUNCTION( FTAKEPICTURE,    "TakePicture"    )                    // == 364
DEFINE_FUNCTION( FIFOPERATORISMACINTOSH,    "IfOperatorIsMacintosh"    )          // == 365
DEFINE_FUNCTION( FIFMODULEHASIDSZ,    "IfModuleHasIDSZ"    )                // == 366
DEFINE_FUNCTION( FMORPHTOTARGET,    "MorphToTarget"    )                  // == 367
DEFINE_FUNCTION( FGIVEMANAFLOWTOTARGET,    "GiveManaFlowToTarget"    )           // == 368
DEFINE_FUNCTION( FGIVEMANARETURNTOTARGET,    "GiveManaReturnToTarget"    )         // == 369
DEFINE_FUNCTION( FSETMONEY,    "SetMoney"    )                       // == 370
DEFINE_FUNCTION( FIFTARGETCANSEEKURSES,    "IfTargetCanSeeKurses"    )           // == 371
DEFINE_FUNCTION( FSPAWNATTACHEDCHARACTER,    "SpawnAttachedCharacter"    )         // == 372
DEFINE_FUNCTION( FKURSETARGET,    "KurseTarget"    )                    // == 373
DEFINE_FUNCTION( FSETCHILDCONTENT,    "SetChildContent"    )                // == 374
DEFINE_FUNCTION( FSETTARGETTOCHILD,    "SetTargetToChild"    )               // == 375
DEFINE_FUNCTION( FSETDAMAGETHRESHOLD,    "SetDamageThreshold"    )    //

DEFINE_CONSTANT(,    "BLAHDEAD"    )    //
DEFINE_CONSTANT(,    "BLAHENEMIES"    )    //
DEFINE_CONSTANT(,    "BLAHFRIENDS"    )    //
DEFINE_CONSTANT(,    "BLAHITEMS"    )    //
DEFINE_CONSTANT(,    "BLAHINVERTID"    )    //
DEFINE_CONSTANT(,    "BLAHPLAYERS"    )    //

DEFINE_CONSTANT(,    "STATEPARRY"    )    //
DEFINE_CONSTANT(,    "STATEWANDER"    )    //
DEFINE_CONSTANT(,    "STATEGUARD"    )    //
DEFINE_CONSTANT(,    "STATEFOLLOW"    )    //
DEFINE_CONSTANT(,    "STATESURROUND"    )    //
DEFINE_CONSTANT(,    "STATERETREAT"    )    //
DEFINE_CONSTANT(,    "STATECHARGE"    )    //
DEFINE_CONSTANT(,    "STATECOMBAT"    )    //

DEFINE_CONSTANT( GRIP_ONLY,    "GRIPONLY"    )    //
DEFINE_CONSTANT( GRIP_LEFT,    "GRIPLEFT"    )    //
DEFINE_CONSTANT( GRIP_RIGHT,    "GRIPRIGHT"    )    //
DEFINE_CONSTANT( GRIP_ORIGIN,    "SPAWNORIGIN"    )    //
DEFINE_CONSTANT( GRIP_LAST,    "SPAWNLAST"    )    //

DEFINE_CONSTANT( LATCHBUTTON_LEFT,    "LATCHLEFT"    )    //
DEFINE_CONSTANT( LATCHBUTTON_RIGHT,    "LATCHRIGHT"    )    //
DEFINE_CONSTANT( LATCHBUTTON_JUMP,    "LATCHJUMP"    )    //
DEFINE_CONSTANT( LATCHBUTTON_ALTLEFT,    "LATCHALTLEFT"    )    //
DEFINE_CONSTANT( LATCHBUTTON_ALTRIGHT,    "LATCHALTRIGHT"    )    //
DEFINE_CONSTANT( LATCHBUTTON_PACKLEFT,    "LATCHPACKLEFT"    )    //
DEFINE_CONSTANT( LATCHBUTTON_PACKRIGHT,    "LATCHPACKRIGHT"    )    //

DEFINE_CONSTANT( DAMAGE_SLASH,    "DAMAGESLASH"    )    //
DEFINE_CONSTANT( DAMAGE_CRUSH,    "DAMAGECRUSH"    )    //
DEFINE_CONSTANT( DAMAGE_POKE ,    "DAMAGEPOKE"    )    //
DEFINE_CONSTANT( DAMAGE_HOLY ,    "DAMAGEHOLY"    )    //
DEFINE_CONSTANT( DAMAGE_EVIL ,    "DAMAGEEVIL"    )    //
DEFINE_CONSTANT( DAMAGE_FIRE ,    "DAMAGEFIRE"    )    //
DEFINE_CONSTANT( DAMAGE_ICE  ,    "DAMAGEICE"    )    //
DEFINE_CONSTANT( DAMAGE_ZAP  ,    "DAMAGEZAP"    )    //

DEFINE_CONSTANT( ACTION_DA,    "ACTIONDA"    )    //
DEFINE_CONSTANT( ACTION_DB,    "ACTIONDB"    )    //
DEFINE_CONSTANT( ACTION_DC,    "ACTIONDC"    )    //
DEFINE_CONSTANT( ACTION_DD,    "ACTIONDD"    )    //
DEFINE_CONSTANT( ACTION_UA,    "ACTIONUA"    )    //
DEFINE_CONSTANT( ACTION_UB,    "ACTIONUB"    )    //
DEFINE_CONSTANT( ACTION_UC,    "ACTIONUC"    )    //
DEFINE_CONSTANT( ACTION_UD,    "ACTIONUD"    )    //
DEFINE_CONSTANT( ACTION_TA,    "ACTIONTA"    )    //
DEFINE_CONSTANT( ACTION_TB,    "ACTIONTB"    )    //
DEFINE_CONSTANT( ACTION_TC,    "ACTIONTC"    )    //
DEFINE_CONSTANT( ACTION_TD,    "ACTIONTD"    )    //
DEFINE_CONSTANT( ACTION_CA,    "ACTIONCA"    )    //
DEFINE_CONSTANT( ACTION_CB,    "ACTIONCB"    )    //
DEFINE_CONSTANT( ACTION_CC,    "ACTIONCC"    )    //
DEFINE_CONSTANT( ACTION_CD,    "ACTIONCD"    )    //
DEFINE_CONSTANT( ACTION_SA,    "ACTIONSA"    )    //
DEFINE_CONSTANT( ACTION_SB,    "ACTIONSB"    )    //
DEFINE_CONSTANT( ACTION_SC,    "ACTIONSC"    )    //
DEFINE_CONSTANT( ACTION_SD,    "ACTIONSD"    )    //
DEFINE_CONSTANT( ACTION_BA,    "ACTIONBA"    )    //
DEFINE_CONSTANT( ACTION_BB,    "ACTIONBB"    )    //
DEFINE_CONSTANT( ACTION_BC,    "ACTIONBC"    )    //
DEFINE_CONSTANT( ACTION_BD,    "ACTIONBD"    )    //
DEFINE_CONSTANT( ACTION_LA,    "ACTIONLA"    )    //
DEFINE_CONSTANT( ACTION_LB,    "ACTIONLB"    )    //
DEFINE_CONSTANT( ACTION_LC,    "ACTIONLC"    )    //
DEFINE_CONSTANT( ACTION_LD,    "ACTIONLD"    )    //
DEFINE_CONSTANT( ACTION_XA,    "ACTIONXA"    )    //
DEFINE_CONSTANT( ACTION_XB,    "ACTIONXB"    )    //
DEFINE_CONSTANT( ACTION_XC,    "ACTIONXC"    )    //
DEFINE_CONSTANT( ACTION_XD,    "ACTIONXD"    )    //
DEFINE_CONSTANT( ACTION_FA,    "ACTIONFA"    )    //
DEFINE_CONSTANT( ACTION_FB,    "ACTIONFB"    )    //
DEFINE_CONSTANT( ACTION_FC,    "ACTIONFC"    )    //
DEFINE_CONSTANT( ACTION_FD,    "ACTIONFD"    )    //
DEFINE_CONSTANT( ACTION_PA,    "ACTIONPA"    )    //
DEFINE_CONSTANT( ACTION_PB,    "ACTIONPB"    )    //
DEFINE_CONSTANT( ACTION_PC,    "ACTIONPC"    )    //
DEFINE_CONSTANT( ACTION_PD,    "ACTIONPD"    )    //
DEFINE_CONSTANT( ACTION_EA,    "ACTIONEA"    )    //
DEFINE_CONSTANT( ACTION_EB,    "ACTIONEB"    )    //
DEFINE_CONSTANT( ACTION_RA,    "ACTIONRA"    )    //
DEFINE_CONSTANT( ACTION_ZA,    "ACTIONZA"    )    //
DEFINE_CONSTANT( ACTION_ZB,    "ACTIONZB"    )    //
DEFINE_CONSTANT( ACTION_ZC,    "ACTIONZC"    )    //
DEFINE_CONSTANT( ACTION_ZD,    "ACTIONZD"    )    //
DEFINE_CONSTANT( ACTION_WA,    "ACTIONWA"    )    //
DEFINE_CONSTANT( ACTION_WB,    "ACTIONWB"    )    //
DEFINE_CONSTANT( ACTION_WC,    "ACTIONWC"    )    //
DEFINE_CONSTANT( ACTION_WD,    "ACTIONWD"    )    //
DEFINE_CONSTANT( ACTION_JA,    "ACTIONJA"    )    //
DEFINE_CONSTANT( ACTION_JB,    "ACTIONJB"    )    //
DEFINE_CONSTANT( ACTION_JC,    "ACTIONJC"    )    //
DEFINE_CONSTANT( ACTION_HA,    "ACTIONHA"    )    //
DEFINE_CONSTANT( ACTION_HB,    "ACTIONHB"    )    //
DEFINE_CONSTANT( ACTION_HC,    "ACTIONHC"    )    //
DEFINE_CONSTANT( ACTION_HD,    "ACTIONHD"    )    //
DEFINE_CONSTANT( ACTION_KA,    "ACTIONKA"    )    //
DEFINE_CONSTANT( ACTION_KB,    "ACTIONKB"    )    //
DEFINE_CONSTANT( ACTION_KC,    "ACTIONKC"    )    //
DEFINE_CONSTANT( ACTION_KD,    "ACTIONKD"    )    //
DEFINE_CONSTANT( ACTION_MA,    "ACTIONMA"    )    //
DEFINE_CONSTANT( ACTION_MB,    "ACTIONMB"    )    //
DEFINE_CONSTANT( ACTION_MC,    "ACTIONMC"    )    //
DEFINE_CONSTANT( ACTION_MD,    "ACTIONMD"    )    //
DEFINE_CONSTANT( ACTION_ME,    "ACTIONME"    )    //
DEFINE_CONSTANT( ACTION_MF,    "ACTIONMF"    )    //
DEFINE_CONSTANT( ACTION_MG,    "ACTIONMG"    )    //
DEFINE_CONSTANT( ACTION_MH,    "ACTIONMH"    )    //
DEFINE_CONSTANT( ACTION_MI,    "ACTIONMI"    )    //
DEFINE_CONSTANT( ACTION_MJ,    "ACTIONMJ"    )    //
DEFINE_CONSTANT( ACTION_MK,    "ACTIONMK"    )    //
DEFINE_CONSTANT( ACTION_ML,    "ACTIONML"    )    //
DEFINE_CONSTANT( ACTION_MM,    "ACTIONMM"    )    //
DEFINE_CONSTANT( ACTION_MN,    "ACTIONMN"    )    //

DEFINE_CONSTANT( XP_FINDSECRET,    "EXPSECRET"    )    //
DEFINE_CONSTANT( XP_WINQUEST,    "EXPQUEST"    )    //
DEFINE_CONSTANT( XP_USEDUNKOWN,    "EXPDARE"    )    //
DEFINE_CONSTANT( XP_KILLENEMY,    "EXPKILL"    )    //
DEFINE_CONSTANT( XP_KILLSLEEPY,    "EXPMURDER"    )    //
DEFINE_CONSTANT( XP_KILLHATED,    "EXPREVENGE"    )    //
DEFINE_CONSTANT( XP_TEAMKILL,    "EXPTEAMWORK"    )    //
DEFINE_CONSTANT( XP_TALKGOOD,    "EXPROLEPLAY"    )    //

DEFINE_CONSTANT(,    "MESSAGEDEATH"    )    //
DEFINE_CONSTANT(,    "MESSAGEHATE"    )    //
DEFINE_CONSTANT(,    "MESSAGEOUCH"    )    //
DEFINE_CONSTANT(,    "MESSAGEFRAG"    )    //
DEFINE_CONSTANT(,    "MESSAGEACCIDENT"    )    //
DEFINE_CONSTANT(,    "MESSAGECOSTUME"    )    //

DEFINE_CONSTANT(,    "ORDERMOVE"    )    //
DEFINE_CONSTANT(,    "ORDERATTACK"    )    //
DEFINE_CONSTANT(,    "ORDERASSIST"    )    //
DEFINE_CONSTANT(,    "ORDERSTAND"    )    //
DEFINE_CONSTANT(,    "ORDERTERRAIN"    )    //

DEFINE_CONSTANT(,    "WHITE"    )    //
DEFINE_CONSTANT(,    "RED"    )    //
DEFINE_CONSTANT(,    "YELLOW"    )    //
DEFINE_CONSTANT(,    "GREEN"    )    //
DEFINE_CONSTANT(,    "BLUE"    )    //
DEFINE_CONSTANT(,    "PURPLE"    )    //

DEFINE_CONSTANT(,    "FXNOREFLECT"    )    //
DEFINE_CONSTANT(,    "FXDRAWREFLECT"    )    //
DEFINE_CONSTANT(,    "FXANIM"    )    //
DEFINE_CONSTANT(,    "FXWATER"    )    //
DEFINE_CONSTANT(,    "FXBARRIER"    )    //
DEFINE_CONSTANT(,    "FXIMPASS"    )    //
DEFINE_CONSTANT(,    "FXDAMAGE"    )    //
DEFINE_CONSTANT(,    "FXSLIPPY"    )    //

DEFINE_CONSTANT(,    "MOVEMELEE"    )    //
DEFINE_CONSTANT(,    "MOVERANGED"    )    //
DEFINE_CONSTANT(,    "MOVEDISTANCE"    )    //
DEFINE_CONSTANT(,    "MOVERETREAT"    )    //
DEFINE_CONSTANT(,    "MOVECHARGE"    )    //

DEFINE_CONSTANT(,    "TEAMA"    )    //
DEFINE_CONSTANT(,    "TEAMB"    )    //
DEFINE_CONSTANT(,    "TEAMC"    )    //
DEFINE_CONSTANT(,    "TEAMD"    )    //
DEFINE_CONSTANT(,    "TEAME"    )    //
DEFINE_CONSTANT(,    "TEAMF"    )    //
DEFINE_CONSTANT(,    "TEAMG"    )    //
DEFINE_CONSTANT(,    "TEAMH"    )    //
DEFINE_CONSTANT(,    "TEAMI"    )    //
DEFINE_CONSTANT(,    "TEAMJ"    )    //
DEFINE_CONSTANT(,    "TEAMK"    )    //
DEFINE_CONSTANT(,    "TEAML"    )    //
DEFINE_CONSTANT(,    "TEAMM"    )    //
DEFINE_CONSTANT(,    "TEAMN"    )    //
DEFINE_CONSTANT(,    "TEAMO"    )    //
DEFINE_CONSTANT(,    "TEAMP"    )    //
DEFINE_CONSTANT(,    "TEAMQ"    )    //
DEFINE_CONSTANT(,    "TEAMR"    )    //
DEFINE_CONSTANT(,    "TEAMS"    )    //
DEFINE_CONSTANT(,    "TEAMT"    )    //
DEFINE_CONSTANT(,    "TEAMV"    )    //
DEFINE_CONSTANT(,    "TEAMW"    )    //
DEFINE_CONSTANT(,    "TEAMX"    )    //
DEFINE_CONSTANT(,    "TEAMY"    )    //
DEFINE_CONSTANT(,    "TEAMZ"    )    //

DEFINE_CONSTANT(,    "INVENTORY"    )    //
DEFINE_CONSTANT(,    "LEFT"    )    //
DEFINE_CONSTANT(,    "RIGHT"    )    //

DEFINE_CONSTANT(,    "EASY"    )    //
DEFINE_CONSTANT(,    "NORMAL"    )    //
DEFINE_CONSTANT(,    "HARD"    )    //

DEFINE_VARIABLE( VARTMPX = 0,    "tmpx"    )             // == 0
DEFINE_VARIABLE( VARTMPY,    "tmpy"    )                 // == 1
DEFINE_VARIABLE( VARTMPDISTANCE,    "tmpdistance"    )          // == 2
DEFINE_VARIABLE( VARTMPTURN,    "tmpturn"    )              // == 3
DEFINE_VARIABLE( VARTMPARGUMENT,    "tmpargument"    )          // == 4
DEFINE_VARIABLE( VARRAND,    "rand"    )                 // == 5
DEFINE_VARIABLE( VARSELFX,    "selfx"    )                // == 6
DEFINE_VARIABLE( VARSELFY,    "selfy"    )                // == 7
DEFINE_VARIABLE( VARSELFTURN,    "selfturn"    )             // == 8
DEFINE_VARIABLE( VARSELFCOUNTER,    "selfcounter"    )          // == 9
DEFINE_VARIABLE( VARSELFORDER,    "selforder"    )            // == 10
DEFINE_VARIABLE( VARSELFMORALE,    "selfmorale"    )           // == 11
DEFINE_VARIABLE( VARSELFLIFE,    "selflife"    )             // == 12
DEFINE_VARIABLE( VARTARGETX,    "targetx"    )              // == 13
DEFINE_VARIABLE( VARTARGETY,    "targety"    )              // == 14
DEFINE_VARIABLE( VARTARGETDISTANCE,    "targetdistance"    )       // == 15
DEFINE_VARIABLE( VARTARGETTURN,    "targetturn"    )           // == 16
DEFINE_VARIABLE( VARLEADERX,    "leaderx"    )              // == 17
DEFINE_VARIABLE( VARLEADERY,    "leadery"    )              // == 18
DEFINE_VARIABLE( VARLEADERDISTANCE,    "leaderdistance"    )       // == 19
DEFINE_VARIABLE( VARLEADERTURN,    "leaderturn"    )           // == 20
DEFINE_VARIABLE( VARGOTOX,    "gotox"    )                // == 21
DEFINE_VARIABLE( VARGOTOY,    "gotoy"    )                // == 22
DEFINE_VARIABLE( VARGOTODISTANCE,    "gotodistance"    )         // == 23
DEFINE_VARIABLE( VARTARGETTURNTO,    "targetturnto"    )         // == 24
DEFINE_VARIABLE( VARPASSAGE,    "passage"    )              // == 25
DEFINE_VARIABLE( VARWEIGHT,    "weight"    )               // == 26
DEFINE_VARIABLE( VARSELFALTITUDE,    "selfaltitude"    )         // == 27
DEFINE_VARIABLE( VARSELFID,    "selfid"    )               // == 28
DEFINE_VARIABLE( VARSELFHATEID,    "selfhateid"    )           // == 29
DEFINE_VARIABLE( VARSELFMANA,    "selfmana"    )             // == 30
DEFINE_VARIABLE( VARTARGETSTR,    "targetstr"    )            // == 31
DEFINE_VARIABLE( VARTARGETWIS,    "targetwis"    )            // == 32
DEFINE_VARIABLE( VARTARGETINT,    "targetint"    )            // == 33
DEFINE_VARIABLE( VARTARGETDEX,    "targetdex"    )            // == 34
DEFINE_VARIABLE( VARTARGETLIFE,    "target_life"    )           // == 35
DEFINE_VARIABLE( VARTARGETMANA,    "target_mana"    )           // == 36
DEFINE_VARIABLE( VARTARGETLEVEL,    "targetlevel"    )          // == 37
DEFINE_VARIABLE( VARTARGETSPEEDX,    "targetspeedx"    )         // == 38
DEFINE_VARIABLE( VARTARGETSPEEDY,    "targetspeedy"    )         // == 39
DEFINE_VARIABLE( VARTARGETSPEEDZ,    "targetspeedz"    )         // == 40
DEFINE_VARIABLE( VARSELFSPAWNX,    "selfspawnx"    )           // == 41
DEFINE_VARIABLE( VARSELFSPAWNY,    "selfspawny"    )           // == 42
DEFINE_VARIABLE( VARSELFSTATE,    "selfstate"    )            // == 43
DEFINE_VARIABLE( VARSELFSTR,    "selfstr"    )              // == 44
DEFINE_VARIABLE( VARSELFWIS,    "selfwis"    )              // == 45
DEFINE_VARIABLE( VARSELFINT,    "selfint"    )              // == 46
DEFINE_VARIABLE( VARSELFDEX,    "selfdex"    )              // == 47
DEFINE_VARIABLE( VARSELFMANAFLOW,    "selfmanaflow"    )         // == 48
DEFINE_VARIABLE( VARTARGETMANAFLOW,    "targetmanaflow"    )       // == 49
DEFINE_VARIABLE( VARSELFATTACHED,    "selfattached"    )         // == 50
DEFINE_VARIABLE( VARSWINGTURN,    "swingturn"    )            // == 51
DEFINE_VARIABLE( VARXYDISTANCE,    "xydistance"    )           // == 52
DEFINE_VARIABLE( VARSELFZ,    "selfz"    )                // == 53
DEFINE_VARIABLE( VARTARGETALTITUDE,    "targetaltitude"    )       // == 54
DEFINE_VARIABLE( VARTARGETZ,    "targetz"    )              // == 55
DEFINE_VARIABLE( VARSELFINDEX,    "selfindex"    )            // == 56
DEFINE_VARIABLE( VAROWNERX,    "ownerx"    )               // == 57
DEFINE_VARIABLE( VAROWNERY,    "ownery"    )               // == 58
DEFINE_VARIABLE( VAROWNERTURN,    "ownerturn"    )            // == 59
DEFINE_VARIABLE( VAROWNERDISTANCE,    "ownerdistance"    )        // == 60
DEFINE_VARIABLE( VAROWNERTURNTO,    "ownerturnto"    )          // == 61
DEFINE_VARIABLE( VARXYTURNTO,    "xyturnto"    )             // == 62
DEFINE_VARIABLE( VARSELFMONEY,    "selfmoney"    )            // == 63
DEFINE_VARIABLE( VARSELFACCEL,    "selfaccel"    )            // == 64
DEFINE_VARIABLE( VARTARGETEXP,    "targetexp"    )            // == 65
DEFINE_VARIABLE( VARSELFAMMO,    "selfammo"    )             // == 66
DEFINE_VARIABLE( VARTARGETAMMO,    "targetammo"    )           // == 67
DEFINE_VARIABLE( VARTARGETMONEY,    "targetmoney"    )          // == 68
DEFINE_VARIABLE( VARTARGETTURNAWAY,    "targetturnfrom"    )       // == 69
DEFINE_VARIABLE( VARSELFLEVEL,    "selflevel"    )            // == 70
DEFINE_VARIABLE( VARTARGETRELOADTIME,    "targetreloadtime"    )     // == 71
DEFINE_VARIABLE( VARSELFCONTENT,    "selfcontent"    )          // == 72
DEFINE_VARIABLE( VARSPAWNDISTANCE,    "spawndistance"    )        // == 73
DEFINE_VARIABLE( VARTARGETMAXLIFE,    "targetmaxlife"    )        // == 74
DEFINE_VARIABLE( VARTARGETTEAM,    "targetteam"    )           // == 75
DEFINE_VARIABLE( VARTARGETARMOR,    "targetarmor"    )          // == 76
DEFINE_VARIABLE( VARDIFFICULTY        // == 77,    "difficulty"    )    //

DEFINE_OPERATOR( OPADD,    "+”    )    //
DEFINE_OPERATOR( OPSUB,    "-"    )    //
DEFINE_OPERATOR( OPAND,    "&"    )    //
DEFINE_OPERATOR( OPSHR,    ">"    )    //
DEFINE_OPERATOR( OPSHL,    "<"    )    //
DEFINE_OPERATOR( OPMUL,    "*"    )    //
DEFINE_OPERATOR( OPDIV,    "/"    )    //
DEFINE_OPERATOR( OPMOD     "%"    )    //

// Aliases
DEFINE_FUNCTION( FIFATLASTWAYPOINT,              IfPutAway    )
DEFINE_FUNCTION( FSETTARGETTOWHOEVERATTACKED,    SetTargetToWhoeverHealed    )
DEFINE_FUNCTION( FIFGRABBED,    IfMounted    )
DEFINE_FUNCTION( FIFDROPPED,    IfDismounted    )
DEFINE_FUNCTION( FIFXISLESSTHANY,    IfYIsMoreThanX    )
DEFINE_FUNCTION( FIFSITTING,    IfHeld    )
DEFINE_FUNCTION( FIFYISLESSTHANX,    IfXIsMoreThanY    )
DEFINE_FUNCTION( FIFSTATEIS0,    IfStateIsParry    )
DEFINE_FUNCTION( FIFSTATEIS1,    IfStateIsWander    )
DEFINE_FUNCTION( FIFSTATEIS2,    IfStateIsGuard    )
DEFINE_FUNCTION( FIFSTATEIS3,    IfStateIsFollow    )
DEFINE_FUNCTION( FIFSTATEIS4,    IfStateIsSurround    )
DEFINE_FUNCTION( FIFSTATEIS5,    IfStateIsRetreat    )
DEFINE_FUNCTION( FIFSTATEIS6,    IfStateIsCharge    )
DEFINE_FUNCTION( FIFSTATEIS7,    IfStateIsCombat    )
DEFINE_FUNCTION( FIFXISEQUALTOY,    IfYIsEqualToX    )
DEFINE_FUNCTION( FIFNOTPUTAWAY,    IfNotTakenOut    )
*/

//--------------------------------------------------------------------------------------------
// int load_parsed_line( int read )
// {
//   /// @details ZZ@> This function loads a line into the line buffer
//
//   char cTmp;

//   // Parse to start to maintain indentation
//   iLineSize = 0;
//   cTmp = cLoadBuffer[read];

//   while ( CSTR_END != cTmp )
//   {
//       cLineBuffer[iLineSize] = cTmp;  iLineSize++;
//       read++;  cTmp = cLoadBuffer[read];
//   }

//   cLineBuffer[iLineSize] = CSTR_END;
//   read++; // skip terminating zero for next call of load_parsed_line()
//   return read;
// }

//--------------------------------------------------------------------------------------------
// void parse_null_terminate_comments()
// {
//   /// @details ZZ@> This function removes comments and endline codes, replacing
//   ///    them with a 0
//
//   int read, write;

//   read = 0;
//   write = 0;

//   while ( read < iLoadSize )
//   {
//       read = load_one_line( read );

//       if ( iLineSize > 2 )
//       {
//           copy_one_line( write );
//           write += iLineSize;
//       }
//   }
// }