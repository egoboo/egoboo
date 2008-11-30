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
#define FIFSPAWNED                          0   // Scripted AI functions (v0.10)
#define FIFTIMEOUT                          1   //
#define FIFATWAYPOINT                       2   //
#define FIFATLASTWAYPOINT                   3   //
#define FIFATTACKED                         4   //
#define FIFBUMPED                           5   //
#define FIFORDERED                          6   //
#define FIFCALLEDFORHELP                    7   //
#define FSETCONTENT                         8   //
#define FIFKILLED                           9   //
#define FIFTARGETKILLED                     10  //
#define FCLEARWAYPOINTS                     11  //
#define FADDWAYPOINT                        12  //
#define FFINDPATH                           13  //
#define FCOMPASS                            14  //
#define FGETTARGETARMORPRICE                15  //
#define FSETTIME                            16  //
#define FGETCONTENT                         17  //
#define FJOINTARGETTEAM                     18  //
#define FSETTARGETTONEARBYENEMY             19  //
#define FSETTARGETTOTARGETLEFTHAND          20  //
#define FSETTARGETTOTARGETRIGHTHAND         21  //
#define FSETTARGETTOWHOEVERATTACKED         22  //
#define FSETTARGETTOWHOEVERBUMPED           23  //
#define FSETTARGETTOWHOEVERCALLEDFORHELP    24  //
#define FSETTARGETTOOLDTARGET               25  //
#define FSETTURNMODETOVELOCITY              26  //
#define FSETTURNMODETOWATCH                 27  //
#define FSETTURNMODETOSPIN                  28  //
#define FSETBUMPHEIGHT                      29  //
#define FIFTARGETHASID                      30  //
#define FIFTARGETHASITEMID                  31  //
#define FIFTARGETHOLDINGITEMID              32  //
#define FIFTARGETHASSKILLID                 33  //
#define FELSE                               34  //
#define FRUN                                35  //
#define FWALK                               36  //
#define FSNEAK                              37  //
#define FDOACTION                           38  //
#define FKEEPACTION                         39  //
#define FISSUEORDER                         40  //
#define FDROPWEAPONS                        41  //
#define FTARGETDOACTION                     42  //
#define FOPENPASSAGE                        43  //
#define FCLOSEPASSAGE                       44  //
#define FIFPASSAGEOPEN                      45  //
#define FGOPOOF                             46  //
#define FCOSTTARGETITEMID                   47  //
#define FDOACTIONOVERRIDE                   48  //
#define FIFHEALED                           49  //
#define FSENDMESSAGE                        50  //
#define FCALLFORHELP                        51  //
#define FADDIDSZ                            52  //
#define FEND                                53  //
#define FSETSTATE                           54  // Scripted AI functions (v0.20)
#define FGETSTATE                           55  //
#define FIFSTATEIS                          56  //
#define FIFTARGETCANOPENSTUFF               57  // Scripted AI functions (v0.30)
#define FIFGRABBED                          58  //
#define FIFDROPPED                          59  //
#define FSETTARGETTOWHOEVERISHOLDING        60  //
#define FDAMAGETARGET                       61  //
#define FIFXISLESSTHANY                     62  //
#define FSETWEATHERTIME                     63  // Scripted AI functions (v0.40)
#define FGETBUMPHEIGHT                      64  //
#define FIFREAFFIRMED                       65  //
#define FUNKEEPACTION                       66  //
#define FIFTARGETISONOTHERTEAM              67  //
#define FIFTARGETISONHATEDTEAM              68  // Scripted AI functions (v0.50)
#define FPRESSLATCHBUTTON                   69  //
#define FSETTARGETTOTARGETOFLEADER          70  //
#define FIFLEADERKILLED                     71  //
#define FBECOMELEADER                       72  //
#define FCHANGETARGETARMOR                  73  // Scripted AI functions (v0.60)
#define FGIVEMONEYTOTARGET                  74  //
#define FDROPKEYS                           75  //
#define FIFLEADERISALIVE                    76  //
#define FIFTARGETISOLDTARGET                77  //
#define FSETTARGETTOLEADER                  78  //
#define FSPAWNCHARACTER                     79  //
#define FRESPAWNCHARACTER                   80  //
#define FCHANGETILE                         81  //
#define FIFUSED                             82  //
#define FDROPMONEY                          83  //
#define FSETOLDTARGET                       84  //
#define FDETACHFROMHOLDER                   85  //
#define FIFTARGETHASVULNERABILITYID         86  //
#define FCLEANUP                            87  //
#define FIFCLEANEDUP                        88  //
#define FIFSITTING                          89  //
#define FIFTARGETISHURT                     90  //
#define FIFTARGETISAPLAYER                  91  //
#define FPLAYSOUND                          92  //
#define FSPAWNPARTICLE                      93  //
#define FIFTARGETISALIVE                    94  //
#define FSTOP                               95  //
#define FDISAFFIRMCHARACTER                 96  //
#define FREAFFIRMCHARACTER                  97  //
#define FIFTARGETISSELF                     98  //
#define FIFTARGETISMALE                     99  //
#define FIFTARGETISFEMALE                   100 //
#define FSETTARGETTOSELF                    101 // Scripted AI functions (v0.70)
#define FSETTARGETTORIDER                   102 //
#define FGETATTACKTURN                      103 //
#define FGETDAMAGETYPE                      104 //
#define FBECOMESPELL                        105 //
#define FBECOMESPELLBOOK                    106 //
#define FIFSCOREDAHIT                       107 //
#define FIFDISAFFIRMED                      108 //
#define FTRANSLATEORDER                     109 //
#define FSETTARGETTOWHOEVERWASHIT           110 //
#define FSETTARGETTOWIDEENEMY               111 //
#define FIFCHANGED                          112 //
#define FIFINWATER                          113 //
#define FIFBORED                            114 //
#define FIFTOOMUCHBAGGAGE                   115 //
#define FIFGROGGED                          116 //
#define FIFDAZED                            117 //
#define FIFTARGETHASSPECIALID               118 //
#define FPRESSTARGETLATCHBUTTON             119 //
#define FIFINVISIBLE                        120 //
#define FIFARMORIS                          121 //
#define FGETTARGETGROGTIME                  122 //
#define FGETTARGETDAZETIME                  123 //
#define FSETDAMAGETYPE                      124 //
#define FSETWATERLEVEL                      125 //
#define FENCHANTTARGET                      126 //
#define FENCHANTCHILD                       127 //
#define FTELEPORTTARGET                     128 //
#define FGIVEEXPERIENCETOTARGET             129 //
#define FINCREASEAMMO                       130 //
#define FUNKURSETARGET                      131 //
#define FGIVEEXPERIENCETOTARGETTEAM         132 //
#define FIFUNARMED                          133 //
#define FRESTOCKTARGETAMMOIDALL             134 //
#define FRESTOCKTARGETAMMOIDFIRST           135 //
#define FFLASHTARGET                        136 //
#define FSETREDSHIFT                        137 //
#define FSETGREENSHIFT                      138 //
#define FSETBLUESHIFT                       139 //
#define FSETLIGHT                           140 //
#define FSETALPHA                           141 //
#define FIFHITFROMBEHIND                    142 //
#define FIFHITFROMFRONT                     143 //
#define FIFHITFROMLEFT                      144 //
#define FIFHITFROMRIGHT                     145 //
#define FIFTARGETISONSAMETEAM               146 //
#define FKILLTARGET                         147 //
#define FUNDOENCHANT                        148 //
#define FGETWATERLEVEL                      149 //
#define FCOSTTARGETMANA                     150 //
#define FIFTARGETHASANYID                   151 //
#define FSETBUMPSIZE                        152 //
#define FIFNOTDROPPED                       153 //
#define FIFYISLESSTHANX                     154 //
#define FSETFLYHEIGHT                       155 //
#define FIFBLOCKED                          156 //
#define FIFTARGETISDEFENDING                157 //
#define FIFTARGETISATTACKING                158 //
#define FIFSTATEIS0                         159 //
#define FIFSTATEIS1                         160 //
#define FIFSTATEIS2                         161 //
#define FIFSTATEIS3                         162 //
#define FIFSTATEIS4                         163 //
#define FIFSTATEIS5                         164 //
#define FIFSTATEIS6                         165 //
#define FIFSTATEIS7                         166 //
#define FIFCONTENTIS                        167 //
#define FSETTURNMODETOWATCHTARGET           168 //
#define FIFSTATEISNOT                       169 //
#define FIFXISEQUALTOY                      170 //
#define FDEBUGMESSAGE                       171 //
#define FBLACKTARGET                        172 // Scripted AI functions (v0.80)
#define FSENDMESSAGENEAR                    173 //
#define FIFHITGROUND                        174 //
#define FIFNAMEISKNOWN                      175 //
#define FIFUSAGEISKNOWN                     176 //
#define FIFHOLDINGITEMID                    177 //
#define FIFHOLDINGRANGEDWEAPON              178 //
#define FIFHOLDINGMELEEWEAPON               179 //
#define FIFHOLDINGSHIELD                    180 //
#define FIFKURSED                           181 //
#define FIFTARGETISKURSED                   182 //
#define FIFTARGETISDRESSEDUP                183 //
#define FIFOVERWATER                        184 //
#define FIFTHROWN                           185 //
#define FMAKENAMEKNOWN                      186 //
#define FMAKEUSAGEKNOWN                     187 //
#define FSTOPTARGETMOVEMENT                 188 //
#define FSETXY                              189 //
#define FGETXY                              190 //
#define FADDXY                              191 //
#define FMAKEAMMOKNOWN                      192 //
#define FSPAWNATTACHEDPARTICLE              193 //
#define FSPAWNEXACTPARTICLE                 194 //
#define FACCELERATETARGET                   195 //
#define FIFDISTANCEISMORETHANTURN           196 //
#define FIFCRUSHED                          197 //
#define FMAKECRUSHVALID                     198 //
#define FSETTARGETTOLOWESTTARGET            199 //
#define FIFNOTPUTAWAY                       200 //
#define FIFTAKENOUT                         201 //
#define FIFAMMOOUT                          202 //
#define FPLAYSOUNDLOOPED                    203 //
#define FSTOPSOUND                          204 //
#define FHEALSELF                           205 //
#define FEQUIP                              206 //
#define FIFTARGETHASITEMIDEQUIPPED          207 //
#define FSETOWNERTOTARGET                   208 //
#define FSETTARGETTOOWNER                   209 //
#define FSETFRAME                           210 //
#define FBREAKPASSAGE                       211 //
#define FSETRELOADTIME                      212 //
#define FSETTARGETTOWIDEBLAHID              213 //
#define FPOOFTARGET                         214 //
#define FCHILDDOACTIONOVERRIDE              215 //
#define FSPAWNPOOF                          216 //
#define FSETSPEEDPERCENT                    217 //
#define FSETCHILDSTATE                      218 //
#define FSPAWNATTACHEDSIZEDPARTICLE         219 //
#define FCHANGEARMOR                        220 //
#define FSHOWTIMER                          221 //
#define FIFFACINGTARGET                     222 //
#define FPLAYSOUNDVOLUME                    223 //
#define FSPAWNATTACHEDFACEDPARTICLE         224 //
#define FIFSTATEISODD                       225 //
#define FSETTARGETTODISTANTENEMY            226 //
#define FTELEPORT                           227 //
#define FGIVESTRENGTHTOTARGET               228 //
#define FGIVEWISDOMTOTARGET                 229 //
#define FGIVEINTELLIGENCETOTARGET           230 //
#define FGIVEDEXTERITYTOTARGET              231 //
#define FGIVELIFETOTARGET                   232 //
#define FGIVEMANATOTARGET                   233 //
#define FSHOWMAP                            234 //
#define FSHOWYOUAREHERE                     235 //
#define FSHOWBLIPXY                         236 //
#define FHEALTARGET                         237 //
#define FPUMPTARGET                         238 //
#define FCOSTAMMO                           239 //
#define FMAKESIMILARNAMESKNOWN              240 //
#define FSPAWNATTACHEDHOLDERPARTICLE        241 //
#define FSETTARGETRELOADTIME                242 //
#define FSETFOGLEVEL                        243 //
#define FGETFOGLEVEL                        244 //
#define FSETFOGTAD                          245 //
#define FSETFOGBOTTOMLEVEL                  246 //
#define FGETFOGBOTTOMLEVEL                  247 //
#define FCORRECTACTIONFORHAND               248 //
#define FIFTARGETISMOUNTED                  249 //
#define FSPARKLEICON                        250 //
#define FUNSPARKLEICON                      251 //
#define FGETTILEXY                          252 //
#define FSETTILEXY                          253 //
#define FSETSHADOWSIZE                      254 //
#define FORDERTARGET                        255 //
#define FSETTARGETTOWHOEVERISINPASSAGE      256 //
#define FIFCHARACTERWASABOOK                257 //
#define FSETENCHANTBOOSTVALUES              258 // Scripted AI functions (v0.90)
#define FSPAWNCHARACTERXYZ                  259 //
#define FSPAWNEXACTCHARACTERXYZ             260 //
#define FCHANGETARGETCLASS                  261 //
#define FPLAYFULLSOUND                      262 //
#define FSPAWNEXACTCHASEPARTICLE            263 //
#define FCREATEORDER                        264 //
#define FORDERSPECIALID                     265 //
#define FUNKURSETARGETINVENTORY             266 //
#define FIFTARGETISSNEAKING                 267 //
#define FDROPITEMS                          268 //
#define FRESPAWNTARGET                      269 //
#define FTARGETDOACTIONSETFRAME             270 //
#define FIFTARGETCANSEEINVISIBLE            271 //
#define FSETTARGETTONEARESTBLAHID           272 //
#define FSETTARGETTONEARESTENEMY            273 //
#define FSETTARGETTONEARESTFRIEND           274 //
#define FSETTARGETTONEARESTLIFEFORM         275 //
#define FFLASHPASSAGE                       276 //
#define FFINDTILEINPASSAGE                  277 //
#define FIFHELDINLEFTHAND                   278 //
#define FNOTANITEM                          279 //
#define FSETCHILDAMMO                       280 //
#define FIFHITVULNERABLE                    281 //
#define FIFTARGETISFLYING                   282 //
#define FIDENTIFYTARGET                     283 //
#define FBEATMODULE                         284 //
#define FENDMODULE                          285 //
#define FDISABLEEXPORT                      286 //
#define FENABLEEXPORT                       287 //
#define FGETTARGETSTATE                     288 //
#define FIFEQUIPPED                         289 // Redone in v 0.95f
#define FDROPTARGETMONEY                    290 //
#define FGETTARGETCONTENT                   291 //
#define FDROPTARGETKEYS                     292 //
#define FJOINTEAM							293 //
#define FTARGETJOINTEAM                     294 //
#define FCLEARMUSICPASSAGE                  295 // Below is original code again
#define FCLEARENDMESSAGE                    296 //
#define FADDENDMESSAGE                      297 //
#define FPLAYMUSIC                          298 //
#define FSETMUSICPASSAGE                    299 //
#define FMAKECRUSHINVALID                   300 //
#define FSTOPMUSIC                          301 //
#define FFLASHVARIABLE                      302 //
#define FACCELERATEUP                       303 //
#define FFLASHVARIABLEHEIGHT                304 //
#define FSETDAMAGETIME                      305 //
#define FIFSTATEIS8                         306 //
#define FIFSTATEIS9                         307 //
#define FIFSTATEIS10                        308 //
#define FIFSTATEIS11                        309 //
#define FIFSTATEIS12                        310 //
#define FIFSTATEIS13                        311 //
#define FIFSTATEIS14                        312 //
#define FIFSTATEIS15                        313 //
#define FIFTARGETISAMOUNT                   314 //
#define FIFTARGETISAPLATFORM                315 //
#define FADDSTAT                            316 //
#define FDISENCHANTTARGET                   317 //
#define FDISENCHANTALL                      318 //
#define FSETVOLUMENEARESTTEAMMATE           319 //
#define FADDSHOPPASSAGE                     320 //
#define FTARGETPAYFORARMOR                  321 //
#define FJOINEVILTEAM                       322 //
#define FJOINNULLTEAM                       323 //
#define FJOINGOODTEAM                       324 //
#define FPITSKILL                           325 //
#define FSETTARGETTOPASSAGEID               326 //
#define FMAKENAMEUNKNOWN                    327 //
#define FSPAWNEXACTPARTICLEENDSPAWN         328 //
#define FSPAWNPOOFSPEEDSPACINGDAMAGE        329 //
#define FGIVEEXPERIENCETOGOODTEAM           330 //
#define FDONOTHING                          331 // Scripted AI functions (v0.95)
#define FGROGTARGET                         332 //
#define FDAZETARGET                         333 //
#define FENABLERESPAWN                      334 //
#define FDISABLERESPAWN                     335 //
#define FIFHOLDERSCOREDAHIT					336 //
#define FIFHOLDERBLOCKED					337 //
#define FGETSKILLLEVEL						338 //
#define FIFTARGETHASNOTFULLMANA				339 //
#define FENABLELISTENSKILL					340 //
#define FSETTARGETTOLASTITEMUSED			341 //
#define FFOLLOWLINK							342 // Scripted AI functions (v1.00)
#define FIFOPERATORISLINUX					343 //
#define FIFTARGETISAWEAPON					344 //
#define FIFSOMEONEISSTEALING				345 //
#define FIFTARGETISARCANESPELL				346 //
#define FIFBACKSTABBED						347 //
#define FGETTARGETDAMAGETYPE				348 //
#define FADDQUEST							349 //
#define FBEATQUESTALLPLAYERS				350 //
#define FIFTARGETHASQUEST					351 //
#define FSETQUESTLEVEL						352 //
#define FADDQUESTALLPLAYERS					353 //


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
