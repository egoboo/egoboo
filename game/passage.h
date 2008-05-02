#pragma once

#include "egoboo_types.inl"

#define MAXPASS             256                     // Maximum number of passages ( mul 32 )

//Passages
extern Uint32 numpassage;             // Number of passages in the module
extern int    passtlx[MAXPASS];       // Passage positions
extern int    passtly[MAXPASS];
extern int    passbrx[MAXPASS];
extern int    passbry[MAXPASS];
extern int    passagemusic[MAXPASS];  //Music track appointed to the specific passage
extern Uint32 passmask[MAXPASS];
extern bool_t passopen[MAXPASS];   // Is the passage open?
extern Uint16 passowner[MAXPASS];  // Who controls the passage?

// For shops
extern Uint16 numshoppassage;
extern Uint16 shoppassage[MAXPASS];  // The passage number
extern Uint16 shopowner[MAXPASS];    // Who gets the gold?
#define NOOWNER 65535