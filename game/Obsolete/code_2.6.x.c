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

// Contains code fragments that were permanently deleted from the codebase in version 2.6.x and later
// Keep them in this file in case Aaron thought of a solution that we are missing :)

#define NETREFRESH          1000                    // Every second
#define NONETWORK           numservice              //
#define MAXSEQUENCE         256                     // Number of tracks in sequence
#define WATCHMIN            .01                     //
#define PRTLEVELFIX         20                      // Fix for shooting over cliffs
#define PACKDELAY           25                      // Time before inventory rotate again
#define GRABDELAY           25                      // Time before grab again
#define NOSKINOVERRIDE      -1                      // For import
#define PITDEPTH            -30                     // Depth to kill character
#define PITNOSOUND          -256                    // Stop sound at bottom of pits...

#define EXPKEEP 0.85                                // Experience to keep when respawning
#define INVALID_TX_ID (~(GLuint)0)

#define EDGE                128                     // Camera bounds
#define FARTRACK            1200                    // For outside modules...
#define EDGETRACK           800                     // Camtrack bounds
#define MAXNUMINPACK        6                       // Max number of items to carry in pack

#define THROWFIX            30.0                    // To correct thrown velocities
#define MINTHROWVELOCITY    15.0                    //
#define MAXTHROWVELOCITY    45.0                    //


#define MINDAMAGE           256                     // Minimum damage for hurt animation
#define MSGDISTANCE         2000                    // Range for SendMessageNear
#define MANARETURNSHIFT     4                       //
#define HURTDAMAGE          (1*256)                 //

#define MAXTOTALMESSAGE     1024                    //
#define NULLICON            0                       // Empty hand image

#define RIPPLEAND           15                      // How often ripples spawn
#define RIPPLETOLERANCE     60                      // For deep water
#define SPLASHTOLERANCE     10                      //
#define CLOSETOLERANCE      2                       // For closing doors

 #define JUMPATTACKVEL       -2                      //
#define MAXSTOR             16                       // Storage data
#define STORAND             15                       //

//AI stuff
#define MAXWAY              8                       // Waypoints
#define WAYTHRESH           128                     // Threshold for reaching waypoint
#define AISMAXCOMPILESIZE   (128*4096/4)            // For parsing AI scripts
#define MAXLINESIZE         1024                    //
#define MAXAI               129                     //
#define MAXCODE             1024                    // Number of lines in AICODES.TXT
#define MAXCODENAMESIZE     64                      //

#define SPINRATE            200                     // How fast spinners spin
 #define MAXLEVEL            6                       // Basic Levels 0-5
#define FLYDAMPEN           .001                    // Levelling rate for flyers

#define JUMPINFINITE        255                     // Flying character
#define JUMPTOLERANCE       20                      // Distance above ground to be jumping
#define SLIDETOLERANCE      10                      // Stick to ground better
#define PLATTOLERANCE       50 //5 //10             // Platform tolerance...
#define PLATADD             -10                     // Height add...
#define PLATASCEND          .10                     // Ascension rate
#define PLATKEEP            .90                     // Retention rate
#define DONTFLASH 255                               //

#define FOV                             60          // Field of view

#define CAMKEYTURN                      10          // Keyboard camera rotation
#define CAMJOYTURN                      .01         // Joystick camera rotation

// Multi cam
#define MINZOOM                         500         // Camera distance
#define MAXZOOM                         600         //
#define MINZADD                         800         // Camera height
#define MAXZADD                         1500  //1000        //
#define MINUPDOWN                       (.24*PI)    // Camera updown angle
#define MAXUPDOWN                       (.18*PI)//(.15*PI) // (.18*PI)


#define MAXVERTICES                     512    //128     // Max number of points in a model
#define MAXPARTICLEIMAGE                256         // Number of particle images ( frames )


#define STOPBOUNCING                    0.1 //1.0         // To make objects stop bouncing
#define STOPBOUNCINGPART                5.0         // To make particles stop bouncing
#define BORETIME                        (rand()&255)+120
#define CAREFULTIME                     50
#define REEL                            7600.0      // Dampen for melee knock back
#define REELBASE                        .35         //

/* PORT
LPVOID                  netlpconnectionbuffer[MAXSERVICE];          // Location of service info
LPGUID                  netlpsessionguid[MAXSESSION];               // GUID for joining
DPID                    netplayerid[MAXNETPLAYER];                  // Player ID
DPID                    selfid;                                     // Player ID
*/

// The ID number for host searches
// {A0F72DE8-2C17-11d3-B7FE-444553540000}
/* PORT
DEFINE_GUID(NETWORKID, 0xa0f72de8, 0x2c17, 0x11d3, 0xb7, 0xfe, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);
*/

/* PORT
GUID FAR* enum_id;                                  // Ben's Voodoo search
int enum_nonnull EQ(0);                                 //
char enum_desc[100];                                //
*/


EXTERN unsigned char           pitskill  EQ(bfalse);            // Do they kill?
EXTERN unsigned char parseerror  EQ(0);
 EXTERN bool_t                    rtscontrol;                 // Play as a real-time stragedy? BAD REMOVE
EXTERN bool_t                    allselect;                  // Select entire team at start?
#define TURNTIME 16
//PORT: Use sdlkeybuffer instead.
//EXTERN char                    keybuffer[256];             // Keyboard key states
//EXTERN char                    keypress[256];              // Keyboard new hits
//EXTERN	GLTexture	TxTrimX;									//OpenGL trim surface
//EXTERN	GLTexture	TxTrimY;									//OpenGL trim surface
EXTERN	GLTexture		TxTrim;
//EXTERN	GLTexture		TxTrim;
//EXTERN  GLTexture       TxTrimX;                                        /* trim */
//EXTERN  GLTexture       TxTrimY;                                        /* trim */
//#define TRIMX 640
//#define TRIMY 480
EXTERN rect_t                    trimrect;                   // The menu trim rectangle
#define SEEKURSEAND         31                      // Blacking flash
#define SEEINVISIBLE        128                     // Cutoff for invisible characters

// Particle template
#define DYNAOFF   0
#define DYNAON    1
#define DYNALOCAL 2
#define DYNAFANS  12
#define MAXFALLOFF 1400

//float           meshvrtx[MAXTOTALMESHVERTICES];                     // Vertex position
//float           meshvrty[MAXTOTALMESHVERTICES];                     //
//float           meshvrtz[MAXTOTALMESHVERTICES];                     // Vertex elevation
//unsigned char   meshvrta[MAXTOTALMESHVERTICES];                     // Vertex starting light
//unsigned char   meshvrtl[MAXTOTALMESHVERTICES];                     // Vertex light
#define ALERTIFSPAWNED                      1           // 0
#define ALERTIFHITVULNERABLE                2           // 1
#define ALERTIFATWAYPOINT                   4           // 2
#define ALERTIFATLASTWAYPOINT               8           // 3
#define ALERTIFATTACKED                     16          // 4
#define ALERTIFBUMPED                       32          // 5
#define ALERTIFORDERED                      64          // 6
#define ALERTIFCALLEDFORHELP                128         // 7
#define ALERTIFKILLED                       256         // 8
#define ALERTIFTARGETKILLED                 512         // 9
#define ALERTIFDROPPED                      1024        // 10
#define ALERTIFGRABBED                      2048        // 11
#define ALERTIFREAFFIRMED                   4096        // 12
#define ALERTIFLEADERKILLED                 8192        // 13
#define ALERTIFUSED                         16384       // 14
#define ALERTIFCLEANEDUP                    32768       // 15
#define ALERTIFSCOREDAHIT                   65536       // 16
#define ALERTIFHEALED                       131072      // 17
#define ALERTIFDISAFFIRMED                  262144      // 18
#define ALERTIFCHANGED                      524288      // 19
#define ALERTIFINWATER                      1048576     // 20
#define ALERTIFBORED                        2097152     // 21
#define ALERTIFTOOMUCHBAGGAGE               4194304     // 22
#define ALERTIFGROGGED                      8388608     // 23
#define ALERTIFDAZED                        16777216    // 24
#define ALERTIFHITGROUND                    33554432    // 25
#define ALERTIFNOTDROPPED                   67108864    // 26
#define ALERTIFBLOCKED                      134217728   // 27
#define ALERTIFTHROWN                       268435456   // 28
#define ALERTIFCRUSHED                      536870912   // 29
#define ALERTIFNOTPUTAWAY                   1073741824  // 30
#define ALERTIFTAKENOUT                     2147483648u // 31
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
#define FIFEQUIPPED                         289 //Redone in v 0.95
#define FDROPTARGETMONEY                    290 //
#define FGETTARGETCONTENT	                291 //
#define FDROPTARGETKEYS                     292 //
#define FJOINTEAM		                    293 //
#define FTARGETJOINTEAM	                    294 //
#define FCLEARMUSICPASSAGE                  295 //Below is original code again
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
#define FIFHOLDERSCOREDAHIT				    336 //
#define FIFHOLDERBLOCKED				    337 //
#define FGETSKILLLEVEL   				    338 //
#define FIFTARGETHASNOTFULLMANA   			339 //
#define FENABLELISTENSKILL   				340 //
#define FSETTARGETTOLASTITEMUSED   			341 //
#define FFOLLOWLINK				   			342 //

