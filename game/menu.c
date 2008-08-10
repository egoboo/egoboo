/* Egoboo - menu.c
 * Implements the main menu tree, using the code in Ui.*
 */
 
 /*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "egoboo.h"
#include "Ui.h"
#include "Menu.h"
#include "Log.h"

// TEMPORARY!
#define NET_DONE_SENDING_FILES 10009
#define NET_NUM_FILES_TO_SEND  10010

#ifdef __unix__
#define max(a,b) ( ((a)>(b))? (a):(b) )
#endif

//--------------------------------------------------------------------------------------------
// New menu code
//--------------------------------------------------------------------------------------------

enum MenuStates
{
	MM_Begin,
	MM_Entering,
	MM_Running,
	MM_Leaving,
	MM_Finish,
};

static int selectedPlayer = 0;
static int selectedModule = 0;

/* Copyright text variables.  Change these to change how the copyright text appears */
const char copyrightText[] = "Welcome to Egoboo!\nhttp://egoboo.sourceforge.net\nVersion 2.6.4";
static int copyrightLeft = 0;
static int copyrightTop  = 0;

/* Options info text variables.  Change these to change how the options text appears */
const char optionsText[] = "Change your audio, input and video\nsettings here.";
static int optionsTextLeft = 0;
static int optionsTextTop  = 0;

/* Button labels.  Defined here for consistency's sake, rather than leaving them as constants */
const char *mainMenuButtons[] = {
	"Single Player",
	"Multi-player",
	"Options",
	"Quit",
	""
};

const char *singlePlayerButtons[] = {
	"New Player",
	"Load Saved Player",
	"Back",
	""
};

const char *optionsButtons[] = {
	"Audio Options",
	"Input Controls",
	"Video Settings",
	"Back",
	""
};

const char *audioOptionsButtons[] = {
	"N/A",				//Enable sound
	"N/A",				//Sound volume
	"N/A",				//Enable music
	"N/A",				//Music volume
	"N/A",				//Sound channels
	"N/A",				//Sound buffer
	"Save Settings",
	""
};

const char *videoOptionsButtons[] = {
	"N/A",		//Antialaising
	"N/A",		//Color depth
	"N/A",		//Fast & ugly
	"N/A",		//Fullscreen
	"N/A",		//Reflections
	"N/A",		//Texture filtering
	"N/A",		//Shadows
	"N/A",		//Z bit
	"N/A",		//Fog
	"N/A",		//3D effects
	"N/A",		//Multi water layer
	"N/A",		//Max messages
	"N/A",		//Screen resolution
	"Save Settings",
	""
};


/* Button position for the "easy" menus, like the main one */
static int buttonLeft = 0;
static int buttonTop = 0;

static bool_t startNewPlayer = bfalse;

/* The font used for drawing text.  It's smaller than the button font */
Font *menuFont = NULL;

// "Slidy" buttons used in some of the menus.  They're shiny.
struct {
	float lerp;
	int top;
	int left;
	char **buttons;
}SlidyButtonState;

static void initSlidyButtons(float lerp, const char *buttons[])
{
	SlidyButtonState.lerp = lerp;
	SlidyButtonState.buttons = (char**)buttons;
}

static void updateSlidyButtons(float deltaTime)
{
	SlidyButtonState.lerp += (deltaTime * 1.5f);
}

static void drawSlidyButtons()
{
	int i;

	for (i = 0; SlidyButtonState.buttons[i][0] != 0; i++)
	{
		int x = buttonLeft - (360 - i * 35)  * SlidyButtonState.lerp;
		int y = buttonTop + (i * 35);

		ui_doButton(UI_Nothing, SlidyButtonState.buttons[i], x, y, 200, 30);
	}
}

/** initMenus
 * Loads resources for the menus, and figures out where things should
 * be positioned.  If we ever allow changing resolution on the fly, this
 * function will have to be updated/called more than once.
 */

int initMenus()
{
	int i;

	menuFont = fnt_loadFont("basicdat/Negatori.ttf", 18);
	if (!menuFont)
	{
		log_error("Could not load the menu font!\n");
		return 0;
	}

	// Figure out where to draw the buttons
	buttonLeft = 40;
	buttonTop = displaySurface->h - 20;
	for (i = 0; mainMenuButtons[i][0] != 0; i++)
	{
		buttonTop -= 35;
	}

	// Figure out where to draw the copyright text
	copyrightLeft = 0;
	copyrightLeft = 0;
	fnt_getTextBoxSize(menuFont, copyrightText, 20, &copyrightLeft, &copyrightTop);
	// Draw the copyright text to the right of the buttons
	copyrightLeft = 280;
	// And relative to the bottom of the screen
	copyrightTop = displaySurface->h - copyrightTop - 20;

	// Figure out where to draw the options text
	optionsTextLeft = 0;
	optionsTextLeft = 0;
	fnt_getTextBoxSize(menuFont, optionsText, 20, &optionsTextLeft, &optionsTextTop);
	// Draw the copyright text to the right of the buttons
	optionsTextLeft = 280;
	// And relative to the bottom of the screen
	optionsTextTop = displaySurface->h - optionsTextTop - 20;

	return 1;
}

int doMainMenu(float deltaTime)
{
	static int menuState = MM_Begin;
	static GLTexture background;
	static float lerp;
	static int menuChoice = 0;

	int result = 0;

	switch(menuState)
	{
	case MM_Begin:
		// set up menu variables
		GLTexture_Load(&background, "basicdat/menu/menu_main.bmp");
		menuChoice = 0;
		menuState = MM_Entering;

		initSlidyButtons(1.0f, mainMenuButtons);
		// let this fall through into MM_Entering

	case MM_Entering:
		// do buttons sliding in animation, and background fading in
		// background
		glColor4f(1, 1, 1, 1 -SlidyButtonState.lerp);
		ui_drawImage(0, &background, (displaySurface->w - background.imgWidth), 0, 0, 0);

		// "Copyright" text
		fnt_drawTextBox(menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20);
	
		drawSlidyButtons();
		updateSlidyButtons(-deltaTime);

		// Let lerp wind down relative to the time elapsed
		if (SlidyButtonState.lerp <= 0.0f)
		{
			menuState = MM_Running;
		}

		break;

	case MM_Running:
		// Do normal run
		// Background
		glColor4f(1, 1, 1, 1);
		ui_drawImage(0, &background, (displaySurface->w - background.imgWidth), 0, 0, 0);

		// "Copyright" text
		fnt_drawTextBox(menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20);

		// Buttons
		if(ui_doButton(1, mainMenuButtons[0], buttonLeft, buttonTop, 200, 30) == 1)
		{
			// begin single player stuff
			menuChoice = 1;
		}

		if(ui_doButton(2, mainMenuButtons[1], buttonLeft, buttonTop + 35, 200, 30) == 1)
		{
			// begin multi player stuff
			menuChoice = 2;
		}

		if(ui_doButton(3, mainMenuButtons[2], buttonLeft, buttonTop + 35 * 2, 200, 30) == 1)
		{
			// go to options menu
			menuChoice = 3;
		}

		if(ui_doButton(4, mainMenuButtons[3], buttonLeft, buttonTop + 35 * 3, 200, 30) == 1)
		{
			// quit game
			menuChoice = 4;
		}

		if(menuChoice != 0)
		{
			menuState = MM_Leaving;
			initSlidyButtons(0.0f, mainMenuButtons);
		}
		break;

	case MM_Leaving:
		// Do buttons sliding out and background fading
		// Do the same stuff as in MM_Entering, but backwards
		glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);
		ui_drawImage(0, &background, (displaySurface->w - background.imgWidth), 0, 0, 0);

		// "Copyright" text
		fnt_drawTextBox(menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20);

		// Buttons
		drawSlidyButtons();
		updateSlidyButtons(deltaTime);
		if(SlidyButtonState.lerp >= 1.0f) {
			menuState = MM_Finish;
		}
		break;

	case MM_Finish:
		// Free the background texture; don't need to hold onto it
		GLTexture_Release(&background);
		menuState = MM_Begin;	// Make sure this all resets next time doMainMenu is called

		// Set the next menu to load
		result = menuChoice;
		break;
	};

	return result;
}

