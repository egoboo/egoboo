#pragma once

#include "egobootypedef.h"

#define LINK_COUNT 16

struct sLink
{
  bool_t  valid;
  char    modname[256];
  Uint16  passage;
};

typedef struct sLink Link_t;

extern Link_t LinkList[LINK_COUNT];

bool_t link_export_all();
bool_t link_follow(Link_t list[], int ilink);
bool_t link_build(const char * fname, Link_t list[]);