#define OPADD 0								// +
#define OPSUB 1								// -
#define OPAND 2								// &
#define OPSHR 3								// >
#define OPSHL 4								// <
#define OPMUL 5								// *
#define OPDIV 6								// /
#define OPMOD 7								// %

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
#define VARSELFLEVEL	    70
#define VARTARGETRELOADTIME	71

EXTERN unsigned short valueoldtarget EQ(0);
#define NOOWNER 65535
EXTERN Uint32 particletrans  EQ(0x80000000);
EXTERN Uint32 antialiastrans  EQ(0xC0000000);

#define VOLMIN          -4000						// Minumum Volume level
#ifndef ABS
 #define ABS(X)  (((X) > 0) ? (X) : -(X))
#endif
#ifndef MIN
#define MIN(x, y)  (((x) > (y)) ? (y) : (x))
#ifndef MAX
#define MAX(x, y)  (((x) > (y)) ? (x) : (y))
#endif



//--------------------------------------------------------------------------------------------
void flash_select()
{
   // ZZ> This function makes the selected characters blink
    int cnt;
    unsigned char value;

    if((wldframe&31)==0 && allselect==bfalse)
    {
        value = ((wldframe&32)<<3) - ((wldframe&32)>>5);
        cnt = 0;
        while(cnt < numrtsselect)
        {
            flash_character(rtsselect[cnt], value);
            cnt++;
        }
    }
}


//--------------------------------------------------------------------------------------------
//OBSOLETE
void general_error(int a, int b, char *szerrortext)
{
    // ZZ> This function displays an error message
    // Steinbach's Guideline for Systems Programming:
    //   Never test for an error condition you don't know how to handle.
    char                buf[256];
    FILE*               filewrite;
    sprintf(buf, "%d, %d... %s\n", 0, 0, szerrortext);

    fprintf(stderr,"ERROR: %s\n",szerrortext);

    filewrite = fopen("errorlog.txt", "w");
    if(filewrite)
    {
        fprintf(filewrite, "I'M MELTING\n");
        fprintf(filewrite, "%d, %d... %s\n", a, b, szerrortext);
        fclose(filewrite);
    }
    release_module();
    close_session();

    release_grfx();
    fclose(globalnetworkerr);
    //DestroyWindow(hWnd);

    SDL_Quit ();
    exit(0);
}


void send_rts_order(int x, int y, unsigned char order, unsigned char target)
{
	// ZZ> This function asks the host to order the selected characters
	unsigned int what, when, whichorder, cnt;

	if(numrtsselect > 0)
	{
		x = (x >> 6) & 1023;
		y = (y >> 6) & 1023;
		what = (target << 24) | (x << 14) | (y << 4) | (order&15);
		if(hostactive)
		{
			when = wldframe + orderlag;
			whichorder = get_empty_order();
			if(whichorder != MAXORDER)
			{
				// Add a new order on own machine
				orderwhen[whichorder] = when;
				orderwhat[whichorder] = what;
				cnt = 0;
				while(cnt < numrtsselect)
				{
					orderwho[whichorder][cnt] = rtsselect[cnt];
					cnt++;
				}
				while(cnt < MAXSELECT)
				{
					orderwho[whichorder][cnt] = MAXCHR;
					cnt++;
				}


				// Send the order off to everyone else
				if(networkon)
				{
					net_startNewPacket();
					packet_addUnsignedShort(TO_REMOTE_RTS);
					cnt = 0;
					while(cnt < MAXSELECT)
					{
						packet_addUnsignedByte(orderwho[whichorder][cnt]);
						cnt++;
					}
					packet_addUnsignedInt(what);
					packet_addUnsignedInt(when);
					net_sendPacketToAllPlayersGuaranteed();
				}
			}
		}
		else
		{
			// Send the order off to the host
			net_startNewPacket();
			packet_addUnsignedShort(TO_HOST_RTS);
			cnt = 0;
			while(cnt < numrtsselect)
			{
				packet_addUnsignedByte(rtsselect[cnt]);
				cnt++;
			}
			while(cnt < MAXSELECT)
			{
				packet_addUnsignedByte(MAXCHR);
				cnt++;
			}
			packet_addUnsignedInt(what);
			net_sendPacketToHostGuaranteed();
		}
	}
}

/* Old Menu Code */


//--------------------------------------------------------------------------------------------
void menu_service_select()
{
    // ZZ> This function lets the user choose a network service to use
    char text[256];
    int x, y;
    float open;
    int cnt;
    int stillchoosing;


    networkservice = NONETWORK;
    if(numservice > 0)
    {
        // Open a big window
        open = 0;
        while(open < 1.0)
        {
            //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    			glLoadIdentity();
            draw_trim_box_opening(0, 0, scrx, scry, open);
            draw_trim_box_opening(0, 0, 320, fontyspacing*(numservice+4), open);
            flip_pages();
            open += .030;
        }
        // Tell the user which ones we found ( in setup_network )
        stillchoosing = btrue;
        while(stillchoosing)
        {
            //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    			glLoadIdentity();
            draw_trim_box(0, 0, scrx, scry);
            draw_trim_box(0, 0, 320, fontyspacing*(numservice+4));
            y = 8;
            sprintf(text, "Network options...");
            draw_string(text, 14, y);
            y += fontyspacing;
            cnt = 0;
            while(cnt < numservice)
            {
                sprintf(text, "%s", netservicename[cnt]);
                draw_string(text, 50, y);
                y += fontyspacing;
                cnt++;
            }
            sprintf(text, "No Network");
            draw_string(text, 50, y);
            do_cursor();
            x = cursorx - 50;
            y = (cursory - 8 - fontyspacing);
            if(x > 0 && x < 300 && y >= 0)
            {
                y = y/fontyspacing;
                if(y <= numservice)
                {
                    if(mousebutton[0] || mousebutton[1])
                    {
                        stillchoosing = bfalse;
                        networkservice = y;
                    }
                }
            }
            flip_pages();
        }
    }
//    turn_on_service(networkservice);
}

//--------------------------------------------------------------------------------------------
void menu_start_or_join()
{
    // ZZ> This function lets the user start or join a game for a network game
    char text[256];
    int x, y;
    float open;
    int stillchoosing;


    // Open another window
    if(networkon)
    {
        open = 0;
        while(open < 1.0)
        {
			//clear_surface(lpDDSBack);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glLoadIdentity();
			draw_trim_box_opening(0, 0, scrx, scry, open);
			draw_trim_box_opening(0, 0, 280, 102, open);
			flip_pages();
			open += .030;
		}
        // Give the user some options
        stillchoosing = btrue;
        while(stillchoosing)
        {
			//clear_surface(lpDDSBack);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glLoadIdentity();
			draw_trim_box(0, 0, scrx, scry);
			draw_trim_box(0, 0, 280, 102);

			// Draw the menu text
			y = 8;
			sprintf(text, "Game options...");
			draw_string(text, 14, y);
			y += fontyspacing;
			sprintf(text, "New Game");
			draw_string(text, 50, y);
			y += fontyspacing;
			sprintf(text, "Join Game");
			draw_string(text, 50, y);
			y += fontyspacing;
			sprintf(text, "Quit Game");
			draw_string(text, 50, y);

			do_cursor();

//			sprintf(text, "Cursor position: %03d, %03d", cursorx, cursory);
//			draw_string(text, 14, 400);

			x = cursorx - 50;
			// The adjustments to y here were figured out empirically; I still
			// don't understand the reasoning behind it.  I don't think the text
			// draws where it says it's going to.
			y = (cursory - 21 - fontyspacing);



            if(x > 0 && x < 280 && y >= 0)
            {
                y = y/fontyspacing;
                if(y < 3)
                {
                    if(mousebutton[0] || mousebutton[1])
                    {
                        if(y == 0)
                        {
                            if(sv_hostGame())
                            {
                                hostactive = btrue;
                                nextmenu = MENUD;
                                stillchoosing = bfalse;
                            }
                        }
                        if(y == 1 && networkservice != NONETWORK)
                        {
                            nextmenu = MENUC;
                            stillchoosing = bfalse;
                        }
                        if(y == 2)
                        {
                            nextmenu = MENUB;
                            menuactive = bfalse;
                            stillchoosing = bfalse;
                            gameactive = bfalse;
                        }
                    }
                }
            }
            flip_pages();
        }
    }
    else
    {
        hostactive = btrue;
        nextmenu = MENUD;
    }
}

