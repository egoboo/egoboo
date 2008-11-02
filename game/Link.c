#include "Link.h"
#include "proto.h"
#include "egoboo.h"


Link_t LinkList[LINK_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t link_export_all()
{
  // BB> This function saves all the players to the players dir 
  //     and also copies them into the imports dir to prepare for the next module

  bool_t is_local;
  int cnt, tnc, i, character;
  char srcDir[64], destDir[64];

  // do the normal export to save all the player settings
  export_all_players(btrue);

	// build the import directory using the player info
	empty_import_directory();
	fs_createDirectory("import");
  for(tnc = 0, cnt = 0; cnt < MAXPLAYER; cnt++)
  {
    if( !plavalid[cnt] ) continue;

    // Is it alive?
    character = plaindex[cnt];
    if(!chron[character] || !chralive[character]) continue;

    is_local == (0 != pladevice[cnt]);

    // copy the values to the import values
    strcpy( loadplayerdir[tnc], chrname[character] );
		
		localcontrol[tnc] = INPUTKEY | INPUTMOUSE | INPUTJOYA;
		localslot[tnc]    = tnc * 9;

		// Copy the character to the import directory
    if(is_local)
    {
		  sprintf(loadplayerdir[tnc], "players/%s", chrname[character]);
    }
    else
    {
      sprintf(loadplayerdir[tnc], "remote/%s", chrname[character]);
    }
		sprintf(destDir, "import/temp%04d.obj", localslot[tnc]);
		fs_copyDirectory(loadplayerdir[tnc], destDir);

		// Copy all of the character's items to the import directory
		for(i = 0;i < 8;i++)
		{
			sprintf(srcDir, "%s/%d.obj", loadplayerdir[tnc], i);
			sprintf(destDir, "import/temp%04d.obj", localslot[tnc] + i + 1);

			fs_copyDirectory(srcDir, destDir);
		}

    tnc++;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t link_follow(Link_t list[], int ilink)
{
  // BB> This causes the game to follow a link

  bool_t retval = bfalse;
  Link_t * plink;

  if(ilink < 0 || ilink>=LINK_COUNT) return bfalse;
  plink = list + ilink;

  if(!plink->valid) return bfalse;

  // finish with the old module
  link_export_all();
  release_module();

  //start the new module
  load_module(plink->modname); 

	pressed = bfalse;
	make_onwhichfan();
	reset_camera();
	reset_timers();
	figure_out_what_to_draw();
	make_character_matrices();
	attach_particles();

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t link_build(const char * fname, Link_t list[])
{
  FILE * pfile;
  bool_t retval = bfalse;
  int i;

  if(NULL == fname || '\0' == fname) return bfalse;

  pfile = fopen( fname, "r" );
  if(NULL == pfile) return bfalse;

  i = 0;
  while( goto_colon_yesno(pfile) && i<LINK_COUNT )
  {
    fscanf(pfile, "%s", &(list[i].modname)); 
	list[i].valid = btrue;
    i++;
  }
  

  return i > 0;
};