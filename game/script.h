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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

// AI stuff
#define AISMAXCOMPILESIZE   (128*4096/4)            // For parsing AI scripts
#define MAXLINESIZE         1024                    //
#define MAXAI               129                     //
#define MAXCODE             1024                    // Number of lines in AICODES.TXT
#define MAXCODENAMESIZE     64                      //

#define MSGDISTANCE         2000                    // Range for SendMessageNear
#define PITNOSOUND          -256                    // Stop sound at bottom of pits...



// SCRIPT FUNCTIONS
enum e_AICODES
{
  FIFSPAWNED = 0                      ,  // = 0 Scripted AI functions (v0.10)
  FIFTIMEOUT                          ,  // = 1   
  FIFATWAYPOINT                       ,  // = 2   
  FIFATLASTWAYPOINT                   ,  // = 3   
  FIFATTACKED                         ,  // = 4   
  FIFBUMPED                           ,  // = 5   
  FIFORDERED                          ,  // = 6   
  FIFCALLEDFORHELP                    ,  // = 7   
  FSETCONTENT                         ,  // = 8   
  FIFKILLED                           ,  // = 9   
  FIFTARGETKILLED                     ,  // = 10  
  FCLEARWAYPOINTS                     ,  // = 11  
  FADDWAYPOINT                        ,  // = 12  
  FFINDPATH                           ,  // = 13  
  FCOMPASS                            ,  // = 14  
  FGETTARGETARMORPRICE                ,  // = 15  
  FSETTIME                            ,  // = 16  
  FGETCONTENT                         ,  // = 17  
  FJOINTARGETTEAM                     ,  // = 18  
  FSETTARGETTONEARBYENEMY             ,  // = 19  
  FSETTARGETTOTARGETLEFTHAND          ,  // = 20  
  FSETTARGETTOTARGETRIGHTHAND         ,  // = 21  
  FSETTARGETTOWHOEVERATTACKED         ,  // = 22  
  FSETTARGETTOWHOEVERBUMPED           ,  // = 23  
  FSETTARGETTOWHOEVERCALLEDFORHELP    ,  // = 24  
  FSETTARGETTOOLDTARGET               ,  // = 25  
  FSETTURNMODETOVELOCITY              ,  // = 26  
  FSETTURNMODETOWATCH                 ,  // = 27  
  FSETTURNMODETOSPIN                  ,  // = 28  
  FSETBUMPHEIGHT                      ,  // = 29  
  FIFTARGETHASID                      ,  // = 30  
  FIFTARGETHASITEMID                  ,  // = 31  
  FIFTARGETHOLDINGITEMID              ,  // = 32  
  FIFTARGETHASSKILLID                 ,  // = 33  
  FELSE                               ,  // = 34  
  FRUN                                ,  // = 35  
  FWALK                               ,  // = 36  
  FSNEAK                              ,  // = 37  
  FDOACTION                           ,  // = 38  
  FKEEPACTION                         ,  // = 39  
  FISSUEORDER                         ,  // = 40  
  FDROPWEAPONS                        ,  // = 41  
  FTARGETDOACTION                     ,  // = 42  
  FOPENPASSAGE                        ,  // = 43  
  FCLOSEPASSAGE                       ,  // = 44  
  FIFPASSAGEOPEN                      ,  // = 45  
  FGOPOOF                             ,  // = 46  
  FCOSTTARGETITEMID                   ,  // = 47  
  FDOACTIONOVERRIDE                   ,  // = 48  
  FIFHEALED                           ,  // = 49  
  FSENDMESSAGE                        ,  // = 50  
  FCALLFORHELP                        ,  // = 51  
  FADDIDSZ                            ,  // = 52  
  FEND                                ,  // = 53  
  FSETSTATE                           ,  // = 54   Scripted AI functions (v0.20)
  FGETSTATE                           ,  // = 55  
  FIFSTATEIS                          ,  // = 56  
  FIFTARGETCANOPENSTUFF               ,  // = 57   Scripted AI functions (v0.30)
  FIFGRABBED                          ,  // = 58  
  FIFDROPPED                          ,  // = 59  
  FSETTARGETTOWHOEVERISHOLDING        ,  // = 60  
  FDAMAGETARGET                       ,  // = 61  
  FIFXISLESSTHANY                     ,  // = 62  
  FSETWEATHERTIME                     ,  // = 63   Scripted AI functions (v0.40)
  FGETBUMPHEIGHT                      ,  // = 64  
  FIFREAFFIRMED                       ,  // = 65  
  FUNKEEPACTION                       ,  // = 66  
  FIFTARGETISONOTHERTEAM              ,  // = 67  
  FIFTARGETISONHATEDTEAM              ,  // = 68   Scripted AI functions (v0.50)
  FPRESSLATCHBUTTON                   ,  // = 69  
  FSETTARGETTOTARGETOFLEADER          ,  // = 70  
  FIFLEADERKILLED                     ,  // = 71  
  FBECOMELEADER                       ,  // = 72  
  FCHANGETARGETARMOR                  ,  // = 73   Scripted AI functions (v0.60)
  FGIVEMONEYTOTARGET                  ,  // = 74  
  FDROPKEYS                           ,  // = 75  
  FIFLEADERISALIVE                    ,  // = 76  
  FIFTARGETISOLDTARGET                ,  // = 77  
  FSETTARGETTOLEADER                  ,  // = 78  
  FSPAWNCHARACTER                     ,  // = 79  
  FRESPAWNCHARACTER                   ,  // = 80  
  FCHANGETILE                         ,  // = 81  
  FIFUSED                             ,  // = 82  
  FDROPMONEY                          ,  // = 83  
  FSETOLDTARGET                       ,  // = 84  
  FDETACHFROMHOLDER                   ,  // = 85  
  FIFTARGETHASVULNERABILITYID         ,  // = 86  
  FCLEANUP                            ,  // = 87  
  FIFCLEANEDUP                        ,  // = 88  
  FIFSITTING                          ,  // = 89  
  FIFTARGETISHURT                     ,  // = 90  
  FIFTARGETISAPLAYER                  ,  // = 91  
  FPLAYSOUND                          ,  // = 92  
  FSPAWNPARTICLE                      ,  // = 93  
  FIFTARGETISALIVE                    ,  // = 94  
  FSTOP                               ,  // = 95  
  FDISAFFIRMCHARACTER                 ,  // = 96  
  FREAFFIRMCHARACTER                  ,  // = 97  
  FIFTARGETISSELF                     ,  // = 98  
  FIFTARGETISMALE                     ,  // = 99  
  FIFTARGETISFEMALE                   ,  // = 100 
  FSETTARGETTOSELF                    ,  // = 101  Scripted AI functions (v0.70)
  FSETTARGETTORIDER                   ,  // = 102 
  FGETATTACKTURN                      ,  // = 103 
  FGETDAMAGETYPE                      ,  // = 104 
  FBECOMESPELL                        ,  // = 105 
  FBECOMESPELLBOOK                    ,  // = 106 
  FIFSCOREDAHIT                       ,  // = 107 
  FIFDISAFFIRMED                      ,  // = 108 
  FTRANSLATEORDER                     ,  // = 109 
  FSETTARGETTOWHOEVERWASHIT           ,  // = 110 
  FSETTARGETTOWIDEENEMY               ,  // = 111 
  FIFCHANGED                          ,  // = 112 
  FIFINWATER                          ,  // = 113 
  FIFBORED                            ,  // = 114 
  FIFTOOMUCHBAGGAGE                   ,  // = 115 
  FIFGROGGED                          ,  // = 116 
  FIFDAZED                            ,  // = 117 
  FIFTARGETHASSPECIALID               ,  // = 118 
  FPRESSTARGETLATCHBUTTON             ,  // = 119 
  FIFINVISIBLE                        ,  // = 120 
  FIFARMORIS                          ,  // = 121 
  FGETTARGETGROGTIME                  ,  // = 122 
  FGETTARGETDAZETIME                  ,  // = 123 
  FSETDAMAGETYPE                      ,  // = 124 
  FSETWATERLEVEL                      ,  // = 125 
  FENCHANTTARGET                      ,  // = 126 
  FENCHANTCHILD                       ,  // = 127 
  FTELEPORTTARGET                     ,  // = 128 
  FGIVEEXPERIENCETOTARGET             ,  // = 129 
  FINCREASEAMMO                       ,  // = 130 
  FUNKURSETARGET                      ,  // = 131 
  FGIVEEXPERIENCETOTARGETTEAM         ,  // = 132 
  FIFUNARMED                          ,  // = 133 
  FRESTOCKTARGETAMMOIDALL             ,  // = 134 
  FRESTOCKTARGETAMMOIDFIRST           ,  // = 135 
  FFLASHTARGET                        ,  // = 136 
  FSETREDSHIFT                        ,  // = 137 
  FSETGREENSHIFT                      ,  // = 138 
  FSETBLUESHIFT                       ,  // = 139 
  FSETLIGHT                           ,  // = 140 
  FSETALPHA                           ,  // = 141 
  FIFHITFROMBEHIND                    ,  // = 142 
  FIFHITFROMFRONT                     ,  // = 143 
  FIFHITFROMLEFT                      ,  // = 144 
  FIFHITFROMRIGHT                     ,  // = 145 
  FIFTARGETISONSAMETEAM               ,  // = 146 
  FKILLTARGET                         ,  // = 147 
  FUNDOENCHANT                        ,  // = 148 
  FGETWATERLEVEL                      ,  // = 149 
  FCOSTTARGETMANA                     ,  // = 150 
  FIFTARGETHASANYID                   ,  // = 151 
  FSETBUMPSIZE                        ,  // = 152 
  FIFNOTDROPPED                       ,  // = 153 
  FIFYISLESSTHANX                     ,  // = 154 
  FSETFLYHEIGHT                       ,  // = 155 
  FIFBLOCKED                          ,  // = 156 
  FIFTARGETISDEFENDING                ,  // = 157 
  FIFTARGETISATTACKING                ,  // = 158 
  FIFSTATEIS0                         ,  // = 159 
  FIFSTATEIS1                         ,  // = 160 
  FIFSTATEIS2                         ,  // = 161 
  FIFSTATEIS3                         ,  // = 162 
  FIFSTATEIS4                         ,  // = 163 
  FIFSTATEIS5                         ,  // = 164 
  FIFSTATEIS6                         ,  // = 165 
  FIFSTATEIS7                         ,  // = 166 
  FIFCONTENTIS                        ,  // = 167 
  FSETTURNMODETOWATCHTARGET           ,  // = 168 
  FIFSTATEISNOT                       ,  // = 169 
  FIFXISEQUALTOY                      ,  // = 170 
  FDEBUGMESSAGE                       ,  // = 171 
  FBLACKTARGET                        ,  // = 172  Scripted AI functions (v0.80)
  FSENDMESSAGENEAR                    ,  // = 173 
  FIFHITGROUND                        ,  // = 174 
  FIFNAMEISKNOWN                      ,  // = 175 
  FIFUSAGEISKNOWN                     ,  // = 176 
  FIFHOLDINGITEMID                    ,  // = 177 
  FIFHOLDINGRANGEDWEAPON              ,  // = 178 
  FIFHOLDINGMELEEWEAPON               ,  // = 179 
  FIFHOLDINGSHIELD                    ,  // = 180 
  FIFKURSED                           ,  // = 181 
  FIFTARGETISKURSED                   ,  // = 182 
  FIFTARGETISDRESSEDUP                ,  // = 183 
  FIFOVERWATER                        ,  // = 184 
  FIFTHROWN                           ,  // = 185 
  FMAKENAMEKNOWN                      ,  // = 186 
  FMAKEUSAGEKNOWN                     ,  // = 187 
  FSTOPTARGETMOVEMENT                 ,  // = 188 
  FSETXY                              ,  // = 189 
  FGETXY                              ,  // = 190 
  FADDXY                              ,  // = 191 
  FMAKEAMMOKNOWN                      ,  // = 192 
  FSPAWNATTACHEDPARTICLE              ,  // = 193 
  FSPAWNEXACTPARTICLE                 ,  // = 194 
  FACCELERATETARGET                   ,  // = 195 
  FIFDISTANCEISMORETHANTURN           ,  // = 196 
  FIFCRUSHED                          ,  // = 197 
  FMAKECRUSHVALID                     ,  // = 198 
  FSETTARGETTOLOWESTTARGET            ,  // = 199 
  FIFNOTPUTAWAY                       ,  // = 200 
  FIFTAKENOUT                         ,  // = 201 
  FIFAMMOOUT                          ,  // = 202 
  FPLAYSOUNDLOOPED                    ,  // = 203 
  FSTOPSOUND                          ,  // = 204 
  FHEALSELF                           ,  // = 205 
  FEQUIP                              ,  // = 206 
  FIFTARGETHASITEMIDEQUIPPED          ,  // = 207 
  FSETOWNERTOTARGET                   ,  // = 208 
  FSETTARGETTOOWNER                   ,  // = 209 
  FSETFRAME                           ,  // = 210 
  FBREAKPASSAGE                       ,  // = 211 
  FSETRELOADTIME                      ,  // = 212 
  FSETTARGETTOWIDEBLAHID              ,  // = 213 
  FPOOFTARGET                         ,  // = 214 
  FCHILDDOACTIONOVERRIDE              ,  // = 215 
  FSPAWNPOOF                          ,  // = 216 
  FSETSPEEDPERCENT                    ,  // = 217 
  FSETCHILDSTATE                      ,  // = 218 
  FSPAWNATTACHEDSIZEDPARTICLE         ,  // = 219 
  FCHANGEARMOR                        ,  // = 220 
  FSHOWTIMER                          ,  // = 221 
  FIFFACINGTARGET                     ,  // = 222 
  FPLAYSOUNDVOLUME                    ,  // = 223 
  FSPAWNATTACHEDFACEDPARTICLE         ,  // = 224 
  FIFSTATEISODD                       ,  // = 225 
  FSETTARGETTODISTANTENEMY            ,  // = 226 
  FTELEPORT                           ,  // = 227 
  FGIVESTRENGTHTOTARGET               ,  // = 228 
  FGIVEWISDOMTOTARGET                 ,  // = 229 
  FGIVEINTELLIGENCETOTARGET           ,  // = 230 
  FGIVEDEXTERITYTOTARGET              ,  // = 231 
  FGIVELIFETOTARGET                   ,  // = 232 
  FGIVEMANATOTARGET                   ,  // = 233 
  FSHOWMAP                            ,  // = 234 
  FSHOWYOUAREHERE                     ,  // = 235 
  FSHOWBLIPXY                         ,  // = 236 
  FHEALTARGET                         ,  // = 237 
  FPUMPTARGET                         ,  // = 238 
  FCOSTAMMO                           ,  // = 239 
  FMAKESIMILARNAMESKNOWN              ,  // = 240 
  FSPAWNATTACHEDHOLDERPARTICLE        ,  // = 241 
  FSETTARGETRELOADTIME                ,  // = 242 
  FSETFOGLEVEL                        ,  // = 243 
  FGETFOGLEVEL                        ,  // = 244 
  FSETFOGTAD                          ,  // = 245 
  FSETFOGBOTTOMLEVEL                  ,  // = 246 
  FGETFOGBOTTOMLEVEL                  ,  // = 247 
  FCORRECTACTIONFORHAND               ,  // = 248 
  FIFTARGETISMOUNTED                  ,  // = 249 
  FSPARKLEICON                        ,  // = 250 
  FUNSPARKLEICON                      ,  // = 251 
  FGETTILEXY                          ,  // = 252 
  FSETTILEXY                          ,  // = 253 
  FSETSHADOWSIZE                      ,  // = 254 
  FORDERTARGET                        ,  // = 255 
  FSETTARGETTOWHOEVERISINPASSAGE      ,  // = 256 
  FIFCHARACTERWASABOOK                ,  // = 257 
  FSETENCHANTBOOSTVALUES              ,  // = 258  Scripted AI functions (v0.90)
  FSPAWNCHARACTERXYZ                  ,  // = 259 
  FSPAWNEXACTCHARACTERXYZ             ,  // = 260 
  FCHANGETARGETCLASS                  ,  // = 261 
  FPLAYFULLSOUND                      ,  // = 262 
  FSPAWNEXACTCHASEPARTICLE            ,  // = 263 
  FCREATEORDER                        ,  // = 264 
  FORDERSPECIALID                     ,  // = 265 
  FUNKURSETARGETINVENTORY             ,  // = 266 
  FIFTARGETISSNEAKING                 ,  // = 267 
  FDROPITEMS                          ,  // = 268 
  FRESPAWNTARGET                      ,  // = 269 
  FTARGETDOACTIONSETFRAME             ,  // = 270 
  FIFTARGETCANSEEINVISIBLE            ,  // = 271 
  FSETTARGETTONEARESTBLAHID           ,  // = 272 
  FSETTARGETTONEARESTENEMY            ,  // = 273 
  FSETTARGETTONEARESTFRIEND           ,  // = 274 
  FSETTARGETTONEARESTLIFEFORM         ,  // = 275 
  FFLASHPASSAGE                       ,  // = 276 
  FFINDTILEINPASSAGE                  ,  // = 277 
  FIFHELDINLEFTHAND                   ,  // = 278 
  FNOTANITEM                          ,  // = 279 
  FSETCHILDAMMO                       ,  // = 280 
  FIFHITVULNERABLE                    ,  // = 281 
  FIFTARGETISFLYING                   ,  // = 282 
  FIDENTIFYTARGET                     ,  // = 283 
  FBEATMODULE                         ,  // = 284 
  FENDMODULE                          ,  // = 285 
  FDISABLEEXPORT                      ,  // = 286 
  FENABLEEXPORT                       ,  // = 287 
  FGETTARGETSTATE                     ,  // = 288 
  FIFEQUIPPED                         ,  // = 289  Redone in v 0.95f
  FDROPTARGETMONEY                    ,  // = 290 
  FGETTARGETCONTENT                   ,  // = 291 
  FDROPTARGETKEYS                     ,  // = 292 
  FJOINTEAM                           ,  // = 293 
  FTARGETJOINTEAM                     ,  // = 294 
  FCLEARMUSICPASSAGE                  ,  // = 295  Below is original code again
  FCLEARENDMESSAGE                    ,  // = 296 
  FADDENDMESSAGE                      ,  // = 297 
  FPLAYMUSIC                          ,  // = 298 
  FSETMUSICPASSAGE                    ,  // = 299 
  FMAKECRUSHINVALID                   ,  // = 300 
  FSTOPMUSIC                          ,  // = 301 
  FFLASHVARIABLE                      ,  // = 302 
  FACCELERATEUP                       ,  // = 303 
  FFLASHVARIABLEHEIGHT                ,  // = 304 
  FSETDAMAGETIME                      ,  // = 305 
  FIFSTATEIS8                         ,  // = 306 
  FIFSTATEIS9                         ,  // = 307 
  FIFSTATEIS10                        ,  // = 308 
  FIFSTATEIS11                        ,  // = 309 
  FIFSTATEIS12                        ,  // = 310 
  FIFSTATEIS13                        ,  // = 311 
  FIFSTATEIS14                        ,  // = 312 
  FIFSTATEIS15                        ,  // = 313 
  FIFTARGETISAMOUNT                   ,  // = 314 
  FIFTARGETISAPLATFORM                ,  // = 315 
  FADDSTAT                            ,  // = 316 
  FDISENCHANTTARGET                   ,  // = 317 
  FDISENCHANTALL                      ,  // = 318 
  FSETVOLUMENEARESTTEAMMATE           ,  // = 319 
  FADDSHOPPASSAGE                     ,  // = 320 
  FTARGETPAYFORARMOR                  ,  // = 321 
  FJOINEVILTEAM                       ,  // = 322 
  FJOINNULLTEAM                       ,  // = 323 
  FJOINGOODTEAM                       ,  // = 324 
  FPITSKILL                           ,  // = 325 
  FSETTARGETTOPASSAGEID               ,  // = 326 
  FMAKENAMEUNKNOWN                    ,  // = 327 
  FSPAWNEXACTPARTICLEENDSPAWN         ,  // = 328 
  FSPAWNPOOFSPEEDSPACINGDAMAGE        ,  // = 329 
  FGIVEEXPERIENCETOGOODTEAM           ,  // = 330 
  FDONOTHING                          ,  // = 331  Scripted AI functions (v0.95)
  FGROGTARGET                         ,  // = 332 
  FDAZETARGET                         ,  // = 333 
  FENABLERESPAWN                      ,  // = 334 
  FDISABLERESPAWN                     ,  // = 335 
  FIFHOLDERSCOREDAHIT                 ,  // = 336 
  FIFHOLDERBLOCKED                    ,  // = 337 
  FGETSKILLLEVEL                      ,  // = 338 
  FIFTARGETHASNOTFULLMANA             ,  // = 339 
  FENABLELISTENSKILL                  ,  // = 340 
  FSETTARGETTOLASTITEMUSED            ,  // = 341 
  FFOLLOWLINK                         ,  // = 342  Scripted AI functions (v1.00)
  FIFOPERATORISLINUX                  ,  // = 343 
  FIFTARGETISAWEAPON                  ,  // = 344 
  FIFSOMEONEISSTEALING                ,  // = 345 
  FIFTARGETISARCANESPELL              ,  // = 346 
  FIFBACKSTABBED                      ,  // = 347 
  FGETTARGETDAMAGETYPE                   // = 348 
};