//--------------------------------------------------------------------------------------------
void draw_module_tag(int module, int y)
{
    // ZZ> This function draws a module tag
    char text[256];
    draw_trim_box(0, y, 136, y+136);
    draw_trim_box(132, y, scrx, y+136);
    if(module < globalnummodule)
    {
        draw_titleimage(module, 4, y+4);
        y+=6;
        sprintf(text, "%s", modlongname[module]);  draw_string(text, 150, y);  y+=fontyspacing;
        sprintf(text, "%s", modrank[module]);  draw_string(text, 150, y);  y+=fontyspacing;
        if(modmaxplayers[module] > 1)
        {
            if(modminplayers[module]==modmaxplayers[module])
            {
                sprintf(text, "%d players", modminplayers[module]);
            }
            else
            {
                sprintf(text, "%d-%d players", modminplayers[module], modmaxplayers[module]);
            }
        }
        else
        {
            sprintf(text, "1 player");
        }
        draw_string(text, 150, y);  y+=fontyspacing;
        if(modimportamount[module] == 0 && modallowexport[module]==bfalse)
        {
            sprintf(text, "No Import/Export");  draw_string(text, 150, y);  y+=fontyspacing;
        }
        else
        {
            if(modimportamount[module] == 0)
            {
                sprintf(text, "No Import");  draw_string(text, 150, y);  y+=fontyspacing;
            }
            if(modallowexport[module]==bfalse)
            {
                sprintf(text, "No Export");  draw_string(text, 150, y);  y+=fontyspacing;
            }
        }
        if(modrespawnvalid[module] == bfalse)
        {
            sprintf(text, "No Respawn");  draw_string(text, 150, y);  y+=fontyspacing;
        }
        if(modrtscontrol[module] == btrue)
        {
            sprintf(text, "RTS");  draw_string(text, 150, y);  y+=fontyspacing;
        }
        if(modrtscontrol[module] == ALLSELECT)
        {
            sprintf(text, "Diaboo RTS");  draw_string(text, 150, y);  y+=fontyspacing;
        }
    }
}

//--------------------------------------------------------------------------------------------
void menu_pick_player(int module)
{
    // ZZ> This function handles the display for picking players to import
    int x, y;
    float open;
    int cnt, tnc, start, numshow;
    int stillchoosing;
    int import;
    unsigned char control, sparkle;
    char fromdir[128];
    char todir[128];
	int clientFilesSent = 0;
	int hostFilesSent = 0;
	int pending;

    // Set the important flags
    respawnvalid = bfalse;
    respawnanytime = bfalse;
    if(modrespawnvalid[module])  respawnvalid = btrue;
    if(modrespawnvalid[module]==ANYTIME)  respawnanytime = btrue;
    rtscontrol = bfalse;
    if(modrtscontrol[module] != bfalse)
    {
        rtscontrol = btrue;
        allselect = bfalse;
        if(modrtscontrol[module] == ALLSELECT)
            allselect = btrue;
    }
    exportvalid = modallowexport[module];
    importvalid = (modimportamount[module] > 0);
    importamount = modimportamount[module];
    playeramount = modmaxplayers[module];
    fs_createDirectory("import");  // Just in case...


    start = 0;
    if(importvalid)
    {
        // Figure out which characters are available
        check_player_import("players");
        numshow = (scry-80-fontyspacing-fontyspacing)>>5;


        // Open some windows
        y = fontyspacing + 8;
        open = 0;
        while(open < 1.0)
        {
            //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    			glLoadIdentity();
            draw_trim_box_opening(0, 0, scrx, scry, open);
            draw_trim_box_opening(0, 0, scrx, 40, open);
            draw_trim_box_opening(0, scry-40, scrx, scry, open);
            flip_pages();
            open += .030;
        }


        wldframe = 0;  // For sparkle
        stillchoosing = btrue;
        while(stillchoosing)
        {
            // Draw the windows
            //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    			glLoadIdentity();
            draw_trim_box(0, 0, scrx, scry);
            draw_trim_box(0, 40, scrx, scry-40);

            // Draw the Up/Down buttons
            if(start == 0)
            {
                // Show the instructions
                x = (scrx-270)>>1;
                draw_string("Setup controls", x, 10);
            }
            else
            {
                x = (scrx-40)>>1;
                draw_string("Up", x, 10);
            }
            x = (scrx-80)>>1;
            draw_string("Down", x, scry-fontyspacing-20);


            // Draw each import character
            y = 40+fontyspacing;
            cnt = 0;
            while(cnt < numshow && cnt + start < numloadplayer)
            {
                sparkle = NOSPARKLE;
                if(keybplayer == (cnt+start))
                {
                    draw_one_icon(keybicon, 32, y, NOSPARKLE);
                    sparkle = 0;  // White
                }
                else
                    draw_one_icon(nullicon, 32, y, NOSPARKLE);
                if(mousplayer == (cnt+start))
                {
                    draw_one_icon(mousicon, 64, y, NOSPARKLE);
                    sparkle = 0;  // White
                }
                else
                    draw_one_icon(nullicon, 64, y, NOSPARKLE);
                if(joyaplayer == (cnt+start) && joyaon)
                {
                    draw_one_icon(joyaicon, 128, y, NOSPARKLE);
                    sparkle = 0;  // White
                }
                else
                    draw_one_icon(nullicon, 128, y, NOSPARKLE);
                if(joybplayer == (cnt+start) && joybon)
                {
                    draw_one_icon(joybicon, 160, y, NOSPARKLE);
                    sparkle = 0;  // White
                }
                else
                    draw_one_icon(nullicon, 160, y, NOSPARKLE);
                draw_one_icon((cnt+start), 96, y, sparkle);
                draw_string(loadplayername[cnt+start], 200, y+6);
                y+=32;
                cnt++;
            }
            wldframe++;  // For sparkle


            // Handle other stuff...
            do_cursor();
            if(pending_click)
            {
	        pending_click=bfalse;
                if(cursory < 40 && start > 0)
                {
                    // Up button
                    start--;
                }
                if(cursory >= (scry-40) && (start + numshow) < numloadplayer)
                {
                    // Down button
                    start++;
                }
            }
            if(mousebutton[0])
            {
                x = (cursorx - 32) >> 5;
                y = (cursory - 44) >> 5;
                if(y >= 0 && y < numshow)
                {
                    y += start;
                    // Assign the controls
                    if(y < numloadplayer)  // !!!BAD!!! do scroll
                    {
                        if(x == 0)  keybplayer = y;
                        if(x == 1)  mousplayer = y;
                        if(x == 3)  joyaplayer = y;
                        if(x == 4)  joybplayer = y;
                    }
                }
            }
            if(mousebutton[1])
            {
                // Done picking
                stillchoosing = bfalse;
            }
            flip_pages();
        }
        wldframe = 0;  // For sparkle


		// Tell the user we're loading
		y = fontyspacing + 8;
		open = 0;
		while(open < 1.0)
		{
			//clear_surface(lpDDSBack);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glLoadIdentity();
			draw_trim_box_opening(0, 0, scrx, scry, open);
			flip_pages();
			open += .030;
		}

		//clear_surface(lpDDSBack);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();
		draw_trim_box(0, 0, scrx, scry);
		draw_string("Copying the imports...", y, y);
		flip_pages();


        // Now build the import directory...
        empty_import_directory();
        cnt = 0;
        numimport = 0;
        while(cnt < numloadplayer)
        {
            if((cnt == keybplayer && keyon)   ||
               (cnt == mousplayer && mouseon) ||
               (cnt == joyaplayer && joyaon)  ||
               (cnt == joybplayer && joybon))
            {
                // This character has been selected
                control = INPUTNONE;
                if(cnt == keybplayer)  control = control | INPUTKEY;
                if(cnt == mousplayer)  control = control | INPUTMOUSE;
                if(cnt == joyaplayer)  control = control | INPUTJOYA;
                if(cnt == joybplayer)  control = control | INPUTJOYB;
                localcontrol[numimport] = control;
//                localslot[numimport] = (numimport+(localmachine*4))*9;
				localslot[numimport] = (numimport + localmachine) * 9;


                // Copy the character to the import directory
                sprintf(fromdir, "players/%s", loadplayerdir[cnt]);
                sprintf(todir, "import/temp%04d.obj", localslot[numimport]);

				// This will do a local copy if I'm already on the host machine, other
				// wise the directory gets sent across the network to the host
				net_copyDirectoryToHost(fromdir, todir);

				// Copy all of the character's items to the import directory
				tnc = 0;
				while(tnc < 8)
				{
					sprintf(fromdir, "players/%s/%d.obj", loadplayerdir[cnt], tnc);
					sprintf(todir, "import/temp%04d.obj", localslot[numimport]+tnc+1);

					net_copyDirectoryToHost(fromdir, todir);
					tnc++;
				}

                numimport++;
            }
            cnt++;
        }

		// Have clients wait until all files have been sent to the host
		clientFilesSent = net_pendingFileTransfers();
		if(networkon && !hostactive)
		{
			pending = net_pendingFileTransfers();

			// Let the host know how many files you're sending it
			net_startNewPacket();
			packet_addUnsignedShort(NET_NUM_FILES_TO_SEND);
			packet_addUnsignedShort((unsigned short)pending);
			net_sendPacketToHostGuaranteed();

			while(pending)
			{
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glLoadIdentity();

				draw_trim_box(0, 0, scrx, scry);
				y = fontyspacing + 8;

				sprintf(todir, "Sending file %d of %d...", clientFilesSent - pending, clientFilesSent);
				draw_string(todir, fontyspacing + 8, y);
				flip_pages();

				// do this to let SDL do it's window events stuff, so that windows doesn't think
				// the game has hung while transferring files
				do_cursor();

				net_updateFileTransfers();

				pending = net_pendingFileTransfers();
			}

			// Tell the host I'm done sending files
			net_startNewPacket();
			packet_addUnsignedShort(NET_DONE_SENDING_FILES);
			net_sendPacketToHostGuaranteed();
		}

		if(networkon)
		{
			if(hostactive)
			{
				// Host waits for all files from all remotes
				numfile = 0;
				numfileexpected = 0;
				numplayerrespond = 1;
				while(numplayerrespond < numplayer)
				{
					//clear_surface(lpDDSBack);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					glLoadIdentity();
					draw_trim_box(0, 0, scrx, scry);
					y = fontyspacing + 8;
					draw_string("Incoming files...", fontyspacing+8, y);  y+=fontyspacing;
					sprintf(todir, "File %d/%d", numfile, numfileexpected);
					draw_string(todir, fontyspacing+20, y); y+=fontyspacing;
					sprintf(todir, "Play %d/%d", numplayerrespond, numplayer);
					draw_string(todir, fontyspacing+20, y);
					flip_pages();

					listen_for_packets();

					do_cursor();

					if(SDLKEYDOWN(SDLK_ESCAPE))
					{
						gameactive = bfalse;
						menuactive = bfalse;
						close_session();
						break;
					}
				}


                // Say you're done
                //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    			glLoadIdentity();
                draw_trim_box(0, 0, scrx, scry);
                y = fontyspacing + 8;
                draw_string("Sending files to remotes...", fontyspacing+8, y);  y+=fontyspacing;
                flip_pages();


                // Host sends import directory to all remotes, deletes extras
                numfilesent = 0;
                import = 0;
                cnt = 0;
				if(numplayer > 1)
				{
					while(cnt < MAXIMPORT)
					{
						sprintf(todir, "import/temp%04d.obj", cnt);
						strncpy(fromdir, todir, 128);
						if(fs_fileIsDirectory(fromdir))
						{
							// Only do directories that actually exist
							if((cnt % 9)==0) import++;
							if(import > importamount)
							{
								// Too many directories
								fs_removeDirectoryAndContents(fromdir);
							}
							else
							{
								// Ship it out
								net_copyDirectoryToAllPlayers(fromdir, todir);
							}
						}
						cnt++;
					}

					hostFilesSent = net_pendingFileTransfers();
					pending = hostFilesSent;

					// Let the client know how many are coming
					net_startNewPacket();
					packet_addUnsignedShort(NET_NUM_FILES_TO_SEND);
					packet_addUnsignedShort((unsigned short)pending);
					net_sendPacketToAllPlayersGuaranteed();

					while(pending > 0)
					{
						glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
						glLoadIdentity();

						draw_trim_box(0, 0, scrx, scry);
						y = fontyspacing + 8;

						sprintf(todir, "Sending file %d of %d...", hostFilesSent - pending, hostFilesSent);
						draw_string(todir, fontyspacing + 8, y);
						flip_pages();

						// do this to let SDL do it's window events stuff, so that windows doesn't think
						// the game has hung while transferring files
						do_cursor();

						net_updateFileTransfers();

						pending = net_pendingFileTransfers();
					}

					// Tell the players I'm done sending files
					net_startNewPacket();
					packet_addUnsignedShort(NET_DONE_SENDING_FILES);
					net_sendPacketToAllPlayersGuaranteed();
				}
            }
            else
            {
				// Remotes wait for all files in import directory
				log_info("menu_pick_player: Waiting for files to come from the host...\n");
				numfile = 0;
				numfileexpected = 0;
				numplayerrespond = 0;
				while(numplayerrespond < 1)
				{
					//clear_surface(lpDDSBack);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					glLoadIdentity();
					draw_trim_box(0, 0, scrx, scry);
					y = fontyspacing + 8;
					draw_string("Incoming files from host...", fontyspacing+8, y);  y+=fontyspacing;
					sprintf(todir, "File %d/%d", numfile, numfileexpected);
					draw_string(todir, fontyspacing+20, y);
					flip_pages();

					listen_for_packets();
					do_cursor();

					if(SDLKEYDOWN(SDLK_ESCAPE))
					{
						gameactive = bfalse;
						menuactive = bfalse;
						break;
						close_session();
					}
                }
            }
        }
    }
    nextmenu = MENUG;
}

