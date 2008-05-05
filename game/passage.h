#pragma once

#include "egoboo_types.inl"

#define MAXPASS             256                     // Maximum number of passages ( mul 32 )
#define NOOWNER             65535

//Passages
typedef struct passage_t
{
  IRect   area;            // Passage positions
  int     music;           // Music track appointed to the specific passage
  Uint32  mask;
  bool_t  open;            // Is the passage open?
  CHR_REF owner;           // Who controls the passage?
} PASSAGE;

extern Uint32  passage_count;             // Number of passages in the module
extern PASSAGE PassList[MAXPASS];

typedef struct shop_t
{
  Uint16 passage;  // The passage number
  Uint16 owner;    // Who gets the gold?
} SHOP;

// For shops
extern Uint16 shop_count;
extern SHOP ShopList[MAXPASS];