int doSinglePlayerMenu(float deltaTime)
{
	static int menuState = MM_Begin;
	static GLTexture background;
	static int menuChoice;
	int result = 0;
	
	switch(menuState)
	{
	case MM_Begin:
		// Load resources for this menu
		GLTexture_Load(&background, "basicdat/menu/menu_advent.bmp");
		menuChoice = 0;

		menuState = MM_Entering;
		
		initSlidyButtons(1.0f, singlePlayerButtons);

		// Let this fall through

	case MM_Entering:
		glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);

		// Draw the background image
		ui_drawImage(0, &background, displaySurface->w - background.imgWidth, 0, 0, 0);

		// "Copyright" text
		fnt_drawTextBox(menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20);

		drawSlidyButtons();
		updateSlidyButtons(-deltaTime);

		if (SlidyButtonState.lerp <= 0.0f)
			menuState = MM_Running;

		break;

	case MM_Running:

		// Draw the background image
		ui_drawImage(0, &background, displaySurface->w - background.imgWidth, 0, 0, 0);

		// "Copyright" text
		fnt_drawTextBox(menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20);

		// Buttons
        if (ui_doButton(1, singlePlayerButtons[0], buttonLeft, buttonTop, 200, 30) == 1)
		{
			menuChoice = 1;
		}
        
		if (ui_doButton(2, singlePlayerButtons[1], buttonLeft, buttonTop + 35, 200, 30) == 1)
		{
			menuChoice = 2;
		}
        
		if (ui_doButton(3, singlePlayerButtons[2], buttonLeft, buttonTop + 35 * 2, 200, 30) == 1)
		{
			menuChoice = 3;
		}

		if(menuChoice != 0)
		{
			menuState = MM_Leaving;
			initSlidyButtons(0.0f, singlePlayerButtons);
		}

		break;

	case MM_Leaving:
		// Do buttons sliding out and background fading
		// Do the same stuff as in MM_Entering, but backwards
		glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);
		ui_drawImage(0, &background, displaySurface->w - background.imgWidth, 0, 0, 0);

		// "Copyright" text
		fnt_drawTextBox(menuFont, copyrightText, copyrightLeft, copyrightTop, 0, 0, 20);

		drawSlidyButtons();
		updateSlidyButtons(deltaTime);

		if(SlidyButtonState.lerp >= 1.0f)
		{
			menuState = MM_Finish;
		}
		break;

	case MM_Finish:
		// Release the background texture
		GLTexture_Release(&background);

		// Set the next menu to load
		result = menuChoice;

		// And make sure that if we come back to this menu, it resets
		// properly
		menuState = MM_Begin;
	}

	return result;
}