//--------------------------------------------------------------------------------------------
void menu_module_loading(int module)
{
    // ZZ> This function handles the display for when a module is loading
    char text[256];
    int y;
    float open;
    int cnt;


    // Open some windows
    y = fontyspacing + 8;
    open = 0;
    while(open < 1.0)
    {
        //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    			glLoadIdentity();
        draw_trim_box_opening(0, y, 136, y+136, open);
        draw_trim_box_opening(132, y, scrx, y+136, open);
        draw_trim_box_opening(0, y+132, scrx, scry, open);
        flip_pages();
        open += .030;
    }


    // Put the stuff in the windows
    //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    			glLoadIdentity();
    y = 0;
    sprintf(text, "Loading...  Wait!!!");  draw_string(text, 0, y);  y+=fontyspacing;
    y+=8;
    draw_module_tag(module, y);
    draw_trim_box(0, y+132, scrx, scry);


    // Show the summary
    sprintf(text, "modules/%s/gamedat/menu.txt", modloadname[module]);
    get_module_summary(text);
    y = fontyspacing+152;
    cnt = 0;
    while(cnt < SUMMARYLINES)
    {
        sprintf(text, "%s", modsummary[cnt]);  draw_string(text, 14, y);  y+=fontyspacing;
        cnt++;
    }
    flip_pages();
    nextmenu = MENUB;
    menuactive = bfalse;
}

