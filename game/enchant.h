#pragma once

#include "egoboo_types.inl"
#include "object.h"

#define MAXEVE                          MAXPROFILE  // One enchant type per model
#define MAXENCHANT                      128         // Number of enchantments

//------------------------------------
//Enchantment variables
//------------------------------------

typedef enum eve_set_e
{
  SETDAMAGETYPE           = 0,
  SETNUMBEROFJUMPS,
  SETLIFEBARCOLOR,
  SETMANABARCOLOR,
  SETSLASHMODIFIER,       //Damage modifiers
  SETCRUSHMODIFIER,
  SETPOKEMODIFIER,
  SETHOLYMODIFIER,
  SETEVILMODIFIER,
  SETFIREMODIFIER,
  SETICEMODIFIER,
  SETZAPMODIFIER,
  SETFLASHINGAND,
  SETLIGHTBLEND,
  SETALPHABLEND,
  SETSHEEN,                //Shinyness
  SETFLYTOHEIGHT,
  SETWALKONWATER,
  SETCANSEEINVISIBLE,
  SETMISSILETREATMENT,
  SETCOSTFOREACHMISSILE,
  SETMORPH,                //Morph character?
  SETCHANNEL,               //Can channel life as mana?
  EVE_SET_COUNT
} EVE_SET;

typedef enum eve_add_e
{
  ADDJUMPPOWER = 0,
  ADDBUMPDAMPEN,
  ADDBOUNCINESS,
  ADDDAMAGE,
  ADDSIZE,
  ADDACCEL,
  ADDRED,             //Red shift
  ADDGRN,             //Green shift
  ADDBLU,             //Blue shift
  ADDDEFENSE,         //Defence adjustments
  ADDMANA,
  ADDLIFE,
  ADDSTRENGTH,
  ADDWISDOM,
  ADDINTELLIGENCE,
  ADDDEXTERITY,
  EVE_ADD_COUNT      // Number of adds
} EVE_ADD;

extern Uint16    numfreeenchant;             // For allocating new ones
extern Uint16    freeenchant[MAXENCHANT];    //

typedef struct eve_t
{
  bool_t          valid;                       // Enchant.txt loaded?
  bool_t          override;                    // Override other enchants?
  bool_t          removeoverridden;            // Remove other enchants?
  bool_t          setyesno[EVE_SET_COUNT];    // Set this value?
  Uint8           setvalue[EVE_SET_COUNT];    // Value to use
  Sint8           addvalue[EVE_ADD_COUNT];    // The values to add
  bool_t          retarget;                    // Pick a weapon?
  bool_t          killonend;                   // Kill the target on end?
  bool_t          poofonend;                   // Spawn a poof on end?
  bool_t          endifcantpay;                // End on out of mana
  bool_t          stayifnoowner;               // Stay if owner has died?
  Sint16          time;                        // Time in seconds
  Sint8           endmessage;                  // Message for end -1 for none
  Sint16          ownermana;                   // Boost values
  Sint16          ownerlife;                   //
  Sint16          targetmana;                  //
  Sint16          targetlife;                  //
  DAMAGE          dontdamagetype;              // Don't work if ...
  DAMAGE          onlydamagetype;              // Only work if ...
  IDSZ            removedbyidsz;               // By particle or [NONE]
  Uint16          contspawntime;               // Spawn timer
  Uint8           contspawnamount;             // Spawn amount
  Uint16          contspawnfacingadd;          // Spawn in circle
  Uint16          contspawnpip;                // Spawn type ( local )
  Sint8           endsound;                    // Sound on end (-1 for none)
  Uint16          frequency;                   // Sound frequency
  Uint16          overlay;                     // Spawn an overlay?
  bool_t          canseekurse;                 // Allow target to see kurses?
} EVE;

extern EVE EveList[MAXEVE];

typedef struct enc_t
{
  Uint8           on;                      // Enchantment on
  Uint16          eve;                     // The type
  Uint16          target;                  // Who it enchants
  Uint16          nextenchant;             // Next in the list
  Uint16          owner;                   // Who cast the enchant
  Uint16          spawner;                 // The spellbook character
  Uint16          overlay;                 // The overlay character
  Sint16          ownermana;               // Boost values
  Sint16          ownerlife;               //
  Sint16          targetmana;              //
  Sint16          targetlife;              //
  bool_t          setyesno[EVE_SET_COUNT]; // Was it set?
  Uint8           setsave[EVE_SET_COUNT];  // The value to restore
  Sint16          addsave[EVE_ADD_COUNT];  // The value to take away
  Sint16          time;                    // Time before end
  float           spawntime;               // Time before spawn
} ENC;

extern ENC EncList[MAXENCHANT];

typedef enum disenchant_mode_e
{
  LEAVE_ALL   = 0,
  LEAVE_FIRST,
  LEAVE_NONE,
} DISENCHANT_MODE;


void reset_character_alpha( CHR_REF character );
void reset_character_accel( CHR_REF character );

void   load_one_eve( char * szObjectpath, char * szObjectname, Uint16 profile );
Uint16 get_free_enchant();
void   unset_enchant_value( Uint16 enchantindex, Uint8 valueindex );
void   remove_enchant_value( Uint16 enchantindex, Uint8 valueindex );