// TODO: I totally fudged the layout of this menu by adding an offset for when
// the game isn't in 640x480.  Needs to be fixed.
int doChooseModule(float deltaTime)
{
	static int menuState = MM_Begin;
	static int startIndex;
	static GLTexture background;
	static int validModules[MAXMODULE];
	static int numValidModules;

	static int moduleMenuOffsetX;
	static int moduleMenuOffsetY;

	int result = 0;
	int i, x, y;
	char txtBuffer[128];

	switch(menuState)
	{
	case MM_Begin:
		//Reload all modules, something might be unlocked
		load_all_menu_images();

		// Load font & background
		GLTexture_Load(&background, "basicdat/menu/menu_sleepy.bmp");
		startIndex = 0;
		selectedModule = -1;

		// Find the module's that we want to allow loading for.  If startNewPlayer
		// is true, we want ones that don't allow imports (e.g. starter modules).
		// Otherwise, we want modules that allow imports
		memset(validModules, 0, sizeof(int) * MAXMODULE);
		numValidModules = 0;
		for (i = 0;i < globalnummodule; i++)
		{
			if (modimportamount[i] == 0)
			{
				if (startNewPlayer)
				{
					validModules[numValidModules] = i;
					numValidModules++;
				}
			} else
			{
				if (!startNewPlayer)
				{
					validModules[numValidModules] = i;
					numValidModules++;
				}
			}
		}

		// Figure out at what offset we want to draw the module menu.
		moduleMenuOffsetX = (displaySurface->w - 640) / 2;
		moduleMenuOffsetY = (displaySurface->h - 480) / 2;

		menuState = MM_Entering;

		// fall through...

	case MM_Entering:
		menuState = MM_Running;
		
		// fall through for now...

	case MM_Running:
		// Draw the background
		glColor4f(1, 1, 1, 1);
		x = (displaySurface->w / 2) - (background.imgWidth / 2);
		y = displaySurface->h - background.imgHeight;
		ui_drawImage(0, &background, x, y, 0, 0);

		// Fudged offset here.. DAMN!  Doesn't work, as the mouse tracking gets skewed
		// I guess I'll do it the uglier way
		//glTranslatef(moduleMenuOffsetX, moduleMenuOffsetY, 0);
		

		// Draw the arrows to pick modules
		if(ui_doButton(1051, "<-", moduleMenuOffsetX + 20, moduleMenuOffsetY + 74, 30, 30))
		{
			startIndex--;
		}

		if(ui_doButton(1052, "->", moduleMenuOffsetX + 590, moduleMenuOffsetY + 74, 30, 30))
		{
			startIndex++;

			if(startIndex + 3 >= numValidModules)
			{
				startIndex = numValidModules - 3;
			}
		}

		// Clamp startIndex to 0
		startIndex = max(0, startIndex);

		// Draw buttons for the modules that can be selected
		x = 93;
		y = 20;
		for(i = startIndex; i < (startIndex + 3) && i < numValidModules; i++)
		{
			if(ui_doImageButton(i, &TxTitleImage[validModules[i]], 
				moduleMenuOffsetX + x, moduleMenuOffsetY + y, 138, 138))
			{
				selectedModule = i;
			}

			x += 138 + 20;	// Width of the button, and the spacing between buttons
		}

		// Draw an unused button as the backdrop for the text for now
		ui_drawButton(0xFFFFFFFF, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 230);

		// And draw the next & back buttons
		if (ui_doButton(53, "Select Module",
			moduleMenuOffsetX + 327, moduleMenuOffsetY + 173, 200, 30))
		{
			// go to the next menu with this module selected
			selectedModule = validModules[selectedModule];
			menuState = MM_Leaving;
		}

		if (ui_doButton(54, "Back", moduleMenuOffsetX + 327, moduleMenuOffsetY + 208, 200, 30))
		{
			// Signal doMenu to go back to the previous menu
			selectedModule = -1;
			menuState = MM_Leaving;
		}

		// Draw the text description of the selected module
		if(selectedModule > -1)
		{
			y = 173 + 5;
			x = 21 + 5;
			glColor4f(1, 1, 1, 1);
			fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y,
				modlongname[validModules[selectedModule]]);
			y += 20;

			snprintf(txtBuffer, 128, "Difficulty: %s", modrank[validModules[selectedModule]]);
			fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer);
			y += 20;

			if(modmaxplayers[validModules[selectedModule]] > 1)
			{
				if(modminplayers[validModules[selectedModule]] == modmaxplayers[validModules[selectedModule]])
				{
					snprintf(txtBuffer, 128, "%d Players", modminplayers[validModules[selectedModule]]);	
				} else
				{
					snprintf(txtBuffer, 128, "%d - %d Players", modminplayers[validModules[selectedModule]], modmaxplayers[validModules[selectedModule]]);
				}
			} else
			{
				snprintf(txtBuffer, 128, "Starter Module");
			}
			fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer);
			y += 20;

			// And finally, the summary
			snprintf(txtBuffer, 128, "modules/%s/gamedat/menu.txt", modloadname[validModules[selectedModule]]);
			get_module_summary(txtBuffer);
			for(i = 0;i < SUMMARYLINES;i++)
			{
				fnt_drawText(menuFont, moduleMenuOffsetX + x, moduleMenuOffsetY + y, modsummary[i]);
				y += 20;
			}
		}

		break;

	case MM_Leaving:
		menuState = MM_Finish;
		// fall through for now
		
	case MM_Finish:
		GLTexture_Release(&background);

		menuState = MM_Begin;

		if(selectedModule == -1)
		{
			result = -1;
		} else
		{
			// Save the name of the module that we've picked
			strncpy(pickedmodule, modloadname[selectedModule], 64);

			// If the module allows imports, return 1.  Else, return 2
			if(modimportamount[selectedModule] > 0) 
			{
				importvalid = btrue;
				importamount = modimportamount[selectedModule];
				result = 1;
			}
			else
			{
				importvalid = bfalse;
				result = 2;
			}

			exportvalid = modallowexport[selectedModule];
			playeramount = modmaxplayers[selectedModule];

			respawnvalid = bfalse;
			respawnanytime = bfalse;
			if(modrespawnvalid[selectedModule]) respawnvalid = btrue;
			if(modrespawnvalid[selectedModule] == ANYTIME) respawnanytime = btrue;

			rtscontrol = bfalse;
		}
		break;
	}
	
	return result;
}

int doChoosePlayer(float deltaTime)
{
	static int menuState = MM_Begin;
	static GLTexture background;
	int result = 0;
	int numVertical, numHorizontal;
	int i, j, x, y;
	int player;
	char srcDir[64], destDir[64];

	switch(menuState)
	{
	case MM_Begin:
		selectedPlayer = 0;
		
		GLTexture_Load(&background, "basicdat/menu/menu_sleepy.bmp");
		
		// load information for all the players that could be imported
		check_player_import("players");

		menuState = MM_Entering;
		// fall through

	case MM_Entering:
		menuState = MM_Running;
		// fall through

	case MM_Running:
		// Figure out how many players we can show without scrolling
		numVertical = 6;
		numHorizontal = 2;

		// Draw the player selection buttons
		// I'm being tricky, and drawing two buttons right next to each other
		// for each player: one with the icon, and another with the name.  I'm 
		// given those buttons the same ID, so they'll act like the same button.
		player = 0;
		x = 20;
		for(j = 0;j < numHorizontal && player < numloadplayer;j++)
		{
			y = 20;
			for(i = 0;i < numVertical && player < numloadplayer;i++)
			{
				if(ui_doImageButtonWithText(player, &TxIcon[player], loadplayername[player],  x, y, 175, 42))
				{
					selectedPlayer = player;
				}

				player++;
				y += 47;
			}
			x += 180;
		}

		// Draw the background
		x = (displaySurface->w / 2) - (background.imgWidth / 2);
		y = displaySurface->h - background.imgHeight;
		ui_drawImage(0, &background, x, y, 0, 0);


		// Buttons for going ahead
		if (ui_doButton(100, "Play!", 40, 350, 200, 30))
		{
			menuState = MM_Leaving;
		}

		if (ui_doButton(101, "Back", 40, 385, 200, 30))
		{
			selectedPlayer = -1;
			menuState = MM_Leaving;
		}

		break;

	case MM_Leaving:
		menuState = MM_Finish;
		// fall through

	case MM_Finish:
		GLTexture_Release(&background);
		menuState = MM_Begin;

		if(selectedPlayer == -1) result = -1;
		else
		{
			// Build the import directory
			// I'm just allowing 1 player for now...
			empty_import_directory();
			fs_createDirectory("import");
						
			localcontrol[0] = INPUTKEY | INPUTMOUSE | INPUTJOYA;
			localslot[0] = localmachine * 9;

			// Copy the character to the import directory
			sprintf(srcDir, "players/%s", loadplayerdir[selectedPlayer]);
			sprintf(destDir, "import/temp%04d.obj", localslot[0]);
			fs_copyDirectory(srcDir, destDir);

			// Copy all of the character's items to the import directory
			for(i = 0;i < 8;i++)
			{
				sprintf(srcDir, "players/%s/%d.obj", loadplayerdir[selectedPlayer], i);
				sprintf(destDir, "import/temp%04d.obj", localslot[0] + i + 1);

				fs_copyDirectory(srcDir, destDir);
			}

			numimport = 1;
			result = 1;
		}

		break;
	}

	return result;
}