//--------------------------------------------------------------------------------------------
void menu_join_multiplayer()
{
	// JF> This function attempts to join the multiplayer game hosted
	//     by whatever server is named in the HOST_NAME part of setup.txt
	char text[256];
	float open;

	if(networkon)
	{
		// Do the little opening menu animation
		open = 0;
		while(open < 1.0)
		{
			//clear_surface(lpDDSBack);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glLoadIdentity();
			draw_trim_box_opening(0, 0, scrx, scry, open);
			flip_pages();
			open += .030;
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();
		draw_trim_box(0, 0, scrx, scry);

		strncpy(text, "Attempting to join game at:", 256);
		draw_string(text, (scrx>>1)-240, (scry>>1)-fontyspacing);

		strncpy(text, nethostname, 256);
		draw_string(text, (scrx>>1)-240, (scry>>1));
		flip_pages();

		if(cl_joinGame(nethostname))
		{
			nextmenu = MENUE;
		} else
		{
			nextmenu = MENUB;
		}
	}
}

//--------------------------------------------------------------------------------------------
void menu_choose_host()
{
    // ZZ> This function lets the player choose a host
    char text[256];
    int x, y;
    float open;
    int cnt;
    int stillchoosing;


    if(networkon)
    {
        // Bring up a helper window
        open = 0;
        while(open < 1.0)
        {
            //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    			glLoadIdentity();
            draw_trim_box_opening(0, 0, scrx, scry, open);
            flip_pages();
            open += .030;
        }
        //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    			glLoadIdentity();
        draw_trim_box(0, 0, scrx, scry);
        sprintf(text, "Press Enter if");
        draw_string(text, (scrx>>1)-120, (scry>>1)-fontyspacing);
        sprintf(text, "nothing happens");
        draw_string(text, (scrx>>1)-120, (scry>>1));
        flip_pages();



        // Find available games
//        find_open_sessions();       // !!!BAD!!!  Do this every now and then

        // Open a big window
        open = 0;
        while(open < 1.0)
        {
            //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    			glLoadIdentity();
            draw_trim_box_opening(0, 0, scrx, scry, open);
            draw_trim_box_opening(0, 0, 320, fontyspacing*(numsession+4), open);
            flip_pages();
            open += .030;
        }

        // Tell the user which ones we found
        stillchoosing = btrue;
        while(stillchoosing)
        {
            //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    			glLoadIdentity();
            draw_trim_box(0, 0, scrx, scry);
            draw_trim_box(0, 0, 320, fontyspacing*(numsession+4));
            y = 8;
            sprintf(text, "Open hosts...");
            draw_string(text, 14, y);
            y += fontyspacing;
            cnt = 0;
            while(cnt < numsession)
            {
                sprintf(text, "%s", netsessionname[cnt]);
                draw_string(text, 50, y);
                y += fontyspacing;
                cnt++;
            }
            sprintf(text, "Go Back...");
            draw_string(text, 50, y);
            do_cursor();
            x = cursorx - 50;
            y = (cursory - 8 - fontyspacing);
            if(x > 0 && x < 300 && y >= 0)
            {
                y = y/fontyspacing;
                if(y <= numsession)
                {
                    if(mousebutton[0] || mousebutton[1])
                    {
                        //if(y == numsession)
                        //{
                        //    nextmenu = MENUB;
                        //    stillchoosing = bfalse;
                        //}
                        //else
                        {
                            if(cl_joinGame("solace2.csusm.edu"))
                            {
                                nextmenu = MENUE;
                                stillchoosing = bfalse;
                            }
                        }
                    }
                }
            }
            flip_pages();
        }
    }
    else
    {
        // This should never happen
        nextmenu = MENUB;
    }
}

//--------------------------------------------------------------------------------------------
void menu_choose_module()
{
    // ZZ> This function lets the host choose a module
    int numtag;
    char text[256];
    int x, y, ystt;
    float open;
    int cnt;
    int module;
    int stillchoosing;
    if(hostactive)
    {
        // Figure out how many tags to display
        numtag = (scry-4-40)/132;
        ystt = (scry-(numtag*132)-4)>>1;


        // Open the tag windows
        open = 0;
        while(open < 1.0)
        {
            //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    			glLoadIdentity();
            draw_trim_box_opening(0, 0, scrx, scry, open);
            y = ystt;
            cnt = 0;
            while(cnt < numtag)
            {
                draw_trim_box_opening(0, y, 136, y+136, open);
                draw_trim_box_opening(132, y, scrx, y+136, open);
                y+=132;
                cnt++;
            }
            flip_pages();
            open += .030;
        }




        // Let the user pick a module
        module = 0;
        stillchoosing = btrue;
        while(stillchoosing)
        {
            // Draw the tags
            //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		    	glLoadIdentity();
            draw_trim_box(0, 0, scrx, scry);
            y = ystt;
            cnt = 0;
            while(cnt < numtag)
            {
                draw_module_tag(module+cnt, y);
                y+=132;
                cnt++;
            }

            // Draw the Up/Down buttons
            sprintf(text, "Up");
            x = (scrx-40)>>1;
            draw_string(text, x, 10);
            sprintf(text, "Down");
            x = (scrx-80)>>1;
            draw_string(text, x, scry-fontyspacing-20);


            // Handle the mouse
            do_cursor();
            y = (cursory - ystt)/132;
            if(pending_click)
	      {
	        pending_click=bfalse;
                if(cursory < ystt && module > 0)
		  {
                    // Up button
                    module--;
		  }
                if(y >= numtag && module + numtag < globalnummodule)
		  {
                    // Down button
                    module++;
		  }
		if(cursory > ystt && y > -1 && y < numtag)
		  {
		    y = module + y;
		    if((mousebutton[0] || mousebutton[1]) && y < globalnummodule)
		      {
			// Set start infow
			playersready = 1;
			seed = time(0);
			pickedindex = y;
			sprintf(pickedmodule, "%s", modloadname[y]);
			readytostart = btrue;
			stillchoosing = bfalse;
		      }
		  }
	      }
            // Check for quitters
            if(SDLKEYDOWN(SDLK_ESCAPE) && networkservice == NONETWORK)
            {
                nextmenu = MENUB;
                menuactive = bfalse;
                stillchoosing = bfalse;
                gameactive = bfalse;
            }
            flip_pages();
        }
    }
    nextmenu = MENUE;
}

//--------------------------------------------------------------------------------------------
void menu_boot_players()
{
    // ZZ> This function shows all the active players and lets the host kick 'em out
    //     !!!BAD!!!  Let the host boot players
    char text[256];
    int x, y, starttime, time;
    float open;
    int cnt, player;
    int stillchoosing;


    numplayer = 1;
    if(networkon)
    {
        // Find players
        sv_letPlayersJoin();

        // Open a big window
        open = 0;
        while(open < 1.0)
        {
            //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    			glLoadIdentity();
            draw_trim_box_opening(0, 0, scrx, scry, open);
            draw_trim_box_opening(0, 0, 320, fontyspacing*(numplayer+4), open);
            flip_pages();
            open += .030;
        }

        // Tell the user which ones we found
        starttime = SDL_GetTicks();
        stillchoosing = btrue;
        while(stillchoosing)
        {
			time = SDL_GetTicks();
			if((time-starttime) > NETREFRESH)
			{
				sv_letPlayersJoin();
				starttime = time;
			}

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glLoadIdentity();
			draw_trim_box(0, 0, scrx, scry);
			draw_trim_box(0, 0, 320, fontyspacing*(numplayer+4));

			if(hostactive)
			{
				y = 8;
				sprintf(text, "Active machines...");
				draw_string(text, 14, y);
				y += fontyspacing;

				cnt = 0;
				while(cnt < numplayer)
				{
					sprintf(text, "%s", netplayername[cnt]);
					draw_string(text, 50, y);
					y += fontyspacing;
					cnt++;
				}

				sprintf(text, "Start Game");
                draw_string(text, 50, y);
			} else
			{
				strncpy(text, "Connected to host:", 256);
				draw_string(text, 14, 8);
				draw_string(nethostname, 14, 8 + fontyspacing);
				listen_for_packets();  // This happens implicitly for the host in sv_letPlayersJoin
			}

            do_cursor();
            x = cursorx - 50;
			// Again, y adjustments were figured out empirically in menu_start_or_join
            y = (cursory - 21 - fontyspacing);

            if(SDLKEYDOWN(SDLK_ESCAPE)) // !!!BAD!!!
            {
                nextmenu = MENUB;
                menuactive = bfalse;
                stillchoosing = bfalse;
                gameactive = bfalse;
            }
            if(x > 0 && x < 300 && y >= 0 && (mousebutton[0] || mousebutton[1]) && hostactive)
            {
                // Let the host do things
                y = y/fontyspacing;
                if(y < numplayer && hostactive)
                {
                    // Boot players
                }
                if(y == numplayer && readytostart)
                {
                    // Start the modules
                    stillchoosing = bfalse;
                }
            }
            if(readytostart && hostactive == bfalse)
            {
                // Remotes automatically start
                stillchoosing = bfalse;
            }
            flip_pages();
        }
    }
    if(networkon && hostactive)
    {
        // Let the host coordinate start
        stop_players_from_joining();
        sv_letPlayersJoin();
        cnt = 0;
        readytostart = bfalse;
        if(numplayer == 1)
        {
            // Don't need to bother, since the host is alone
            readytostart = btrue;
        }
        while(readytostart==bfalse)
        {
            //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    			glLoadIdentity();
            draw_trim_box(0, 0, scrx, scry);
            y = 8;
            sprintf(text, "Waiting for replies...");
            draw_string(text, 14, y);
            y += fontyspacing;
            do_cursor();
            if(SDLKEYDOWN(SDLK_ESCAPE)) // !!!BAD!!!
            {
                nextmenu = MENUB;
                menuactive = bfalse;
                stillchoosing = bfalse;
                gameactive = bfalse;
                readytostart = btrue;
            }
            if((cnt&63)==0)
            {
                sprintf(text, "  Lell...");
                draw_string(text, 14, y);
                player = 0;
                while(player < numplayer-1)
                {
                    net_startNewPacket();
                    packet_addUnsignedShort(TO_REMOTE_MODULE);
                    packet_addUnsignedInt(seed);
                    packet_addUnsignedByte(player+1);
                    packet_addString(pickedmodule);
//                    send_packet_to_all_players();
                    net_sendPacketToOnePlayerGuaranteed(player);
                    player++;
                }
            }
            listen_for_packets();
            cnt++;
            flip_pages();
        }
    }


    nextmenu=MENUF;
}

//--------------------------------------------------------------------------------------------
void menu_end_text()
{
    // ZZ> This function gives the player the ending text
    float open;
    int stillchoosing;
//    SDL_Event ev;


    // Open the text window
    open = 0;
    while(open < 1.0)
    {
        //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    			glLoadIdentity();
        draw_trim_box_opening(0, 0, scrx, scry, open);
        flip_pages();
        open += .030;
    }



    // Wait for input
    stillchoosing = btrue;
    while(stillchoosing)
    {
        // Show the text
        //clear_surface(lpDDSBack);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    			glLoadIdentity();
        draw_trim_box(0, 0, scrx, scry);
        draw_wrap_string(endtext, 14, 8, scrx-40);



        // Handle the mouse
        do_cursor();
        if(pending_click || SDLKEYDOWN(SDLK_ESCAPE))
        {
	    pending_click = bfalse;
            stillchoosing = bfalse;
        }
        flip_pages();
    }
    nextmenu = MENUB;
}

//--------------------------------------------------------------------------------------------
void menu_initial_text()
{
    // ZZ> This function gives the player the initial title screen
    float open;
    char text[1024];
    int stillchoosing;


    //fprintf(stderr,"DIAG: In menu_initial_text()\n");
    //draw_trim_box(0, 0, scrx, scry);//draw_trim_box(60, 60, 320, 200); // JUST TEST BOX

    // Open the text window
    open = 0;
    while(open < 1.0)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    	glLoadIdentity();

        // clear_surface(lpDDSBack); PORT!
        draw_trim_box_opening(0, 0, scrx, scry, open);
        flip_pages();
        open += .030;
    }

	/*fprintf(stderr,"waiting to read a scanf\n");
    scanf("%s",text);
    exit(0);*/

    // Wait for input
    stillchoosing = btrue;
    while(stillchoosing)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    	glLoadIdentity();

        // Show the text
        // clear_surface(lpDDSBack); PORT!
        draw_trim_box(0, 0, scrx, scry);
        sprintf(text, "Egoboo v2.22");
        draw_string(text, (scrx>>1)-200, ((scry>>1)-30));
        sprintf(text, "http://egoboo.sourceforge.net");
        draw_string(text, (scrx>>1)-200, ((scry>>1)));
        sprintf(text, "See controls.txt to configure input");
        draw_string(text, (scrx>>1)-200, ((scry>>1)+30));

		// get input
		read_input(NULL);

        // Handle the mouse
        do_cursor();
		if ( pending_click || SDLKEYDOWN(SDLK_ESCAPE) )
		{
		        pending_click = bfalse;
			stillchoosing = bfalse;
		}
        flip_pages();
    }
    nextmenu = MENUA;
}