typedef enum e_AICODES AICODES_t;

// OPERATORS
#define OPADD 0                // +
#define OPSUB 1                // -
#define OPAND 2                // &
#define OPSHR 3                // >
#define OPSHL 4                // <
#define OPMUL 5                // *
#define OPDIV 6                // /
#define OPMOD 7                // %


// VARIABLES
#define VARTMPX             0
#define VARTMPY             1
#define VARTMPDISTANCE      2
#define VARTMPTURN          3
#define VARTMPARGUMENT      4
#define VARRAND             5
#define VARSELFX            6
#define VARSELFY            7
#define VARSELFTURN         8
#define VARSELFCOUNTER      9
#define VARSELFORDER        10
#define VARSELFMORALE       11
#define VARSELFLIFE         12
#define VARTARGETX          13
#define VARTARGETY          14
#define VARTARGETDISTANCE   15
#define VARTARGETTURN       16
#define VARLEADERX          17
#define VARLEADERY          18
#define VARLEADERDISTANCE   19
#define VARLEADERTURN       20
#define VARGOTOX            21
#define VARGOTOY            22
#define VARGOTODISTANCE     23
#define VARTARGETTURNTO     24
#define VARPASSAGE          25
#define VARWEIGHT           26
#define VARSELFALTITUDE     27
#define VARSELFID           28
#define VARSELFHATEID       29
#define VARSELFMANA         30
#define VARTARGETSTR        31
#define VARTARGETWIS        32
#define VARTARGETINT        33
#define VARTARGETDEX        34
#define VARTARGETLIFE       35
#define VARTARGETMANA       36
#define VARTARGETLEVEL      37
#define VARTARGETSPEEDX     38
#define VARTARGETSPEEDY     39
#define VARTARGETSPEEDZ     40
#define VARSELFSPAWNX       41
#define VARSELFSPAWNY       42
#define VARSELFSTATE        43
#define VARSELFSTR          44
#define VARSELFWIS          45
#define VARSELFINT          46
#define VARSELFDEX          47
#define VARSELFMANAFLOW     48
#define VARTARGETMANAFLOW   49
#define VARSELFATTACHED     50
#define VARSWINGTURN        51
#define VARXYDISTANCE       52
#define VARSELFZ            53
#define VARTARGETALTITUDE   54
#define VARTARGETZ          55
#define VARSELFINDEX        56
#define VAROWNERX           57
#define VAROWNERY           58
#define VAROWNERTURN        59
#define VAROWNERDISTANCE    60
#define VAROWNERTURNTO      61
#define VARXYTURNTO         62
#define VARSELFMONEY        63
#define VARSELFACCEL        64
#define VARTARGETEXP        65
#define VARSELFAMMO         66
#define VARTARGETAMMO       67
#define VARTARGETMONEY      68
#define VARTARGETTURNAWAY   69
#define VARSELFLEVEL      70
#define VARTARGETRELOADTIME  71