//TODO: This needs to be finished
int doOptions(float deltaTime)
{
	static int menuState = MM_Begin;
	static GLTexture background;
	static float lerp;
	static int menuChoice = 0;

	int result = 0;

	switch(menuState)
	{
	case MM_Begin:
		// set up menu variables
		GLTexture_Load(&background, "basicdat/menu/menu_gnome.bmp");
		menuChoice = 0;
		menuState = MM_Entering;

		initSlidyButtons(1.0f, optionsButtons);
		// let this fall through into MM_Entering

	case MM_Entering:
		// do buttons sliding in animation, and background fading in
		// background
		glColor4f(1, 1, 1, 1 -SlidyButtonState.lerp);

		//Draw the background
		ui_drawImage(0, &background, (displaySurface->w - background.imgWidth), 0, 0, 0);

		// "Copyright" text
		fnt_drawTextBox(menuFont, optionsText, optionsTextLeft, optionsTextTop, 0, 0, 20);
	
		drawSlidyButtons();
		updateSlidyButtons(-deltaTime);

		// Let lerp wind down relative to the time elapsed
		if (SlidyButtonState.lerp <= 0.0f)
		{
			menuState = MM_Running;
		}

		break;

	case MM_Running:
		// Do normal run
		// Background
		glColor4f(1, 1, 1, 1);
		ui_drawImage(0, &background, (displaySurface->w - background.imgWidth), 0, 0, 0);

		// "Options" text
		fnt_drawTextBox(menuFont, optionsText, optionsTextLeft, optionsTextTop, 0, 0, 20);

		// Buttons
		if(ui_doButton(1, optionsButtons[0], buttonLeft, buttonTop, 200, 30) == 1)
		{
			//audio options
			menuChoice = 1;
		}

		if(ui_doButton(2, optionsButtons[1], buttonLeft, buttonTop + 35, 200, 30) == 1)
		{
			//input options
			menuChoice = 2;
		}

		if(ui_doButton(3, optionsButtons[2], buttonLeft, buttonTop + 35 * 2, 200, 30) == 1)
		{
			//video options
			menuChoice = 3;
		}

		if(ui_doButton(4, optionsButtons[3], buttonLeft, buttonTop + 35 * 3, 200, 30) == 1)
		{
			//back to main menu
			menuChoice = 4;
		}

		if(menuChoice != 0)
		{
			menuState = MM_Leaving;
			initSlidyButtons(0.0f, optionsButtons);
		}
		break;

	case MM_Leaving:
		// Do buttons sliding out and background fading
		// Do the same stuff as in MM_Entering, but backwards
		glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);
		ui_drawImage(0, &background, (displaySurface->w - background.imgWidth), 0, 0, 0);

		// "Options" text
		fnt_drawTextBox(menuFont, optionsText, optionsTextLeft, optionsTextTop, 0, 0, 20);

		// Buttons
		drawSlidyButtons();
		updateSlidyButtons(deltaTime);
		if(SlidyButtonState.lerp >= 1.0f) {
			menuState = MM_Finish;
		}
		break;

	case MM_Finish:
		// Free the background texture; don't need to hold onto it
		GLTexture_Release(&background);
		menuState = MM_Begin;	// Make sure this all resets next time doMainMenu is called

		// Set the next menu to load
		result = menuChoice;
		break;
	}
	return result;
}

int doAudioOptions(float deltaTime)
{
	static int menuState = MM_Begin;
	static GLTexture background;
	static float lerp;
	static int menuChoice = 0;
	static char Cmaxsoundchannel[128];
	static char Cbuffersize[128];
	static char Csoundvolume[128];
	static char Cmusicvolume[128];

	int result = 0;

	switch(menuState)
	{
	case MM_Begin:
		// set up menu variables
		GLTexture_Load(&background, "basicdat/menu/menu_gnome.bmp");
		menuChoice = 0;
		menuState = MM_Entering;
		// let this fall through into MM_Entering

	case MM_Entering:
		// do buttons sliding in animation, and background fading in
		// background
		glColor4f(1, 1, 1, 1 -SlidyButtonState.lerp);

		//Draw the background
		ui_drawImage(0, &background, (displaySurface->w - background.imgWidth), 0, 0, 0);

		//Load the current settings
		if(soundvalid) audioOptionsButtons[0] = "On";
		else audioOptionsButtons[0] = "Off";

		sprintf(Csoundvolume, "%i", soundvolume);
		audioOptionsButtons[1] = Csoundvolume;

		if(musicvalid) audioOptionsButtons[2] = "On";
		else audioOptionsButtons[2] = "Off";

		sprintf(Cmusicvolume, "%i", musicvolume);
		audioOptionsButtons[3] = Cmusicvolume;

		sprintf(Cmaxsoundchannel, "%i", maxsoundchannel);
		audioOptionsButtons[4] = Cmaxsoundchannel;

		sprintf(Cbuffersize, "%i", buffersize);
		audioOptionsButtons[5] = Cbuffersize;

		//Fall trough
		menuState = MM_Running;
		break;

	case MM_Running:
		// Do normal run
		// Background
		glColor4f(1, 1, 1, 1);
		ui_drawImage(0, &background, (displaySurface->w - background.imgWidth), 0, 0, 0);

		fnt_drawTextBox(menuFont, "Sound:", buttonLeft, displaySurface->h-270, 0, 0, 20);
		// Buttons
		if(ui_doButton(1, audioOptionsButtons[0], buttonLeft + 150, displaySurface->h-270, 100, 30) == 1)
		{
			if(soundvalid) 
			{
				soundvalid = bfalse;
				audioOptionsButtons[0] = "Off";
			}
			else
			{
				soundvalid = btrue;
				audioOptionsButtons[0] = "On";
			}
		}

		fnt_drawTextBox(menuFont, "Sound Volume:", buttonLeft, displaySurface->h-235, 0, 0, 20);
		if(ui_doButton(2, audioOptionsButtons[1], buttonLeft + 150, displaySurface->h-235, 100, 30) == 1)
		{
			sprintf(Csoundvolume, "%i", soundvolume);
			audioOptionsButtons[1] = Csoundvolume;
		}

		fnt_drawTextBox(menuFont, "Music:", buttonLeft, displaySurface->h-165, 0, 0, 20);
		if(ui_doButton(3, audioOptionsButtons[2], buttonLeft + 150, displaySurface->h-165, 100, 30) == 1)
		{
			if(musicvalid) 
			{
				musicvalid = bfalse;
				audioOptionsButtons[2] = "Off";
			}
			else
			{
				musicvalid = btrue;
				audioOptionsButtons[2] = "On";
			}
		}

		fnt_drawTextBox(menuFont, "Music Volume:", buttonLeft, displaySurface->h-130, 0, 0, 20);
		if(ui_doButton(4, audioOptionsButtons[3], buttonLeft + 150, displaySurface->h-130, 100, 30) == 1)
		{
			sprintf(Cmusicvolume, "%i", musicvolume);
			audioOptionsButtons[3] = Cmusicvolume;
		}

		fnt_drawTextBox(menuFont, "Sound Channels:", buttonLeft + 300, displaySurface->h-200, 0, 0, 20);
		if(ui_doButton(5, audioOptionsButtons[4], buttonLeft + 450, displaySurface->h-200, 100, 30) == 1)
		{
			switch(maxsoundchannel)
			{
				case 128:
					maxsoundchannel = 8;
				break;

				case 8:
					maxsoundchannel = 16;
				break;

				case 16:
					maxsoundchannel = 24;
				break;

				case 24:
					maxsoundchannel = 32;
				break;

				case 32:
					maxsoundchannel = 64;
				break;

				case 64:
					maxsoundchannel = 128;
				break;

				default:
					maxsoundchannel = 16;
				break;
			}

			sprintf(Cmaxsoundchannel, "%i", maxsoundchannel);
			audioOptionsButtons[4] = Cmaxsoundchannel;
		}

		fnt_drawTextBox(menuFont, "Buffer Size:", buttonLeft + 300, displaySurface->h-165, 0, 0, 20);
		if(ui_doButton(6, audioOptionsButtons[5], buttonLeft + 450, displaySurface->h-165, 100, 30) == 1)
		{
			switch(buffersize)
			{
				case 8192:
					buffersize = 512;
				break;

				case 512:
					buffersize = 1024;
				break;

				case 1024:
					buffersize = 2048;
				break;

				case 2048:
					buffersize = 4096;
				break;

				case 4096:
					buffersize = 8192;
				break;

				default:
					buffersize = 2048;
				break;
			}
			sprintf(Cbuffersize, "%i", buffersize);
			audioOptionsButtons[5] = Cbuffersize;
		}

		if(ui_doButton(7, audioOptionsButtons[6], buttonLeft, displaySurface->h-60, 200, 30) == 1)
		{
			//save settings and go back
			save_settings();
			if(musicvalid) play_music(0, 0, -1);
			else if(mixeron) Mix_PauseMusic();
			if(!musicvalid && !soundvalid)
			{
				Mix_CloseAudio();
				mixeron = bfalse;
			}
			menuState = MM_Leaving;
		}

		break;

	case MM_Leaving:
		// Do buttons sliding out and background fading
		// Do the same stuff as in MM_Entering, but backwards
		glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);
		ui_drawImage(0, &background, (displaySurface->w - background.imgWidth), 0, 0, 0);

		//Fall trough
		menuState = MM_Finish;
		break;

	case MM_Finish:
		// Free the background texture; don't need to hold onto it
		GLTexture_Release(&background);
		menuState = MM_Begin;	// Make sure this all resets next time doMainMenu is called

		// Set the next menu to load
		result = 1;
		break;
	}
	return result;
}