//--------------------------------------------------------------------------------------------
void fiddle_with_menu()
{
    // ZZ> This function gives a nice little menu to play around in.

    menuactive = btrue;
    readytostart = bfalse;
    playersready = 0;
    localmachine = 0;
    rtslocalteam = 0;
    numfile = 0;
    numfilesent = 0;
    numfileexpected = 0;
    while(menuactive)
    {
        switch(nextmenu)
        {
            case MENUA:
                // MENUA...  Let the user choose a network service
                //printf("MENUA\n");
                if(menuaneeded)
                {
                    menu_service_select();
                    menuaneeded = bfalse;
                }
                nextmenu = MENUB;
                break;
            case MENUB:
                // MENUB...  Let the user start or join
                //printf("MENUB\n");
                menu_start_or_join();
                break;
            case MENUC:
                // MENUC...  Choose an open game to join
                //printf("MENUC\n");
                //menu_choose_host();
				menu_join_multiplayer();
                break;
            case MENUD:
                // MENUD...  Choose a module to run
                //printf("MENUD\n");
                menu_choose_module();
                break;
            case MENUE:
                // MENUE...  Wait for all the players
                //printf("MENUE\n");
                menu_boot_players();
                break;
            case MENUF:
                // MENUF...  Let the players choose characters
                //printf("MENUF\n");
                menu_pick_player(pickedindex);
                break;
            case MENUG:
                // MENUG...  Let the user read while it loads
                //printf("MENUG\n");
                menu_module_loading(pickedindex);
                break;
            case MENUH:
                // MENUH...  Show the end text
                //printf("MENUH\n");
                menu_end_text();
                break;
            case MENUI:
                // MENUI...  Show the initial text
                //printf("MENUI\n");
                menu_initial_text();
                break;
        }
    }
    //printf("Left menu system\n");
}

//--------------------------------------------------------------------------------------------
void release_menu_trim()
{
	// ZZ> This function frees the menu trim memory
	//GLTexture_Release( &TxTrimX );		//RELEASE(lpDDSTrimX);
	//GLTexture_Release( &TxTrimY );		//RELEASE(lpDDSTrimY);
	GLTexture_Release( &TxBlip );		//RELEASE(lpDDSBlip);
	GLTexture_Release( &TxTrim );

}

//--------------------------------------------------------------------------------------------
void release_menu()
{
	// ZZ> This function releases all the menu images
	GLTexture_Release( &TxFont );		//RELEASE(lpDDSFont);
    release_all_titleimages();
    release_all_icons();

}
#endif

void draw_trimx(int x, int y, int length)
{
	// ZZ> This function draws a horizontal trim bar
	GLfloat	txWidth, txHeight, txLength;

	if ( GLTexture_GetTextureID( &TxTrim ) != 0 )//if( lpDDSTrimX )
	{
		/*while( length > 0 )
        {
			trimrect.right = length;
			if(length > TRIMX)  trimrect.right = TRIMX;
			trimrect.bottom = 4;
			lpDDSBack->BltFast(x, y, lpDDSTrimX, &trimrect, DDBLTFAST_NOCOLORKEY);
			length-=TRIMX;
			x+=TRIMX;
		}*/

		/* Calculate the texture width, height, and length */
		txWidth = ( GLfloat )( GLTexture_GetImageWidth( &TxTrim )/GLTexture_GetDimensions( &TxTrim ) );
		txHeight = ( GLfloat )( GLTexture_GetImageHeight( &TxTrim )/GLTexture_GetDimensions( &TxTrim ) );
		txLength = ( GLfloat )( length/GLTexture_GetImageWidth( &TxTrim ) );


		/* Bind our texture */
		glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &TxTrim ) );

		/* Draw the trim */
		glColor4f( 1, 1, 1, 1 );
		glBegin( GL_QUADS );
			glTexCoord2f( 0, 1 );	glVertex2f( x, scry - y );
			glTexCoord2f( 0, 1 - txHeight );	glVertex2f( x, scry - y - GLTexture_GetImageHeight( &TxTrim ) );
			glTexCoord2f( txWidth*txLength, 1 - txHeight );	glVertex2f( x + length, scry - y - GLTexture_GetImageHeight( &TxTrim ) );
			glTexCoord2f( txWidth*txLength, 1 );	glVertex2f( x + length, scry - y );
		glEnd();
	}
}

//--------------------------------------------------------------------------------------------
void draw_trimy(int x, int y, int length)
{
	// ZZ> This function draws a vertical trim bar
	GLfloat	txWidth, txHeight, txLength;

	if ( GLTexture_GetTextureID( &TxTrim ) != 0 )//if(lpDDSTrimY)
	{
		/*while(length > 0)
		{
			trimrect.bottom = length;
			if(length > TRIMY)  trimrect.bottom = TRIMY;
			trimrect.right = 4;
			lpDDSBack->BltFast(x, y, lpDDSTrimY, &trimrect, DDBLTFAST_NOCOLORKEY);
			length-=TRIMY;
			y+=TRIMY;
		}*/

		/* Calculate the texture width, height, and length */
		txWidth = ( GLfloat )( GLTexture_GetImageWidth( &TxTrim )/GLTexture_GetDimensions( &TxTrim ) );
		txHeight = ( GLfloat )( GLTexture_GetImageHeight( &TxTrim )/GLTexture_GetDimensions( &TxTrim ) );
		txLength = ( GLfloat )( length/GLTexture_GetImageHeight( &TxTrim ) );

		/* Bind our texture */
		glBindTexture( GL_TEXTURE_2D, GLTexture_GetTextureID( &TxTrim ) );

		/* Draw the trim */
		glColor4f( 1, 1, 1, 1 );
		glBegin( GL_QUADS );
			glTexCoord2f( 0, 1 );	glVertex2f( x, scry - y );
			glTexCoord2f( 0, 1 - txHeight*txLength );	glVertex2f( x, scry - y - length );
			glTexCoord2f( txWidth, 1 - txHeight*txLength );	glVertex2f( x + GLTexture_GetImageWidth( &TxTrim ), scry - y - length );
			glTexCoord2f( txWidth, 1 );	glVertex2f( x + GLTexture_GetImageWidth( &TxTrim ), scry - y );
		glEnd();
	}
}

//--------------------------------------------------------------------------------------------
void draw_trim_box(int left, int top, int right, int bottom)
{
    // ZZ> This function draws a trim rectangle
    float l,t,r,b;
    l=((float)left)/scrx;
    r=((float)right)/scrx;
    t=((float)top)/scry;
    b=((float)bottom)/scry;

	Begin2DMode();

	draw_trimx(left, top, right-left);
	draw_trimx(left, bottom-4, right-left);
	draw_trimy(left, top, bottom-top);
	draw_trimy(right-4, top, bottom-top);

	End2DMode();
}

//--------------------------------------------------------------------------------------------
void draw_trim_box_opening(int left, int top, int right, int bottom, float amount)
{
    // ZZ> This function draws a trim rectangle, scaled around its center
    int x = (left + right)>>1;
    int y = (top + bottom)>>1;
    left   = (x * (1.0-amount)) + (left * amount);
    right  = (x * (1.0-amount)) + (right * amount);
    top    = (y * (1.0-amount)) + (top * amount);
    bottom = (y * (1.0-amount)) + (bottom * amount);
    draw_trim_box(left, top, right, bottom);
}

//--------------------------------------------------------------------------------------------
void load_menu()
{
    // ZZ> This function loads all of the menu data...  Images are loaded into system
    // memory

    load_font( "basicdat/font.bmp", "basicdat/font.txt", btrue );
    //load_all_menu_images();
}

//--------------------------------------------------------------------------------------------
void do_cursor_rts()
{
    // This function implements the RTS mouse cursor
    int sttx, stty, endx, endy, target, leader;
    signed short sound;


    if(mousebutton[1] == 0)
    {
        cursorx+=mousex;
        cursory+=mousey;
    }
    if(cursorx < 6)  cursorx = 6;  if (cursorx > scrx-16)  cursorx = scrx-16;
    if(cursory < 8)  cursory = 8;  if (cursory > scry-24)  cursory = scry-24;
    move_rtsxy();
    if(mousebutton[0])
    {
        // Moving the end select point
        pressed = btrue;
        rtsendx = cursorx+5;
        rtsendy = cursory+7;

        // Draw the selection rectangle
        if(allselect == bfalse)
        {
            sttx = rtssttx;  endx = rtsendx;  if(sttx > endx)  {  sttx = rtsendx;  endx = rtssttx; }
            stty = rtsstty;  endy = rtsendy;  if(stty > endy)  {  stty = rtsendy;  endy = rtsstty; }
            draw_trim_box(sttx, stty, endx, endy);
        }
    }
    else
    {
        if(pressed)
        {
            // See if we selected anyone
            if((ABS(rtssttx - rtsendx) + ABS(rtsstty - rtsendy)) > 10 && allselect == bfalse)
            {
                // We drew a box alright
                sttx = rtssttx;  endx = rtsendx;  if(sttx > endx)  {  sttx = rtsendx;  endx = rtssttx; }
                stty = rtsstty;  endy = rtsendy;  if(stty > endy)  {  stty = rtsendy;  endy = rtsstty; }
                build_select(sttx, stty, endx, endy, rtslocalteam);
            }
            else
            {
                // We want to issue an order
                if(numrtsselect > 0)
                {
                    leader = rtsselect[0];
                    sttx = rtssttx-20;  endx = rtssttx+20;
                    stty = rtsstty-20;  endy = rtsstty+20;
                    target = build_select_target(sttx, stty, endx, endy, rtslocalteam);
                    if(target == MAXCHR)
                    {
                        // No target...
                        if(SDLKEYDOWN(SDLK_LSHIFT) || SDLKEYDOWN(SDLK_RSHIFT))
                        {
                            send_rts_order(rtsx, rtsy, RTSTERRAIN, target);
                            sound = chrwavespeech[leader][SPEECHTERRAIN];
                        }
                        else
                        {
                            send_rts_order(rtsx, rtsy, RTSMOVE, target);
                            sound = wldframe&1;  // Move or MoveAlt
                            sound = chrwavespeech[leader][sound];
                        }
                    }
                    else
                    {
                        if(teamhatesteam[rtslocalteam][chrteam[target]])
                        {
                            // Target is an enemy, so issue an attack order
                            send_rts_order(rtsx, rtsy, RTSATTACK, target);
                            sound = chrwavespeech[leader][SPEECHATTACK];
                        }
                        else
                        {
                            // Target is a friend, so issue an assist order
                            send_rts_order(rtsx, rtsy, RTSASSIST, target);
                            sound = chrwavespeech[leader][SPEECHASSIST];
                        }
                    }
                    // Do unit speech at 11025 KHz
                    if(sound >= 0 && sound < MAXWAVE)
                    {
                        play_sound_pvf(capwaveindex[chrmodel[leader]][sound], PANMID, VOLMAX, 11025);
                    }
                }
            }
            pressed = bfalse;
        }


        // Moving the select point
        rtssttx = cursorx+5;
        rtsstty = cursory+7;
        rtsendx = cursorx+5;
        rtsendy = cursory+7;
    }

    // GAC - Don't forget to BeginText() and EndText();
    BeginText();
    draw_one_font(11, cursorx-5, cursory-7);
    EndText ();
}


//--------------------------------------------------------------------------------------------
void build_select(float tlx, float tly, float brx, float bry, Uint8 team)
{
    // ZZ> This function checks which characters are in the selection rectangle
/*PORT
  D3DLVERTEX v[MAXPRT];
    D3DTLVERTEX vt[MAXPRT];
    int numbertocheck, character, cnt, first, sound;


    // Unselect old ones
    clear_select();


    // Figure out who to check
    numbertocheck = 0;
    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(chrteam[cnt] == team && chron[cnt] && !chrinpack[cnt])
        {
            v[numbertocheck].x = (D3DVALUE) chrxpos[cnt];
            v[numbertocheck].y = (D3DVALUE) chrypos[cnt];
            v[numbertocheck].z = (D3DVALUE) chrzpos[cnt];
            v[numbertocheck].color = cnt;  // Store an index in the color slot...
            v[numbertocheck].dwReserved = 0;
            numbertocheck++;
        }
        cnt++;
    }


    // Figure out where the points go onscreen
    lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &mWorld);
    transform_vertices(numbertocheck, v, vt);


    first = btrue;
    cnt = 0;
    while(cnt < numbertocheck)
    {
        // Only check if in front of camera
        if(vt[cnt].dvRHW > 0)
        {
            // Check the rectangle
            if(vt[cnt].dvSX > tlx && vt[cnt].dvSX < brx)
            {
                if(vt[cnt].dvSY > tly && vt[cnt].dvSY < bry)
                {
                    // Select the character
                    character = v[cnt].color;
                    add_select(character);
                    if(first)
                    {
                        // Play the select speech for the first one picked
                        sound = chrwavespeech[character][SPEECHSELECT];
                        if(sound >= 0 && sound < MAXWAVE)
                        play_sound_pvf(capwaveindex[chrmodel[character]][sound], PANMID, VOLMAX, 11025);
                        first = bfalse;
                    }
                }
            }
        }
        cnt++;
    }
*/
}

//--------------------------------------------------------------------------------------------
Uint16 build_select_target(float tlx, float tly, float brx, float bry, Uint8 team)
{
    // ZZ> This function checks which characters are in the selection rectangle,
    //     and returns the first one found
/*PORT
  D3DLVERTEX v[MAXPRT];
    D3DTLVERTEX vt[MAXPRT];
    int numbertocheck, character, cnt;


    // Figure out who to check
    numbertocheck = 0;
    // Add enemies first
    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(teamhatesteam[team][chrteam[cnt]] && chron[cnt] && !chrinpack[cnt])
        {
            v[numbertocheck].x = (D3DVALUE) chrxpos[cnt];
            v[numbertocheck].y = (D3DVALUE) chrypos[cnt];
            v[numbertocheck].z = (D3DVALUE) chrzpos[cnt];
            v[numbertocheck].color = cnt;  // Store an index in the color slot...
            v[numbertocheck].dwReserved = 0;
            numbertocheck++;
        }
        cnt++;
    }
    // Add allies next
    cnt = 0;
    while(cnt < MAXCHR)
    {
        if(teamhatesteam[team][chrteam[cnt]] == bfalse && chron[cnt] && !chrinpack[cnt])
        {
            v[numbertocheck].x = (D3DVALUE) chrxpos[cnt];
            v[numbertocheck].y = (D3DVALUE) chrypos[cnt];
            v[numbertocheck].z = (D3DVALUE) chrzpos[cnt];
            v[numbertocheck].color = cnt;  // Store an index in the color slot...
            v[numbertocheck].dwReserved = 0;
            numbertocheck++;
        }
        cnt++;
    }


    // Figure out where the points go onscreen
    lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &mWorld);
    transform_vertices(numbertocheck, v, vt);


    cnt = 0;
    while(cnt < numbertocheck)
    {
        // Only check if in front of camera
        if(vt[cnt].dvRHW > 0)
        {
            // Check the rectangle
            if(vt[cnt].dvSX > tlx && vt[cnt].dvSX < brx)
            {
                if(vt[cnt].dvSY > tly && vt[cnt].dvSY < bry)
                {
                    // Select the character
                    character = v[cnt].color;
                    return character;
                }
            }
        }
        cnt++;
    }
    return MAXCHR;
*/
return 0;
}