int doVideoOptions(float deltaTime)
{
	static int menuState = MM_Begin;
	static GLTexture background;
	static float lerp;
	static int menuChoice = 0;
	int result = 0;
	static char Cmaxmessage[128];
	static char Cscrd[128];
	static char Cscrz[128];

	switch(menuState)
	{
	case MM_Begin:
		// set up menu variables
		GLTexture_Load(&background, "basicdat/menu/menu_gnome.bmp");
		menuChoice = 0;
		menuState = MM_Entering;		// let this fall through into MM_Entering

	case MM_Entering:
		// do buttons sliding in animation, and background fading in
		// background
		glColor4f(1, 1, 1, 1 -SlidyButtonState.lerp);

		//Draw the background
		ui_drawImage(0, &background, (displaySurface->w - background.imgWidth), 0, 0, 0);

		//Load all the current video settings
		if(antialiasing) videoOptionsButtons[0] = "On";
		else videoOptionsButtons[0] = "Off";

		switch(scrd)
		{
		case 16: 
			videoOptionsButtons[1] = "Low";
		break;
		case 24: 
			videoOptionsButtons[1] = "Medium";
		break;
		case 32: 
			videoOptionsButtons[1] = "High";
		break;
		default:									//Set to defaults
			videoOptionsButtons[1] = "Low";
			scrd = 16;
		break;
		}

		switch(texturefilter)
		{
		case 1: 
			videoOptionsButtons[5] = "Linear";
		break;
		case 2: 
			videoOptionsButtons[5] = "Bilinear";
		break;
		case 3: 
			videoOptionsButtons[5] = "Trilinear";
		break;
		case 4: 
			videoOptionsButtons[5] = "Ansiotropic";
		break;
		default:									//Set to defaults
			videoOptionsButtons[5] = "Linear";
			texturefilter = 1;
		break;
		}

		if(dither & !shading) videoOptionsButtons[2] = "Yes";
		else										//Set to defaults
		{
			videoOptionsButtons[2] = "No";
			dither = bfalse;						
			shading = btrue;
		}

		if(fullscreen) videoOptionsButtons[3] = "True";
		else videoOptionsButtons[3] = "False";

		if(refon) 
		{
			videoOptionsButtons[4] = "Low";
			if(reffadeor == 0)
			{
				videoOptionsButtons[4] = "Medium";
				if(zreflect) videoOptionsButtons[4] = "High";
			}
		}
		else videoOptionsButtons[4] = "Off";


		sprintf(Cmaxmessage, "%i", maxmessage);
		if(maxmessage > MAXMESSAGE || maxmessage < 0) maxmessage = MAXMESSAGE-1;
		if(maxmessage == 0) sprintf(Cmaxmessage, "None");								//Set to default
		videoOptionsButtons[11] = Cmaxmessage;

		if(shaon) 
		{
			videoOptionsButtons[6] = "Normal";
			if(!shasprite) videoOptionsButtons[6] = "Best";
		}
		else videoOptionsButtons[6] = "Off";

		if(scrz != 32 && scrz != 16 && scrz != 24)
		{
			scrz = 16;							//Set to default
		}
		sprintf(Cscrz, "%i", scrz);				//Convert the integer to a char we can use
		videoOptionsButtons[7] = Cscrz;

		if(fogallowed) videoOptionsButtons[8] = "Enable";
		else videoOptionsButtons[8] = "Disable";

		if(phongon)
		{
			videoOptionsButtons[9] = "Okay";
			if(overlayvalid && backgroundvalid)
			{
				videoOptionsButtons[9] = "Good";
				if(perspective) videoOptionsButtons[9] = "Superb";
			}
			else														//Set to defaults
			{
				perspective = bfalse;
				backgroundvalid = bfalse;
				overlayvalid = bfalse;
			}
		}
		else															//Set to defaults
		{
			perspective = bfalse;
			backgroundvalid = bfalse;
			overlayvalid = bfalse;
			videoOptionsButtons[9] = "Off";
		}

		if(twolayerwateron) videoOptionsButtons[10] = "On";
		else videoOptionsButtons[10] = "Off";

		switch(scrx)
		{
			case 1024: videoOptionsButtons[12] = "1024X768";	
			break;

			case 640: videoOptionsButtons[12] = "640X480";	
			break;

			case 800: videoOptionsButtons[12] = "800X600";	
			break;

			default: 
				scrx = 640;
				scry = 480;
				videoOptionsButtons[12] = "640X480";	
			break;
			}

		menuState = MM_Running;
		break;

	case MM_Running:
		// Do normal run
		// Background
		glColor4f(1, 1, 1, 1);
		ui_drawImage(0, &background, (displaySurface->w - background.imgWidth), 0, 0, 0);

		//Antialiasing Button
		fnt_drawTextBox(menuFont, "Antialiasing:", buttonLeft, displaySurface->h-215, 0, 0, 20);
		if(ui_doButton(1, videoOptionsButtons[0], buttonLeft + 150, displaySurface->h-215, 100, 30) == 1)
		{
			if(antialiasing == btrue) 
			{
				videoOptionsButtons[0] = "Off";
				antialiasing = bfalse;
			}else
			{
				videoOptionsButtons[0] = "On";
				antialiasing = btrue;
			}
		}

	    //Color depth
		fnt_drawTextBox(menuFont, "Texture Quality:", buttonLeft, displaySurface->h-180, 0, 0, 20);
		if(ui_doButton(2, videoOptionsButtons[1], buttonLeft + 150, displaySurface->h-180, 100, 30) == 1)
		{
			switch(scrd)
			{
				case 32:
					videoOptionsButtons[1] = "Low";
					scrd = 16;
				break;

				case 16:
					videoOptionsButtons[1] = "Medium";
					scrd = 24;
				break;

				case 24:
					videoOptionsButtons[1] = "High";
					scrd = 32;
				break;

				default:
					videoOptionsButtons[1] = "Low";
					scrd = 16;
				break;
			}
		}

		//Dithering and Gourad Shading
		fnt_drawTextBox(menuFont, "Fast and Ugly:", buttonLeft, displaySurface->h-145, 0, 0, 20);
		if(ui_doButton(3, videoOptionsButtons[2], buttonLeft + 150, displaySurface->h-145, 100, 30) == 1)
		{
			if(dither && !shading) 
			{
				videoOptionsButtons[2] = "No";
				dither = bfalse;
				shading = btrue;
			}else
			{
				videoOptionsButtons[2] = "Yes";
				dither = btrue;
				shading = bfalse;
			}
		}

		//Fullscreen
		fnt_drawTextBox(menuFont, "Fullscreen:", buttonLeft, displaySurface->h-110, 0, 0, 20);
		if(ui_doButton(4, videoOptionsButtons[3], buttonLeft + 150, displaySurface->h-110, 100, 30) == 1)
		{
			if(fullscreen == btrue) 
			{
				videoOptionsButtons[3] = "False";
				fullscreen = bfalse;
			}else
			{
				videoOptionsButtons[3] = "True";
				fullscreen = btrue;
			}
		}

		//Reflection
		fnt_drawTextBox(menuFont, "Reflections:", buttonLeft, displaySurface->h-250, 0, 0, 20);
		if(ui_doButton(5, videoOptionsButtons[4], buttonLeft + 150, displaySurface->h-250, 100, 30) == 1)
		{
			if(refon && reffadeor==0 && zreflect)
			{
				refon = bfalse;
				reffadeor = bfalse;
				zreflect = bfalse;
				videoOptionsButtons[4] = "Off";
			}
			else
			{
				if(refon && reffadeor==255 && !zreflect)
				{
					videoOptionsButtons[4] = "Medium";
					reffadeor = 0;
					zreflect = bfalse;
				}
				else
				{
					if(refon && reffadeor==0 && !zreflect)
					{
						videoOptionsButtons[4] = "High";
						zreflect = btrue;
					}
					else
					{
						refon = btrue;
						reffadeor = 255;				//Just in case so we dont get stuck at "Low"
						videoOptionsButtons[4] = "Low";
						zreflect = bfalse;
					}
				}
			}
		}

		//Texture Filtering
		fnt_drawTextBox(menuFont, "Texture Filtering:", buttonLeft, displaySurface->h-285, 0, 0, 20);
		if(ui_doButton(6, videoOptionsButtons[5], buttonLeft + 150, displaySurface->h-285, 130, 30) == 1)
		{
			switch(texturefilter)
			{
				case 4:
					texturefilter = 1;
					videoOptionsButtons[5] = "Linear";
				break;

				case 1:
					texturefilter = 2;
					videoOptionsButtons[5] = "Bilinear";
				break;

				case 2:
					texturefilter = 3;
					videoOptionsButtons[5] = "Trilinear";
				break;

				case 3:
					texturefilter = 4;
					videoOptionsButtons[5] = "Anisotropic";
				break;


				default:
					texturefilter = 1;
					videoOptionsButtons[5] = "Linear";
				break;
			}
		}

		//Shadows
		fnt_drawTextBox(menuFont, "Shadows:", buttonLeft, displaySurface->h-320, 0, 0, 20);
		if(ui_doButton(7, videoOptionsButtons[6], buttonLeft + 150, displaySurface->h-320, 100, 30) == 1)
		{
			if(shaon && !shasprite)
			{
				shaon = bfalse;
				shasprite = bfalse;								//Just in case
				videoOptionsButtons[6] = "Off";
			}
			else
			{
				if(shaon && shasprite)
				{
					videoOptionsButtons[6] = "Best";
					shasprite = bfalse;
				}
				else
				{
					shaon = btrue;
					shasprite = btrue;
					videoOptionsButtons[6] = "Normal";
				}
			}
		}

		//Z bit
		fnt_drawTextBox(menuFont, "Z Bit:", buttonLeft + 300, displaySurface->h-320, 0, 0, 20);
		if(ui_doButton(8, videoOptionsButtons[7], buttonLeft + 450, displaySurface->h-320, 100, 30) == 1)
		{
			switch(scrz)
			{
				case 32:
					videoOptionsButtons[7] = "16";
					scrz = 16;
				break;

				case 16:
					videoOptionsButtons[7] = "24";
					scrz = 24;
				break;

				case 24:
					videoOptionsButtons[7] = "32";
					scrz = 32;
				break;

				default:
					videoOptionsButtons[7] = "16";
					scrz = 16;
				break;
			}
		}

		//Fog
		fnt_drawTextBox(menuFont, "Fog Effects:", buttonLeft + 300, displaySurface->h-285, 0, 0, 20);
		if(ui_doButton(9, videoOptionsButtons[8], buttonLeft + 450, displaySurface->h-285, 100, 30) == 1)
		{
			if(fogallowed)
			{
				fogallowed = bfalse;
				videoOptionsButtons[8] = "Disable";
			}
			else
			{
				fogallowed = btrue;
				videoOptionsButtons[8] = "Enable";
			}
		}

		//Perspective correction and phong mapping
		fnt_drawTextBox(menuFont, "3D Effects:", buttonLeft + 300, displaySurface->h-250, 0, 0, 20);
		if(ui_doButton(10, videoOptionsButtons[9], buttonLeft + 450, displaySurface->h-250, 100, 30) == 1)
		{
			if(phongon && perspective && overlayvalid && backgroundvalid)
			{
				phongon = bfalse;
				perspective = bfalse;
				overlayvalid = bfalse;
				backgroundvalid = bfalse;
				videoOptionsButtons[9] = "Off";
			}
			else
			{
				if(!phongon)
				{
					videoOptionsButtons[9] = "Okay";
					phongon = btrue;
				}
				else
				{
					if(!perspective && overlayvalid && backgroundvalid)
					{
						videoOptionsButtons[9] = "Superb";
						perspective = btrue;
					}
					else
					{
						overlayvalid = btrue;
						backgroundvalid = btrue;
						videoOptionsButtons[9] = "Good";
					}
				}
			}
		}

		//Water Quality
		fnt_drawTextBox(menuFont, "Good Water:", buttonLeft + 300, displaySurface->h-215, 0, 0, 20);
		if(ui_doButton(11, videoOptionsButtons[10], buttonLeft + 450, displaySurface->h-215, 100, 30) == 1)
		{
			if(twolayerwateron)
			{
				videoOptionsButtons[10] = "Off";
				twolayerwateron = bfalse;
			}
			else
			{
				videoOptionsButtons[10] = "On";
				twolayerwateron = btrue;
			}
		}

		//Text messages
		fnt_drawTextBox(menuFont, "Max  Messages:", buttonLeft + 300, displaySurface->h-145, 0, 0, 20);
		if(ui_doButton(12, videoOptionsButtons[11], buttonLeft + 450, displaySurface->h-145, 75, 30) == 1)
		{
			if(maxmessage != MAXMESSAGE)
			{
				maxmessage++;
				messageon = btrue;
				sprintf(Cmaxmessage, "%i", maxmessage);			//Convert integer to a char we can use
			}
			else 
			{
				maxmessage = 0;
				messageon = bfalse;
				sprintf(Cmaxmessage, "None");
			}
			videoOptionsButtons[11] = Cmaxmessage;
		}

		//Screen Resolution
		fnt_drawTextBox(menuFont, "Resolution:", buttonLeft + 300, displaySurface->h-110, 0, 0, 20);
		if(ui_doButton(13, videoOptionsButtons[12], buttonLeft + 450, displaySurface->h-110, 125, 30) == 1)
		{
			//TODO: add widescreen support
			switch(scrx)
			{
				case 1024:
					scrx = 640;
					scry = 480;
					videoOptionsButtons[12] = "640X480";	
				break;

				case 640:
					scrx = 800;
					scry = 600;
					videoOptionsButtons[12] = "800X600";	
				break;

				case 800:
					scrx = 1024;
					scry = 768;
					videoOptionsButtons[12] = "1024X768";	
				break;

				default: 
					scrx = 640;
					scry = 480;
					videoOptionsButtons[12] = "640X480";	
				break;
			}
		}

		//Save settings button
		if(ui_doButton(14, videoOptionsButtons[13], buttonLeft, displaySurface->h-60, 200, 30) == 1)
		{
			menuChoice = 1;
			save_settings();

			//Reload some of the graphics
			load_graphics();
		}

		if(menuChoice != 0)
		{
			menuState = MM_Leaving;
			initSlidyButtons(0.0f, videoOptionsButtons);
		}
		break;

	case MM_Leaving:
		// Do buttons sliding out and background fading
		// Do the same stuff as in MM_Entering, but backwards
		glColor4f(1, 1, 1, 1 - SlidyButtonState.lerp);
		ui_drawImage(0, &background, (displaySurface->w - background.imgWidth), 0, 0, 0);

		// "Options" text
		fnt_drawTextBox(menuFont, optionsText, optionsTextLeft, optionsTextTop, 0, 0, 20);

		//Fall trough
		menuState = MM_Finish;
		break;

	case MM_Finish:
		// Free the background texture; don't need to hold onto it
		GLTexture_Release(&background);
		menuState = MM_Begin;	// Make sure this all resets next time doMainMenu is called

		// Set the next menu to load
		result = menuChoice;
		break;
	}
	return result;
}