//--------------------------------------------------------------------------------------------
void move_rtsxy()
{
    // ZZ> This function iteratively transforms the cursor back to world coordinates
/*PORT
  D3DLVERTEX v[1];
    D3DTLVERTEX vt[1];
    int numbertocheck, x, y, fan;
    float sin, cos, trailrate, level;



    // Figure out where the rtsxy is at on the screen
    x = rtsx;
    y = rtsy;
    fan = meshfanstart[y>>7]+(x>>7);
    level = get_level(rtsx, rtsy, fan, bfalse);
    v[0].x = (D3DVALUE) rtsx;
    v[0].y = (D3DVALUE) rtsy;
    v[0].z = level;
    v[0].color = 0;
    v[0].dwReserved = 0;
    numbertocheck = 1;


    // Figure out where the points go onscreen
    lpD3DDDevice->SetTransform(D3DTRANSFORMSTATE_WORLD, &mWorld);
    transform_vertices(numbertocheck, v, vt);


    if(vt[0].dvRHW < 0)
    {
        // Move it to camtrackxy if behind the camera
        rtsx = camtrackx;
        rtsy = camtracky;
    }
    else
    {
        // Move it to closer to the onscreen cursor
        trailrate = ABS(cursorx-vt[0].dvSX) + ABS(cursory-vt[0].dvSY);
        trailrate *= rtstrailrate;
        sin = turntosin[camturnleftrightshort>>2]*trailrate;
        cos = turntosin[((camturnleftrightshort>>2)+4096)&16383]*trailrate;
        if(vt[0].dvSX < cursorx)
        {
            rtsx += cos;
            rtsy -= sin;
        }
        else
        {
            rtsx -= cos;
            rtsy += sin;
        }



        if(vt[0].dvSY < cursory)
        {
            rtsx += sin;
            rtsy += cos;
        }
        else
        {
            rtsx -= sin;
            rtsy -= cos;
        }
    }
*/
}

//--------------------------------------------------------------------------------------------
void reset_press()
{
    // ZZ> This function resets key press information
/*PORT
    int cnt;
    cnt = 0;
    while(cnt < 256)
    {
        keypress[cnt] = bfalse;
        cnt++;
    }
*/
}

//--------------------------------------------------------------------------------------------
void check_add( Uint8 key, char bigletter, char littleletter )
{
  // ZZ> This function adds letters to the net message
  /*PORT
  if(KEYDOWN(key))
    {
  if(keypress[key]==bfalse)
    {
  keypress[key] = btrue;
  if(netmessagewrite < MESSAGESIZE-2)
    {
  if(KEYDOWN(DIK_LSHIFT) || KEYDOWN(DIK_RSHIFT))
    {
  netmessage[netmessagewrite] = bigletter;
    }
  else
    {
  netmessage[netmessagewrite] = littleletter;
    }
  netmessagewrite++;
  netmessage[netmessagewrite] = '?'; // The flashing input cursor
  netmessage[netmessagewrite+1] = 0;
    }
    }
    }
  else
    {
  keypress[key] = bfalse;
    }
  */
}

//--------------------------------------------------------------------------------------------
void input_net_message()
{
  // ZZ> This function lets players communicate over network by hitting return, then
  //     typing text, then return again
  /*PORT
  int cnt;
  char cTmp;


  if(netmessagemode)
    {
  // Add new letters
  check_add(DIK_A, 'A', 'a');
  check_add(DIK_B, 'B', 'b');
  check_add(DIK_C, 'C', 'c');
  check_add(DIK_D, 'D', 'd');
  check_add(DIK_E, 'E', 'e');
  check_add(DIK_F, 'F', 'f');
  check_add(DIK_G, 'G', 'g');
  check_add(DIK_H, 'H', 'h');
  check_add(DIK_I, 'I', 'i');
  check_add(DIK_J, 'J', 'j');
  check_add(DIK_K, 'K', 'k');
  check_add(DIK_L, 'L', 'l');
  check_add(DIK_M, 'M', 'm');
  check_add(DIK_N, 'N', 'n');
  check_add(DIK_O, 'O', 'o');
  check_add(DIK_P, 'P', 'p');
  check_add(DIK_Q, 'Q', 'q');
  check_add(DIK_R, 'R', 'r');
  check_add(DIK_S, 'S', 's');
  check_add(DIK_T, 'T', 't');
  check_add(DIK_U, 'U', 'u');
  check_add(DIK_V, 'V', 'v');
  check_add(DIK_W, 'W', 'w');
  check_add(DIK_X, 'X', 'x');
  check_add(DIK_Y, 'Y', 'y');
  check_add(DIK_Z, 'Z', 'z');


  check_add(DIK_1, '!', '1');
  check_add(DIK_2, '@', '2');
  check_add(DIK_3, '#', '3');
  check_add(DIK_4, '$', '4');
  check_add(DIK_5, '%', '5');
  check_add(DIK_6, '^', '6');
  check_add(DIK_7, '&', '7');
  check_add(DIK_8, '*', '8');
  check_add(DIK_9, '(', '9');
  check_add(DIK_0, ')', '0');


  check_add(DIK_APOSTROPHE, 34, 39);
  check_add(DIK_SPACE,      ' ', ' ');
  check_add(DIK_SEMICOLON,  ':', ';');
  check_add(DIK_PERIOD,     '>', '.');
  check_add(DIK_COMMA,      '<', ',');
  check_add(DIK_GRAVE,      '`', '`');
  check_add(DIK_MINUS,      '_', '-');
  check_add(DIK_EQUALS,     '+', '=');
  check_add(DIK_LBRACKET,   '{', '[');
  check_add(DIK_RBRACKET,   '}', ']');
  check_add(DIK_BACKSLASH,  '|', '\\');
  check_add(DIK_SLASH,      '?', '/');



  // Make cursor flash
  if(netmessagewrite < MESSAGESIZE-1)
    {
  if((wldframe & 8) == 0)
    {
  netmessage[netmessagewrite] = '#';
    }
  else
    {
  netmessage[netmessagewrite] = '+';
    }
    }


  // Check backspace and return
  if(netmessagedelay == 0)
    {
  if(KEYDOWN(DIK_BACK))
    {
  if(netmessagewrite < MESSAGESIZE)  netmessage[netmessagewrite] = 0;
  if(netmessagewrite > netmessagewritemin) netmessagewrite--;
  netmessagedelay = 3;
    }


  // Ship out the message
  if(KEYDOWN(DIK_RETURN))
    {
  // Is it long enough to bother?
  if(netmessagewrite > 0)
    {
  // Yes, so send it
  netmessage[netmessagewrite] = 0;
  if(networkon)
    {
  start_building_packet();
  add_packet_us(TO_ANY_TEXT);
  add_packet_sz(netmessage);
  send_packet_to_all_players();
    }
    }
  netmessagemode = bfalse;
  netmessagedelay = 20;
    }
    }
  else
    {
  netmessagedelay--;
    }
    }
  else
    {
  // Input a new message?
  if(netmessagedelay == 0)
    {
  if(KEYDOWN(DIK_RETURN))
    {
  // Copy the name
  cnt = 0;
  cTmp = netmessagename[cnt];
  while(cTmp != 0 && cnt < 64)
    {
  netmessage[cnt] = cTmp;
  cnt++;
  cTmp = netmessagename[cnt];
    }
  netmessage[cnt] = '>';  cnt++;
  netmessage[cnt] = ' ';  cnt++;
  netmessage[cnt] = '?';
  netmessage[cnt+1] = 0;
  netmessagewrite = cnt;
  netmessagewritemin = cnt;

  netmessagemode = btrue;
  netmessagedelay = 20;
    }
    }
  else
    {
  netmessagedelay--;
    }
    }
  */
}