int doShowMenuResults(float deltaTime)
{
	int x, y;
	char text[128];
	Font *font;

	SDL_Surface *screen = SDL_GetVideoSurface();
	font = ui_getFont();

	ui_drawButton(0xFFFFFFFF, 30, 30, screen->w - 60, screen->h - 65);

	x = 35;
	y = 35;
	glColor4f(1, 1, 1, 1);
	snprintf(text, 128, "Module selected: %s", modloadname[selectedModule]);
	fnt_drawText(font, x, y, text);
	y += 35;

	if(importvalid)
	{
		snprintf(text, 128, "Player selected: %s", loadplayername[selectedPlayer]);
	} else
	{
		snprintf(text, 128, "Starting a new player.");
	}
	fnt_drawText(font, x, y, text);
	
	return 1;
}

int doNotImplemented(float deltaTime)
{
	int x, y;
	int w, h;
	char notImplementedMessage[] = "Not implemented yet!  Check back soon!";


	fnt_getTextSize(ui_getFont(), notImplementedMessage, &w, &h);
	w += 50; // add some space on the sides

	x = displaySurface->w / 2 - w / 2;
	y = displaySurface->h / 2 - 17;

	if(ui_doButton(1, notImplementedMessage, x, y, w, 30) == 1)
	{
		return 1;
	}

	return 0;
}

// All the different menus.  yay!
enum
{
	MainMenu,
	SinglePlayer,
	MultiPlayer,
	ChooseModule,
	ChoosePlayer,
	TestResults,
	Options,
	VideoOptions,
	AudioOptions,
	InputOptions,
	NewPlayer,
	LoadPlayer,
	HostGame,
	JoinGame,
};

int doMenu(float deltaTime)
{
	static int whichMenu = MainMenu;
	static int lastMenu = MainMenu;
	int result = 0;

	switch(whichMenu)
	{
	case MainMenu:
		result = doMainMenu(deltaTime);
		if(result != 0)
		{
			lastMenu = MainMenu;
			if(result == 1) whichMenu = SinglePlayer;
			else if(result == 2) whichMenu = MultiPlayer;
			else if(result == 3) whichMenu = Options;
			else if(result == 4) return -1;	// need to request a quit somehow
		}
		break;

	case SinglePlayer:
		result = doSinglePlayerMenu(deltaTime);
		if(result != 0)
		{
			lastMenu = SinglePlayer;
			if(result == 1)
			{
				whichMenu = ChooseModule;
				startNewPlayer = btrue;
			} else if(result == 2)
			{
				whichMenu = ChooseModule;
				startNewPlayer = bfalse;
			}
			else if(result == 3) whichMenu = MainMenu;
			else whichMenu = NewPlayer;
		}
		break;

	case ChooseModule:
		result = doChooseModule(deltaTime);
		if(result == -1) whichMenu = lastMenu;
		else if(result == 1) whichMenu = ChoosePlayer;
		else if(result == 2) whichMenu = TestResults;
		break;

	case ChoosePlayer:
		result = doChoosePlayer(deltaTime);
		if(result == -1)		whichMenu = ChooseModule;
		else if(result == 1)	whichMenu = TestResults;
		
		break;

	case Options:
		result = doOptions(deltaTime);
		if(result != 0)
		{
			if(result == 1) whichMenu = AudioOptions;
			else if(result == 2) whichMenu = InputOptions;
			else if(result == 3) whichMenu = VideoOptions;
			else if(result == 4) whichMenu = MainMenu;
		}
		break;

	case AudioOptions:
		result = doAudioOptions(deltaTime);
		if(result != 0)
		{
			whichMenu = Options;
		}
		break;

	case VideoOptions:
		result = doVideoOptions(deltaTime);
		if(result != 0)
		{
			whichMenu = Options;
		}
		break;

	case TestResults:
		result = doShowMenuResults(deltaTime);
		if(result != 0)
		{
			whichMenu = MainMenu;
			return 1;
		}
		break;

	default:
		result = doNotImplemented(deltaTime);
		if(result != 0)
		{
			whichMenu = lastMenu;
		}
	}

	return 0;
}

void menu_frameStep()
{

}

void save_settings()
{
	//This function saves all current game settings to setup.txt
	FILE* setupfile;
	char write[256];
	char *TxtTmp;

    setupfile = fopen("setup.txt", "w");
    if(setupfile)
    {
		/*GRAPHIC PART*/
		fputs("{GRAPHIC}\n", setupfile);
		sprintf(write, "[MAX_NUMBER_VERTICES] : \"%i\"\n", maxtotalmeshvertices / 1024);
		fputs(write, setupfile);
		sprintf(write, "[COLOR_DEPTH] : \"%i\"\n", scrd);
		fputs(write, setupfile);
		sprintf(write, "[Z_DEPTH] : \"%i\"\n", scrz);
		fputs(write, setupfile);
		if(fullscreen) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[FULLSCREEN] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		if(zreflect) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[Z_REFLECTION] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		sprintf(write, "[SCREENSIZE_X] : \"%i\"\n", scrx);
		fputs(write, setupfile);
		sprintf(write, "[SCREENSIZE_Y] : \"%i\"\n", scry);
		fputs(write, setupfile);
		sprintf(write, "[MAX_TEXT_MESSAGE] : \"%i\"\n", maxmessage);
		fputs(write, setupfile);
		if(staton) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[STATUS_BAR] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		if(perspective) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[PERSPECTIVE_CORRECT] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		switch(texturefilter)
		{
			case 1:	TxtTmp = "LINEAR";
			break;
			case 2: TxtTmp = "BILINEAR";
			break;
			case 3: TxtTmp = "TRILINEAR";
			break;
			case 4: TxtTmp = "ANISOTROPIC";
			break;
			default: 
				TxtTmp = "LINEAR";
				texturefilter = 1;
			break;
		}
		sprintf(write, "[TEXTURE_FILTERING] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		if(shading) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[GOURAUD_SHADING] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		if(antialiasing) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[ANTIALIASING] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		if(dither) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[DITHERING] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		if(refon) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[REFLECTION] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		if(shaon) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[SHADOWS] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		if(shasprite) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[SHADOW_AS_SPRITE] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		if(phongon) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[PHONG] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		if(fogallowed) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[FOG] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		if(reffadeor == 0) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[FLOOR_REFLECTION_FADEOUT] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		if(twolayerwateron) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[MULTI_LAYER_WATER] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		if(overlayvalid) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[OVERLAY] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		if(backgroundvalid) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[BACKGROUND] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);

		/*SOUND PART*/
		sprintf(write, "\n{SOUND}\n");
		fputs(write, setupfile);
		if(musicvalid) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[MUSIC] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		sprintf(write, "[MUSIC_VOLUME] : \"%i\"\n", musicvolume);
		fputs(write, setupfile);
		if(soundvalid) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[SOUND] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		sprintf(write, "[SOUND_VOLUME] : \"%i\"\n", soundvolume);
		fputs(write, setupfile);
		sprintf(write, "[OUTPUT_BUFFER_SIZE] : \"%i\"\n", buffersize);
		fputs(write, setupfile);
		sprintf(write, "[MAX_SOUND_CHANNEL] : \"%i\"\n", maxsoundchannel);
		fputs(write, setupfile);

		/*CAMERA PART*/
		sprintf(write, "\n{CONTROL}\n");
		fputs(write, setupfile);
		switch(autoturncamera)
		{
			case 255: TxtTmp = "GOOD";
			break;
			case btrue: TxtTmp = "TRUE";
			break;
			case bfalse: TxtTmp = "FALSE";
			break;
		}
		sprintf(write, "[AUTOTURN_CAMERA] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);

		/*NETWORK PART*/
		sprintf(write, "\n{NETWORK}\n");
		fputs(write, setupfile);
		if(networkon) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[NETWORK_ON] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		TxtTmp = nethostname;
		sprintf(write, "[HOST_NAME] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		TxtTmp = netmessagename;
		sprintf(write, "[MULTIPLAYER_NAME] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		sprintf(write, "[LAG_TOLERANCE] : \"%i\"\n", lag);
		fputs(write, setupfile);

		/*DEBUG PART*/
		sprintf(write, "\n{DEBUG}\n");
		fputs(write, setupfile);
		if(fpson) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[DISPLAY_FPS] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		if(gHideMouse) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[HIDE_MOUSE] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);
		if(gGrabMouse) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
		sprintf(write, "[GRAB_MOUSE] : \"%s\"\n", TxtTmp);
		fputs(write, setupfile);

		//Close it up
		fclose(setupfile);
	}
	else
	{
		log_warning("Cannot open setup.txt to write new settings into.\n");
	}
}

/* Old Menu Code */

#if 0
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
