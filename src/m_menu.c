// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief SRB2 selection menu, options, sliders and icons. Kinda widget stuff.
///
///	\warning: \n
///	All V_DrawPatchDirect() has been replaced by V_DrawScaledPatch()
///	so that the menu is scaled to the screen size. The scaling is always
///	an integer multiple of the original size, so that the graphics look
///	good.

#ifdef __GNUC__
#include <unistd.h>
#endif

#include "am_map.h"

#include "doomdef.h"
#include "dstrings.h"
#include "d_main.h"
#include "d_netcmd.h"

#include "console.h"

#include "r_local.h"
#include "hu_stuff.h"
#include "g_game.h"
#include "g_input.h"

#include "m_argv.h"
#include "m_anigif.h"
#include "m_misc.h"

// Data.
#include "sounds.h"
#include "s_sound.h"
#include "i_system.h"

#include "m_menu.h"

// Addfile
#include "filesrch.h"

#include "v_video.h"
#include "i_video.h"

#include "keys.h"
#include "z_zone.h"
#include "w_wad.h"
#include "p_local.h"

#include "f_finale.h"

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

#include "m_misc.h"

#include "byteptr.h"

#include "st_stuff.h"

#include "i_sound.h"

#ifdef PC_DOS
#include <stdio.h> // for snprintf
int	snprintf(char *str, size_t n, const char *fmt, ...);
//int	vsnprintf(char *str, size_t n, const char *fmt, va_list ap);
#endif

boolean menuactive = false;
boolean fromlevelselect = false;
static boolean pandoralevelselect = false;
static INT32 fromloadgame = 0;

customsecrets_t customsecretinfo[15];
INT32 inlevelselect = 0;

static INT32 lastmapnum;
static INT32 oldlastmapnum;

#define SKULLXOFF -32
#define LINEHEIGHT 16
#define STRINGHEIGHT 8
#define FONTBHEIGHT 20
#define SMALLLINEHEIGHT 8
#define SLIDER_RANGE 10
#define SLIDER_WIDTH (8*SLIDER_RANGE+6)
#define MAXSTRINGLENGTH 32

// Stuff for customizing the player select screen Tails 09-22-2003
description_t description[15] =
{
	{"             Fastest\n                 Speed Thok\n             Not a good pick\nfor starters, but when\ncontrolled properly,\nSonic is the most\npowerful of the three.", "SONCCHAR", "", "SONIC"},
	{"             Slowest\n                 Fly/Swim\n             Good for\nbeginners. Tails\nhandles the best. His\nflying and swimming\nwill come in handy.", "TAILCHAR", "", "TAILS"},
	{"             Medium\n                 Glide/Climb\n             A well rounded\nchoice, Knuckles\ncompromises the speed\nof Sonic with the\nhandling of Tails.", "KNUXCHAR", "", "KNUCKLES"},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
	{"             Unknown\n                 Unknown\n             None", "SONCCHAR", "", ""},
};

static INT32 saveSlotSelected = 0;
static char joystickInfo[8][25];

typedef struct
{
	char playername[SKINNAMESIZE];
	char levelname[32];
	UINT8 actnum;
	UINT8 skincolor;
	UINT8 skinnum;
	UINT8 numemeralds;
	INT32 lives;
	INT32 continues;
	INT32 gamemap;
	UINT8 netgame;
} saveinfo_t;

static saveinfo_t savegameinfo[10]; // Extra info about the save games.

#ifndef NONET
static char setupm_ip[16];
#endif
INT16 startmap; // Mario, NiGHTS, or just a plain old normal game?

static INT16 itemOn = 1; // menu item skull is on, Hack by Tails 09-18-2002
static INT16 skullAnimCounter = 10; // skull animation counter

//
// PROTOTYPES
//
static void M_DrawSaveLoadBorder(INT32 x,INT32 y);

static void M_DrawThermo(INT32 x,INT32 y,consvar_t *cv);
static void M_DrawSlider(INT32 x, INT32 y, const consvar_t *cv);
static void M_CentreText(INT32 y, const char *string); // write text centered
static void M_CustomLevelSelect(INT32 choice);
static void M_StopMessage(INT32 choice);
static void M_GameOption(INT32 choice);

static void M_GametypeOptions(INT32 choice);
static void M_Setup1PControlsMenu(INT32 choice);

#if defined (HWRENDER) && defined (SHUFFLE)
static void M_OpenGLOption(INT32 choice);
#endif

static void M_Addons(INT32 choice);
static void M_AddonsOptions(INT32 choice);
static patch_t *addonsp[NUM_EXT+5];

static void M_DrawAddons(void);
static void M_DrawMessageMenu(void);

#define numaddonsshown 4

static const char *ALREADYPLAYING = "You are already playing.\nDo you wish to end the\ncurrent game? (Y/N)\n";

// current menudef
menu_t *currentMenu = &MainDef;
//===========================================================================
//Generic Stuffs (more easy to create menus :))
//===========================================================================

static void M_DrawMenuTitle(void)
{
	if (currentMenu->menutitlepic)
	{
		patch_t *p = W_CachePatchName(currentMenu->menutitlepic, PU_CACHE);

		INT32 xtitle = (BASEVIDWIDTH - SHORT(p->width))/2;
		INT32 ytitle = (currentMenu->y - SHORT(p->height))/2;

		if (xtitle < 0)
			xtitle = 0;
		if (ytitle < 0)
			ytitle = 0;
		V_DrawScaledPatch(xtitle, ytitle, 0, p);
	}
}

void M_DrawGenericMenu(void)
{
	INT32 x, y, i, cursory = 0;

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;

	// draw title (or big pic)
	M_DrawMenuTitle();

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (i == itemOn)
			cursory = y;
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_PATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
				{
					if (currentMenu->menuitems[i].status & IT_CENTER)
					{
						patch_t *p;
						p = W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE);
						V_DrawScaledPatch((BASEVIDWIDTH - SHORT(p->width))/2, y, 0, p);
					}
					else
					{
						V_DrawScaledPatch(x, y, 0,
							W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE));
					}
				}
			case IT_NOTHING:
			case IT_DYBIGSPACE:
				y += LINEHEIGHT;
				break;
			case IT_BIGSLIDER:
				M_DrawThermo(x, y, (consvar_t *)currentMenu->menuitems[i].itemaction);
				y += LINEHEIGHT;
				break;
			case IT_STRING:
			case IT_WHITESTRING:
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
				if (i == itemOn)
					cursory = y;

				if ((currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawString(x, y, V_YELLOWMAP, currentMenu->menuitems[i].text);

				// Cvar specific handling
				switch (currentMenu->menuitems[i].status & IT_TYPE)
					case IT_CVAR:
					{
						consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
						switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_SLIDER:
								M_DrawSlider(x, y, cv);
							case IT_CV_NOPRINT: // color use this
								break;
							case IT_CV_STRING:
								M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
								V_DrawString(x + 8, y + 12, V_ALLOWLOWERCASE, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawCharacter(x + 8 + V_StringWidth(cv->string), y + 12,
										'_' | 0x80, false);
								y += 16;
								break;
							default:
								V_DrawString(BASEVIDWIDTH - x - V_StringWidth(cv->string), y,
									V_YELLOWMAP, cv->string);
								break;
						}
						break;
					}
					y += STRINGHEIGHT;
					break;
			case IT_STRING2:
				V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
			case IT_DYLITLSPACE:
				y += SMALLLINEHEIGHT;
				break;
			case IT_GRAYPATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
					V_DrawMappedPatch(x, y, 0,
						W_CachePatchName(currentMenu->menuitems[i].patch,PU_CACHE), graymap);
				y += LINEHEIGHT;
				break;
		}
	}

	// DRAW THE SKULL CURSOR
	if (((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_PATCH)
		|| ((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_NOTHING))
	{
		V_DrawScaledPatch(currentMenu->x + SKULLXOFF, cursory - 5, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
	}
	else
	{
		V_DrawScaledPatch(currentMenu->x - 24, cursory, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
		V_DrawString(currentMenu->x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);
	}
}

static void M_DrawCenteredMenu(void)
{
	INT32 x, y, i, cursory = 0;

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;

	// draw title (or big pic)
	M_DrawMenuTitle();

	for (i = 0; i < currentMenu->numitems; i++)
	{
		if (i == itemOn)
			cursory = y;
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_PATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
				{
					if (currentMenu->menuitems[i].status & IT_CENTER)
					{
						patch_t *p;
						p = W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE);
						V_DrawScaledPatch((BASEVIDWIDTH - SHORT(p->width))/2, y, 0, p);
					}
					else
					{
						V_DrawScaledPatch(x, y, 0,
							W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE));
					}
				}
			case IT_NOTHING:
			case IT_DYBIGSPACE:
				y += LINEHEIGHT;
				break;
			case IT_BIGSLIDER:
				M_DrawThermo(x, y, (consvar_t *)currentMenu->menuitems[i].itemaction);
				y += LINEHEIGHT;
				break;
			case IT_STRING:
			case IT_WHITESTRING:
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
				if (i == itemOn)
					cursory = y;

				if ((currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawCenteredString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawCenteredString(x, y, V_YELLOWMAP, currentMenu->menuitems[i].text);

				// Cvar specific handling
				switch(currentMenu->menuitems[i].status & IT_TYPE)
					case IT_CVAR:
					{
						consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
						switch(currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_SLIDER:
								M_DrawSlider(x, y, cv);
							case IT_CV_NOPRINT: // color use this
								break;
							case IT_CV_STRING:
								M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
								V_DrawString(x + 8, y + 12, V_ALLOWLOWERCASE, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawCharacter(x + 8 + V_StringWidth(cv->string), y + 12,
										'_' | 0x80, false);
								y += 16;
								break;
							default:
								V_DrawString(BASEVIDWIDTH - x - V_StringWidth(cv->string), y,
									V_YELLOWMAP, cv->string);
								break;
						}
						break;
					}
					y += STRINGHEIGHT;
					break;
			case IT_STRING2:
				V_DrawCenteredString(x, y, 0, currentMenu->menuitems[i].text);
			case IT_DYLITLSPACE:
				y += SMALLLINEHEIGHT;
				break;
			case IT_GRAYPATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
					V_DrawMappedPatch(x, y, 0,
						W_CachePatchName(currentMenu->menuitems[i].patch,PU_CACHE), graymap);
				y += LINEHEIGHT;
				break;
		}
	}

	// DRAW THE SKULL CURSOR
	if (((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_PATCH)
		|| ((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_NOTHING))
	{
		V_DrawScaledPatch(x + SKULLXOFF, cursory - 5, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
	}
	else
	{
		V_DrawScaledPatch(x - V_StringWidth(currentMenu->menuitems[itemOn].text)/2 - 24, cursory, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
		V_DrawCenteredString(x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);
	}
}

//
// M_StringHeight
//
// Find string height from hu_font chars
//
static inline size_t M_StringHeight(const char *string)
{
	size_t h = 8, i;

	for (i = 0; i < strlen(string); i++)
		if (string[i] == '\n')
			h += 8;

	return h;
}

//===========================================================================
//MAIN MENU
//===========================================================================

static void M_QuitSRB2(INT32 choice);
static void M_OptionsMenu(INT32 choice);
static void M_SecretsMenu(INT32 choice);
static void M_CustomSecretsMenu(INT32 choice);
static void M_MapChange(INT32 choice);
static void M_TeamChange(INT32 choice);
static void M_ConfirmSpectate(INT32 choice);
static void M_TeamScramble(INT32 choice);
static void M_ConfirmTeamScramble(INT32 choice);
static void M_HandleAddons(INT32 choice);

typedef enum
{
	scramble = 0,
	spectate,
	switchteam,
	switchmap,
	secrets,
	singleplr,
	options,
	addons,
	quitdoom,
	main_end
} main_e;

static menuitem_t MainMenu[] =
{
	{IT_STRING  | IT_CALL,   NULL, "Scramble Teams...", M_TeamScramble,        48},
	{IT_STRING  | IT_CALL,   NULL, "Spectate..."      , M_ConfirmSpectate,     56},
	{IT_STRING  | IT_CALL,   NULL, "Switch Team..."   , M_TeamChange,          64},
	{IT_STRING  | IT_CALL,   NULL, "Switch Map..."    , M_MapChange,           64},
	{IT_CALL    | IT_STRING, NULL, "secrets"          , M_SecretsMenu,         72},
	{IT_SUBMENU | IT_STRING, NULL, "1 player"         , &SinglePlayerDef,      84},
	{IT_CALL    | IT_STRING, NULL, "options"          , M_OptionsMenu,        100},
	{IT_CALL    |IT_STRING,  NULL, "addons"			  , M_Addons,          	  108},
	{IT_CALL    | IT_STRING, NULL, "quit  game"       , M_QuitSRB2,           116},
};

menu_t MainDef =
{
	NULL,
	NULL,
	main_end,
	NULL,
	MainMenu,
	M_DrawCenteredMenu,
	BASEVIDWIDTH/2, 72,
	0,
	NULL
};

static menuitem_t MISC_AddonsMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandleAddons, 0},     // dummy menuitem for the control func
};

menu_t MISC_AddonsDef =
{
	NULL,
	"Addons",
	sizeof (MISC_AddonsMenu)/sizeof (menuitem_t),
	&MainDef,
	MISC_AddonsMenu,
	M_DrawAddons,
	50, 28,
	0,
	NULL
};

static INT32 highlightflags, recommendedflags, warningflags;

static void M_DrawStats(void);
static void M_DrawStats2(void);
static void M_DrawStats3(void);
static void M_DrawStats4(void);
static void M_DrawStats5(void);
static void M_Stats2(INT32 choice);
static void M_Stats3(INT32 choice);
static void M_Stats4(INT32 choice);

// Empty thingy for stats5 menu
typedef enum
{
	statsempty5,
	stats5_end
} stats5_e;

static menuitem_t Stats5Menu[] =
{
	{IT_SUBMENU | IT_STRING, NULL, "NEXT", &StatsDef, 192},
};

menu_t Stats5Def =
{
	NULL,
	NULL,
	stats5_end,
	&SinglePlayerDef,
	Stats5Menu,
	M_DrawStats5,
	280, 185,
	0,
	NULL
};

// Empty thingy for stats4 menu
typedef enum
{
	statsempty4,
	stats4_end
} stats4_e;

static menuitem_t Stats4Menu[] =
{
	{IT_SUBMENU | IT_STRING, NULL, "NEXT", &Stats5Def, 192},
};

menu_t Stats4Def =
{
	NULL,
	NULL,
	stats4_end,
	&SinglePlayerDef,
	Stats4Menu,
	M_DrawStats4,
	280, 185,
	0,
	NULL
};

// Empty thingy for stats3 menu
typedef enum
{
	statsempty3,
	stats3_end
} stats3_e;

static menuitem_t Stats3Menu[] =
{
	{IT_CALL | IT_STRING, NULL, "NEXT", M_Stats4, 192},
};

menu_t Stats3Def =
{
	NULL,
	NULL,
	stats3_end,
	&SinglePlayerDef,
	Stats3Menu,
	M_DrawStats3,
	280, 185,
	0,
	NULL
};

// Empty thingy for stats2 menu
typedef enum
{
	statsempty2,
	stats2_end
} stats2_e;

static menuitem_t Stats2Menu[] =
{
	{IT_CALL | IT_STRING, NULL, "NEXT", M_Stats3, 192},
};

menu_t Stats2Def =
{
	NULL,
	NULL,
	stats2_end,
	&SinglePlayerDef,
	Stats2Menu,
	M_DrawStats2,
	280, 185,
	0,
	NULL
};

// Empty thingy for stats menu
typedef enum
{
	statsempty1,
	stats_end
} stats_e;

static menuitem_t StatsMenu[] =
{
	{IT_CALL | IT_STRING, NULL, "NEXT", M_Stats2, 192},
};

menu_t StatsDef =
{
	NULL,
	NULL,
	stats_end,
	&SinglePlayerDef,
	StatsMenu,
	M_DrawStats,
	280, 185,
	0,
	NULL
};

//===========================================================================
//SINGLE PLAYER MENU
//===========================================================================
// Menu Revamp! Tails 11-30-2000
static void M_NewGame(void);
static void M_LoadGame(INT32 choice);
static void M_Statistics(INT32 choice);
static void M_TimeAttack(INT32 choice);

typedef enum
{
	newgame = 0,
	timeattack,
	statistics,
	endgame,
	single_end
} single_e;

static menuitem_t SinglePlayerMenu[] =
{
	{IT_CALL | IT_STRING, NULL, "Start Game", M_LoadGame,     92},
	{IT_CALL | IT_STRING, NULL, "Time Attack",M_TimeAttack,  100},
	{IT_CALL | IT_STRING, NULL, "Statistics", M_Statistics,  108},
	{IT_CALL | IT_STRING, NULL, "End Game",   M_EndGame,     116},
};

menu_t SinglePlayerDef =
{
	0,
	"Single Player",
	single_end,
	&MainDef,
	SinglePlayerMenu,
	M_DrawGenericMenu,
	130, 72, // Tails 11-30-2000
	0,
	NULL
};

enum
{
	op_screenshot_folder = 2,
	op_movie_folder = 9,
	op_screenshot_capture = 10,
	op_screenshot_gif_start = 11,
	op_screenshot_gif_end = 12,
	op_screenshot_apng_start = 13,
	op_screenshot_apng_end = 16,
};

void Moviemode_mode_Onchange(void) // i guess this can go here?
{
	INT32 i, cstart, cend;

	switch (cv_moviemode.value)
	{
		case MM_GIF:
			cstart = op_screenshot_gif_start;
			cend = op_screenshot_gif_end;
			break;
		default:
			return;
	}
}

static menuitem_t OP_AddonsOptionsMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Location",                    &cv_addons_option,      10},
	{IT_STRING|IT_CVAR|IT_CV_STRING, NULL, "Custom Folder",               &cv_addons_folder,      20},
	{IT_STRING|IT_CVAR,              NULL, "Identify add-ons via",        &cv_addons_md5,         48},
	{IT_STRING|IT_CVAR,              NULL, "Show unsupported file types", &cv_addons_showall,     58},

	{IT_STRING|IT_CVAR,              NULL, "Matching",                    &cv_addons_search_type, 86},
	{IT_STRING|IT_CVAR,              NULL, "Case-sensitive",              &cv_addons_search_case, 96},
};

menu_t OP_AddonsOptionsDef =
{
	0,
	"Room Info",
	sizeof (OP_AddonsOptionsMenu)/sizeof (menuitem_t),
	&MainDef,
	OP_AddonsOptionsMenu,
	M_DrawGenericMenu,
	30,40,
	0,
	NULL
};

enum
{
	op_addons_folder = 2,
};

void Addons_option_Onchange(void)
{
	OP_AddonsOptionsMenu[op_addons_folder].status =
		(cv_addons_option.value == 3 ? IT_CVAR|IT_STRING|IT_CV_STRING : IT_DISABLED);
}


//===========================================================================
// Start Server Menu
//===========================================================================

#define M(A, B) {A,"MAP" B},
static CV_PossibleValue_t map_cons_t[LEVELARRAYSIZE] = {
	M(1,"01")
	M(2,"02")
	M(3,"03")
	M(4,"04")
	M(5,"05")
	M(6,"06")
	M(7,"07")
	M(8,"08")
	M(9,"09")
	M(10,"10")
	M(11,"11")
	M(12,"12")
	M(13,"13")
	M(14,"14")
	M(15,"15")
	M(16,"16")
	M(17,"17")
	M(18,"18")
	M(19,"19")
	M(20,"20")
	M(21,"21")
	M(22,"22")
	M(23,"23")
	M(24,"24")
	M(25,"25")
	M(26,"26")
	M(27,"27")
	M(28,"28")
	M(29,"29")
	M(30,"30")
	M(31,"31")
	M(32,"32")
	M(33,"33")
	M(34,"34")
	M(35,"35")
	M(36,"36")
	M(37,"37")
	M(38,"38")
	M(39,"39")
	M(40,"40")
	M(41,"41")
	M(42,"42")
	M(43,"43")
	M(44,"44")
	M(45,"45")
	M(46,"46")
	M(47,"47")
	M(48,"48")
	M(49,"49")
	M(50,"50")
	M(51,"51")
	M(52,"52")
	M(53,"53")
	M(54,"54")
	M(55,"55")
	M(56,"56")
	M(57,"57")
	M(58,"58")
	M(59,"59")
	M(60,"60")
	M(61,"61")
	M(62,"62")
	M(63,"63")
	M(64,"64")
	M(65,"65")
	M(66,"66")
	M(67,"67")
	M(68,"68")
	M(69,"69")
	M(70,"70")
	M(71,"71")
	M(72,"72")
	M(73,"73")
	M(74,"74")
	M(75,"75")
	M(76,"76")
	M(77,"77")
	M(78,"78")
	M(79,"79")
	M(80,"80")
	M(81,"81")
	M(82,"82")
	M(83,"83")
	M(84,"84")
	M(85,"85")
	M(86,"86")
	M(87,"87")
	M(88,"88")
	M(89,"89")
	M(90,"90")
	M(91,"91")
	M(92,"92")
	M(93,"93")
	M(94,"94")
	M(95,"95")
	M(96,"96")
	M(97,"97")
	M(98,"98")
	M(99,"99")
	M(100, "100")
	M(101, "101")
	M(102, "102")
	M(103, "103")
	M(104, "104")
	M(105, "105")
	M(106, "106")
	M(107, "107")
	M(108, "108")
	M(109, "109")
	M(110, "110")
	M(111, "111")
	M(112, "112")
	M(113, "113")
	M(114, "114")
	M(115, "115")
	M(116, "116")
	M(117, "117")
	M(118, "118")
	M(119, "119")
	M(120, "120")
	M(121, "121")
	M(122, "122")
	M(123, "123")
	M(124, "124")
	M(125, "125")
	M(126, "126")
	M(127, "127")
	M(128, "128")
	M(129, "129")
	M(130, "130")
	M(131, "131")
	M(132, "132")
	M(133, "133")
	M(134, "134")
	M(135, "135")
	M(136, "136")
	M(137, "137")
	M(138, "138")
	M(139, "139")
	M(140, "140")
	M(141, "141")
	M(142, "142")
	M(143, "143")
	M(144, "144")
	M(145, "145")
	M(146, "146")
	M(147, "147")
	M(148, "148")
	M(149, "149")
	M(150, "150")
	M(151, "151")
	M(152, "152")
	M(153, "153")
	M(154, "154")
	M(155, "155")
	M(156, "156")
	M(157, "157")
	M(158, "158")
	M(159, "159")
	M(160, "160")
	M(161, "161")
	M(162, "162")
	M(163, "163")
	M(164, "164")
	M(165, "165")
	M(166, "166")
	M(167, "167")
	M(168, "168")
	M(169, "169")
	M(170, "170")
	M(171, "171")
	M(172, "172")
	M(173, "173")
	M(174, "174")
	M(175, "175")
	M(176, "176")
	M(177, "177")
	M(178, "178")
	M(179, "179")
	M(180, "180")
	M(181, "181")
	M(182, "182")
	M(183, "183")
	M(184, "184")
	M(185, "185")
	M(186, "186")
	M(187, "187")
	M(188, "188")
	M(189, "189")
	M(190, "190")
	M(191, "191")
	M(192, "192")
	M(193, "193")
	M(194, "194")
	M(195, "195")
	M(196, "196")
	M(197, "197")
	M(198, "198")
	M(199, "199")
	M(200, "200")
	M(201, "201")
	M(202, "202")
	M(203, "203")
	M(204, "204")
	M(205, "205")
	M(206, "206")
	M(207, "207")
	M(208, "208")
	M(209, "209")
	M(210, "210")
	M(211, "211")
	M(212, "212")
	M(213, "213")
	M(214, "214")
	M(215, "215")
	M(216, "216")
	M(217, "217")
	M(218, "218")
	M(219, "219")
	M(220, "220")
	M(221, "221")
	M(222, "222")
	M(223, "223")
	M(224, "224")
	M(225, "225")
	M(226, "226")
	M(227, "227")
	M(228, "228")
	M(229, "229")
	M(230, "230")
	M(231, "231")
	M(232, "232")
	M(233, "233")
	M(234, "234")
	M(235, "235")
	M(236, "236")
	M(237, "237")
	M(238, "238")
	M(239, "239")
	M(240, "240")
	M(241, "241")
	M(242, "242")
	M(243, "243")
	M(244, "244")
	M(245, "245")
	M(246, "246")
	M(247, "247")
	M(248, "248")
	M(249, "249")
	M(250, "250")
	M(251, "251")
	M(252, "252")
	M(253, "253")
	M(254, "254")
	M(255, "255")
	M(256, "256")
	M(257, "257")
	M(258, "258")
	M(259, "259")
	M(260, "260")
	M(261, "261")
	M(262, "262")
	M(263, "263")
	M(264, "264")
	M(265, "265")
	M(266, "266")
	M(267, "267")
	M(268, "268")
	M(269, "269")
	M(270, "270")
	M(271, "271")
	M(272, "272")
	M(273, "273")
	M(274, "274")
	M(275, "275")
	M(276, "276")
	M(277, "277")
	M(278, "278")
	M(279, "279")
	M(280, "280")
	M(281, "281")
	M(282, "282")
	M(283, "283")
	M(284, "284")
	M(285, "285")
	M(286, "286")
	M(287, "287")
	M(288, "288")
	M(289, "289")
	M(290, "290")
	M(291, "291")
	M(292, "292")
	M(293, "293")
	M(294, "294")
	M(295, "295")
	M(296, "296")
	M(297, "297")
	M(298, "298")
	M(299, "299")
	M(300, "300")
	M(301, "301")
	M(302, "302")
	M(303, "303")
	M(304, "304")
	M(305, "305")
	M(306, "306")
	M(307, "307")
	M(308, "308")
	M(309, "309")
	M(310, "310")
	M(311, "311")
	M(312, "312")
	M(313, "313")
	M(314, "314")
	M(315, "315")
	M(316, "316")
	M(317, "317")
	M(318, "318")
	M(319, "319")
	M(320, "320")
	M(321, "321")
	M(322, "322")
	M(323, "323")
	M(324, "324")
	M(325, "325")
	M(326, "326")
	M(327, "327")
	M(328, "328")
	M(329, "329")
	M(330, "330")
	M(331, "331")
	M(332, "332")
	M(333, "333")
	M(334, "334")
	M(335, "335")
	M(336, "336")
	M(337, "337")
	M(338, "338")
	M(339, "339")
	M(340, "340")
	M(341, "341")
	M(342, "342")
	M(343, "343")
	M(344, "344")
	M(345, "345")
	M(346, "346")
	M(347, "347")
	M(348, "348")
	M(349, "349")
	M(350, "350")
	M(351, "351")
	M(352, "352")
	M(353, "353")
	M(354, "354")
	M(355, "355")
	M(356, "356")
	M(357, "357")
	M(358, "358")
	M(359, "359")
	M(360, "360")
	M(361, "361")
	M(362, "362")
	M(363, "363")
	M(364, "364")
	M(365, "365")
	M(366, "366")
	M(367, "367")
	M(368, "368")
	M(369, "369")
	M(370, "370")
	M(371, "371")
	M(372, "372")
	M(373, "373")
	M(374, "374")
	M(375, "375")
	M(376, "376")
	M(377, "377")
	M(378, "378")
	M(379, "379")
	M(380, "380")
	M(381, "381")
	M(382, "382")
	M(383, "383")
	M(384, "384")
	M(385, "385")
	M(386, "386")
	M(387, "387")
	M(388, "388")
	M(389, "389")
	M(390, "390")
	M(391, "391")
	M(392, "392")
	M(393, "393")
	M(394, "394")
	M(395, "395")
	M(396, "396")
	M(397, "397")
	M(398, "398")
	M(399, "399")
	M(400, "400")
	M(401, "401")
	M(402, "402")
	M(403, "403")
	M(404, "404")
	M(405, "405")
	M(406, "406")
	M(407, "407")
	M(408, "408")
	M(409, "409")
	M(410, "410")
	M(411, "411")
	M(412, "412")
	M(413, "413")
	M(414, "414")
	M(415, "415")
	M(416, "416")
	M(417, "417")
	M(418, "418")
	M(419, "419")
	M(420, "420")
	M(421, "421")
	M(422, "422")
	M(423, "423")
	M(424, "424")
	M(425, "425")
	M(426, "426")
	M(427, "427")
	M(428, "428")
	M(429, "429")
	M(430, "430")
	M(431, "431")
	M(432, "432")
	M(433, "433")
	M(434, "434")
	M(435, "435")
	M(436, "436")
	M(437, "437")
	M(438, "438")
	M(439, "439")
	M(440, "440")
	M(441, "441")
	M(442, "442")
	M(443, "443")
	M(444, "444")
	M(445, "445")
	M(446, "446")
	M(447, "447")
	M(448, "448")
	M(449, "449")
	M(450, "450")
	M(451, "451")
	M(452, "452")
	M(453, "453")
	M(454, "454")
	M(455, "455")
	M(456, "456")
	M(457, "457")
	M(458, "458")
	M(459, "459")
	M(460, "460")
	M(461, "461")
	M(462, "462")
	M(463, "463")
	M(464, "464")
	M(465, "465")
	M(466, "466")
	M(467, "467")
	M(468, "468")
	M(469, "469")
	M(470, "470")
	M(471, "471")
	M(472, "472")
	M(473, "473")
	M(474, "474")
	M(475, "475")
	M(476, "476")
	M(477, "477")
	M(478, "478")
	M(479, "479")
	M(480, "480")
	M(481, "481")
	M(482, "482")
	M(483, "483")
	M(484, "484")
	M(485, "485")
	M(486, "486")
	M(487, "487")
	M(488, "488")
	M(489, "489")
	M(490, "490")
	M(491, "491")
	M(492, "492")
	M(493, "493")
	M(494, "494")
	M(495, "495")
	M(496, "496")
	M(497, "497")
	M(498, "498")
	M(499, "499")
	M(500, "500")
	M(501, "501")
	M(502, "502")
	M(503, "503")
	M(504, "504")
	M(505, "505")
	M(506, "506")
	M(507, "507")
	M(508, "508")
	M(509, "509")
	M(510, "510")
	M(511, "511")
	M(512, "512")
	M(513, "513")
	M(514, "514")
	M(515, "515")
	M(516, "516")
	M(517, "517")
	M(518, "518")
	M(519, "519")
	M(520, "520")
	M(521, "521")
	M(522, "522")
	M(523, "523")
	M(524, "524")
	M(525, "525")
	M(526, "526")
	M(527, "527")
	M(528, "528")
	M(529, "529")
	M(530, "530")
	M(531, "531")
	M(532, "532")
	M(533, "533")
	M(534, "534")
	M(535, "535")
	M(536, "536")
	M(537, "537")
	M(538, "538")
	M(539, "539")
	M(540, "540")
	M(541, "541")
	M(542, "542")
	M(543, "543")
	M(544, "544")
	M(545, "545")
	M(546, "546")
	M(547, "547")
	M(548, "548")
	M(549, "549")
	M(550, "550")
	M(551, "551")
	M(552, "552")
	M(553, "553")
	M(554, "554")
	M(555, "555")
	M(556, "556")
	M(557, "557")
	M(558, "558")
	M(559, "559")
	M(560, "560")
	M(561, "561")
	M(562, "562")
	M(563, "563")
	M(564, "564")
	M(565, "565")
	M(566, "566")
	M(567, "567")
	M(568, "568")
	M(569, "569")
	M(570, "570")
	M(571, "571")
	M(572, "572")
	M(573, "573")
	M(574, "574")
	M(575, "575")
	M(576, "576")
	M(577, "577")
	M(578, "578")
	M(579, "579")
	M(580, "580")
	M(581, "581")
	M(582, "582")
	M(583, "583")
	M(584, "584")
	M(585, "585")
	M(586, "586")
	M(587, "587")
	M(588, "588")
	M(589, "589")
	M(590, "590")
	M(591, "591")
	M(592, "592")
	M(593, "593")
	M(594, "594")
	M(595, "595")
	M(596, "596")
	M(597, "597")
	M(598, "598")
	M(599, "599")
	M(600, "600")
	M(601, "601")
	M(602, "602")
	M(603, "603")
	M(604, "604")
	M(605, "605")
	M(606, "606")
	M(607, "607")
	M(608, "608")
	M(609, "609")
	M(610, "610")
	M(611, "611")
	M(612, "612")
	M(613, "613")
	M(614, "614")
	M(615, "615")
	M(616, "616")
	M(617, "617")
	M(618, "618")
	M(619, "619")
	M(620, "620")
	M(621, "621")
	M(622, "622")
	M(623, "623")
	M(624, "624")
	M(625, "625")
	M(626, "626")
	M(627, "627")
	M(628, "628")
	M(629, "629")
	M(630, "630")
	M(631, "631")
	M(632, "632")
	M(633, "633")
	M(634, "634")
	M(635, "635")
	M(636, "636")
	M(637, "637")
	M(638, "638")
	M(639, "639")
	M(640, "640")
	M(641, "641")
	M(642, "642")
	M(643, "643")
	M(644, "644")
	M(645, "645")
	M(646, "646")
	M(647, "647")
	M(648, "648")
	M(649, "649")
	M(650, "650")
	M(651, "651")
	M(652, "652")
	M(653, "653")
	M(654, "654")
	M(655, "655")
	M(656, "656")
	M(657, "657")
	M(658, "658")
	M(659, "659")
	M(660, "660")
	M(661, "661")
	M(662, "662")
	M(663, "663")
	M(664, "664")
	M(665, "665")
	M(666, "666")
	M(667, "667")
	M(668, "668")
	M(669, "669")
	M(670, "670")
	M(671, "671")
	M(672, "672")
	M(673, "673")
	M(674, "674")
	M(675, "675")
	M(676, "676")
	M(677, "677")
	M(678, "678")
	M(679, "679")
	M(680, "680")
	M(681, "681")
	M(682, "682")
	M(683, "683")
	M(684, "684")
	M(685, "685")
	M(686, "686")
	M(687, "687")
	M(688, "688")
	M(689, "689")
	M(690, "690")
	M(691, "691")
	M(692, "692")
	M(693, "693")
	M(694, "694")
	M(695, "695")
	M(696, "696")
	M(697, "697")
	M(698, "698")
	M(699, "699")
	M(700, "700")
	M(701, "701")
	M(702, "702")
	M(703, "703")
	M(704, "704")
	M(705, "705")
	M(706, "706")
	M(707, "707")
	M(708, "708")
	M(709, "709")
	M(710, "710")
	M(711, "711")
	M(712, "712")
	M(713, "713")
	M(714, "714")
	M(715, "715")
	M(716, "716")
	M(717, "717")
	M(718, "718")
	M(719, "719")
	M(720, "720")
	M(721, "721")
	M(722, "722")
	M(723, "723")
	M(724, "724")
	M(725, "725")
	M(726, "726")
	M(727, "727")
	M(728, "728")
	M(729, "729")
	M(730, "730")
	M(731, "731")
	M(732, "732")
	M(733, "733")
	M(734, "734")
	M(735, "735")
	M(736, "736")
	M(737, "737")
	M(738, "738")
	M(739, "739")
	M(740, "740")
	M(741, "741")
	M(742, "742")
	M(743, "743")
	M(744, "744")
	M(745, "745")
	M(746, "746")
	M(747, "747")
	M(748, "748")
	M(749, "749")
	M(750, "750")
	M(751, "751")
	M(752, "752")
	M(753, "753")
	M(754, "754")
	M(755, "755")
	M(756, "756")
	M(757, "757")
	M(758, "758")
	M(759, "759")
	M(760, "760")
	M(761, "761")
	M(762, "762")
	M(763, "763")
	M(764, "764")
	M(765, "765")
	M(766, "766")
	M(767, "767")
	M(768, "768")
	M(769, "769")
	M(770, "770")
	M(771, "771")
	M(772, "772")
	M(773, "773")
	M(774, "774")
	M(775, "775")
	M(776, "776")
	M(777, "777")
	M(778, "778")
	M(779, "779")
	M(780, "780")
	M(781, "781")
	M(782, "782")
	M(783, "783")
	M(784, "784")
	M(785, "785")
	M(786, "786")
	M(787, "787")
	M(788, "788")
	M(789, "789")
	M(790, "790")
	M(791, "791")
	M(792, "792")
	M(793, "793")
	M(794, "794")
	M(795, "795")
	M(796, "796")
	M(797, "797")
	M(798, "798")
	M(799, "799")
	M(800, "800")
	M(801, "801")
	M(802, "802")
	M(803, "803")
	M(804, "804")
	M(805, "805")
	M(806, "806")
	M(807, "807")
	M(808, "808")
	M(809, "809")
	M(810, "810")
	M(811, "811")
	M(812, "812")
	M(813, "813")
	M(814, "814")
	M(815, "815")
	M(816, "816")
	M(817, "817")
	M(818, "818")
	M(819, "819")
	M(820, "820")
	M(821, "821")
	M(822, "822")
	M(823, "823")
	M(824, "824")
	M(825, "825")
	M(826, "826")
	M(827, "827")
	M(828, "828")
	M(829, "829")
	M(830, "830")
	M(831, "831")
	M(832, "832")
	M(833, "833")
	M(834, "834")
	M(835, "835")
	M(836, "836")
	M(837, "837")
	M(838, "838")
	M(839, "839")
	M(840, "840")
	M(841, "841")
	M(842, "842")
	M(843, "843")
	M(844, "844")
	M(845, "845")
	M(846, "846")
	M(847, "847")
	M(848, "848")
	M(849, "849")
	M(850, "850")
	M(851, "851")
	M(852, "852")
	M(853, "853")
	M(854, "854")
	M(855, "855")
	M(856, "856")
	M(857, "857")
	M(858, "858")
	M(859, "859")
	M(860, "860")
	M(861, "861")
	M(862, "862")
	M(863, "863")
	M(864, "864")
	M(865, "865")
	M(866, "866")
	M(867, "867")
	M(868, "868")
	M(869, "869")
	M(870, "870")
	M(871, "871")
	M(872, "872")
	M(873, "873")
	M(874, "874")
	M(875, "875")
	M(876, "876")
	M(877, "877")
	M(878, "878")
	M(879, "879")
	M(880, "880")
	M(881, "881")
	M(882, "882")
	M(883, "883")
	M(884, "884")
	M(885, "885")
	M(886, "886")
	M(887, "887")
	M(888, "888")
	M(889, "889")
	M(890, "890")
	M(891, "891")
	M(892, "892")
	M(893, "893")
	M(894, "894")
	M(895, "895")
	M(896, "896")
	M(897, "897")
	M(898, "898")
	M(899, "899")
	M(900, "900")
	M(901, "901")
	M(902, "902")
	M(903, "903")
	M(904, "904")
	M(905, "905")
	M(906, "906")
	M(907, "907")
	M(908, "908")
	M(909, "909")
	M(910, "910")
	M(911, "911")
	M(912, "912")
	M(913, "913")
	M(914, "914")
	M(915, "915")
	M(916, "916")
	M(917, "917")
	M(918, "918")
	M(919, "919")
	M(920, "920")
	M(921, "921")
	M(922, "922")
	M(923, "923")
	M(924, "924")
	M(925, "925")
	M(926, "926")
	M(927, "927")
	M(928, "928")
	M(929, "929")
	M(930, "930")
	M(931, "931")
	M(932, "932")
	M(933, "933")
	M(934, "934")
	M(935, "935")
	M(936, "936")
	M(937, "937")
	M(938, "938")
	M(939, "939")
	M(940, "940")
	M(941, "941")
	M(942, "942")
	M(943, "943")
	M(944, "944")
	M(945, "945")
	M(946, "946")
	M(947, "947")
	M(948, "948")
	M(949, "949")
	M(950, "950")
	M(951, "951")
	M(952, "952")
	M(953, "953")
	M(954, "954")
	M(955, "955")
	M(956, "956")
	M(957, "957")
	M(958, "958")
	M(959, "959")
	M(960, "960")
	M(961, "961")
	M(962, "962")
	M(963, "963")
	M(964, "964")
	M(965, "965")
	M(966, "966")
	M(967, "967")
	M(968, "968")
	M(969, "969")
	M(970, "970")
	M(971, "971")
	M(972, "972")
	M(973, "973")
	M(974, "974")
	M(975, "975")
	M(976, "976")
	M(977, "977")
	M(978, "978")
	M(979, "979")
	M(980, "980")
	M(981, "981")
	M(982, "982")
	M(983, "983")
	M(984, "984")
	M(985, "985")
	M(986, "986")
	M(987, "987")
	M(988, "988")
	M(989, "989")
	M(990, "990")
	M(991, "991")
	M(992, "992")
	M(993, "993")
	M(994, "994")
	M(995, "995")
	M(996, "996")
	M(997, "997")
	M(998, "998")
	M(999, "999")
	M(1000, "1000")
	M(1001, "1001")
	M(1002, "1002")
	M(1003, "1003")
	M(1004, "1004")
	M(1005, "1005")
	M(1006, "1006")
	M(1007, "1007")
	M(1008, "1008")
	M(1009, "1009")
	M(1010, "1010")
	M(1011, "1011")
	M(1012, "1012")
	M(1013, "1013")
	M(1014, "1014")
	M(1015, "1015")
	M(1016, "1016")
	M(1017, "1017")
	M(1018, "1018")
	M(1019, "1019")
	M(1020, "1020")
	M(1021, "1021")
	M(1022, "1022")
	M(1023, "1023")
	M(1024, "1024")
	M(1025, "1025")
	M(1026, "1026")
	M(1027, "1027")
	M(1028, "1028")
	M(1029, "1029")
	M(1030, "1030")
	M(1031, "1031")
	M(1032, "1032")
	M(1033, "1033")
	M(1034, "1034")
	M(1035, "1035")
	{INT16_MAX, NULL}
};

static void Newgametype_OnChange(void);
static void Nextmap_OnChange(void);

// GT_* defined in doomstat.h
consvar_t cv_nextmap = {"nextmap", "MAP01", CV_HIDEN|CV_CALL, map_cons_t, Nextmap_OnChange, 0, NULL, NULL, 0, 0, NULL};

static CV_PossibleValue_t skins_cons_t[MAXSKINS+1] = {{1, DEFAULTSKIN}};
consvar_t cv_chooseskin = {"chooseskin", DEFAULTSKIN, CV_HIDEN|CV_CALL, skins_cons_t, Nextmap_OnChange, 0, NULL, NULL, 0, 0, NULL};

// When you add gametypes here, don't forget
// to update them in CV_AddValue!

CV_PossibleValue_t gametype_cons_t[] =
{
	{GT_COOP, "Coop"}, {GT_MATCH, "Match"},
	{GTF_TEAMMATCH, "Team Match"}, {GT_RACE, "Race"},
	{GTF_CLASSICRACE, "Classic Race"}, {GT_TAG, "Tag"},
	{GTF_HIDEANDSEEK, "Hide and Seek"},
	{GT_CTF, "CTF"},
#ifdef CHAOSISNOTDEADYET
	{GT_CHAOS, "Chaos"},
#endif
	{0, NULL}
};

//
// FindFirstMap
//
// Finds the first map of a particular gametype
// Defaults to 1 if nothing found.
//
static INT32 FindFirstMap(INT32 gtype)
{
	INT32 i;

	for (i = 0; i < NUMMAPS; i++)
	{
		if (mapheaderinfo[i].typeoflevel & gtype)
			return i + 1;
	}

	return 1;
}

consvar_t cv_newgametype = {"newgametype", "Coop", CV_HIDEN|CV_CALL, gametype_cons_t, Newgametype_OnChange, 0, NULL, NULL, 0, 0, NULL};

static void Newgametype_OnChange(void)
{
	if (menuactive)
	{
		if ((cv_newgametype.value == GT_COOP && !(mapheaderinfo[cv_nextmap.value-1].typeoflevel & TOL_COOP)) ||
			((cv_newgametype.value == GT_RACE || cv_newgametype.value == GTF_CLASSICRACE) && !(mapheaderinfo[cv_nextmap.value-1].typeoflevel & TOL_RACE)) ||
			((cv_newgametype.value == GT_MATCH || cv_newgametype.value == GTF_TEAMMATCH) && !(mapheaderinfo[cv_nextmap.value-1].typeoflevel & TOL_MATCH)) ||
#ifdef CHAOSISNOTDEADYET
			(cv_newgametype.value == GT_CHAOS && !(mapheaderinfo[cv_nextmap.value-1].typeoflevel & TOL_CHAOS)) ||
#endif
			((cv_newgametype.value == GT_TAG || cv_newgametype.value == GTF_HIDEANDSEEK) && !(mapheaderinfo[cv_nextmap.value-1].typeoflevel & TOL_TAG)) ||
			(cv_newgametype.value == GT_CTF && !(mapheaderinfo[cv_nextmap.value-1].typeoflevel & TOL_CTF)))
		{
			INT32 value = 0;

			switch (cv_newgametype.value)
			{
				case GT_COOP:
					value = TOL_COOP;
					break;
				case GT_RACE:
					value = TOL_RACE;
					break;
				case GT_MATCH:
					value = TOL_MATCH;
					break;
				case GT_TAG:
					value = TOL_TAG;
					break;
				case GT_CTF:
					value = TOL_CTF;
					break;
			}

			CV_SetValue(&cv_nextmap, FindFirstMap(value));
			CV_AddValue(&cv_nextmap, -1);
			CV_AddValue(&cv_nextmap, 1);
		}
	}
}

static void M_AddonsOptions(INT32 choice)
{
	(void)choice;
	Addons_option_Onchange();

	M_SetupNextMenu(&OP_AddonsOptionsDef);
}

#define LOCATIONSTRING1 "Visit \x83SRB2.ORG/MODS\x80 to get & make add-ons!"
//#define LOCATIONSTRING2 "Visit \x88SRB2.ORG/MODS\x80 to get & make add-ons!"

static void M_Addons(INT32 choice)
{
	const char *pathname = ".";

	(void)choice;

	// If M_GetGameypeColor() is ever ported from Kart, then remove this.
	highlightflags = V_YELLOWMAP;
	recommendedflags = V_GREENMAP;
	warningflags = V_REDMAP;

#if 1
	if (cv_addons_option.value == 0)
		pathname = usehome ? srb2home : srb2path;
	else if (cv_addons_option.value == 1)
		pathname = srb2home;
	else if (cv_addons_option.value == 2)
		pathname = srb2path;
	else
#endif
	if (cv_addons_option.value == 3 && *cv_addons_folder.string != '\0')
		pathname = cv_addons_folder.string;

	strlcpy(menupath, pathname, 1024);
	menupathindex[(menudepthleft = menudepth-1)] = strlen(menupath) + 1;

	if (menupath[menupathindex[menudepthleft]-2] != PATHSEP[0])
	{
		menupath[menupathindex[menudepthleft]-1] = PATHSEP[0];
		menupath[menupathindex[menudepthleft]] = 0;
	}
	else
		--menupathindex[menudepthleft];

	if (!preparefilemenu(false))
	{
		M_StartMessage(va("No files/folders found.\n\n%s\n\n(Press a key)\n",LOCATIONSTRING1),NULL,MM_NOTHING);
			// (recommendedflags == V_SKYMAP ? LOCATIONSTRING2 : LOCATIONSTRING1))
		return;
	}
	else
		dir_on[menudepthleft] = 0;

	addonsp[EXT_FOLDER] = W_CachePatchName("M_FFLDR", PU_STATIC);
	addonsp[EXT_UP] = W_CachePatchName("M_FBACK", PU_STATIC);
	addonsp[EXT_NORESULTS] = W_CachePatchName("M_FNOPE", PU_STATIC);
	addonsp[EXT_TXT] = W_CachePatchName("M_FTXT", PU_STATIC);
	addonsp[EXT_CFG] = W_CachePatchName("M_FCFG", PU_STATIC);
	addonsp[EXT_WAD] = W_CachePatchName("M_FWAD", PU_STATIC);
	addonsp[EXT_SOC] = W_CachePatchName("M_FSOC", PU_STATIC);
	addonsp[NUM_EXT] = W_CachePatchName("M_FUNKN", PU_STATIC);
	addonsp[NUM_EXT+1] = W_CachePatchName("M_FSEL", PU_STATIC);
	addonsp[NUM_EXT+2] = W_CachePatchName("M_FLOAD", PU_STATIC);
	addonsp[NUM_EXT+3] = W_CachePatchName("M_FSRCH", PU_STATIC);
	addonsp[NUM_EXT+4] = W_CachePatchName("M_FSAVE", PU_STATIC);

	MISC_AddonsDef.prevMenu = currentMenu;
	M_SetupNextMenu(&MISC_AddonsDef);
}

#define width 4
#define vpadding 27
#define h (BASEVIDHEIGHT-(2*vpadding))
#define NUMCOLOURS 8 // when toast's coding it's british english hacker fucker
static void M_DrawTemperature(INT32 x, fixed_t t)
{
	INT32 y;

	// bounds check
	if (t > FRACUNIT)
		t = FRACUNIT;
	/*else if (t < 0) -- not needed
		t = 0;*/

	// scale
	if (t > 1)
		t = (FixedMul(h<<FRACBITS, t)>>FRACBITS);

	// border
	V_DrawFill(x - 1, vpadding, 1, h, 120);
	V_DrawFill(x + width, vpadding, 1, h, 120);
	V_DrawFill(x - 1, vpadding-1, width+2, 1, 120);
	V_DrawFill(x - 1, vpadding+h, width+2, 1, 120);

	// bar itself
	y = h;
	if (t)
		for (t = h - t; y > 0; y--)
		{
			UINT8 colours[NUMCOLOURS] = {135, 133, 92, 77, 114, 178, 161, 162};
			UINT8 c;
			if (y <= t) break;
			if (y+vpadding >= BASEVIDHEIGHT/2)
				c = 185;
			else
				c = colours[(NUMCOLOURS*(y-1))/(h/2)];
			V_DrawFill(x, y-1 + vpadding, width, 1, c);
		}

	// fill the rest of the backing
	if (y)
		V_DrawFill(x, vpadding, width, y, 30);
}
#undef width
#undef vpadding
#undef h
#undef NUMCOLOURS

static char *M_AddonsHeaderPath(void)
{
	UINT32 len;
	static char header[1024];

	strlcpy(header, va("%s folder%s", cv_addons_option.string, menupath+menupathindex[menudepth-1]-1), 1024);
	len = strlen(header);
	if (len > 34)
	{
		len = len-34;
		header[len] = header[len+1] = header[len+2] = '.';
	}
	else
		len = 0;

	return header+len;
}

#define UNEXIST S_StartSound(NULL, sfx_lose);\
		M_SetupNextMenu(MISC_AddonsDef.prevMenu);\
		M_StartMessage(va("\x82%s\x80\nThis folder no longer exists!\nAborting to main menu.\n\n(Press a key)\n", M_AddonsHeaderPath()),NULL,MM_NOTHING)

#define CLEARNAME Z_Free(refreshdirname);\
					refreshdirname = NULL

static void M_AddonsClearName(INT32 choice)
{
	CLEARNAME;
	M_StopMessage(choice);
}

// returns whether to do message draw
static boolean M_AddonsRefresh(void)
{
	if ((refreshdirmenu & REFRESHDIR_NORMAL) && !preparefilemenu(true))
	{
		UNEXIST;
		return true;
	}

	if (refreshdirmenu & REFRESHDIR_ADDFILE)
	{
		char *message = NULL;

		if (refreshdirmenu & REFRESHDIR_NOTLOADED)
		{
			S_StartSound(NULL, sfx_lose);
			if (refreshdirmenu & REFRESHDIR_MAX)
				message = va("%c%s\x80\nMaximum number of add-ons reached.\nA file could not be loaded.\nIf you want to play with this add-on, restart the game to clear existing ones.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname);
			else
				message = va("%c%s\x80\nA file was not loaded.\nCheck the console log for more information.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname);
		}
		else if (refreshdirmenu & (REFRESHDIR_WARNING|REFRESHDIR_ERROR))
		{
			S_StartSound(NULL, sfx_spin);
			message = va("%c%s\x80\nA file was loaded with %s.\nCheck the console log for more information.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), refreshdirname, ((refreshdirmenu & REFRESHDIR_ERROR) ? "errors" : "warnings"));
		}

		if (message)
		{
			M_StartMessage(message,M_AddonsClearName,MM_EVENTHANDLER);
			return true;
		}

		S_StartSound(NULL, sfx_strpst);
		CLEARNAME;
	}

	return false;
}

static void M_DrawAddons(void)
{
	INT32 x, y;
	ssize_t i, m;
	const UINT8 *flashcol = NULL;
	UINT8 hilicol;

	// hack - need to refresh at end of frame to handle addfile...
	if (refreshdirmenu & M_AddonsRefresh())
	{
		M_DrawMessageMenu();
		return;
	}

	if (Playing())
		V_DrawCenteredString(BASEVIDWIDTH/2, 5, warningflags, "Adding files mid-game may cause problems.");
	else
		V_DrawCenteredString(BASEVIDWIDTH/2, 5, 0, LOCATIONSTRING1);
			// (recommendedflags == V_SKYMAP ? LOCATIONSTRING2 : LOCATIONSTRING1)

	if (numwadfiles <= mainwads+1)
		y = 0;
	else if (numwadfiles >= MAX_WADFILES)
		y = FRACUNIT;
	else
	{
		x = FixedDiv(((ssize_t)(numwadfiles) - (ssize_t)(mainwads+1))<<FRACBITS, ((ssize_t)MAX_WADFILES - (ssize_t)(mainwads+1))<<FRACBITS);
		y = FixedDiv((((ssize_t)packetsizetally-(ssize_t)mainwadstally)<<FRACBITS), ((((ssize_t)MAXFILENEEDED*sizeof(UINT8)-(ssize_t)mainwadstally)-(5+22))<<FRACBITS)); // 5+22 = (a.ext + checksum length) is minimum addition to packet size tally
		if (x > y)
			y = x;
		if (y > FRACUNIT) // happens because of how we're shrinkin' it a little
			y = FRACUNIT;
	}

	M_DrawTemperature(BASEVIDWIDTH - 19 - 5, y);

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y + 1;

	hilicol = yellowmap[120];

	V_DrawString(x-21, (y - 16) + (16 - 12), highlightflags|V_ALLOWLOWERCASE, M_AddonsHeaderPath());
	V_DrawFill(x-21, (y - 16) + (16 - 3), MAXSTRINGLENGTH*8+6, 1, hilicol);
	V_DrawFill(x-21, (y - 16) + (16 - 2), MAXSTRINGLENGTH*8+6, 1, 30);

	m = (BASEVIDHEIGHT - currentMenu->y + 2) - (y - 1);
	M_DrawTextBox(x - (21 + 5), y - 7, MAXSTRINGLENGTH, m / 8);

	// scrollbar!
	if (sizedirmenu <= (2*numaddonsshown + 1))
		i = 0;
	else
	{
		ssize_t q = m;
		m = ((2*numaddonsshown + 1) * m)/sizedirmenu;
		if (dir_on[menudepthleft] <= numaddonsshown) // all the way up
			i = 0;
		else if (sizedirmenu <= (dir_on[menudepthleft] + numaddonsshown + 1)) // all the way down
			i = q-m;
		else
			i = ((dir_on[menudepthleft] - numaddonsshown) * (q-m))/(sizedirmenu - (2*numaddonsshown + 1));
	}

	V_DrawFill(x + MAXSTRINGLENGTH*8+5 - 21, (y - 1) + i, 1, m, hilicol);

	// get bottom...
	m = dir_on[menudepthleft] + numaddonsshown + 1;
	if (m > (ssize_t)sizedirmenu)
		m = sizedirmenu;

	// then compute top and adjust bottom if needed!
	if (m < (2*numaddonsshown + 1))
	{
		m = min(sizedirmenu, 2*numaddonsshown + 1);
		i = 0;
	}
	else
		i = m - (2*numaddonsshown + 1);

	if (i != 0)
		V_DrawString(19, y+4 - (skullAnimCounter/5), highlightflags, "\x1A");

	if (skullAnimCounter < 4)
		flashcol = yellowmap;

	for (; i < m; i++)
	{
		UINT32 flags = V_ALLOWLOWERCASE;
		if (y > BASEVIDHEIGHT) break;
		if (dirmenu[i])
#define type (UINT8)(dirmenu[i][DIR_TYPE])
		{
			if (type & EXT_LOADED)
			{
				flags |= V_TRANSLUCENT;
				V_DrawSmallScaledPatch(x-(16+4), y, V_TRANSLUCENT, addonsp[(type & ~EXT_LOADED)]);
				V_DrawSmallScaledPatch(x-(16+4), y, 0, addonsp[NUM_EXT+2]);
			}
			else
				V_DrawSmallScaledPatch(x-(16+4), y, 0, addonsp[(type & ~EXT_LOADED)]);

			if ((size_t)i == dir_on[menudepthleft])
			{
				V_DrawSmallScaledPatch((x-(16+4)), (y), 0, addonsp[NUM_EXT+1]);
				flags = V_ALLOWLOWERCASE|highlightflags;
			}

#define charsonside 14
			if (dirmenu[i][DIR_LEN] > (charsonside*2 + 3))
				V_DrawString(x, y+4, flags, va("%.*s...%s", charsonside, dirmenu[i]+DIR_STRING, dirmenu[i]+DIR_STRING+dirmenu[i][DIR_LEN]-(charsonside+1)));
#undef charsonside
			else
				V_DrawString(x, y+4, flags, dirmenu[i]+DIR_STRING);
		}
#undef type
		y += 16;
	}

	if (m != (ssize_t)sizedirmenu)
		V_DrawString(19, y-12 + (skullAnimCounter/5), highlightflags, "\x1B");

	y = BASEVIDHEIGHT - currentMenu->y + 1;

	M_DrawTextBox(x - (21 + 5), y + 2, MAXSTRINGLENGTH, 1);
	if (menusearch[0])
		V_DrawString(x - 18, y + 10, V_ALLOWLOWERCASE, menusearch+1);
	else
		V_DrawString(x - 18, y + 10, V_ALLOWLOWERCASE|V_TRANSLUCENT, "Type to search...");
	if (skullAnimCounter < 4)
		V_DrawCharacter(x - 18 + V_StringWidth(menusearch+1), y + 10,
			'_' | 0x80, false);

	x -= (21 + 5 + 16);
	V_DrawSmallScaledPatch(x, y + 6, (menusearch[0] ? 0 : V_TRANSLUCENT), addonsp[NUM_EXT+3]);

	x = BASEVIDWIDTH - x - 16;
	V_DrawSmallScaledPatch(x, y + 6, ((!modifiedgame || savemoddata) ? 0 : V_TRANSLUCENT), addonsp[NUM_EXT+4]);

	if (modifiedgame)
		V_DrawSmallScaledPatch(x, y + 6, 0, addonsp[NUM_EXT+2]);
}

static void M_AddonExec(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	S_StartSound(NULL, sfx_zoom);
	COM_BufAddText(va("exec \"%s%s\"", menupath, dirmenu[dir_on[menudepthleft]]+DIR_STRING));
}

#define len menusearch[0]
static boolean M_ChangeStringAddons(INT32 choice)
{
	if (shiftdown && choice >= 32 && choice <= 127)
		choice = shiftxform[choice];

	switch (choice)
	{
		case KEY_DEL:
			if (len)
			{
				len = menusearch[1] = 0;
				return true;
			}
			break;
		case KEY_BACKSPACE:
			if (len)
			{
				menusearch[1+--len] = 0;
				return true;
			}
			break;
		default:
			if (choice >= 32 && choice <= 127)
			{
				if (len < MAXSTRINGLENGTH - 1)
				{
					menusearch[1+len++] = (char)choice;
					menusearch[1+len] = 0;
					return true;
				}
			}
			break;
	}
	return false;
}
#undef len

static void M_HandleAddons(INT32 choice)
{
	boolean exitmenu = false; // exit to previous menu

	if (M_ChangeStringAddons(choice))
	{
		char *tempname = NULL;
		if (dirmenu && dirmenu[dir_on[menudepthleft]])
			tempname = Z_StrDup(dirmenu[dir_on[menudepthleft]]+DIR_STRING); // don't need to I_Error if can't make - not important, just QoL
#if 0 // much slower
		if (!preparefilemenu(true))
		{
			UNEXIST;
			return;
		}
#else // streamlined
		searchfilemenu(tempname);
#endif
	}

	switch (choice)
	{
		case KEY_DOWNARROW:
			if (dir_on[menudepthleft] < sizedirmenu-1)
				dir_on[menudepthleft]++;
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_UPARROW:
			if (dir_on[menudepthleft])
				dir_on[menudepthleft]--;
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_PGDN:
			{
				UINT8 i;
				for (i = numaddonsshown; i && (dir_on[menudepthleft] < sizedirmenu-1); i--)
					dir_on[menudepthleft]++;
			}
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_PGUP:
			{
				UINT8 i;
				for (i = numaddonsshown; i && (dir_on[menudepthleft]); i--)
					dir_on[menudepthleft]--;
			}
			S_StartSound(NULL, sfx_menu1);
			break;
		case KEY_ENTER:
			{
				boolean refresh = true;
				if (!dirmenu[dir_on[menudepthleft]])
					S_StartSound(NULL, sfx_lose);
				else
				{
					switch (dirmenu[dir_on[menudepthleft]][DIR_TYPE])
					{
						case EXT_FOLDER:
							strcpy(&menupath[menupathindex[menudepthleft]],dirmenu[dir_on[menudepthleft]]+DIR_STRING);
							if (menudepthleft)
							{
								menupathindex[--menudepthleft] = strlen(menupath);
								menupath[menupathindex[menudepthleft]] = 0;

								if (!preparefilemenu(false))
								{
									S_StartSound(NULL, sfx_spin);
									M_StartMessage(va("%c%s\x80\nThis folder is empty.\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), M_AddonsHeaderPath()),NULL,MM_NOTHING);
									menupath[menupathindex[++menudepthleft]] = 0;

									if (!preparefilemenu(true))
									{
										UNEXIST;
										return;
									}
								}
								else
								{
									S_StartSound(NULL, sfx_menu1);
									dir_on[menudepthleft] = 1;
								}
								refresh = false;
							}
							else
							{
								S_StartSound(NULL, sfx_lose);
								M_StartMessage(va("%c%s\x80\nThis folder is too deep to navigate to!\n\n(Press a key)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), M_AddonsHeaderPath()),NULL,MM_NOTHING);
								menupath[menupathindex[menudepthleft]] = 0;
							}
							break;
						case EXT_UP:
							S_StartSound(NULL, sfx_menu1);
							menupath[menupathindex[++menudepthleft]] = 0;
							if (!preparefilemenu(false))
							{
								UNEXIST;
								return;
							}
							break;
						case EXT_TXT:
							M_StartMessage(va("%c%s\x80\nThis file may not be a console script.\nAttempt to run anyways? \n\n(Press 'Y' to confirm)\n", ('\x80' + (highlightflags>>V_CHARCOLORSHIFT)), dirmenu[dir_on[menudepthleft]]+DIR_STRING),M_AddonExec,MM_YESNO);
							break;
						case EXT_CFG:
							M_AddonExec(KEY_ENTER);
							break;
						// else intentional fallthrough
						case EXT_SOC:
						case EXT_WAD:
							COM_BufAddText(va("addfile \"%s%s\"", menupath, dirmenu[dir_on[menudepthleft]]+DIR_STRING));
							break;
						default:
							S_StartSound(NULL, sfx_lose);
					}
				}
				if (refresh)
					refreshdirmenu |= REFRESHDIR_NORMAL;
			}
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		default:
			break;
	}
	if (exitmenu)
	{
		closefilemenu(true);

		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}

static void M_ChangeLevel(INT32 choice)
{
	char mapname[6];
	(void)choice;

	strlcpy(mapname, G_BuildMapName(cv_nextmap.value), sizeof (mapname));
	strlwr(mapname);
	mapname[5] = '\0';

	M_ClearMenus(true);
	COM_BufAddText(va("map %s -gametype \"%s\"\n", mapname, cv_newgametype.string));
}

static void M_ConfirmSpectate(INT32 choice)
{
	(void)choice;
	M_ClearMenus(true);
	COM_ImmedExecute("changeteam spectator");
}

static void M_ConfirmTeamScramble(INT32 choice)
{
	(void)choice;
	M_ClearMenus(true);

	switch (cv_dummyscramble.value)
	{
		case 0:
			COM_ImmedExecute("teamscramble 1");
			break;
		case 1:
			COM_ImmedExecute("teamscramble 2");
			break;
	}
}

static void M_ConfirmTeamChange(INT32 choice)
{
	(void)choice;
	if (!cv_allowteamchange.value && cv_dummyteam.value)
	{
		M_StartMessage("The server is not allowing\n team changes at this time.\nPress a key.", NULL, MM_NOTHING);
		return;
	}

	M_ClearMenus(true);

	switch (cv_dummyteam.value)
	{
		case 0:
			COM_ImmedExecute("changeteam spectator");
			break;
		case 1:
			COM_ImmedExecute("changeteam red");
			break;
		case 2:
			COM_ImmedExecute("changeteam blue");
			break;
	}
}

static void M_DrawServerMenu(void)
{
	lumpnum_t lumpnum;
	patch_t *PictureOfLevel;

	M_DrawGenericMenu();

	//  A 160x100 image of the level as entry MAPxxP
	lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

	if (lumpnum != LUMPERROR)
		PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_CACHE);
	else
		PictureOfLevel = W_CachePatchName("BLANKLVL", PU_CACHE);

	V_DrawSmallScaledPatch((BASEVIDWIDTH*3/4)-(SHORT(PictureOfLevel->width)/4), ((BASEVIDHEIGHT*3/4)-(SHORT(PictureOfLevel->height)/4)+10), 0, PictureOfLevel);
}

static menuitem_t ChangeLevelMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Game Type",             &cv_newgametype,    30},

	{IT_STRING|IT_CVAR,              NULL, "Level",                 &cv_nextmap,        60},


	{IT_WHITESTRING|IT_CALL,         NULL, "Change Level",                 M_ChangeLevel,     120},
};

menu_t ChangeLevelDef =
{
	0,
	"Change Level",
	sizeof (ChangeLevelMenu)/sizeof (menuitem_t),
	&MainDef,
	ChangeLevelMenu,
	M_DrawServerMenu,
	27,40,
	0,
	NULL
};

static menuitem_t ChangeTeamMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Select Team",             &cv_dummyteam,    30},

	{IT_WHITESTRING|IT_CALL,         NULL, "Confirm",           M_ConfirmTeamChange,      90},
};

menu_t ChangeTeamDef =
{
	0,
	"Change Team",
	sizeof (ChangeTeamMenu)/sizeof (menuitem_t),
	&MainDef,
	ChangeTeamMenu,
	M_DrawGenericMenu,
	27,40,
	0,
	NULL
};

static menuitem_t TeamScrambleMenu[] =
{
	{IT_STRING|IT_CVAR,      NULL, "Scramble Method", &cv_dummyscramble,     30},

	{IT_WHITESTRING|IT_CALL, NULL, "Confirm",         M_ConfirmTeamScramble, 90},
};

menu_t TeamScrambleDef =
{
	0,
	"Scramble Teams",
	sizeof (ChangeTeamMenu)/sizeof (menuitem_t),
	&MainDef,
	TeamScrambleMenu,
	M_DrawGenericMenu,
	27,40,
	0,
	NULL
};

//
// M_PatchLevelNameTable
//
// Populates the cv_nextmap variable
//
// Modes:
// 0 = Create Server Menu
// 1 = Level Select Menu
// 2 = Time Attack Menu
// 3 = SRB1 Level Select Menu
//
static boolean M_PatchLevelNameTable(INT32 mode)
{
	size_t i;
	INT32 j;
	INT32 currentmap;
	boolean foundone = false;

	for (j = 0; j < LEVELARRAYSIZE-2; j++)
	{
		i = 0;
		currentmap = map_cons_t[j].value-1;

		if (mapheaderinfo[currentmap].lvlttl[0] && ((mode == 0 && !mapheaderinfo[currentmap].hideinmenu && !((mapheaderinfo[currentmap].typeoflevel & TOL_SRB1) && !(grade & 2))) || (mode == 1 && mapheaderinfo[currentmap].levelselect && !(mapheaderinfo[currentmap].typeoflevel & TOL_SRB1)) || (mode == 2 && mapheaderinfo[currentmap].timeattack && mapvisited[currentmap]) || (mode == 3 && mapheaderinfo[currentmap].levelselect && (mapheaderinfo[currentmap].typeoflevel & TOL_SRB1))))
		{
			strlcpy(lvltable[j], mapheaderinfo[currentmap].lvlttl, sizeof (lvltable[j]));

			i += strlen(mapheaderinfo[currentmap].lvlttl);

			if (!mapheaderinfo[currentmap].nozone)
			{
				lvltable[j][i++] = ' ';
				lvltable[j][i++] = 'Z';
				lvltable[j][i++] = 'O';
				lvltable[j][i++] = 'N';
				lvltable[j][i++] = 'E';
			}

			if (mapheaderinfo[currentmap].actnum)
			{
				char actnum[3];
				INT32 g;

				lvltable[j][i++] = ' ';

				sprintf(actnum, "%d", mapheaderinfo[currentmap].actnum);

				for (g = 0; g < 3; g++)
				{
					if (actnum[g] == '\0')
						break;

					lvltable[j][i++] = actnum[g];
				}
			}

			lvltable[j][i++] = '\0';
			foundone = true;
		}
		else
			lvltable[j][0] = '\0';

		if (lvltable[j][0] == '\0')
			map_cons_t[j].strvalue = NULL;
		else
			map_cons_t[j].strvalue = lvltable[j];
	}

	if (!foundone)
		return false;

	CV_SetValue(&cv_nextmap, cv_nextmap.value); // This causes crash sometimes?!

	if (mode > 0)
	{
		INT32 value = 0;

		switch (cv_newgametype.value)
		{
			case GT_COOP:
				value = TOL_COOP;
				break;
			case GT_RACE:
				value = TOL_RACE;
				break;
			case GT_MATCH:
				value = TOL_MATCH;
				break;
			case GT_TAG:
				value = TOL_TAG;
				break;
			case GT_CTF:
				value = TOL_CTF;
				break;
		}

		CV_SetValue(&cv_nextmap, FindFirstMap(value));
		CV_AddValue(&cv_nextmap, -1);
		CV_AddValue(&cv_nextmap, 1);
	}
	else
		Newgametype_OnChange(); // Make sure to start on an appropriate map if wads have been added

	return true;
}

static void M_MapChange(INT32 choice)
{
	(void)choice;
	if (!(netgame || multiplayer) || !Playing())
	{
		M_StartMessage("You aren't in a game!\nPress a key.", NULL, MM_NOTHING);
		return;
	}

	inlevelselect = 0;
	M_PatchLevelNameTable(0);

	// Special Cases
	if (gametype == GT_MATCH && cv_matchtype.value) // Team Match
		CV_SetValue(&cv_newgametype, GTF_TEAMMATCH);
	else if (gametype == GT_RACE && cv_racetype.value) // Time-Only Race
		CV_SetValue(&cv_newgametype, GTF_CLASSICRACE);
	else if (gametype == GT_TAG && cv_tagtype.value) // Hide and Seek Mode
		CV_SetValue(&cv_newgametype, GTF_HIDEANDSEEK);
	else
		CV_SetValue(&cv_newgametype, gametype);

	CV_SetValue(&cv_nextmap, gamemap);

	M_SetupNextMenu(&ChangeLevelDef);
}

static void M_TeamChange(INT32 choice)
{
	(void)choice;
	if (!(netgame || multiplayer) || !Playing())
	{
		M_StartMessage("You aren't in a game!\nPress a key.", NULL, MM_NOTHING);
		return;
	}

	M_SetupNextMenu(&ChangeTeamDef);
}

static void M_TeamScramble(INT32 choice)
{
	(void)choice;
	if (!(netgame || multiplayer) || !Playing())
	{
		M_StartMessage("You aren't in a game!\nPress a key.", NULL, MM_NOTHING);
		return;
	}

	if (!server && !adminplayer)
	{
		M_StartMessage("Only the server may use this command.\nPress a key.", NULL, MM_NOTHING);
		return;
	}

	M_SetupNextMenu(&TeamScrambleDef);
}

//
// M_PatchSkinNameTable
//
// Like M_PatchLevelNameTable, but for cv_chooseskin
//
static void M_PatchSkinNameTable(void)
{
	INT32 j;

	memset(skins_cons_t, 0, sizeof (skins_cons_t));

	for (j = 0; j < MAXSKINS; j++)
	{
		if (skins[j].name[0] != '\0')
		{
			skins_cons_t[j].strvalue = skins[j].name;
			skins_cons_t[j].value = j+1;
		}
		else
		{
			skins_cons_t[j].strvalue = NULL;
			skins_cons_t[j].value = 0;
		}
	}

	CV_SetValue(&cv_chooseskin, cv_chooseskin.value); // This causes crash sometimes?!

	CV_SetValue(&cv_chooseskin, 1);
	CV_AddValue(&cv_chooseskin, -1);
	CV_AddValue(&cv_chooseskin, 1);

	return;
}

static void M_DrawSetupChoosePlayerMenu(void);
static boolean M_QuitChoosePlayerMenu(void);
static void M_ChoosePlayer(INT32 choice);
INT32 ultmode;

menuitem_t PlayerMenu[] =
{
	{IT_CALL | IT_STRING, NULL, "SONIC", M_ChoosePlayer,  0},
	{IT_CALL | IT_STRING, NULL, "TAILS", M_ChoosePlayer,  0},
	{IT_CALL | IT_STRING, NULL, "KNUCKLES", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER4", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER5", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER6", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER7", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER8", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER9", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER10", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER11", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER12", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER13", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER14", M_ChoosePlayer,  0},
	{IT_DISABLED,         NULL, "PLAYER15", M_ChoosePlayer,  0},
};

menu_t PlayerDef =
{
	"M_PICKP",
	"Choose Your Character",
	sizeof (PlayerMenu)/sizeof (menuitem_t),//player_end,
	&MainDef,
	PlayerMenu,
	M_DrawSetupChoosePlayerMenu,
	24, 16,
	0,
	M_QuitChoosePlayerMenu
};
// Tails 03-02-2002

////////////////////////////////////////////////////////////////
//                   CHARACTER SELECT SCREEN                  //
////////////////////////////////////////////////////////////////

static inline void M_SetupChoosePlayer(INT32 choice)
{
	(void)choice;
	if (Playing() == false)
	{
		S_StopMusic();
		S_ChangeMusic(mus_chrsel, true);
	}

	M_SetupNextMenu (&PlayerDef);
}

//
//  Draw the choose player setup menu, had some fun with player anim
//
static void M_DrawSetupChoosePlayerMenu(void)
{
	INT32      mx = PlayerDef.x, my = PlayerDef.y;
	patch_t *patch;

	// Black BG
	V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);

	{
		// Compact the menu
		INT32 i;
		UINT8 alpha = 0;
		for (i = 0; i < currentMenu->numitems; i++)
		{
			if (currentMenu->menuitems[i].status == 0
			|| currentMenu->menuitems[i].status == IT_DISABLED)
				continue;

			currentMenu->menuitems[i].alphaKey = alpha;
			alpha += 8;
		}
	}

	// use generic drawer for cursor, items and title
	M_DrawGenericMenu();

	// TEXT BOX!
	// For the character
	M_DrawTextBox(mx+152,my, 16, 16);

	// For description
	M_DrawTextBox(mx-24, my+72, 20, 10);

	patch = W_CachePatchName(description[itemOn].picname, PU_CACHE);

	V_DrawString(mx-16, my+80, V_YELLOWMAP, "Speed:\nAbility:\nNotes:");

	V_DrawScaledPatch(mx+160,my+8,0,patch);
	V_DrawString(mx-16, my+80, 0, description[itemOn].info);
}

//
// Handle Setup Choose Player Menu
//
#if 0
static void M_HandleSetupChoosePlayer(INT32 choice)
{
	boolean  exitmenu = false;  // exit to previous menu and send name change

	switch (choice)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL,sfx_menu1); // Tails
			if (itemOn+1 >= SetupMultiPlayerDef.numitems)
				itemOn = 0;
			else itemOn++;
			break;

		case KEY_UPARROW:
			S_StartSound(NULL,sfx_menu1); // Tails
			if (!itemOn)
				itemOn = (INT16)(SetupMultiPlayerDef.numitems-1);
			else itemOn--;
			break;

		case KEY_ENTER:
			S_StartSound(NULL,sfx_menu1); // Tails
			exitmenu = true;
			break;

		case KEY_ESCAPE:
			exitmenu = true;
			break;

		default:
			break;
	}

	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu (currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}
#endif

static boolean M_QuitChoosePlayerMenu(void)
{
	// Stop music
	S_StopMusic();
	return true;
}


//===========================================================================
//                           NEW GAME FOR SINGLE PLAYER
//===========================================================================
static void M_Statistics(INT32 choice)
{
	(void)choice;
	if (modifiedgame && !savemoddata)
	{
		M_StartMessage("Statistics not available\nin modified games.", NULL, MM_NOTHING);
		return;
	}

	M_SetupNextMenu(&StatsDef);
}

static void M_Stats2(INT32 choice)
{
	(void)choice;
	oldlastmapnum = lastmapnum;
	M_SetupNextMenu(&Stats2Def);
}

static void M_Stats3(INT32 choice)
{
	(void)choice;
	oldlastmapnum = lastmapnum;
	M_SetupNextMenu(&Stats3Def);
}

static void M_Stats4(INT32 choice)
{
	(void)choice;
	oldlastmapnum = lastmapnum;
	M_SetupNextMenu(&Stats4Def);
}

//
// M_GetLevelEmblem
//
// Returns pointer to an emblem if an emblem exists
// for that level, and exists for that player.
// NULL if not found.
//
static emblem_t *M_GetLevelEmblem(INT32 mapnum, INT32 player)
{
	INT32 i;

	for (i = 0; i < numemblems; i++)
	{
		if (emblemlocations[i].level == mapnum
			&& emblemlocations[i].player == player)
			return &emblemlocations[i];
	}
	return NULL;
}

static void M_DrawStats(void)
{
	INT32 found = 0;
	INT32 i;
	char hours[4];
	char minutes[4];
	char seconds[4];
	tic_t besttime = 0;
	boolean displaytimeattack = true;

	for (i = 0; i < MAXEMBLEMS; i++)
	{
		if (emblemlocations[i].collected)
			found++;
	}

	V_DrawString(64, 32, 0, va("x %d/%d", found, numemblems));
	V_DrawScaledPatch(32, 32-4, 0, W_CachePatchName("EMBLICON", PU_STATIC));

	if (G_TicsToHours(totalplaytime) < 10)
		sprintf(hours, "0%i", G_TicsToHours(totalplaytime));
	else
		sprintf(hours, "%i:", G_TicsToHours(totalplaytime));

	if (G_TicsToMinutes(totalplaytime, false) < 10)
		sprintf(minutes, "0%i", G_TicsToMinutes(totalplaytime, false));
	else
		sprintf(minutes, "%i", G_TicsToMinutes(totalplaytime, false));

	if (G_TicsToSeconds(totalplaytime) < 10)
		sprintf(seconds, "0%i", G_TicsToSeconds(totalplaytime));
	else
		sprintf(seconds, "%i", G_TicsToSeconds(totalplaytime));

	V_DrawCenteredString(224, 8, 0, "Total Play Time:");
	V_DrawCenteredString(224, 20, 0, va("%s:%s:%s", hours, minutes, seconds));

	for (i = 0; i < NUMMAPS; i++)
	{
		if (!(mapheaderinfo[i].timeattack))
			continue;

		if (timedata[i].time > 0)
			besttime += timedata[i].time;
		else
			displaytimeattack = false;
	}

	if (displaytimeattack)
	{
		if (G_TicsToHours(besttime) < 10)
			sprintf(hours, "0%i", G_TicsToHours(besttime));
		else
			sprintf(hours, "%i", G_TicsToHours(besttime));

		if (G_TicsToMinutes(besttime, false) < 10)
			sprintf(minutes, "0%i", G_TicsToMinutes(besttime, false));
		else
			sprintf(minutes, "%i", G_TicsToMinutes(besttime, false));

		if (G_TicsToSeconds(besttime) < 10)
			sprintf(seconds, "0%i", G_TicsToSeconds(besttime));
		else
			sprintf(seconds, "%i", G_TicsToSeconds(besttime));

		V_DrawCenteredString(224, 36, 0, "Best Time Attack:");
		V_DrawCenteredString(224, 48, 0, va("%s:%s:%s", hours, minutes, seconds));
	}

	{
		INT32 y = 80;
		char names[8];
		emblem_t *emblem;

		V_DrawString(32+36, y-16, 0, "LEVEL NAME");
		V_DrawString(224+28, y-16, 0, "BEST TIME");

		lastmapnum = 0;
		oldlastmapnum = 0;

		sprintf(names, "%c %c %c", skins[0].name[0], skins[1].name[0], skins[2].name[0]);
		V_DrawString(32, y-16, 0, names);

		for (i = oldlastmapnum; i < NUMMAPS; i++)
		{

			if (mapheaderinfo[i].lvlttl[0] == '\0')
				continue;

			if (!(mapheaderinfo[i].typeoflevel & TOL_SP))
				continue;

			if (!mapvisited[i])
				continue;

			lastmapnum = i;

			emblem = M_GetLevelEmblem(i+1, 0);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 1);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 2);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			if (mapheaderinfo[i].actnum != 0)
				V_DrawString(32+36, y, V_YELLOWMAP, va("%s %d", mapheaderinfo[i].lvlttl, mapheaderinfo[i].actnum));
			else
				V_DrawString(32+36, y, V_YELLOWMAP, mapheaderinfo[i].lvlttl);

			if (timedata[i].time)
			{
				if (G_TicsToMinutes(timedata[i].time, true) < 10)
					sprintf(minutes, "0%i", G_TicsToMinutes(timedata[i].time, true));
				else
					sprintf(minutes, "%i", G_TicsToMinutes(timedata[i].time, true));

				if (G_TicsToSeconds(timedata[i].time) < 10)
					sprintf(seconds, "0%i", G_TicsToSeconds(timedata[i].time));
				else
					sprintf(seconds, "%i", G_TicsToSeconds(timedata[i].time));

				if (G_TicsToCentiseconds(timedata[i].time) < 10)
					sprintf(hours, "0%i", G_TicsToCentiseconds(timedata[i].time));
				else
					sprintf(hours, "%i", G_TicsToCentiseconds(timedata[i].time));

				V_DrawString(224+28, y, 0, va("%s:%s:%s", minutes,seconds,hours));
			}

			y += 8;

			if (y >= BASEVIDHEIGHT-8)
				return;
		}
	}
}

static void M_DrawStats2(void)
{
	char hours[3];
	char minutes[3];
	char seconds[3];

	{
		INT32 i;
		INT32 y = 16;
		emblem_t *emblem;

		V_DrawCenteredString(BASEVIDWIDTH/2, y-16, V_YELLOWMAP, "BEST TIMES");
		V_DrawCenteredString(BASEVIDWIDTH/2, y-8, 0, "Page 2");

		for (i = oldlastmapnum+1; i < NUMMAPS; i++)
		{
			if (mapheaderinfo[i].lvlttl[0] == '\0')
				continue;

			if (!(mapheaderinfo[i].typeoflevel & TOL_SP))
				continue;

			if (!mapvisited[i])
				continue;

			lastmapnum = i;

			emblem = M_GetLevelEmblem(i+1, 0);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 1);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 2);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			if (mapheaderinfo[i].actnum != 0)
				V_DrawString(32+36, y, V_YELLOWMAP, va("%s %d", mapheaderinfo[i].lvlttl, mapheaderinfo[i].actnum));
			else
				V_DrawString(32+36, y, V_YELLOWMAP, mapheaderinfo[i].lvlttl);

			if (timedata[i].time)
			{
				if (G_TicsToMinutes(timedata[i].time, true) < 10)
					sprintf(minutes, "0%i", G_TicsToMinutes(timedata[i].time, true));
				else
					sprintf(minutes, "%i", G_TicsToMinutes(timedata[i].time, true));

				if (G_TicsToSeconds(timedata[i].time) < 10)
					sprintf(seconds, "0%i", G_TicsToSeconds(timedata[i].time));
				else
					sprintf(seconds, "%i", G_TicsToSeconds(timedata[i].time));

				if (G_TicsToCentiseconds(timedata[i].time) < 10)
					sprintf(hours, "0%i", G_TicsToCentiseconds(timedata[i].time));
				else
					sprintf(hours, "%i", G_TicsToCentiseconds(timedata[i].time));

				V_DrawString(224+28, y, 0, va("%s:%s:%s", minutes,seconds,hours));
			}

			y += 8;

			if (y >= BASEVIDHEIGHT-8)
				return;
		}
	}
}

static void M_DrawStats3(void)
{
	char hours[3];
	char minutes[3];
	char seconds[3];

	{
		INT32 i;
		INT32 y = 16;
		emblem_t *emblem;

		V_DrawCenteredString(BASEVIDWIDTH/2, y-16, V_YELLOWMAP, "BEST TIMES");
		V_DrawCenteredString(BASEVIDWIDTH/2, y-8, 0, "Page 3");

		for (i = oldlastmapnum+1; i < NUMMAPS; i++)
		{
			if (mapheaderinfo[i].lvlttl[0] == '\0')
				continue;

			if (!(mapheaderinfo[i].typeoflevel & TOL_SP))
				continue;

			if (!mapvisited[i])
				continue;

			lastmapnum = i;

			emblem = M_GetLevelEmblem(i+1, 0);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 1);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 2);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			if (mapheaderinfo[i].actnum != 0)
				V_DrawString(32+36, y, V_YELLOWMAP, va("%s %d", mapheaderinfo[i].lvlttl, mapheaderinfo[i].actnum));
			else
				V_DrawString(32+36, y, V_YELLOWMAP, mapheaderinfo[i].lvlttl);

			if (timedata[i].time)
			{
				if (G_TicsToMinutes(timedata[i].time, true) < 10)
					sprintf(minutes, "0%i", G_TicsToMinutes(timedata[i].time, true));
				else
					sprintf(minutes, "%i", G_TicsToMinutes(timedata[i].time, true));

				if (G_TicsToSeconds(timedata[i].time) < 10)
					sprintf(seconds, "0%i", G_TicsToSeconds(timedata[i].time));
				else
					sprintf(seconds, "%i", G_TicsToSeconds(timedata[i].time));

				if (G_TicsToCentiseconds(timedata[i].time) < 10)
					sprintf(hours, "0%i", G_TicsToCentiseconds(timedata[i].time));
				else
					sprintf(hours, "%i", G_TicsToCentiseconds(timedata[i].time));

				V_DrawString(224+28, y, 0, va("%s:%s:%s", minutes,seconds,hours));
			}

			y += 8;

			if (y >= BASEVIDHEIGHT-8)
				return;
		}
	}
}

static void M_DrawStats4(void)
{
	char hours[3];
	char minutes[3];
	char seconds[3];

	{
		INT32 i;
		INT32 y = 16;
		emblem_t *emblem;

		V_DrawCenteredString(BASEVIDWIDTH/2, y-16, V_YELLOWMAP, "BEST TIMES");
		V_DrawCenteredString(BASEVIDWIDTH/2, y-8, 0, "Page 4");

		for (i = oldlastmapnum+1; i < NUMMAPS; i++)
		{
			if (mapheaderinfo[i].lvlttl[0] == '\0')
				continue;

			if (!(mapheaderinfo[i].typeoflevel & TOL_SP))
				continue;

			if (!mapvisited[i])
				continue;

			lastmapnum = i;

			emblem = M_GetLevelEmblem(i+1, 0);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 1);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 2);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			if (mapheaderinfo[i].actnum != 0)
				V_DrawString(32+36, y, V_YELLOWMAP, va("%s %d", mapheaderinfo[i].lvlttl, mapheaderinfo[i].actnum));
			else
				V_DrawString(32+36, y, V_YELLOWMAP, mapheaderinfo[i].lvlttl);

			if (timedata[i].time)
			{
				if (G_TicsToMinutes(timedata[i].time, true) < 10)
					sprintf(minutes, "0%i", G_TicsToMinutes(timedata[i].time, true));
				else
					sprintf(minutes, "%i", G_TicsToMinutes(timedata[i].time, true));

				if (G_TicsToSeconds(timedata[i].time) < 10)
					sprintf(seconds, "0%i", G_TicsToSeconds(timedata[i].time));
				else
					sprintf(seconds, "%i", G_TicsToSeconds(timedata[i].time));

				if (G_TicsToCentiseconds(timedata[i].time) < 10)
					sprintf(hours, "0%i", G_TicsToCentiseconds(timedata[i].time));
				else
					sprintf(hours, "%i", G_TicsToCentiseconds(timedata[i].time));

				V_DrawString(224+28, y, 0, va("%s:%s:%s", minutes,seconds,hours));
			}

			y += 8;

			if (y >= BASEVIDHEIGHT-8)
				return;
		}
	}
}

static void M_DrawStats5(void)
{
	char hours[3];
	char minutes[3];
	char seconds[3];

	{
		INT32 i;
		INT32 y = 16;
		emblem_t *emblem;

		V_DrawCenteredString(BASEVIDWIDTH/2, y-16, V_YELLOWMAP, "BEST TIMES");
		V_DrawCenteredString(BASEVIDWIDTH/2, y-8, 0, "Page 5");

		for (i = oldlastmapnum+1; i < NUMMAPS; i++)
		{
			if (mapheaderinfo[i].lvlttl[0] == '\0')
				continue;

			if (!(mapheaderinfo[i].typeoflevel & TOL_SP))
				continue;

			if (!mapvisited[i])
				continue;

			lastmapnum = i;

			emblem = M_GetLevelEmblem(i+1, 0);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(30, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 1);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(42, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			emblem = M_GetLevelEmblem(i+1, 2);

			if (emblem)
			{
				if (emblem->collected)
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("GOTIT", PU_CACHE));
				else
					V_DrawScaledPatch(54, y, 0, W_CachePatchName("NEEDIT", PU_CACHE));
			}

			if (mapheaderinfo[i].actnum != 0)
				V_DrawString(32+36, y, V_YELLOWMAP, va("%s %d", mapheaderinfo[i].lvlttl, mapheaderinfo[i].actnum));
			else
				V_DrawString(32+36, y, V_YELLOWMAP, mapheaderinfo[i].lvlttl);

			if (timedata[i].time)
			{
				if (G_TicsToMinutes(timedata[i].time, true) < 10)
					sprintf(minutes, "0%i", G_TicsToMinutes(timedata[i].time, true));
				else
					sprintf(minutes, "%i", G_TicsToMinutes(timedata[i].time, true));

				if (G_TicsToSeconds(timedata[i].time) < 10)
					sprintf(seconds, "0%i", G_TicsToSeconds(timedata[i].time));
				else
					sprintf(seconds, "%i", G_TicsToSeconds(timedata[i].time));

				if (G_TicsToCentiseconds(timedata[i].time) < 10)
					sprintf(hours, "0%i", G_TicsToCentiseconds(timedata[i].time));
				else
					sprintf(hours, "%i", G_TicsToCentiseconds(timedata[i].time));

				V_DrawString(224+28, y, 0, va("%s:%s:%s", minutes,seconds,hours));
			}

			y += 8;

			if (y >= BASEVIDHEIGHT-8)
				return;
		}
	}
}

static void M_NewGame(void)
{
	fromlevelselect = false;
	pandoralevelselect = false;
	ultmode = false;

	startmap = spstage_start;
	CV_SetValue(&cv_newgametype, GT_COOP); // Graue 09-08-2004

	PlayerDef.prevMenu = currentMenu;
	M_SetupChoosePlayer(0);

}

static void M_SRB1Remake(INT32 choice)
{
	(void)choice;
	if (netgame && Playing())
	{
		M_StartMessage(text[NEWGAME],M_ExitGameResponse,MM_YESNO);
		return;
	}

	startmap = 101;

	PlayerDef.prevMenu = currentMenu;
	M_SetupNextMenu(&PlayerDef);

}

static void M_NightsGame(INT32 choice)
{
	(void)choice;
	if (netgame && Playing())
	{
		M_StartMessage(text[NEWGAME],M_ExitGameResponse,MM_YESNO);
		return;
	}

	startmap = 29;

	PlayerDef.prevMenu = currentMenu;
	M_SetupNextMenu(&PlayerDef);

}

static void M_MarioGame(INT32 choice)
{
	(void)choice;
	if (netgame && Playing())
	{
		M_StartMessage(text[NEWGAME],M_ExitGameResponse,MM_YESNO);
		return;
	}

	startmap = 30;

	PlayerDef.prevMenu = currentMenu;
	M_SetupNextMenu(&PlayerDef);

}

static void M_NAGZGame(INT32 choice)
{
	(void)choice;
	if (netgame && Playing())
	{
		M_StartMessage(text[NEWGAME],M_ExitGameResponse,MM_YESNO);
		return;
	}

	startmap = 40;

	PlayerDef.prevMenu = currentMenu;
	M_SetupNextMenu(&PlayerDef);

}

static void M_CustomWarp(INT32 choice)
{
	if (netgame && Playing())
	{
		M_StartMessage(text[NEWGAME],M_ExitGameResponse,MM_YESNO);
		return;
	}

	startmap = (INT16)(customsecretinfo[choice-1].variable);

	PlayerDef.prevMenu = currentMenu;
	M_SetupNextMenu(&PlayerDef);

}

// Chose the player you want to use Tails 03-02-2002
static void M_ChoosePlayer(INT32 choice)
{
	INT32 skinnum;

	M_ClearMenus(true);

	strlwr(description[choice].skinname);

	skinnum = R_SkinAvailable(description[choice].skinname);

	if (startmap != spstage_start)
		cursaveslot = -1;

	lastmapsaved = 0;
	gamecomplete = false;

	G_DeferedInitNew(ultmode, G_BuildMapName(startmap), skinnum, fromlevelselect);
	COM_BufAddText("dummyconsvar 1\n"); // G_DeferedInitNew doesn't do this
}

static void M_ReplayTimeAttack(INT32 choice);
static void M_ChooseTimeAttackNoRecord(INT32 choice);
static void M_ChooseTimeAttack(INT32 choice);
static void M_DrawTimeAttackMenu(void);

typedef enum
{
	taplayer = 0,
	talevel,
	tareplay,
	tastart,
	timeattack_end
} timeattack_e;

static menuitem_t TimeAttackMenu[] =
{
	{IT_STRING|IT_CVAR,      NULL, "Player",     &cv_chooseskin,   50},
	{IT_STRING|IT_CVAR,      NULL, "Level",      &cv_nextmap,      65},

	{IT_WHITESTRING|IT_CALL, NULL, "Replay",   M_ReplayTimeAttack,        100},
	{IT_WHITESTRING|IT_CALL, NULL, "Start (No Record)",  M_ChooseTimeAttackNoRecord,115},
	{IT_WHITESTRING|IT_CALL, NULL, "Start (Record)",     M_ChooseTimeAttack,        130},
};

menu_t TimeAttackDef =
{
	0,
	"Time Attack",
	sizeof (TimeAttackMenu)/sizeof (menuitem_t),
	&SinglePlayerDef,
	TimeAttackMenu,
	M_DrawTimeAttackMenu,
	40, 40,
	0,
	NULL
};

// Used only for time attack menu
static void Nextmap_OnChange(void)
{
	if (currentMenu != &TimeAttackDef)
		return;

	TimeAttackMenu[tareplay].status = IT_DISABLED;

	// Check if file exists, if not, disable REPLAY option
	if (FIL_FileExists(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%02d.lmp", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), cv_chooseskin.value-1)))
		TimeAttackMenu[tareplay].status = IT_WHITESTRING|IT_CALL;
}

//
// M_TimeAttack
//
static void M_TimeAttack(INT32 choice)
{
	(void)choice;

	if (modifiedgame && !savemoddata)
	{
		M_StartMessage("This cannot be done in a modified game.\n",NULL,MM_NOTHING);
		return;
	}

	if (Playing())
	{
		M_StartMessage(ALREADYPLAYING,M_ExitGameResponse,MM_YESNO);
		return;
	}

	memset(skins_cons_t, 0, sizeof (skins_cons_t));

	if (!(M_PatchLevelNameTable(2)))
	{
		M_StartMessage("No time-attackable levels found.\n",NULL,MM_NOTHING);
		return;
	}

	inlevelselect = 2; // Don't be dependent on cv_newgametype

	M_PatchSkinNameTable();

	M_SetupNextMenu(&TimeAttackDef);

	CV_AddValue(&cv_nextmap, 1);
	CV_AddValue(&cv_nextmap, -1);

	G_SetGamestate(GS_TIMEATTACK);
	S_ChangeMusic(mus_racent, true);
}

//
// M_DrawTimeAttackMenu
//
void M_DrawTimeAttackMenu(void)
{
	patch_t *PictureOfLevel;
	lumpnum_t lumpnum;
	char hours[4];
	char minutes[4];
	char seconds[4];
	char tics[4];
	tic_t besttime = 0;
	INT32 i;

	S_ChangeMusic(mus_racent, true); // Eww, but needed for when user hits escape during demo playback

	V_DrawPatchFill(W_CachePatchName("SRB2BACK", PU_CACHE));

	if (W_CheckNumForName(description[cv_chooseskin.value-1].picname) != LUMPERROR)
		V_DrawSmallScaledPatch(224, 16, 0, W_CachePatchName(description[cv_chooseskin.value-1].picname, PU_CACHE));

	//  A 160x100 image of the level as entry MAPxxP
	lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

	if (lumpnum != LUMPERROR)
		PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_CACHE);
	else
		PictureOfLevel = W_CachePatchName("BLANKLVL", PU_CACHE);

	V_DrawSmallScaledPatch(208, 128, 0, PictureOfLevel);

	for (i = 0; i < NUMMAPS; i++)
	{
		if (!(mapheaderinfo[i].timeattack))
			continue;

		if (timedata[i].time > 0)
			besttime += timedata[i].time;
	}

	sprintf(hours,   "%02i", G_TicsToHours(besttime));
	sprintf(minutes, "%02i", G_TicsToMinutes(besttime, false));
	sprintf(seconds, "%02i", G_TicsToSeconds(besttime));
	sprintf(tics,    "%02i", G_TicsToCentiseconds(besttime));

	V_DrawCenteredString(128, 36, 0, "Best Time Attack:");
	V_DrawCenteredString(128, 48, 0, va("%s:%s:%s.%s", hours, minutes, seconds, tics));

	if (cv_nextmap.value)
	{
		if (timedata[cv_nextmap.value-1].time > 0)
			V_DrawCenteredString(BASEVIDWIDTH/2, 116, 0, va("Best Time: %i:%02i.%02i", G_TicsToMinutes(timedata[cv_nextmap.value-1].time, true),
				G_TicsToSeconds(timedata[cv_nextmap.value-1].time), G_TicsToCentiseconds(timedata[cv_nextmap.value-1].time)));
	}

	M_DrawGenericMenu();
}

//
// M_ChooseTimeAttackNoRecord
//
// Like M_ChooseTimeAttack, but doesn't record a demo.
static void M_ChooseTimeAttackNoRecord(INT32 choice)
{
	(void)choice;
	emeralds = 0;
	M_ClearMenus(true);
	timeattacking = true;
	remove(va("%s"PATHSEP"temp.lmp", srb2home));
	G_DeferedInitNew(false, G_BuildMapName(cv_nextmap.value), cv_chooseskin.value-1, false);
	timeattacking = true;
}

//
// M_ChooseTimeAttack
//
// Player has selected the "START" from the time attack screen
static void M_ChooseTimeAttack(INT32 choice)
{
	(void)choice;
	emeralds = 0;
	M_ClearMenus(true);
	timeattacking = true;
	G_RecordDemo("temp");
	G_BeginRecording();
	G_DeferedInitNew(false, G_BuildMapName(cv_nextmap.value), cv_chooseskin.value-1, false);
	timeattacking = true;
}

//
// M_ReplayTimeAttack
//
// Player has selected the "REPLAY" from the time attack screen
static void M_ReplayTimeAttack(INT32 choice)
{
	(void)choice;
	M_ClearMenus(true);
	G_DoPlayDemo(va("%s"PATHSEP"replay"PATHSEP"%s"PATHSEP"%s-%02d", srb2home, timeattackfolder, G_BuildMapName(cv_nextmap.value), cv_chooseskin.value-1));

	timeattacking = true;
}

static void M_EraseData(INT32 choice);

// Tails 08-11-2002
//===========================================================================
//                        Data OPTIONS MENU
//===========================================================================

static menuitem_t DataOptionsMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Erase Time Attack Data", M_EraseData, 0},
	{IT_STRING | IT_CALL, NULL, "Erase Secrets Data", M_EraseData, 0},
};

menu_t DataOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (DataOptionsMenu)/sizeof (menuitem_t),
	&GameOptionDef,
	DataOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

static void M_TimeDataResponse(INT32 ch)
{
	INT32 i;
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	// Delete the data
	for (i = 0; i < NUMMAPS; i++)
		timedata[i].time = 0;

	M_SetupNextMenu(&DataOptionsDef);
}

static void M_SecretsDataResponse(INT32 ch)
{
	INT32 i;
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	// Delete the data
	for (i = 0; i < MAXEMBLEMS; i++)
		emblemlocations[i].collected = false;

	grade = 0;
	timesbeaten = 0;

	M_ClearMenus(true);
}

static void M_EraseData(INT32 choice)
{
	if (Playing())
	{
		M_StartMessage("A game cannot be running.\nEnd it first.",NULL,MM_NOTHING);
		return;
	}

	else if (choice == 0)
		M_StartMessage("Are you sure you want to delete\nthe time attack data?\n(Y/N)\n",M_TimeDataResponse,MM_YESNO);
	else // 1
		M_StartMessage("Are you sure you want to delete\nthe secrets data?\n(Y/N)\n",M_SecretsDataResponse,MM_YESNO);
}

void M_OnePControlsMenu(INT32 choice);

static menuitem_t ControlsMenu[] =
{
	{IT_CALL | IT_STRING, NULL, "Player 1 Controls...", M_OnePControlsMenu,  20},

	{IT_SUBMENU | IT_STRING, NULL, "Joystick Options...", &JoystickDef  ,  60},
	{IT_SUBMENU | IT_STRING, NULL, "Mouse Options...", &MouseOptionsDef, 70},

	{IT_STRING  | IT_CVAR, NULL, "Control per key", &cv_controlperkey, 100}, // Changed all to normal string Tails 11-30-2000
};

menu_t ControlsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (ControlsMenu)/sizeof (menuitem_t),
	&OptionsDef,
	ControlsMenu,
	M_DrawGenericMenu,
	60, 24,
	0,
	NULL
};

static menuitem_t OnePControlsMenu[] =
{
	{IT_CALL    | IT_STRING, NULL, "Control Configuration...", M_Setup1PControlsMenu,   20},

	{IT_STRING  | IT_CVAR, NULL, "Camera"  , &cv_chasecam  ,  40}, // Changed all to normal string Tails 11-30-2000

	{IT_STRING  | IT_CVAR, NULL, "Analog Control", &cv_useranalog,  60}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING  | IT_CVAR, NULL, "Autoaim" , &cv_autoaim   ,  80}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING  | IT_CVAR, NULL, "Crosshair", &cv_crosshair , 100}, // Changed all to normal string Tails 11-30-2000
};

menu_t OnePControlsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (OnePControlsMenu)/sizeof (menuitem_t),
	&ControlsDef,
	OnePControlsMenu,
	M_DrawGenericMenu,
	60, 24,
	0,
	NULL
};

void M_OnePControlsMenu(INT32 choice)
{
	(void)choice;
	M_SetupNextMenu(&OnePControlsDef);
}

//===========================================================================
//                             OPTIONS MENU
//===========================================================================
//
// M_Options
//

//added : 10-02-98: note: alphaKey member is the y offset
static menuitem_t OptionsMenu[] =
{
	{IT_SUBMENU | IT_STRING, NULL, "Setup Controls...",     &ControlsDef,      10},
	{IT_CALL    | IT_STRING, NULL, "Game Options...",       M_GameOption,      30},
	{IT_CALL    | IT_STRING, NULL, "Gametype Options...",   M_GametypeOptions, 40},
	{IT_SUBMENU | IT_STRING, NULL, "Sound Options...",      &SoundDef,         70},
	{IT_SUBMENU | IT_STRING, NULL, "Video Options...",      &VideoOptionsDef,  80},

	{IT_STRING  | IT_CALL,   NULL, "Add-on Options...",     M_AddonsOptions,  100}, // this too
	{IT_SUBMENU | IT_STRING, NULL, "Retro Options...", 	&RetroDef,	  110} // fits here ig
};

menu_t OptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (OptionsMenu)/sizeof (menuitem_t),
	&MainDef,
	OptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

// Tails 08-18-2002
static void M_OptionsMenu(INT32 choice)
{
	(void)choice;
	M_SetupNextMenu (&OptionsDef);
}

FUNCNORETURN static ATTRNORETURN void M_UltimateCheat(INT32 choice)
{
	(void)choice;
	I_Quit ();
}

static void M_GetAllEmeralds(INT32 choice)
{
	(void)choice;

	if (!(Playing() && gamestate == GS_LEVEL))
	{
		M_StartMessage("You need to be playing and in\na level to do this!",NULL,MM_NOTHING);
		return;
	}

	if (multiplayer || netgame)
	{
		M_StartMessage("You can't do this in\na network game!",NULL,MM_NOTHING);
		return;
	}

	emeralds = ((EMERALD7)*2)-1;
	M_StartMessage("You now have all 7 emeralds.",NULL,MM_NOTHING);
}

static void M_DestroyRobotsResponse(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	// Destroy all robots
	P_DestroyRobots();

	M_ClearMenus(true);
}

static void M_DestroyRobots(INT32 choice)
{
	(void)choice;
	if (!(Playing() && gamestate == GS_LEVEL))
	{
		M_StartMessage("You need to be playing and in\na level to do this!",NULL,MM_NOTHING);
		return;
	}

	if (multiplayer || netgame)
	{
		M_StartMessage("You can't do this in\na network game!",NULL,MM_NOTHING);
		return;
	}

	M_StartMessage("Do you want to destroy all\nrobots in the current level?\n(Y/N)\n",M_DestroyRobotsResponse,MM_YESNO);
}

static void M_LevelSelectWarp(INT32 choice)
{
	(void)choice;
	if (netgame && Playing())
	{
		M_StartMessage(text[NEWGAME],M_ExitGameResponse,MM_YESNO);
		return;
	}

	if (W_CheckNumForName(G_BuildMapName(cv_nextmap.value)) == LUMPERROR)
	{
//		CONS_Printf("\2Internal game map '%s' not found\n", G_BuildMapName(cv_nextmap.value));
		return;
	}

	// Allow character select when level warping from Pandora's Box,
	// even if you are playing a fully completed save.
	if (pandoralevelselect)
	{
		//disassociate our save game since we're using the general level select.
		fromloadgame = 0;
		cursaveslot = -1;
	}

	if (!fromloadgame)
	{
		PlayerDef.prevMenu = currentMenu;
		M_SetupNextMenu(&PlayerDef);
	}

	startmap = (INT16)(cv_nextmap.value);

	fromlevelselect = true;


	if (fromloadgame)
	{
		G_LoadGame((UINT32)fromloadgame - 1, startmap);
		M_ClearMenus(true);
	}
}

/** Checklist of unlockable bonuses.
  */
typedef struct
{
	const char *name;        ///< What you get.
	const char *requirement; ///< What you have to do.
	boolean unlocked;        ///< Whether you've done it.
} checklist_t;

// Tails 12-19-2003
static void M_DrawUnlockChecklist(void)
{
#define NUMCHECKLIST 9
	checklist_t checklist[NUMCHECKLIST];
	INT32 i = 0;
	INT32 y = 8;

	checklist[i].name = "Level Select";
	checklist[i].requirement = "Find All Emblems";
	checklist[i].unlocked = (grade & 8);
	i++;

	checklist[i].name = "SRB1 Remake";
	checklist[i].requirement = "Finish 1P\nw/ Emeralds";
	checklist[i].unlocked = (grade & 2);
	i++;

	checklist[i].name = "Sonic Into Dreams";
	checklist[i].requirement = "Find 10 Emblems";
	checklist[i].unlocked = (grade & 16);
	i++;

	checklist[i].name = "Mario Koopa Blast";
	checklist[i].requirement = "Find 20 Emblems";
	checklist[i].unlocked = (grade & 4);
	i++;

	checklist[i].name = "Pandora's Box";
	checklist[i].requirement = "Find All Emblems";
	checklist[i].unlocked = (grade & 8);
	i++;

	checklist[i].name = "Extra Emblem #1";
	checklist[i].requirement = "Finish 1P";
	checklist[i].unlocked = (emblemlocations[MAXEMBLEMS-2].collected);
	i++;

	checklist[i].name = "Extra Emblem #2";
	checklist[i].requirement = "Finish 1P\nw/ Emeralds";
	checklist[i].unlocked = (emblemlocations[MAXEMBLEMS-1].collected);
	i++;

	checklist[i].name = "Extra Emblem #3";
	checklist[i].requirement = "Finish 1P in\n23 minutes";
	checklist[i].unlocked = (emblemlocations[MAXEMBLEMS-3].collected);
	i++;

	checklist[i].name = "Extra Emblem #4";
	checklist[i].requirement = "Perfect Bonus on\nany stage";
	checklist[i].unlocked = (emblemlocations[MAXEMBLEMS-4].collected);
	i++;

	for (i = 0; i < NUMCHECKLIST; i++)
	{
		V_DrawString(8, y, V_RETURN8, checklist[i].name);
		V_DrawString(160, y, V_RETURN8, checklist[i].requirement);

		if (checklist[i].unlocked)
			V_DrawString(308, y, V_YELLOWMAP, "Y");
		else
			V_DrawString(308, y, V_YELLOWMAP, "N");

		y += 20;
	}
}

boolean M_GotEnoughEmblems(INT32 number)
{
	INT32 i;
	INT32 gottenemblems = 0;

	for (i = 0; i < MAXEMBLEMS; i++)
	{
		if (emblemlocations[i].collected)
			gottenemblems++;
	}

	if (gottenemblems >= number)
		return true;

	return false;
}

boolean M_GotLowEnoughTime(INT32 ptime)
{
	INT32 seconds = 0;
	INT32 i;

	for (i = 0; i < NUMMAPS; i++)
	{
		if (!(mapheaderinfo[i].timeattack))
			continue;

		if (timedata[i].time > 0)
			seconds += timedata[i].time;
		else
			seconds += 800*TICRATE;
	}

	seconds /= TICRATE;

	if (seconds <= ptime)
		return true;

	return false;
}

static void M_DrawCustomChecklist(void)
{
	INT32 numcustom = 0;
	INT32 i;
	INT32 totalnum = 0;
	checklist_t checklist[15];

	memset(checklist, 0, sizeof (checklist));

	for (i = 0; i < 15; i++)
	{
		if (customsecretinfo[i].neededemblems)
		{
			checklist[i].unlocked = M_GotEnoughEmblems(customsecretinfo[i].neededemblems);

			if (checklist[i].unlocked && customsecretinfo[i].neededtime)
				checklist[i].unlocked = M_GotLowEnoughTime(customsecretinfo[i].neededtime);

			if (checklist[i].unlocked && customsecretinfo[i].neededgrade)
				checklist[i].unlocked = (grade & customsecretinfo[i].neededgrade);
		}
		else if (customsecretinfo[i].neededtime)
		{
			checklist[i].unlocked = M_GotLowEnoughTime(customsecretinfo[i].neededtime);

			if (checklist[i].unlocked && customsecretinfo[i].neededgrade)
				checklist[i].unlocked = (grade & customsecretinfo[i].neededgrade);
		}
		else
			checklist[i].unlocked = (grade & customsecretinfo[i].neededgrade);

		if (checklist[i].unlocked)
			totalnum++;
	}

	for (i = 0; i < 15; i++)
	{
		if (checklist[i].unlocked && totalnum > 7)
			continue;

		if (customsecretinfo[i].name[0] == 0)
			continue;

		V_DrawString(8, 8+(24*numcustom), V_RETURN8, customsecretinfo[i].name);
		V_DrawString(160, 8+(24*numcustom), V_RETURN8|V_WORDWRAP, customsecretinfo[i].objective);

		if (checklist[i].unlocked)
			V_DrawString(308, 8+(24*numcustom), V_YELLOWMAP, "Y");
		else
			V_DrawString(308, 8+(24*numcustom), V_YELLOWMAP, "N");

		numcustom++;

		if (numcustom > 6)
			break;
	}
}

// Empty thingy for checklist menu
typedef enum
{
	unlockchecklistempty1,
	unlockchecklist_end
} unlockchecklist_e;

static menuitem_t UnlockChecklistMenu[] =
{
	{IT_CALL | IT_STRING, NULL, "NEXT", M_SecretsMenu, 192},
};

menu_t UnlockChecklistDef =
{
	NULL,
	NULL,
	unlockchecklist_end,
	&SecretsDef,
	UnlockChecklistMenu,
	M_DrawUnlockChecklist,
	280, 185,
	0,
	NULL
};

// Empty thingy for custom checklist menu
typedef enum
{
	customchecklistempty1,
	customchecklist_end
} customchecklist_e;

static menuitem_t CustomChecklistMenu[] =
{
	{IT_CALL | IT_STRING, NULL, "NEXT", M_CustomSecretsMenu, 192},
};

menu_t CustomChecklistDef =
{
	NULL,
	NULL,
	customchecklist_end,
	&CustomSecretsDef,
	CustomChecklistMenu,
	M_DrawCustomChecklist,
	280, 185,
	0,
	NULL
};

static void M_UnlockChecklist(INT32 choice)
{
	(void)choice;
	if (savemoddata)
	{
		M_StartMessage("Checklist does not apply\nfor this mod.\nUse statistics screen instead.", NULL, MM_NOTHING);
		return;
	}
	M_SetupNextMenu(&UnlockChecklistDef);
}

static void M_CustomChecklist(INT32 choice)
{
	(void)choice;
	M_SetupNextMenu(&CustomChecklistDef);
}

static void M_BetaShowcase(INT32 choice)
{
	(void)choice;
}

//===========================================================================
//                             ??? MENU
//===========================================================================
//
// M_Options
//
static void M_Reward(INT32 choice);
static void M_LevelSelect(INT32 choice);
static void M_SRB1LevelSelect(INT32 choice);

typedef enum
{
	unlockchecklist = 0,
	ultimatecheat,
	soundtest,
	secretsgravity,
	ringslinger,
	getemeralds,
	levelselect,
	norobots,
	betashowcase,
	reward,
	secrets_end
} secrets_e;

//added : 10-02-98: note: alphaKey member is the y offset
static menuitem_t SecretsMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Secrets Checklist",  M_UnlockChecklist,    0},
	{IT_STRING | IT_CALL, NULL, "Ultimate Cheat",     M_UltimateCheat,     20},
	{IT_STRING | IT_CVAR, NULL, "Sound Test",         &cv_soundtest,       30},
	{IT_STRING | IT_CVAR, NULL, "Gravity",            &cv_gravity,         50},
	{IT_STRING | IT_CVAR, NULL, "Throw Rings",        &cv_ringslinger,     60},
	{IT_STRING | IT_CALL, NULL, "Get All Emeralds",   M_GetAllEmeralds,    70},
	{IT_STRING | IT_CALL, NULL, "Level Select",       M_LevelSelect,       90},
	{IT_STRING | IT_CALL, NULL, "Destroy All Robots", M_DestroyRobots,    110},
	{IT_STRING | IT_CALL, NULL, "Beta Showcase",      M_BetaShowcase,     120},
	{IT_STRING | IT_CALL, NULL, "Bonus Levels",       M_Reward,           130},
};

menu_t SecretsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	secrets_end,
	&MainDef,
	SecretsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                             Custom Secrets MENU
//===========================================================================
//
//
//

typedef enum
{
	customchecklist = 0,
	custom1,
	custom2,
	custom3,
	custom4,
	custom5,
	custom6,
	custom7,
	custom8,
	custom9,
	custom10,
	custom11,
	custom12,
	custom13,
	custom14,
	custom15,
	customsecrets_end
} customsecrets_e;

//added : 10-02-98: note: alphaKey member is the y offset
static menuitem_t CustomSecretsMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Secrets Checklist",  M_CustomChecklist,    0},
	{IT_STRING | IT_CALL, NULL, "Custom1",   M_CustomLevelSelect,       10},
	{IT_STRING | IT_CALL, NULL, "Custom2",   M_CustomWarp,    20},
	{IT_STRING | IT_CALL, NULL, "Custom3",   M_CustomWarp,    30},
	{IT_STRING | IT_CALL, NULL, "Custom4",   M_CustomWarp,    40},
	{IT_STRING | IT_CALL, NULL, "Custom5",   M_CustomWarp,    50},
	{IT_STRING | IT_CALL, NULL, "Custom6",   M_CustomWarp,    60},
	{IT_STRING | IT_CALL, NULL, "Custom7",   M_CustomWarp,    70},
	{IT_STRING | IT_CALL, NULL, "Custom8",   M_CustomWarp,    80},
	{IT_STRING | IT_CALL, NULL, "Custom9",   M_CustomWarp,    90},
	{IT_STRING | IT_CALL, NULL, "Custom10",   M_CustomWarp,    100},
	{IT_STRING | IT_CALL, NULL, "Custom11",   M_CustomWarp,    110},
	{IT_STRING | IT_CALL, NULL, "Custom12",   M_CustomWarp,    120},
	{IT_STRING | IT_CALL, NULL, "Custom13",   M_CustomWarp,    130},
	{IT_STRING | IT_CALL, NULL, "Custom14",   M_CustomWarp,    140},
	{IT_STRING | IT_CALL, NULL, "Custom15",   M_CustomWarp,    150},
};

menu_t CustomSecretsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	customsecrets_end,
	&MainDef,
	CustomSecretsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                             Reward MENU
//===========================================================================
//
// M_Reward
//
typedef enum
{
	nights,
	mario,
	srb1_remake,
	srb1_levelselect,
	nagz,
	reward_end
} reward_e;

//added : 10-02-98: note: alphaKey member is the y offset
static menuitem_t RewardMenu[] =
{
	{IT_STRING | IT_CALL, NULL, "Sonic Into Dreams", M_NightsGame,    30},
	{IT_STRING | IT_CALL, NULL, "Mario Koopa Blast", M_MarioGame,     50},
	{IT_STRING | IT_CALL, NULL,       "SRB1 Remake", M_SRB1Remake,    70},
	{IT_STRING | IT_CALL, NULL, "SRB1 Level Select", M_SRB1LevelSelect, 80},
	{IT_STRING | IT_CALL, NULL, "Neo Aerial Garden", M_NAGZGame,     100},
};

menu_t RewardDef =
{
	"M_OPTTTL",
	"OPTIONS",
	reward_end,
	&SecretsDef,
	RewardMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

static void M_Reward(INT32 choice)
{
	(void)choice;
	if (grade & 2)
		RewardMenu[srb1_remake].status = IT_STRING | IT_CALL;
	else
		RewardMenu[srb1_remake].status |= IT_DISABLED;

	if (grade & 4)
		RewardMenu[mario].status = IT_STRING | IT_CALL;
	else
		RewardMenu[mario].status |= IT_DISABLED;

	if (grade & 16)
		RewardMenu[nights].status = IT_STRING | IT_CALL;
	else
		RewardMenu[nights].status |= IT_DISABLED;

	if (grade & 1024)
		RewardMenu[srb1_levelselect].status = IT_STRING | IT_CALL;
	else
		RewardMenu[srb1_levelselect].status |= IT_DISABLED;

	if (grade & 2048)
		RewardMenu[nagz].status = IT_STRING | IT_CALL;
	else
		RewardMenu[nagz].status |= IT_DISABLED;

	M_SetupNextMenu (&RewardDef);
}

//===========================================================================
//                             Level Select Menu
//===========================================================================
//
// M_LevelSelect
//

static void M_DrawLevelSelectMenu(void)
{
	M_DrawGenericMenu();

	if (cv_nextmap.value)
	{
		lumpnum_t lumpnum;
		patch_t *PictureOfLevel;

		//  A 160x100 image of the level as entry MAPxxP
		lumpnum = W_CheckNumForName(va("%sP", G_BuildMapName(cv_nextmap.value)));

		if (lumpnum != LUMPERROR)
			PictureOfLevel = W_CachePatchName(va("%sP", G_BuildMapName(cv_nextmap.value)), PU_CACHE);
		else
			PictureOfLevel = W_CachePatchName("BLANKLVL", PU_CACHE);

		V_DrawSmallScaledPatch(200, 110, 0, PictureOfLevel);
	}
}

static menuitem_t LevelSelectMenu[] =
{
	{IT_STRING|IT_CVAR,              NULL, "Level",                 &cv_nextmap,        60},

	{IT_WHITESTRING|IT_CALL,         NULL, "Start",                 M_LevelSelectWarp,     120},
};

menu_t LevelSelectDef =
{
	0,
	"Level Select",
	sizeof (LevelSelectMenu)/sizeof (menuitem_t),
	&SecretsDef,
	LevelSelectMenu,
	M_DrawLevelSelectMenu,
	40, 40,
	0,
	NULL
};

static void M_SRB1LevelSelect(INT32 choice)
{
	(void)choice;
	LevelSelectDef.prevMenu = &SecretsDef;
	inlevelselect = 3;
	pandoralevelselect = true;

	if (!(M_PatchLevelNameTable(3)))
	{
		M_StartMessage("No selectable levels found.\n",NULL,MM_NOTHING);
		return;
	}
	M_SetupNextMenu(&LevelSelectDef);
}

static void M_LevelSelect(INT32 choice)
{
	(void)choice;
	LevelSelectDef.prevMenu = &SecretsDef;
	inlevelselect = 1;
	pandoralevelselect = true;

	if (!(M_PatchLevelNameTable(1)))
	{
		M_StartMessage("No selectable levels found.\n",NULL,MM_NOTHING);
		return;
	}
	M_SetupNextMenu(&LevelSelectDef);
}

static void M_CustomLevelSelect(INT32 choice)
{
	(void)choice;
	LevelSelectDef.prevMenu = &CustomSecretsDef;
	inlevelselect = 1;
	pandoralevelselect = true;

	if (!(M_PatchLevelNameTable(1)))
	{
		M_StartMessage("No selectable levels found.\n",NULL,MM_NOTHING);
		return;
	}
	M_SetupNextMenu(&LevelSelectDef);
}

static void M_SecretsMenu(INT32 choice)
{
	INT32 i;

	// Disable all the menu choices
	(void)choice;
	for (i = ultimatecheat;i < secrets_end;i++)
		SecretsMenu[i].status = IT_DISABLED;

	// Check grade and enable options as appropriate
	if (grade & 8)
	{
		SecretsMenu[norobots].status = IT_STRING | IT_CALL;
		SecretsMenu[ringslinger].status = IT_STRING | IT_CVAR;
		SecretsMenu[secretsgravity].status = IT_STRING | IT_CVAR;
		SecretsMenu[ultimatecheat].status = IT_STRING | IT_CALL;
		SecretsMenu[levelselect].status = IT_STRING | IT_CALL;
		SecretsMenu[getemeralds].status = IT_STRING | IT_CALL;
	}
	else
	{
		SecretsMenu[norobots].status = IT_DISABLED;
		SecretsMenu[ringslinger].status = IT_DISABLED;
		SecretsMenu[secretsgravity].status = IT_DISABLED;
		SecretsMenu[ultimatecheat].status = IT_DISABLED;
		SecretsMenu[levelselect].status = IT_DISABLED;
		SecretsMenu[getemeralds].status = IT_DISABLED;
	}

	if ((grade & 2) ||
	(grade & 4) ||
	(grade & 16))
		SecretsMenu[reward].status = IT_STRING | IT_CALL;
	else
		SecretsMenu[reward].status = IT_DISABLED;

	if (grade & 1)
		SecretsMenu[soundtest].status = IT_STRING | IT_CVAR;
	else
		SecretsMenu[soundtest].status = IT_DISABLED;

//	if (grade & 256)
//		Insert reward for beating Ultimate here!

	M_SetupNextMenu(&SecretsDef);
}

static void M_CustomSecretsMenu(INT32 choice)
{
	INT32 i;
	boolean unlocked;

	// Disable all the menu choices
	(void)choice;
	for (i = custom1;i < customsecrets_end;i++)
		CustomSecretsMenu[i].status = IT_DISABLED;

	for (i = 0; i < 15; i++)
	{
		unlocked = false;

		if (customsecretinfo[i].neededemblems)
		{
			unlocked = M_GotEnoughEmblems(customsecretinfo[i].neededemblems);

			if (unlocked && customsecretinfo[i].neededtime)
				unlocked = M_GotLowEnoughTime(customsecretinfo[i].neededtime);

			if (unlocked && customsecretinfo[i].neededgrade)
				unlocked = (grade & customsecretinfo[i].neededgrade);
		}
		else if (customsecretinfo[i].neededtime)
		{
			unlocked = M_GotLowEnoughTime(customsecretinfo[i].neededtime);

			if (unlocked && customsecretinfo[i].neededgrade)
				unlocked = (grade & customsecretinfo[i].neededgrade);
		}
		else
			unlocked = (grade & customsecretinfo[i].neededgrade);

		if (unlocked)
		{
			CustomSecretsMenu[custom1+i].status = IT_STRING|IT_CALL;

			switch (customsecretinfo[i].type)
			{
				case 0:
					CustomSecretsMenu[custom1+i].itemaction = M_CustomLevelSelect;
					break;
				case 1:
					CustomSecretsMenu[custom1+i].itemaction = M_CustomWarp;
				default:
					break;
			}

			CustomSecretsMenu[custom1+i].text = customsecretinfo[i].name;
		}
	}

	M_SetupNextMenu(&CustomSecretsDef);
}

//
//  A smaller 'Thermo', with range given as percents (0-100)
//
static void M_DrawSlider(INT32 x, INT32 y, const consvar_t *cv)
{
	INT32 i;
	INT32 range;
	patch_t *p;

	for (i = 0; cv->PossibleValue[i+1].strvalue; i++);

	range = ((cv->value - cv->PossibleValue[0].value) * 100 /
	 (cv->PossibleValue[i].value - cv->PossibleValue[0].value));

	if (range < 0)
		range = 0;
	if (range > 100)
		range = 100;

	x = BASEVIDWIDTH - x - SLIDER_WIDTH;

	V_DrawScaledPatch(x - 8, y, 0, W_CachePatchName("M_SLIDEL", PU_CACHE));

	p =  W_CachePatchName("M_SLIDEM", PU_CACHE);
	for (i = 0; i < SLIDER_RANGE; i++)
		V_DrawScaledPatch (x+i*8, y, 0,p);

	p = W_CachePatchName("M_SLIDER", PU_CACHE);
	V_DrawScaledPatch(x+SLIDER_RANGE*8, y, 0, p);

	// draw the slider cursor
	p = W_CachePatchName("M_SLIDEC", PU_CACHE);
	V_DrawMappedPatch(x + ((SLIDER_RANGE-1)*8*range)/100, y, 0, p, yellowmap);
}

//===========================================================================
//                        Video OPTIONS MENU
//===========================================================================

//added : 10-02-98: note: alphaKey member is the y offset
static menuitem_t VideoOptionsMenu[] =
{
	// Tails
	{IT_STRING | IT_SUBMENU, NULL, "Video Modes...",      &VidModeDef,        0},
#if defined (__unix__) || defined (UNIXCOMMON) || defined (SDL) && !defined (__EMSCRIPTEN__)
	{IT_STRING|IT_CVAR,      NULL, "Fullscreen",          &cv_fullscreen,    10},
#endif
#if defined (HWRENDER) && defined (SHUFFLE)
	//17/10/99: added by Hurdler
	{IT_CALL|IT_WHITESTRING, NULL, "3D Card Options...",  M_OpenGLOption,    20},
#endif
	{IT_STRING | IT_CVAR,
	                         NULL, "Brightness",          &cv_usegamma,      40},
	{IT_STRING | IT_CVAR,
	                         NULL, "Saturation",          &cv_usesaturation,      50},

	{IT_STRING | IT_CVAR,    NULL, "V-SYNC",              &cv_vidwait,       60},

	{IT_STRING | IT_CVAR,    NULL, "Rain/Snow Density",   &cv_precipdensity, 70}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING | IT_CVAR,    NULL, "Rain/Snow Draw Dist", &cv_precipdist,    80}, // Changed all to normal string Tails 11-30-2000
	{IT_STRING | IT_CVAR,    NULL, "FPS Meter",           &cv_ticrate,       90},
};

menu_t VideoOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (VideoOptionsMenu)/sizeof (menuitem_t),
	&OptionsDef,
	VideoOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

// retro options

static menuitem_t RetroMenu[] =
{
	{IT_CVAR | IT_STRING, NULL, "GIF Optimization", &cv_gif_optimize, 20},
	{IT_CVAR | IT_STRING, NULL, "GIF Downscaling", &cv_gif_downscale, 30},
	{IT_STRING|IT_CVAR, NULL, "Maximum Filesize", &cv_gif_maxsize,             40},
	{IT_STRING|IT_CVAR, NULL, "Rolling GIFs", &cv_gif_rolling,             50},
};

menu_t RetroDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (RetroMenu)/sizeof (menuitem_t),
	&OptionsDef,
	RetroMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                        Mouse OPTIONS MENU
//===========================================================================

//added : 24-03-00: note: alphaKey member is the y offset
static menuitem_t MouseOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Use Mouse",        &cv_usemouse,         0},
	{IT_STRING | IT_CVAR, NULL, "Always MouseLook", &cv_alwaysfreelook,   0},
	{IT_STRING | IT_CVAR, NULL, "Mouse Move",       &cv_mousemove,        0},
	{IT_STRING | IT_CVAR, NULL, "Invert Mouse",     &cv_invertmouse,      0},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mouse Speed",      &cv_mousesens,        0},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
	                      NULL, "Mlook Speed",      &cv_mlooksens,        0},
};

menu_t MouseOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (MouseOptionsMenu)/sizeof (menuitem_t),
	&OptionsDef,
	MouseOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                        Game OPTIONS MENU
//===========================================================================

static menuitem_t GameOptionsMenu[] =
{
	// Tails
	{IT_STRING | IT_CVAR, NULL, "Show HUD",    &cv_showhud,       20},
#ifdef SEENAMES
	{IT_STRING | IT_CVAR, NULL, "HUD Player Names",    &cv_seenames,       30},
#endif
	{IT_STRING | IT_CVAR, NULL, "High Resolution Timer",    &cv_timetic,       40},

	{IT_STRING | IT_CVAR, NULL, "Console Color", &cons_backcolor, 60},
	{IT_STRING | IT_CVAR, NULL, "Uppercase Console", &cv_allcaps, 70},

	{IT_STRING | IT_SUBMENU, NULL, "Data Options...", &DataOptionsDef, 90},
};

menu_t GameOptionDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (GameOptionsMenu)/sizeof (menuitem_t),
	&OptionsDef,
	GameOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

static void M_GameOption(INT32 choice)
{
	(void)choice;
	M_SetupNextMenu(&GameOptionDef);
}


//===========================================================================
//                        GAMETYPE OPTIONS MENU
//===========================================================================

static menuitem_t GametypeOptionsMenu[] =
{
	{IT_STRING | IT_SUBMENU, NULL, "Coop options...",          &CoopOptionsDef,     20},
	{IT_STRING | IT_SUBMENU, NULL, "Race options...",          &RaceOptionsDef,     30},
	{IT_STRING | IT_SUBMENU, NULL, "Match options...",          &MatchOptionsDef,   40},
	{IT_STRING | IT_SUBMENU, NULL, "Tag options...",          &TagOptionsDef,       50},
	{IT_STRING | IT_SUBMENU, NULL, "CTF options...",          &CTFOptionsDef,       60},
};

menu_t GametypeOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (GametypeOptionsMenu)/sizeof (menuitem_t),
	&OptionsDef,
	GametypeOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                        Coop Mode OPTIONS MENU
//===========================================================================

static menuitem_t CoopOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Players for exit",      &cv_playersforexit,  10},
};

menu_t CoopOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (CoopOptionsMenu)/sizeof (menuitem_t),
	&GametypeOptionsDef,
	CoopOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                        Race Mode OPTIONS MENU
//===========================================================================

static menuitem_t RaceOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Item Boxes",      &cv_raceitemboxes, 10},
	{IT_STRING | IT_CVAR, NULL, "Number of Laps",  &cv_numlaps,       20},
	{IT_STRING | IT_CVAR, NULL, "Countdown Time",  &cv_countdowntime, 30},
};

menu_t RaceOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (RaceOptionsMenu)/sizeof (menuitem_t),
	&GametypeOptionsDef,
	RaceOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                        Match Mode OPTIONS MENU
//===========================================================================

static menuitem_t MatchOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Scoring Type",          &cv_match_scoring,   10},
	{IT_STRING | IT_CVAR, NULL, "Team Match Type",       &cv_matchtype,       20},
	{IT_STRING | IT_CVAR, NULL, "Overtime Tie-Breaker",  &cv_overtime,        30},
};

menu_t MatchOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (MatchOptionsMenu)/sizeof (menuitem_t),
	&GametypeOptionsDef,
	MatchOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                        Tag Mode OPTIONS MENU
//===========================================================================

static menuitem_t TagOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Hide Time",     &cv_hidetime,        10},
};

menu_t TagOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (TagOptionsMenu)/sizeof (menuitem_t),
	&GametypeOptionsDef,
	TagOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                        CTF Mode OPTIONS MENU
//===========================================================================

static menuitem_t CTFOptionsMenu[] =
{
	{IT_STRING | IT_CVAR, NULL, "Flag Respawn Time",           &cv_flagtime,         10},
	{IT_STRING | IT_CVAR, NULL, "Autobalance",                 &cv_autobalance,      20},
	{IT_STRING | IT_CVAR, NULL, "Team Scrambler",              &cv_scrambleonchange, 30},
	{IT_STRING | IT_CVAR, NULL, "Overtime Tie-Breaker",        &cv_overtime,         40},
};

menu_t CTFOptionsDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (CTFOptionsMenu)/sizeof (menuitem_t),
	&GametypeOptionsDef,
	CTFOptionsMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                        Monitor Toggle MENU
//===========================================================================


static void M_GametypeOptions(INT32 choice)
{
	(void)choice;
	if (!(server || (adminplayer == consoleplayer)))
	{
		M_StartMessage("You are not the server\nYou can't change the options\n", NULL, MM_NOTHING);
		return;
	}

	if (!(netgame || multiplayer) || !Playing())
	{
		M_StartMessage("You aren't in a game!\nPress a key.", NULL, MM_NOTHING);
		return;
	}

	M_SetupNextMenu(&GametypeOptionsDef);
}

//===========================================================================
//                          Read This! MENU 1
//===========================================================================

static void M_DrawReadThis1(void);
static void M_DrawReadThis2(void);

typedef enum
{
	rdthsempty1,
	read1_end
} read_e;

static menuitem_t ReadMenu1[] =
{
	{IT_SUBMENU | IT_NOTHING, NULL, "", &MainDef, 0},
};

menu_t ReadDef1 =
{
	NULL,
	NULL,
	read1_end,
	NULL,
	ReadMenu1,
	M_DrawReadThis1,
	330, 165,
	0,
	NULL
};

//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
static void M_DrawReadThis1(void)
{
	V_DrawScaledPatch (0,0,0,W_CachePatchName("HELP",PU_CACHE));
	return;
}

//===========================================================================
//                          *B^D 'Menu'
//===========================================================================

typedef enum
{
	rdthsempty2,
	read2_end
} read_e2;

static menuitem_t ReadMenu2[] =
{
	{IT_SUBMENU | IT_NOTHING, NULL, "", &MainDef, 0},
};

menu_t ReadDef2 =
{
	NULL,
	NULL,
	read2_end,
	NULL,
	ReadMenu2,
	M_DrawReadThis2,
	330, 175,
	0,
	NULL
};


//
// Read This Menus - optional second page.
//
static void M_DrawReadThis2(void)
{
	V_DrawScaledPic (0,0,0,W_GetNumForName ("BULMER"));
	HU_Drawer();
	return;
}

// M_ToggleSFX
// M_ToggleDigital
// M_ToggleMIDI
//
// Toggles sound systems in-game.
//
static void M_ToggleSFX(INT32 choice)
{
	(void)choice;
	if (nosound)
	{
		nosound = false;
		I_StartupSound();
		if (nosound) return;
		S_Init(cv_soundvolume.value, cv_digmusicvolume.value, cv_midimusicvolume.value);
		M_StartMessage("SFX Enabled\n", NULL, MM_NOTHING);
	}
	else
	{
		if (sound_disabled)
		{
			sound_disabled = false;
			M_StartMessage("SFX Enabled\n", NULL, MM_NOTHING);
		}
		else
		{
			sound_disabled = true;
			S_StopSounds();
			M_StartMessage("SFX Disabled\n", NULL, MM_NOTHING);
		}
	}
}

static void M_ToggleDigital(INT32 choice)
{
	(void)choice;
	if (nodigimusic)
	{
		nodigimusic = false;
		I_InitDigMusic();
		if (nodigimusic) return;
		S_Init(cv_soundvolume.value, cv_digmusicvolume.value, cv_midimusicvolume.value);
		S_StopMusic();
		S_ChangeMusic(mus_lclear, false);
		M_StartMessage("Digital Music Enabled\n", NULL, MM_NOTHING);
	}
	else
	{
		if (digital_disabled)
		{
			digital_disabled = false;
			M_StartMessage("Digital Music Enabled\n", NULL, MM_NOTHING);
		}
		else
		{
			digital_disabled = true;
			S_StopMusic();
			M_StartMessage("Digital Music Disabled\n", NULL, MM_NOTHING);
		}
	}
}

static void M_ToggleMIDI(INT32 choice)
{
	(void)choice;
	if (nomidimusic)
	{
		nomidimusic = false;
		I_InitMIDIMusic();
		if (nomidimusic) return;
		S_Init(cv_soundvolume.value, cv_digmusicvolume.value, cv_midimusicvolume.value);
		S_ChangeMusic(mus_lclear, false);
		M_StartMessage("MIDI Music Enabled\n", NULL, MM_NOTHING);
	}
	else
	{
		if (music_disabled)
		{
			music_disabled = false;
			M_StartMessage("MIDI Music Enabled\n", NULL, MM_NOTHING);
		}
		else
		{
			music_disabled = true;
			S_StopMusic();
			M_StartMessage("MIDI Music Disabled\n", NULL, MM_NOTHING);
		}
	}
}

//===========================================================================
//                        SOUND VOLUME MENU
//===========================================================================

typedef enum
{
	sfx_vol,
	sfx_empty1,
	digmusic_vol,
	sfx_empty2,
	midimusic_vol,
	sfx_empty3,
#ifdef PC_DOS
	cdaudio_vol,
	sfx_empty4,
#endif
	tog_sfx,
	tog_dig,
	tog_midi,
	sound_end
} sound_e;

static menuitem_t SoundMenu[] =
{
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
                              NULL, "Sound Volume" , &cv_soundvolume,     0},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
                              NULL, "Music Volume" , &cv_digmusicvolume,  10},
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
                              NULL, "MIDI Volume"  , &cv_midimusicvolume, 20},
#ifdef PC_DOS
	{IT_STRING | IT_CVAR | IT_CV_SLIDER,
                              NULL, "CD Volume"    , &cd_volume,          30},
#endif
	{IT_STRING    | IT_CALL,  NULL,  "Toggle SFX"   , M_ToggleSFX,         40},
	{IT_STRING    | IT_CALL,  NULL,  "Toggle Digital Music", M_ToggleDigital,     50},
	{IT_STRING    | IT_CALL,  NULL,  "Toggle MIDI Music", M_ToggleMIDI,        60},
};

menu_t SoundDef =
{
	"M_SVOL",
	"Sound Volume",
	sizeof (SoundMenu)/sizeof (menuitem_t),
	&OptionsDef,
	SoundMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};

//===========================================================================
//                          JOYSTICK MENU
//===========================================================================
static void M_Setup1PJoystickMenu(INT32 choice);
static void M_Setup2PJoystickMenu(INT32 choice);

typedef enum
{
	p1joy,
	p1set,
	p1turn,
	p1move,
	p1side,
	p1look,
	p1fire,
	p1nfire,
	p2joy,
	p2set,
	p2turn,
	p2move,
	p2side,
	p2look,
	p2fire,
	p2nfire,
	joystick_end
} joy_e;


static menuitem_t JoystickMenu[] =
{
	{IT_WHITESTRING | IT_SPACE, NULL, "Player 1 Joystick" , NULL                 ,  10},
	{IT_STRING      | IT_CALL,  NULL, "Select Joystick...", M_Setup1PJoystickMenu,  20},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Turning"  , &cv_turnaxis         ,  30},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Moving"   , &cv_moveaxis         ,  40},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Strafe"   , &cv_sideaxis         ,  50},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Looking"  , &cv_lookaxis         ,  60},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Firing"   , &cv_fireaxis         ,  70},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For NFiring"  , &cv_firenaxis        ,  80},
	{IT_WHITESTRING | IT_SPACE, NULL, "Player 2 Joystick" , NULL                 ,  90},
	{IT_STRING      | IT_CALL,  NULL, "Select Joystick...", M_Setup2PJoystickMenu, 100},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Turning"  , &cv_turnaxis2        , 110},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Moving"   , &cv_moveaxis2        , 120},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Strafe"   , &cv_sideaxis2        , 130},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Looking"  , &cv_lookaxis2        , 140},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For Firing"   , &cv_fireaxis2        , 150},
	{IT_STRING      | IT_CVAR,  NULL, "Axis For NFiring"  , &cv_firenaxis2       , 160},

};

menu_t JoystickDef =
{
	"M_CONTRO",
	"Setup Joystick",
	joystick_end,
	&ControlsDef,
	JoystickMenu,
	M_DrawGenericMenu,
	50, 20,
	1,
	NULL
};

static void M_DrawJoystick(void);
static void M_AssignJoystick(INT32 choice);

typedef enum
{
	joy0 = 0,
	joy1,
	joy2,
	joy3,
	joy4,
	joy5,
	joy6,
	joystickset_end
} joyset_e;

static menuitem_t JoystickSetMenu[] =
{
	{IT_CALL | IT_NOTHING, "None", NULL, M_AssignJoystick, '0'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '1'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '2'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '3'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '4'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '5'},
	{IT_CALL | IT_NOTHING, "", NULL, M_AssignJoystick, '6'},
};

static menu_t JoystickSetDef =
{
	"M_CONTRO",
	"Select Joystick",
	sizeof (JoystickSetMenu)/sizeof (menuitem_t),
	&JoystickDef,
	JoystickSetMenu,
	M_DrawJoystick,
	50, 40,
	0,
	NULL
};

//===========================================================================
//                          CONTROLS MENU
//===========================================================================
static void M_DrawControl(void);               // added 3-1-98
static void M_ChangeControl(INT32 choice);
static void M_ControlDef2(INT32 choice);

//
// this is the same for all control pages
//
static menuitem_t ControlMenu[] =
{
	{IT_CALL | IT_STRING2, NULL, "Forward",      M_ChangeControl, gc_forward    },
	{IT_CALL | IT_STRING2, NULL, "Reverse",      M_ChangeControl, gc_backward   },
	{IT_CALL | IT_STRING2, NULL, "Turn Left",    M_ChangeControl, gc_turnleft   },
	{IT_CALL | IT_STRING2, NULL, "Turn Right",   M_ChangeControl, gc_turnright  },
	{IT_CALL | IT_STRING2, NULL, "Jump",         M_ChangeControl, gc_jump       },
	{IT_CALL | IT_STRING2, NULL, "Spin",         M_ChangeControl, gc_use        }, // Tails 12-04-99
	{IT_CALL | IT_STRING2, NULL, "Ring Toss",    M_ChangeControl, gc_fire       },
	{IT_CALL | IT_STRING2, NULL, "Ring Toss Normal",
	                                             M_ChangeControl, gc_firenormal },
	{IT_CALL | IT_STRING2, NULL, "Taunt",        M_ChangeControl, gc_taunt      },
	{IT_CALL | IT_STRING2, NULL, "Toss Flag",    M_ChangeControl, gc_tossflag   },
	{IT_CALL | IT_STRING2, NULL, "Strafe On",    M_ChangeControl, gc_strafe     },
	{IT_CALL | IT_STRING2, NULL, "Strafe Left",  M_ChangeControl, gc_strafeleft },
	{IT_CALL | IT_STRING2, NULL, "Strafe Right", M_ChangeControl, gc_straferight},
	{IT_CALL | IT_STRING2, NULL, "Look Up",      M_ChangeControl, gc_lookup     },
	{IT_CALL | IT_STRING2, NULL, "Look Down",    M_ChangeControl, gc_lookdown   },
	{IT_CALL | IT_STRING2, NULL, "Center View",  M_ChangeControl, gc_centerview },
	{IT_CALL | IT_STRING2, NULL, "Mouselook",    M_ChangeControl, gc_mouseaiming},

	{IT_CALL | IT_WHITESTRING,
	                       NULL, "next",         M_ControlDef2,               144},
};

menu_t ControlDef =
{
	"M_CONTRO",
	"Setup Controls",
	sizeof (ControlMenu)/sizeof (menuitem_t),
	&ControlsDef,
	ControlMenu,
	M_DrawControl,
	24, 40,
	0,
	NULL
};

//
//  Controls page 2
//
// WARNING!: IF YOU MODIFY THIS CHECK "UGLY HACK"
// COMMENTS BELOW TO MAINTAIN CONSISTENCY!!!
//
static menuitem_t ControlMenu2[] =
{
	{IT_CALL | IT_STRING2, NULL, "Talk key",         M_ChangeControl, gc_talkkey      },
	{IT_CALL | IT_STRING2, NULL, "Team-Talk key",    M_ChangeControl, gc_teamkey      },
	{IT_CALL | IT_STRING2, NULL, "Rankings/Scores",  M_ChangeControl, gc_scores       },
	{IT_CALL | IT_STRING2, NULL, "Console",          M_ChangeControl, gc_console      },
	{IT_CALL | IT_STRING2, NULL, "Next Weapon",      M_ChangeControl, gc_weaponnext   },
	{IT_CALL | IT_STRING2, NULL, "Prev Weapon",      M_ChangeControl, gc_weaponprev   },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 1",    M_ChangeControl, gc_normalring   },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 2",    M_ChangeControl, gc_autoring     },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 3",    M_ChangeControl, gc_bouncering   },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 4",    M_ChangeControl, gc_scatterring  },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 5",    M_ChangeControl, gc_grenadering  },
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 6",    M_ChangeControl, gc_explosionring},
	{IT_CALL | IT_STRING2, NULL, "Weapon Slot 7",    M_ChangeControl, gc_railring     },
	{IT_CALL | IT_STRING2, NULL, "Rotate Camera L",  M_ChangeControl, gc_camleft      },
	{IT_CALL | IT_STRING2, NULL, "Rotate Camera R",  M_ChangeControl, gc_camright     },
	{IT_CALL | IT_STRING2, NULL, "Reset Camera",     M_ChangeControl, gc_camreset     },
	{IT_CALL | IT_STRING2, NULL, "Pause",            M_ChangeControl, gc_pause        },

	{IT_SUBMENU | IT_WHITESTRING,
	                       NULL, "next",             &ControlDef,     140             },
};

menu_t ControlDef2 =
{
	"M_CONTRO",
	"Setup Controls",
	sizeof (ControlMenu2)/sizeof (menuitem_t),
	&ControlsDef,
	ControlMenu2,
	M_DrawControl,
	24, 40,
	0,
	NULL
};


//
// Start the controls menu, setting it up for either the primary
// or secondary joystick
//
static  boolean setupcontrols_secondaryplayer;
static  INT32   (*setupcontrols)[2];  // pointer to the gamecontrols of the player being edited

static void M_ControlDef2(INT32 choice)
{
	(void)choice;
	M_SetupNextMenu(&ControlDef2);
}

static void M_DrawJoystick(void)
{
	INT32 i;

	M_DrawGenericMenu();

	for (i = joy0;i < joystickset_end; i++)
	{
		M_DrawSaveLoadBorder(JoystickSetDef.x,JoystickSetDef.y+LINEHEIGHT*i);

		if ((setupcontrols_secondaryplayer && (i == cv_usejoystick2.value))
			|| (!setupcontrols_secondaryplayer && (i == cv_usejoystick.value)))
			V_DrawString(JoystickSetDef.x,JoystickSetDef.y+LINEHEIGHT*i,V_YELLOWMAP,joystickInfo[i]);
		else
			V_DrawString(JoystickSetDef.x,JoystickSetDef.y+LINEHEIGHT*i,0,joystickInfo[i]);
	}
}

static void M_SetupJoystickMenu(INT32 choice)
{
	INT32 i = 0;
	const char *joyname = "None";
	const char *joyNA = "Unavailable";
	INT32 n = I_NumJoys();
	(void)choice;

	strcpy(joystickInfo[i], joyname);

	for (i = joy1; i < joystickset_end; i++)
	{
		if (i <= n && (joyname = I_GetJoyName(i)) != NULL)
		{
			strncpy(joystickInfo[i], joyname, 24);
			joystickInfo[i][24] = '\0';
		}
		else
			strcpy(joystickInfo[i], joyNA);
	}

	M_SetupNextMenu(&JoystickSetDef);
}

static void M_Setup1PJoystickMenu(INT32 choice)
{
	setupcontrols_secondaryplayer = false;
	M_SetupJoystickMenu(choice);
}

static void M_Setup2PJoystickMenu(INT32 choice)
{
	setupcontrols_secondaryplayer = true;
	M_SetupJoystickMenu(choice);
}

static void M_AssignJoystick(INT32 choice)
{
	if (setupcontrols_secondaryplayer)
		CV_SetValue(&cv_usejoystick2, choice);
	else
		CV_SetValue(&cv_usejoystick, choice);
}

static void M_Setup1PControlsMenu(INT32 choice)
{
	(void)choice;
	setupcontrols_secondaryplayer = false;
	setupcontrols = gamecontrol;        // was called from main Options (for console player, then)
	currentMenu->lastOn = itemOn;
	M_SetupNextMenu(&ControlDef);
}

static void M_DrawControlsGenerics(void)
{
	INT32 x, y, i, cursory = 0;

	// DRAW MENU
	x = currentMenu->x;
	y = currentMenu->y;

	// draw title (or big pic)
	M_DrawMenuTitle();

	// UGLY HACK!
	if (setupcontrols_secondaryplayer
		&& currentMenu == &ControlDef2)
	{
		for (i = 0; i < 0; i++) //vertical adjustable lines
		{
			if (currentMenu->menuitems[i].alphaKey)
				y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
		}
		if (itemOn < 0) //will stop and not display the item above.
			itemOn = 0;
	}

	for (i = 0; i < currentMenu->numitems; i++)
	{
		// UGLY HACK!
		if (setupcontrols_secondaryplayer
			&& currentMenu == &ControlDef2
			&& i < 0) //vertical adjusted lines.
			continue;

		if (i == itemOn)
			cursory = y;
		switch (currentMenu->menuitems[i].status & IT_DISPLAY)
		{
			case IT_PATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
				{
					V_DrawScaledPatch(x, y, 0,
						W_CachePatchName(currentMenu->menuitems[i].patch, PU_CACHE));
				}
			case IT_NOTHING:
			case IT_DYBIGSPACE:
				y += LINEHEIGHT;
				break;
			case IT_BIGSLIDER:
				M_DrawThermo(x, y, (consvar_t *)currentMenu->menuitems[i].itemaction);
				y += LINEHEIGHT;
				break;
			case IT_STRING:
			case IT_WHITESTRING:
				if (currentMenu->menuitems[i].alphaKey)
					y = currentMenu->y+currentMenu->menuitems[i].alphaKey;
				if (i == itemOn)
					cursory = y;

				if ((currentMenu->menuitems[i].status & IT_DISPLAY)==IT_STRING)
					V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
				else
					V_DrawString(x, y, V_YELLOWMAP, currentMenu->menuitems[i].text);

				// Cvar specific handling
				switch (currentMenu->menuitems[i].status & IT_TYPE)
					case IT_CVAR:
					{
						consvar_t *cv = (consvar_t *)currentMenu->menuitems[i].itemaction;
						switch (currentMenu->menuitems[i].status & IT_CVARTYPE)
						{
							case IT_CV_SLIDER:
								M_DrawSlider(x, y, cv);
							case IT_CV_NOPRINT: // color use this
								break;
							case IT_CV_STRING:
								M_DrawTextBox(x, y + 4, MAXSTRINGLENGTH, 1);
								V_DrawString(x + 8, y + 12, V_ALLOWLOWERCASE, cv->string);
								if (skullAnimCounter < 4 && i == itemOn)
									V_DrawCharacter(x + 8 + V_StringWidth(cv->string), y + 12,
										'_' | 0x80,false);
								y += 16;
								break;
							default:
								V_DrawString(BASEVIDWIDTH - x - V_StringWidth(cv->string), y,
									V_YELLOWMAP, cv->string);
								break;
						}
						break;
					}
					y += STRINGHEIGHT;
					break;
			case IT_STRING2:
				V_DrawString(x, y, 0, currentMenu->menuitems[i].text);
			case IT_DYLITLSPACE:
				y += SMALLLINEHEIGHT;
				break;
			case IT_GRAYPATCH:
				if (currentMenu->menuitems[i].patch && currentMenu->menuitems[i].patch[0])
					V_DrawMappedPatch(x, y, 0,
						W_CachePatchName(currentMenu->menuitems[i].patch,PU_CACHE), graymap);
				y += LINEHEIGHT;
				break;
		}
	}

	// DRAW THE SKULL CURSOR
	if (((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_PATCH)
		|| ((currentMenu->menuitems[itemOn].status & IT_DISPLAY) == IT_NOTHING))
	{
		V_DrawScaledPatch(currentMenu->x + SKULLXOFF, cursory - 5, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
	}
	else
	{
		V_DrawScaledPatch(currentMenu->x - 22, cursory, 0,
			W_CachePatchName("M_CURSOR", PU_CACHE));
		V_DrawString(currentMenu->x, cursory, V_YELLOWMAP, currentMenu->menuitems[itemOn].text);
	}
}
//
//  Draws the Customise Controls menu
//
static void M_DrawControl(void)
{
	char     tmp[50];
	INT32    i;
	INT32    keys[2];

	// draw title, strings and submenu
	M_DrawControlsGenerics();

	M_CentreText (ControlDef.y-12,
		 (setupcontrols_secondaryplayer ? "SET CONTROLS FOR SECONDARY PLAYER" :
		                                  "PRESS ENTER TO CHANGE, BACKSPACE TO CLEAR"));

	for (i = 0;i < currentMenu->numitems;i++)
	{
		if (currentMenu->menuitems[i].status != IT_CONTROL)
			continue;

		if (setupcontrols_secondaryplayer
			&& currentMenu == &ControlDef2
			&& i < 3)
			continue;

		keys[0] = setupcontrols[currentMenu->menuitems[i].alphaKey][0];
		keys[1] = setupcontrols[currentMenu->menuitems[i].alphaKey][1];

		tmp[0] ='\0';
		if (keys[0] == KEY_NULL && keys[1] == KEY_NULL)
		{
			strcpy(tmp, "---");
		}
		else
		{
			if (keys[0] != KEY_NULL)
				strcat (tmp, G_KeynumToString (keys[0]));

			if (keys[0] != KEY_NULL && keys[1] != KEY_NULL)
				strcat(tmp," or ");

			if (keys[1] != KEY_NULL)
				strcat (tmp, G_KeynumToString (keys[1]));


		}
		V_DrawString(BASEVIDWIDTH-ControlDef.x-V_StringWidth(tmp), ControlDef.y + i*8,V_YELLOWMAP, tmp);
	}

}

static INT32 controltochange;

static void M_ChangecontrolResponse(event_t *ev)
{
	INT32        control;
	INT32        found;
	INT32        ch = ev->data1;

	// ESCAPE cancels
	if (ch != KEY_ESCAPE)
	{

		switch (ev->type)
		{
			// ignore mouse/joy movements, just get buttons
			case ev_mouse:
			case ev_mouse2:
			case ev_joystick:
			case ev_joystick2:
				ch = KEY_NULL;      // no key
			break;

			// keypad arrows are converted for the menu in cursor arrows
			// so use the event instead of ch
			case ev_keydown:
				ch = ev->data1;
			break;

			default:
			break;
		}

		control = controltochange;

		// check if we already entered this key
		found = -1;
		if (setupcontrols[control][0] ==ch)
			found = 0;
		else if (setupcontrols[control][1] ==ch)
			found = 1;
		if (found >= 0)
		{
			// replace mouse and joy clicks by double clicks
			if (ch >= KEY_MOUSE1 && ch <= KEY_MOUSE1+MOUSEBUTTONS)
				setupcontrols[control][found] = ch-KEY_MOUSE1+KEY_DBLMOUSE1;
			else if (ch >= KEY_JOY1 && ch <= KEY_JOY1+JOYBUTTONS)
				setupcontrols[control][found] = ch-KEY_JOY1+KEY_DBLJOY1;
			else if (ch >= KEY_2MOUSE1 && ch <= KEY_2MOUSE1+MOUSEBUTTONS)
				setupcontrols[control][found] = ch-KEY_2MOUSE1+KEY_DBL2MOUSE1;
			else if (ch >= KEY_2JOY1 && ch <= KEY_2JOY1+JOYBUTTONS)
				setupcontrols[control][found] = ch-KEY_2JOY1+KEY_DBL2JOY1;
		}
		else
		{
			// check if change key1 or key2, or replace the two by the new
			found = 0;
			if (setupcontrols[control][0] == KEY_NULL)
				found++;
			if (setupcontrols[control][1] == KEY_NULL)
				found++;
			if (found == 2)
			{
				found = 0;
				setupcontrols[control][1] = KEY_NULL;  //replace key 1,clear key2
			}
			G_CheckDoubleUsage(ch);
			setupcontrols[control][found] = ch;
		}

	}

	M_StopMessage(0);
}

static void M_ChangeControl(INT32 choice)
{
	static char tmp[55];

	controltochange = currentMenu->menuitems[choice].alphaKey;
	sprintf(tmp, "Hit the new key for\n%s\nESC for Cancel",
		currentMenu->menuitems[choice].text);

	M_StartMessage(tmp, M_ChangecontrolResponse, MM_EVENTHANDLER);
}

//===========================================================================
//                        VIDEO MODE MENU
//===========================================================================
static void M_DrawVideoMode(void);             //added : 30-01-98:

static void M_HandleVideoMode(INT32 ch);

static menuitem_t VideoModeMenu[] =
{
	{IT_KEYHANDLER | IT_NOTHING, NULL, "", M_HandleVideoMode, '\0'},     // dummy menuitem for the control func
};

menu_t VidModeDef =
{
	"M_VIDEO",
	"Video Mode",
	1,                  // # of menu items
	//sizeof (VideoModeMenu)/sizeof (menuitem_t),
	&VideoOptionsDef,   // previous menu
	VideoModeMenu,      // menuitem_t ->
	M_DrawVideoMode,    // drawing routine ->
	48, 36,             // x,y
	0,                  // lastOn
	NULL
};

//added : 30-01-98:
#define MAXCOLUMNMODES   10     //max modes displayed in one column
#define MAXMODEDESCS     (MAXCOLUMNMODES*3)

// shhh... what am I doing... nooooo!
static INT32 vidm_testingmode = 0;
static INT32 vidm_previousmode;
static INT32 vidm_current = 0;
static INT32 vidm_nummodes;
static INT32 vidm_column_size;

typedef struct
{
	INT32 modenum; // video mode number in the vidmodes list
	const char *desc;  // XXXxYYY
	INT32 iscur;   // 1 if it is the current active mode
} modedesc_t;

static modedesc_t modedescs[MAXMODEDESCS];

//
// Draw the video modes list, a-la-Quake
//
static void M_DrawVideoMode(void)
{
	INT32 i, j, vdup, row, col, nummodes;
	const char *desc;
	char temp[80];
	INT32 width, height;

	// draw title
	M_DrawMenuTitle();

#if defined (__unix__) || defined (UNIXCOMMON) || defined (SDL)
	VID_PrepareModeList(); // FIXME: hack
#endif
	vidm_nummodes = 0;
	nummodes = VID_NumModes();

#ifdef _WINDOWS
	// clean that later: skip windowed mode 0, video modes menu only shows FULL SCREEN modes
	if (nummodes < 1)
	{
		// put the windowed mode so that there is at least one mode
		modedescs[0].modenum = 0;
		modedescs[0].desc = VID_GetModeName(0);
		modedescs[0].iscur = 1;
		vidm_nummodes = 1;
	}
	for (i = 1; i <= nummodes && vidm_nummodes < MAXMODEDESCS; i++)
#else
	// DOS does not skip mode 0, because mode 0 is ALWAYS present
	for (i = 0; i < nummodes && vidm_nummodes < MAXMODEDESCS; i++)
#endif
	{
		desc = VID_GetModeName(i);
		if (desc)
		{
			vdup = 0;

			// when a resolution exists both under VGA and VESA, keep the
			// VESA mode, which is always a higher modenum
			for (j = 0; j < vidm_nummodes; j++)
			{
				if (!strcmp(modedescs[j].desc, desc))
				{
					// mode(0): 320x200 is always standard VGA, not vesa
					if (modedescs[j].modenum)
					{
						modedescs[j].modenum = i;
						vdup = 1;

						if (i == vid.modenum)
							modedescs[j].iscur = 1;
					}
					else
						vdup = 1;

					break;
				}
			}

			if (!vdup)
			{
				modedescs[vidm_nummodes].modenum = i;
				modedescs[vidm_nummodes].desc = desc;
				modedescs[vidm_nummodes].iscur = 0;

				if (i == vid.modenum)
					modedescs[vidm_nummodes].iscur = 1;

				vidm_nummodes++;
			}
		}
	}

	vidm_column_size = (vidm_nummodes+2) / 3;

	row = 41;
	col = VidModeDef.y;
	for (i = 0; i < vidm_nummodes; i++)
	{
		// Pull out the width and height
		sscanf(modedescs[i].desc, "%u%*c%u", &width, &height);

		// Show multiples of 320x200 as green.
		if ((width % BASEVIDWIDTH == 0 && height % BASEVIDHEIGHT == 0) &&
			(width / BASEVIDWIDTH == height / BASEVIDHEIGHT))
			V_DrawString(row, col, modedescs[i].iscur ? V_YELLOWMAP : V_GREENMAP, modedescs[i].desc);
		else
			V_DrawString(row, col, modedescs[i].iscur ? V_YELLOWMAP : 0, modedescs[i].desc);

		col += 8;
		if ((i % vidm_column_size) == (vidm_column_size-1))
		{
			row += 7*13;
			col = 36;
		}
	}

	V_DrawCenteredString(BASEVIDWIDTH/2, 168, V_GREENMAP, "Green modes are recommended.");
	V_DrawCenteredString(BASEVIDWIDTH/2, 176, V_GREENMAP, "Non-green modes are known to cause");
	V_DrawCenteredString(BASEVIDWIDTH/2, 184, V_GREENMAP, "random crashes. Use at own risk.");

	if (vidm_testingmode > 0)
	{
		sprintf(temp, "TESTING MODE %s", modedescs[vidm_current].desc);
		M_CentreText(VidModeDef.y + 80 + 16, temp);
		M_CentreText(VidModeDef.y + 90 + 16, "Please wait 5 seconds...");
	}
	else
	{
		M_CentreText(VidModeDef.y + 60 + 16, "Press ENTER to set mode");
		M_CentreText(VidModeDef.y + 70 + 16, "T to test mode for 5 seconds");

		sprintf(temp, "D to make %s the default", VID_GetModeName(vid.modenum));
		M_CentreText(VidModeDef.y + 80 + 16,temp);

		sprintf(temp, "Current default is %dx%d (%d bits)", cv_scr_width.value,
			cv_scr_height.value, cv_scr_depth.value);
		M_CentreText(VidModeDef.y + 90 + 16,temp);

		M_CentreText(VidModeDef.y + 100 + 16,"Press ESC to exit");
	}

	// Draw the cursor for the VidMode menu
	if (skullAnimCounter < 4) // use the Skull anim counter to blink the cursor
	{
		i = 41 - 10 + ((vidm_current / vidm_column_size)*7*13);
		j = VidModeDef.y + ((vidm_current % vidm_column_size)*8);
		V_DrawCharacter(i - 8, j, '*',false);
	}
}

// special menuitem key handler for video mode list
static void M_HandleVideoMode(INT32 ch)
{
	if (vidm_testingmode > 0)
	{
		// change back to the previous mode quickly
		if (ch == KEY_ESCAPE)
		{
			setmodeneeded = vidm_previousmode + 1;
			vidm_testingmode = 0;
		}
		return;
	}

	switch (ch)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL, sfx_menu1);
			vidm_current++;
			if (vidm_current >= vidm_nummodes)
				vidm_current = 0;
			break;

		case KEY_UPARROW:
			S_StartSound(NULL, sfx_menu1);
			vidm_current--;
			if (vidm_current < 0)
				vidm_current = vidm_nummodes - 1;
			break;

		case KEY_LEFTARROW:
			S_StartSound(NULL, sfx_menu1);
			vidm_current -= vidm_column_size;
			if (vidm_current < 0)
				vidm_current = (vidm_column_size*3) + vidm_current;
			if (vidm_current >= vidm_nummodes)
				vidm_current = vidm_nummodes - 1;
			break;

		case KEY_RIGHTARROW:
			S_StartSound(NULL, sfx_menu1);
			vidm_current += vidm_column_size;
			if (vidm_current >= (vidm_column_size*3))
				vidm_current %= vidm_column_size;
			if (vidm_current >= vidm_nummodes)
				vidm_current = vidm_nummodes - 1;
			break;

		case KEY_ENTER:
			S_StartSound(NULL, sfx_menu1);
			if (!setmodeneeded) // in case the previous setmode was not finished
				setmodeneeded = modedescs[vidm_current].modenum + 1;
			break;

		case KEY_ESCAPE: // this one same as M_Responder
			if (currentMenu->prevMenu)
				M_SetupNextMenu(currentMenu->prevMenu);
			else
				M_ClearMenus(true);
			break;

		case 'T':
		case 't':
			vidm_testingmode = TICRATE*5;
			vidm_previousmode = vid.modenum;
			if (!setmodeneeded) // in case the previous setmode was not finished
				setmodeneeded = modedescs[vidm_current].modenum + 1;
			break;

		case 'D':
		case 'd':
			// current active mode becomes the default mode.
			SCR_SetDefaultMode();
			break;

		default:
			break;
	}
}

//===========================================================================
//LOAD GAME MENU
//===========================================================================
static void M_DrawLoad(void);

static void M_LoadSelect(INT32 choice);
static void M_PlayWithNoSave(INT32 choice);

typedef enum
{
	load1,
	load2,
	load3,
	load4,
	load5,
	nosave,
	load_end
} load_e;

static menuitem_t LoadGameMenu[] =
{
	{IT_CALL | IT_NOTHING, "", NULL, M_LoadSelect, '1'},
	{IT_CALL | IT_NOTHING, "", NULL, M_LoadSelect, '2'},
	{IT_CALL | IT_NOTHING, "", NULL, M_LoadSelect, '3'},
	{IT_CALL | IT_NOTHING, "", NULL, M_LoadSelect, '4'},
	{IT_CALL | IT_NOTHING, "", NULL, M_LoadSelect, '5'},
	{IT_CALL | IT_NOTHING, "", NULL, M_PlayWithNoSave, '6'},
};

menu_t LoadDef =
{
	"M_PICKG",
	"Load Game",
	load_end,
	&SinglePlayerDef,
	LoadGameMenu,
	M_DrawLoad,
	80, 54,
	0,
	NULL
};

static void M_DrawGameStats(void)
{
	INT32 ecks;
	saveSlotSelected = itemOn;

	ecks = LoadDef.x + 24;
	M_DrawTextBox(LoadDef.x-8,144, 23, 4);

	if (savegameinfo[saveSlotSelected].lives == -42) // Empty
	{
		V_DrawString(ecks + 16, 152, 0, "EMPTY");
		return;
	}
	else if (saveSlotSelected == 5) //No save option
	{
		V_DrawString(ecks + 16, 152, 0, "NO SAVE");
		return;
	}

	if (savegameinfo[saveSlotSelected].skincolor == 0)
		V_DrawScaledPatch(LoadDef.x,144+8,0,W_CachePatchName(skins[savegameinfo[saveSlotSelected].skinnum].faceprefix, PU_CACHE));
	else
	{
		const UINT8 *colormap = (const UINT8 *) translationtables[savegameinfo[saveSlotSelected].skinnum] - 256 + (savegameinfo[saveSlotSelected].skincolor<<8);
		V_DrawMappedPatch(LoadDef.x,144+8,0,W_CachePatchName(skins[savegameinfo[saveSlotSelected].skinnum].faceprefix, PU_CACHE), colormap);
	}

	V_DrawString(ecks + 16, 152, 0, savegameinfo[saveSlotSelected].playername);

	if (savegameinfo[saveSlotSelected].gamemap == spstage_end)
		V_DrawString(ecks + 16, 160, 0, "COMPLETED!");
	else
	{
// Don't show the act so people know it saves per-zone.
//	if (savegameinfo[saveSlotSelected].actnum == 0)
		V_DrawString(ecks + 16, 160, 0, va("%s", savegameinfo[saveSlotSelected].levelname));
//	else
//		V_DrawString(ecks + 16, 160, 0, va("%s %d", savegameinfo[saveSlotSelected].levelname, savegameinfo[saveSlotSelected].actnum));
	}

	V_DrawScaledPatch(ecks + 16, 168, 0, W_CachePatchName("CHAOS1", PU_CACHE));
	V_DrawString(ecks + 36, 172, 0, va("x %d", savegameinfo[saveSlotSelected].numemeralds));

	V_DrawScaledPatch(ecks + 64, 169, 0, W_CachePatchName("ONEUP", PU_CACHE));
	V_DrawString(ecks + 84, 172, 0, va("x %d", savegameinfo[saveSlotSelected].lives));

	V_DrawScaledPatch(ecks + 120, 168, 0, W_CachePatchName("CONTINS", PU_CACHE));
	V_DrawString(ecks + 140, 172, 0, va("x %d", savegameinfo[saveSlotSelected].continues));
}

//
// M_LoadGame & Cie.
//
static void M_DrawLoad(void)
{
	INT32 i;

	M_DrawGenericMenu();

	V_DrawCenteredString(BASEVIDWIDTH/2, 40, 0, "Hit backspace to delete a save.");

	for (i = 0; i < load_end - 1; i++) //nosave is the last one.
	{
		M_DrawSaveLoadBorder(LoadDef.x,LoadDef.y+LINEHEIGHT*i);
		V_DrawString(LoadDef.x,LoadDef.y+LINEHEIGHT*i,0,va("Save Slot %d", i+1));
	}

	// Option to play with no save.
	M_DrawSaveLoadBorder(LoadDef.x,LoadDef.y+LINEHEIGHT*i);
	V_DrawString(LoadDef.x,LoadDef.y+LINEHEIGHT*i,0,"Play Without Saving");

	M_DrawGameStats();
}

//
// User wants to load this game
//
static void M_LoadSelect(INT32 choice)
{
	if (Playing())
	{
		M_StartMessage(ALREADYPLAYING,M_ExitGameResponse,MM_YESNO);
		return;
	}
	else if (modifiedgame && !savemoddata)
	{
		M_DrawTextBox(24,64-4,32,3);

		V_DrawCenteredString(160, 64+4, 0, "Note: Game must be reset to record");
		V_DrawCenteredString(160, 64+16, 0, "statistics or unlock secrets.");
	}

	if (!FIL_ReadFileOK(va(savegamename, choice)))
	{
		// This slot is empty, so start a new game here.
		M_NewGame();
	}
	else if (savegameinfo[saveSlotSelected].gamemap == spstage_end) // Completed
	{
		fromloadgame = saveSlotSelected + 1;
		M_LevelSelect(0);
		pandoralevelselect = false; //this is set to true in the above function.
	}
	else
	{
		G_LoadGame((UINT32)choice, 0);
		M_ClearMenus(true);
	}

	cursaveslot = choice;
}

//
// User wants to play without saving
//
static void M_PlayWithNoSave(INT32 choice)
{
	(void)choice;
	if (Playing())
	{
		M_StartMessage(ALREADYPLAYING,M_ExitGameResponse,MM_YESNO);
		return;
	}
	else if (modifiedgame && !savemoddata)
	{
		M_DrawTextBox(24,64-4,32,3);

		V_DrawCenteredString(160, 64+4, 0, "Note: Game must be reset to record");
		V_DrawCenteredString(160, 64+16, 0, "statistics or unlock secrets.");
	}

	// Start a new game here.
	M_NewGame();
	cursaveslot = -1;
}

#define VERSIONSIZE             16
// Reads the save file to list lives, level, player, etc.
// Tails 05-29-2003
static void M_ReadSavegameInfo(UINT32 slot)
{
#define BADSAVE I_Error("Bad savegame in slot %u", slot);
#define CHECKPOS if (save_p >= end_p) BADSAVE
	size_t length;
	char savename[255];
	UINT8 *savebuffer;
	UINT8 *end_p; // buffer end point, don't read past here
	UINT8 *save_p;
	INT32 fake; // Dummy variable
	char temp[sizeof(timeattackfolder)];

	sprintf(savename, savegamename, slot);

	length = FIL_ReadFile(savename, &savebuffer);
	if (length == 0)
	{
		CONS_Printf("%s %s", text[HUSTR_MSGU], savename);
		savegameinfo[slot].lives = -42;
		return;
	}

	end_p = savebuffer + length;

	// skip the description field
	save_p = savebuffer;

	save_p += VERSIONSIZE;

	// dearchive all the modifications
	// P_UnArchiveMisc()

	CHECKPOS
	fake = READINT16(save_p);
	if (fake-1 >= NUMMAPS) BADSAVE
	strcpy(savegameinfo[slot].levelname, mapheaderinfo[fake-1].lvlttl);
	savegameinfo[slot].gamemap = fake;

	savegameinfo[slot].actnum = mapheaderinfo[fake-1].actnum;

	CHECKPOS
	fake = READUINT16(save_p)-357; // emeralds

	savegameinfo[slot].numemeralds = 0;

	if (fake & EMERALD1)
		savegameinfo[slot].numemeralds++;
	if (fake & EMERALD2)
		savegameinfo[slot].numemeralds++;
	if (fake & EMERALD3)
		savegameinfo[slot].numemeralds++;
	if (fake & EMERALD4)
		savegameinfo[slot].numemeralds++;
	if (fake & EMERALD5)
		savegameinfo[slot].numemeralds++;
	if (fake & EMERALD6)
		savegameinfo[slot].numemeralds++;
	if (fake & EMERALD7)
		savegameinfo[slot].numemeralds++;

	CHECKPOS
	READSTRINGN(save_p, temp, sizeof(temp)); // mod it belongs to

	// P_UnArchivePlayer()
	CHECKPOS
	savegameinfo[slot].skincolor = READUINT8(save_p);
	CHECKPOS
	savegameinfo[slot].skinnum = READUINT8(save_p);
	strcpy(savegameinfo[slot].playername,
		skins[savegameinfo[slot].skinnum].name);

	CHECKPOS
	(void)READINT32(save_p); // Score

	CHECKPOS
	savegameinfo[slot].lives = READINT32(save_p); // lives
	CHECKPOS
	savegameinfo[slot].continues = READINT32(save_p); // continues

	// done
	Z_Free(savebuffer);
#undef CHECKPOS
#undef BADSAVE
}

//
// M_ReadSaveStrings
//  read the strings from the savegame files
//  and put it in savegamestrings global variable
//
static void M_ReadSaveStrings(void)
{
	FILE *handle;
	UINT32 i;
	char name[256];

	for (i = 0; i < load_end - 1; i++) //nosave is the last one.
	{
		snprintf(name, sizeof name, savegamename, i);
		name[sizeof name - 1] = '\0';

		handle = fopen(name, "rb");
		if (handle == NULL)
		{
			LoadGameMenu[i].status = 0;
			savegameinfo[i].lives = -42;
			continue;
		}
		fclose(handle);
		LoadGameMenu[i].status = 1;
		M_ReadSavegameInfo(i);
	}
}

static INT32 curSaveSelected;

//
// User wants to delete this game
//
static void M_SaveGameDeleteResponse(INT32 ch)
{
	char name[256];

	if (ch != 'y')
		return;

	// delete savegame
	snprintf(name, sizeof name, savegamename, curSaveSelected);
	name[sizeof name - 1] = '\0';
	remove(name);

	// Refresh savegame menu info
	M_ReadSaveStrings();
}

//
// Selected from SRB2 menu
//
static void M_LoadGame(INT32 choice)
{
	(void)choice;
	// change can't load message to can't load in server mode
	if (netgame && !server)
	{
		M_StartMessage(text[LOADNET],NULL,MM_NOTHING);
		return;
	}

	M_SetupNextMenu(&LoadDef);
	M_ReadSaveStrings();
}

//
// Draw border for the savegame description
//
static void M_DrawSaveLoadBorder(INT32 x,INT32 y)
{
	INT32 i;

	V_DrawScaledPatch (x-8,y+7,0,W_CachePatchName("M_LSLEFT",PU_CACHE));

	for (i = 0;i < 24;i++)
	{
		V_DrawScaledPatch (x,y+7,0,W_CachePatchName("M_LSCNTR",PU_CACHE));
		x += 8;
	}

	V_DrawScaledPatch (x,y+7,0,W_CachePatchName("M_LSRGHT",PU_CACHE));
}

//===========================================================================
//                                 END GAME
//===========================================================================

//
// M_EndGame
//
static void M_EndGameResponse(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	currentMenu->lastOn = itemOn;
	M_ClearMenus(true);
	//Command_ExitGame_f();
	G_SetExitGameFlag();
}

void M_EndGame(INT32 choice)
{
	(void)choice;
	if (demoplayback || demorecording)
		return;

	if (!Playing())
		return;

	M_StartMessage(text[ENDGAME],M_EndGameResponse,MM_YESNO);
}

//===========================================================================
//                                 Quit Game
//===========================================================================

//
// M_QuitSRB2
//
static INT32 quitsounds2[8] =
{
	sfx_spring, // Tails 11-09-99
	sfx_itemup, // Tails 11-09-99
	sfx_jump, // Tails 11-09-99
	sfx_pop,
	sfx_gloop, // Tails 11-09-99
	sfx_splash, // Tails 11-09-99
	sfx_floush, // Tails 11-09-99
	sfx_chchng // Tails 11-09-99
};

void M_ExitGameResponse(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	//Command_ExitGame_f();
	G_SetExitGameFlag();
}

void M_QuitResponse(INT32 ch)
{
	tic_t ptime;
	if (ch != 'y' && ch != KEY_ENTER)
		return;
	if (!(netgame || cv_debug))
	{
		if (quitsounds2[(gametic>>2)&7]) S_StartSound(NULL, quitsounds2[(gametic>>2)&7]); // Use quitsounds2, not quitsounds Tails 11-09-99

		//added : 12-02-98: do that instead of I_WaitVbl which does not work
		ptime = I_GetTime() + TICRATE*3; // Shortened the quit time, used to be 2 seconds Tails 03-26-2001
		while (ptime > I_GetTime())
		{
			V_DrawScaledPatch(0, 0, 0, W_CachePatchName("GAMEQUIT", PU_CACHE)); // Demo 3 Quit Screen Tails 06-16-2001
			I_FinishUpdate(); // Update the screen with the image Tails 06-19-2001
			I_Sleep();
			if (moviemode)
				M_SaveFrame();
			if (takescreenshot)
				M_DoScreenShot();
		}
	}
	I_Quit();
}

static void M_QuitSRB2(INT32 choice)
{
	// We pick index 0 which is language sensitive, or one at random,
	// between 1 and maximum number.
	static char s[200];
	(void)choice;
	sprintf(s, text[DOSY], text[QUITMSG + (gametic % NUM_QUITMESSAGES)]);
	M_StartMessage(s, M_QuitResponse, MM_YESNO);
}

//===========================================================================
//                              Some Draw routine
//===========================================================================

//
// Menu Functions
//
static void M_DrawThermo(INT32 x, INT32 y, consvar_t *cv)
{
	INT32 xx = x, i;
	lumpnum_t leftlump, rightlump, centerlump[2], cursorlump;
	patch_t *p;

	leftlump = W_GetNumForName("M_THERML");
	rightlump = W_GetNumForName("M_THERMR");
	centerlump[0] = W_GetNumForName("M_THERMM");
	centerlump[1] = W_GetNumForName("M_THERMM");
	cursorlump = W_GetNumForName("M_THERMO");

	V_DrawScaledPatch(xx, y, 0, p = W_CachePatchNum(leftlump,PU_CACHE));
	xx += SHORT(p->width) - SHORT(p->leftoffset);
	for (i = 0; i < 16; i++)
	{
		V_DrawScaledPatch(xx, y, V_WRAPX, W_CachePatchNum(centerlump[i & 1], PU_CACHE));
		xx += 8;
	}
	V_DrawScaledPatch(xx, y, 0, W_CachePatchNum(rightlump, PU_CACHE));

	xx = (cv->value - cv->PossibleValue[0].value) * (15*8) /
		(cv->PossibleValue[1].value - cv->PossibleValue[0].value);

	V_DrawScaledPatch((x + 8) + xx, y, 0, W_CachePatchNum(cursorlump, PU_CACHE));
}

//
//  Draw a textbox, like Quake does, because sometimes it's difficult
//  to read the text with all the stuff in the background...
//
//added : 06-02-98:
void M_DrawTextBox(INT32 x, INT32 y, INT32 width, INT32 boxlines)
{
	patch_t *p;
	INT32 cx = x, cy = y, n;
	INT32 step = 8, boff = 8;

	// draw left side
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_TL], PU_CACHE));
	cy += boff;
	p = W_CachePatchNum(viewborderlump[BRDR_L], PU_CACHE);
	for (n = 0; n < boxlines; n++)
	{
		V_DrawScaledPatch(cx, cy, V_WRAPY, p);
		cy += step;
	}
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_BL], PU_CACHE));

	// draw middle
	V_DrawFlatFill(x + boff, y + boff, width*step, boxlines*step, st_borderpatchnum);

	cx += boff;
	cy = y;
	while (width > 0)
	{
		V_DrawScaledPatch(cx, cy, V_WRAPX, W_CachePatchNum(viewborderlump[BRDR_T], PU_CACHE));
		V_DrawScaledPatch(cx, y + boff + boxlines*step, V_WRAPX, W_CachePatchNum(viewborderlump[BRDR_B], PU_CACHE));
		width--;
		cx += step;
	}

	// draw right side
	cy = y;
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_TR], PU_CACHE));
	cy += boff;
	p = W_CachePatchNum(viewborderlump[BRDR_R], PU_CACHE);
	for (n = 0; n < boxlines; n++)
	{
		V_DrawScaledPatch(cx, cy, V_WRAPY, p);
		cy += step;
	}
	V_DrawScaledPatch(cx, cy, 0, W_CachePatchNum(viewborderlump[BRDR_BR], PU_CACHE));
}

//==========================================================================
//                        Message is now a (hackable) Menu
//==========================================================================

static menuitem_t MessageMenu[] =
{
	// TO HACK
	{0,NULL, NULL, NULL,0}
};

menu_t MessageDef =
{
	NULL,               // title
	NULL,
	1,                  // # of menu items
	NULL,               // previous menu       (TO HACK)
	MessageMenu,        // menuitem_t ->
	M_DrawMessageMenu,  // drawing routine ->
	0, 0,               // x, y                (TO HACK)
	0,                  // lastOn, flags       (TO HACK)
	NULL
};


void M_StartMessage(const char *string, void *routine,
	menumessagetype_t itemtype)
{
	size_t max = 0, start = 0, i, strlines;
	static char *message = NULL;
	Z_Free(message);
	message = Z_StrDup(string);
	DEBFILE(message);

	M_StartControlPanel(); // can't put menuactive to true

	if (currentMenu == &MessageDef) // Prevent recursion
		MessageDef.prevMenu = &MainDef;
	else
		MessageDef.prevMenu = currentMenu;

	MessageDef.menuitems[0].text     = message;
	MessageDef.menuitems[0].alphaKey = (UINT8)itemtype;
	if (!routine && itemtype != MM_NOTHING) itemtype = MM_NOTHING;
	switch (itemtype)
	{
		case MM_NOTHING:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = M_StopMessage;
			break;
		case MM_YESNO:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = routine;
			break;
		case MM_EVENTHANDLER:
			MessageDef.menuitems[0].status     = IT_MSGHANDLER;
			MessageDef.menuitems[0].itemaction = routine;
			break;
	}
	//added : 06-02-98: now draw a textbox around the message
	// compute lenght max and the numbers of lines
	for (strlines = 0; *(message+start); strlines++)
	{
		for (i = 0;i < strlen(message+start);i++)
		{
			if (*(message+start+i) == '\n')
			{
				if (i > max)
					max = i;
				start += i;
				i = (size_t)-1; //added : 07-02-98 : damned!
				start++;
				break;
			}
		}

		if (i == strlen(message+start))
			start += i;
	}

	MessageDef.x = (INT16)((BASEVIDWIDTH  - 8*max-16)/2);
	MessageDef.y = (INT16)((BASEVIDHEIGHT - M_StringHeight(message))/2);

	MessageDef.lastOn = (INT16)((strlines<<8)+max);

	//M_SetupNextMenu();
	currentMenu = &MessageDef;
	itemOn = 0;
}

#define MAXMSGLINELEN 256

static void M_DrawMessageMenu(void)
{
	INT32 y = currentMenu->y;
	size_t i, start = 0;
	INT16 max;
	char string[MAXMSGLINELEN];
	INT32 mlines;
	const char *msg = currentMenu->menuitems[0].text;

	mlines = currentMenu->lastOn>>8;
	max = (INT16)((UINT8)(currentMenu->lastOn & 0xFF)*8);
	M_DrawTextBox(currentMenu->x, y - 8, (max+7)>>3, mlines);

	while (*(msg+start))
	{
		size_t len = strlen(msg+start);

		for (i = 0; i < len; i++)
		{
			if (*(msg+start+i) == '\n')
			{
				memset(string, 0, MAXMSGLINELEN);
				if (i >= MAXMSGLINELEN)
				{
					CONS_Printf("M_DrawMessageMenu: too long segment in %s\n", msg);
					return;
				}
				else
				{
					strncpy(string,msg+start, i);
					string[i] = '\0';
					start += i;
					i = (size_t)-1; //added : 07-02-98 : damned!
					start++;
				}
				break;
			}
		}

		if (i == strlen(msg+start))
		{
			if (i >= MAXMSGLINELEN)
			{
				CONS_Printf("M_DrawMessageMenu: too long segment in %s\n", msg);
				return;
			}
			else
			{
				strcpy(string, msg + start);
				start += i;
			}
		}

		V_DrawString((BASEVIDWIDTH - V_StringWidth(string))/2,y,0,string);
		y += 8; //SHORT(hu_font[0]->height);
	}
}

// default message handler
static void M_StopMessage(INT32 choice)
{
	(void)choice;
	M_SetupNextMenu(MessageDef.prevMenu);
}

//==========================================================================
//                        Menu stuffs
//==========================================================================

//added : 30-01-98:
//
//  Write a string centered using the hu_font
//
static void M_CentreText(INT32 y, const char *string)
{
	INT32 x;
	//added : 02-02-98 : centre on 320, because V_DrawString centers on vid.width...
	x = (BASEVIDWIDTH - V_StringWidth(string))>>1;
	V_DrawString(x,y,0,string);
}


//
// CONTROL PANEL
//

static void M_ChangeCvar(INT32 choice)
{
	consvar_t *cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;

	if (((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_SLIDER)
	    ||((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_NOMOD))
	{
		CV_SetValue(cv,cv->value+choice*2-1);
	}
	else if (cv->flags & CV_FLOAT)
	{
		char s[20];
		sprintf(s,"%f",FIXED_TO_FLOAT(cv->value)+(choice*2-1)*(1.0f/16.0f));
		CV_Set(cv,s);
	}
	else
		CV_AddValue(cv,choice*2-1);
}

static boolean M_ChangeStringCvar(INT32 choice)
{
	consvar_t *cv = (consvar_t *)currentMenu->menuitems[itemOn].itemaction;
	char buf[255];
	size_t len;

	switch (choice)
	{
		case KEY_BACKSPACE:
			len = strlen(cv->string);
			if (len > 0)
			{
				M_Memcpy(buf, cv->string, len);
				buf[len-1] = 0;
				CV_Set(cv, buf);
			}
			return true;
		default:
			if (choice >= 32 && choice <= 127)
			{
				len = strlen(cv->string);
				if (len < MAXSTRINGLENGTH - 1)
				{
					M_Memcpy(buf, cv->string, len);
					buf[len++] = (char)choice;
					buf[len] = 0;
					CV_Set(cv, buf);
				}
				return true;
			}
			break;
	}
	return false;
}

//
// M_Responder
//
boolean M_Responder(event_t *ev)
{
	INT32 ch = -1;
//	INT32 i;
	static tic_t joywait = 0, mousewait = 0;
	static boolean shiftdown = false;
	static INT32 pmousex = 0, pmousey = 0;
	static INT32 lastx = 0, lasty = 0;
	void (*routine)(INT32 choice); // for some casting problem

	if (gamestate == GS_INTRO || gamestate == GS_INTRO2 || gamestate == GS_CUTSCENE)
		return false;

	if (CON_Ready())
		return false;

	if (ev->type == ev_keyup && (ev->data1 == KEY_LSHIFT || ev->data1 == KEY_RSHIFT))
	{
		shiftdown = false;
		return false;
	}
	else if (ev->type == ev_keydown)
	{
		ch = ev->data1;

		// added 5-2-98 remap virtual keys (mouse & joystick buttons)
		switch (ch)
		{
			case KEY_LSHIFT:
			case KEY_RSHIFT:
				shiftdown = true;
				break; //return false;
			case KEY_MOUSE1:
			case KEY_JOY1:
			case KEY_JOY1 + 2:
				ch = KEY_ENTER;
				break;
			case KEY_JOY1 + 3:
				ch = 'n';
				break;
			case KEY_MOUSE1 + 1:
			case KEY_JOY1 + 1:
				ch = KEY_BACKSPACE;
				break;
			case KEY_HAT1:
				ch = KEY_UPARROW;
				break;
			case KEY_HAT1 + 1:
				ch = KEY_DOWNARROW;
				break;
			case KEY_HAT1 + 2:
				ch = KEY_LEFTARROW;
				break;
			case KEY_HAT1 + 3:
				ch = KEY_RIGHTARROW;
				break;
		}
	}
	else if (menuactive)
	{
		if (ev->type == ev_joystick  && ev->data1 == 0 && joywait < I_GetTime())
		{
			if (ev->data3 == -1)
			{
				ch = KEY_UPARROW;
				joywait = I_GetTime() + TICRATE/7;
			}
			else if (ev->data3 == 1)
			{
				ch = KEY_DOWNARROW;
				joywait = I_GetTime() + TICRATE/7;
			}

			if (ev->data2 == -1)
			{
				ch = KEY_LEFTARROW;
				joywait = I_GetTime() + TICRATE/17;
			}
			else if (ev->data2 == 1)
			{
				ch = KEY_RIGHTARROW;
				joywait = I_GetTime() + TICRATE/17;
			}
		}
		else if (ev->type == ev_mouse && mousewait < I_GetTime())
		{
			pmousey += ev->data3;
			if (pmousey < lasty-30)
			{
				ch = KEY_DOWNARROW;
				mousewait = I_GetTime() + TICRATE/7;
				pmousey = lasty -= 30;
			}
			else if (pmousey > lasty + 30)
			{
				ch = KEY_UPARROW;
				mousewait = I_GetTime() + TICRATE/7;
				pmousey = lasty += 30;
			}

			pmousex += ev->data2;
			if (pmousex < lastx - 30)
			{
				ch = KEY_LEFTARROW;
				mousewait = I_GetTime() + TICRATE/7;
				pmousex = lastx -= 30;
			}
			else if (pmousex > lastx+30)
			{
				ch = KEY_RIGHTARROW;
				mousewait = I_GetTime() + TICRATE/7;
				pmousex = lastx += 30;
			}
		}
	}

	if (ch == -1)
		return false;

	// F-Keys
	if (!menuactive || ch == KEY_F8) //allow screenshots
	{
		switch (ch)
		{
			case KEY_F1: // Help key
				M_StartControlPanel();
				currentMenu = &ReadDef1;
				itemOn = 0;
				return true;

			case KEY_F2: // Empty
				return true;

			case KEY_F3: // Empty
				return true;

			case KEY_F4: // Sound Volume
				M_StartControlPanel();
				currentMenu = &SoundDef;
				itemOn = sfx_vol;
				return true;

#ifndef DC
			case KEY_F5: // Video Mode
				M_StartControlPanel();
				M_SetupNextMenu(&VidModeDef);
				return true;
#endif

			case KEY_F6: // Empty
				return true;

			case KEY_F7: // Options
				M_StartControlPanel();
				M_OptionsMenu(0);
				return true;

			case KEY_F8: // Screenshot
				COM_ImmedExecute("screenshot\n");
				return true;

			case KEY_F9: // Empty
#ifdef HAVE_PNG
				((moviemode) ? M_StopMovie : M_StartMovie)();
#endif
				return true;

			case KEY_F10: // Quit SRB2
				M_QuitSRB2(0);
				return true;

			case KEY_F11: // Fullscreen
				CV_AddValue(&cv_fullscreen, 1);
				return true;

			case KEY_ESCAPE: // Pop up menu
				if (chat_on)
					HU_clearChatChars();
				else
					M_StartControlPanel();
				return true;
		}
		return false;
	}

	routine = currentMenu->menuitems[itemOn].itemaction;

	// Handle menuitems which need a specific key handling
	if (routine && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_KEYHANDLER)
	{
		if (shiftdown && ch >= 32 && ch <= 127)
			ch = shiftxform[ch];
		routine(ch);
		return true;
	}

	if (currentMenu->menuitems[itemOn].status == IT_MSGHANDLER)
	{
		if (currentMenu->menuitems[itemOn].alphaKey != MM_EVENTHANDLER)
		{
			if (ch == ' ' || ch == 'n' || ch == 'y' || ch == KEY_ESCAPE || ch == KEY_ENTER)
			{
				if (routine)
					routine(ch);
				M_StopMessage(0);
				return true;
			}
			return true;
		}
		else
		{
			// dirty hack: for customising controls, I want only buttons/keys, not moves
			if (ev->type == ev_mouse || ev->type == ev_mouse2 || ev->type == ev_joystick
				|| ev->type == ev_joystick2)
				return true;
			if (routine)
			{
				void (*otherroutine)(event_t *sev) = currentMenu->menuitems[itemOn].itemaction;
				otherroutine(ev); //Alam: what a hack
			}
			return true;
		}
	}

	// BP: one of the more big hack i have never made
	if (routine && (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR)
	{
		if ((currentMenu->menuitems[itemOn].status & IT_CVARTYPE) == IT_CV_STRING)
		{
			if (shiftdown && ch >= 32 && ch <= 127)
				ch = shiftxform[ch];
			if (M_ChangeStringCvar(ch))
				return true;
			else
				routine = NULL;
		}
		else
			routine = M_ChangeCvar;
	}

	// Keys usable within menu
	switch (ch)
	{
		case KEY_DOWNARROW:
			do
			{
				if (itemOn + 1 > currentMenu->numitems - 1)
					itemOn = 0;
				else
					itemOn++;
			} while ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE);

			S_StartSound(NULL, sfx_menu1);
			return true;

		case KEY_UPARROW:
			do
			{
				if (!itemOn)
					itemOn = (INT16)(currentMenu->numitems - 1);
				else
					itemOn--;
			} while ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_SPACE);

			S_StartSound(NULL, sfx_menu1);
			return true;

		case KEY_LEFTARROW:
			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
				if (currentMenu != &SoundDef)
					S_StartSound(NULL, sfx_menu1);
				routine(0);
			}
			return true;

		case KEY_RIGHTARROW:
			if (routine && ((currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_ARROWS
				|| (currentMenu->menuitems[itemOn].status & IT_TYPE) == IT_CVAR))
			{
				if (currentMenu != &SoundDef)
					S_StartSound(NULL, sfx_menu1);
				routine(1);
			}
			return true;

		case KEY_ENTER:
			currentMenu->lastOn = itemOn;
			if (routine)
			{
				switch (currentMenu->menuitems[itemOn].status & IT_TYPE)
				{
					case IT_CVAR:
					case IT_ARROWS:
						routine(1); // right arrow
						S_StartSound(NULL, sfx_menu1);
						break;
					case IT_CALL:
						routine(itemOn);
						S_StartSound(NULL, sfx_menu1);
						break;
					case IT_SUBMENU:
						currentMenu->lastOn = itemOn;
						M_SetupNextMenu((menu_t *)currentMenu->menuitems[itemOn].itemaction);
						S_StartSound(NULL, sfx_menu1);
						break;
				}
			}
			return true;

		case KEY_ESCAPE:
			currentMenu->lastOn = itemOn;
			if (currentMenu->prevMenu)
			{
				// Catch Switch Map option in case we quit a game using the menu somewhere...
				if (!(netgame || multiplayer) || !Playing()
					|| !(server || adminplayer == consoleplayer))
				{
					MainMenu[switchmap].status = IT_DISABLED;
					MainMenu[scramble].status = IT_DISABLED;
				}
				else
				{
					MainMenu[switchmap].status = IT_STRING | IT_CALL;

					if((gametype == GT_MATCH && cv_matchtype.value) || gametype == GT_CTF)
						MainMenu[scramble].status = IT_STRING | IT_CALL;
				}

				// Make sure the Switch Team / Spectate option only shows up in gametypes that apply.
				if (!(gametype == GT_MATCH || gametype == GT_TAG || gametype == GT_CTF)
					|| !(netgame || multiplayer) || !Playing())
				{
					MainMenu[spectate].status = IT_DISABLED;
					MainMenu[switchteam].status = IT_DISABLED;
					MainMenu[scramble].status = IT_DISABLED;
				}
				else if ((gametype == GT_MATCH && cv_matchtype.value) || gametype == GT_CTF)
				{
					MainMenu[spectate].status = IT_DISABLED;
					MainMenu[switchteam].status = IT_STRING | IT_CALL;

					if(server || adminplayer == consoleplayer)
						MainMenu[scramble].status = IT_STRING | IT_CALL;
				}
				else
				{
					MainMenu[spectate].status = IT_STRING | IT_CALL;
					MainMenu[switchteam].status = IT_DISABLED;
					MainMenu[scramble].status = IT_DISABLED;
				}

				if (currentMenu == &TimeAttackDef)
				{
					// Fade to black first
					if (rendermode != render_none)
					{
						V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31);
						F_WipeEndScreen(0, 0, vid.width, vid.height);

						F_RunWipe(2*TICRATE, false);
					}
					menuactive = false;
					I_UpdateMouseGrab();
					D_StartTitle();
				}
				else if (currentMenu == &LevelSelectDef)
				{
					// Don't let people backdoor their way into Pandora's Box if they havn't earned it.
					if (pandoralevelselect)
					{
						currentMenu = &SecretsDef;
						itemOn = currentMenu->lastOn;
					}
					else
					{
						currentMenu = &LoadDef;
						itemOn = currentMenu->lastOn;
					}
				}
				else
				{
					currentMenu = currentMenu->prevMenu;
					itemOn = currentMenu->lastOn;
				}
			}
			else
				M_ClearMenus(true);

			return true;

		case KEY_BACKSPACE:
			if ((currentMenu->menuitems[itemOn].status) == IT_CONTROL)
			{
				// detach any keys associated with the game control
				G_ClearControlKeys(setupcontrols, currentMenu->menuitems[itemOn].alphaKey);
				return true;
			}
			else if (currentMenu == &LoadDef)
			{
				if (curSaveSelected != 5) //Don't delete the "No Save" option.
				{
					curSaveSelected = itemOn; // Eww eww!
					M_StartMessage("Are you sure you want to delete\nthis save game?\n(Y/N)\n",M_SaveGameDeleteResponse,MM_YESNO);
					return true;
				}
			}
			else if (currentMenu == &LevelSelectDef)
			{
				// Don't let people backdoor their way into Pandora's Box if they havn't earned it.
				if (pandoralevelselect)
				{
					currentMenu = &SecretsDef;
					itemOn = currentMenu->lastOn;
				}
				else
				{
					currentMenu = &LoadDef;
					itemOn = currentMenu->lastOn;
				}
				return true;
			}
			currentMenu->lastOn = itemOn;
			if (currentMenu->prevMenu)
			{
				currentMenu = currentMenu->prevMenu;
				itemOn = currentMenu->lastOn;
			}
			return true;

		default:
/*			for (i = itemOn + 1; i < currentMenu->numitems; i++)
				if (currentMenu->menuitems[i].alphaKey == ch && !(currentMenu->menuitems[i].status & IT_DISABLED))
				{
					itemOn = (INT16)i;
					S_StartSound(NULL, sfx_menu1);
					return true;
				}
			for (i = 0; i <= itemOn; i++)
				if (currentMenu->menuitems[i].alphaKey == ch && !(currentMenu->menuitems[i].status & IT_DISABLED))
				{
					itemOn = (INT16)i;
					S_StartSound(NULL, sfx_menu1);
					return true;
				}*/
			CON_Responder(ev);
			break;
	}

	return true;
}

//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer(void)
{
	if (currentMenu == &MessageDef)
		menuactive = true;

	if (!menuactive)
		return;

	// now that's more readable with a faded background (yeah like Quake...)
	if (!WipeInAction)
		V_DrawFadeScreen();

	if (currentMenu->drawroutine)
		currentMenu->drawroutine(); // call current menu Draw routine

	// Draw version down in corner
	if (customversionstring[0] != '\0')
		V_DrawString(0, BASEVIDHEIGHT - 8, V_TRANSLUCENT|V_SNAPTOBOTTOM|V_SNAPTOLEFT, customversionstring);
	else
	{
#ifdef DEVELOP // Development -- show revision / branch info
		V_DrawString(0, BASEVIDHEIGHT - 16, V_TRANSLUCENT|V_SNAPTOBOTTOM|V_SNAPTOLEFT, compbranch);
		V_DrawString(0, BASEVIDHEIGHT - 8, V_TRANSLUCENT|V_SNAPTOBOTTOM|V_SNAPTOLEFT, comprevision);
#else // Regular build
		V_DrawString(0, BASEVIDHEIGHT - 8, V_TRANSLUCENT|V_SNAPTOBOTTOM|V_SNAPTOLEFT, VERSIONSTRING);
#endif
	}
}

//
// M_StartControlPanel
//
void M_StartControlPanel(void)
{
	// intro might call this repeatedly
	if (menuactive)
	{
		CON_ToggleOff(); // move away console
		return;
	}

	menuactive = true;

	if (!(netgame || multiplayer) || !Playing()
		|| !(server || adminplayer == consoleplayer))
	{
		MainMenu[switchmap].status = IT_DISABLED;
		MainMenu[scramble].status = IT_DISABLED;
	}
	else
	{
		MainMenu[switchmap].status = IT_STRING | IT_CALL;

		if((gametype == GT_MATCH && cv_matchtype.value) || gametype == GT_CTF)
			MainMenu[scramble].status = IT_STRING | IT_CALL;
	}

	if (!(gametype == GT_MATCH || gametype == GT_TAG || gametype == GT_CTF))
	{
		MainMenu[spectate].status = IT_DISABLED;
		MainMenu[switchteam].status = IT_DISABLED;
		MainMenu[scramble].status = IT_DISABLED;
	}
	else if ((gametype == GT_MATCH && cv_matchtype.value) || gametype == GT_CTF)
	{
		MainMenu[spectate].status = IT_DISABLED;
		MainMenu[switchteam].status = IT_STRING | IT_CALL;
	}
	else
	{
		MainMenu[spectate].status = IT_STRING | IT_CALL;
		MainMenu[switchteam].status = IT_DISABLED;
	}

	MainMenu[secrets].status = IT_DISABLED;

	// Check for the ??? menu
	if (grade > 0)
		MainMenu[secrets].status = IT_STRING | IT_CALL;

	if (savemoddata)
		MainMenu[secrets].itemaction = M_CustomSecretsMenu;
	else
		MainMenu[secrets].itemaction = M_SecretsMenu;

	currentMenu = &MainDef;
	itemOn = singleplr;

	//CON_ToggleOff(); // move away console

	if (timeattacking) // Cancel recording
	{
		G_CheckDemoStatus();

		if (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION)
			Command_ExitGame_f();

		currentMenu = &TimeAttackDef;
		itemOn = currentMenu->lastOn;
		timeattacking = false;
		G_SetGamestate(GS_TIMEATTACK);
		S_ChangeMusic(mus_racent, true);
		CV_AddValue(&cv_nextmap, 1);
		CV_AddValue(&cv_nextmap, -1);
		return;
	}
}

//
// M_ClearMenus
//
void M_ClearMenus(boolean callexitmenufunc)
{
	if (!menuactive)
		return;

	if (currentMenu->quitroutine && callexitmenufunc && !currentMenu->quitroutine())
		return; // we can't quit this menu (also used to set parameter from the menu)

#ifndef DC // Save the config file. I'm sick of crashing the game later and losing all my changes!
	COM_BufAddText(va("saveconfig \"%s\" -silent\n", configfile));
#endif //Alam: But not on the Dreamcast's VMUs

	if (currentMenu != &MessageDef)
		menuactive = false;

	I_UpdateMouseGrab();
}

//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
{
	INT32 i;

	if (currentMenu->quitroutine)
	{
		if (!currentMenu->quitroutine())
			return; // we can't quit this menu (also used to set parameter from the menu)
	}
	currentMenu = menudef;
	itemOn = currentMenu->lastOn;

	// in case of...
	if (itemOn >= currentMenu->numitems)
		itemOn = (INT16)(currentMenu->numitems - 1);

	// the curent item can be disabled,
	// this code go up until an enabled item found
	if (currentMenu->menuitems[itemOn].status == IT_DISABLED)
	{
		for (i = 0; i < currentMenu->numitems; i++)
		{
			if ((currentMenu->menuitems[i].status != IT_DISABLED))
			{
				itemOn = (INT16)i;
				break;
			}
		}
	}
}

// Guess I'll put this here, idk
boolean M_MouseNeeded(void)
{
	return (currentMenu == &MessageDef && (currentMenu->prevMenu == &ControlDef || currentMenu->prevMenu == &ControlDef2));
}

//
// M_Ticker
//
void M_Ticker(void)
{
	if (--skullAnimCounter <= 0)
		skullAnimCounter = 8 * NEWTICRATERATIO;

	//added : 30-01-98 : test mode for five seconds
	if (vidm_testingmode > 0)
	{
		// restore the previous video mode
		if (--vidm_testingmode == 0)
			setmodeneeded = vidm_previousmode + 1;
	}
}

//
// M_Init
//
void M_Init(void)
{
	CV_RegisterVar(&cv_nextmap);
	CV_RegisterVar(&cv_newgametype);
	CV_RegisterVar(&cv_chooseskin);

	// This is used because DOOM 2 had only one HELP
	//  page. I use CREDIT as second page now, but
	//  kept this hack for educational purposes.
	ReadMenu1[0].itemaction = &MainDef;

	//todo put this somewhere better...
	CV_RegisterVar(&cv_allcaps);
}

//======================================================================
// OpenGL specific options
//======================================================================

#ifdef HWRENDER

static void M_DrawOpenGLMenu(void);
static void M_OGL_DrawFogMenu(void);
static void M_OGL_DrawColorMenu(void);
static void M_HandleFogColor(INT32 choice);

static menuitem_t OpenGLOptionsMenu[] =
{
	{IT_STRING|IT_CVAR,         NULL, "Field of view",   &cv_grfov,            10},
	{IT_STRING|IT_CVAR,         NULL, "Quality",         &cv_scr_depth,        20},
	{IT_STRING|IT_CVAR,         NULL, "Texture Filter",  &cv_grfiltermode,     30},
	{IT_STRING|IT_CVAR,         NULL, "Anisotropic",     &cv_granisotropicmode,40},
#ifdef _WINDOWS
	{IT_STRING|IT_CVAR,         NULL, "Fullscreen",      &cv_fullscreen,       50},
#endif
	{IT_STRING|IT_CVAR|IT_CV_SLIDER,
	                            NULL, "Translucent HUD", &cv_grtranslucenthud, 60},
#ifdef ALAM_LIGHTING
	{IT_SUBMENU|IT_WHITESTRING, NULL, "Lighting...",     &OGL_LightingDef,     70},
#endif
	{IT_SUBMENU|IT_WHITESTRING, NULL, "Fog...",          &OGL_FogDef,          80},
	{IT_SUBMENU|IT_WHITESTRING, NULL, "Gamma...",        &OGL_ColorDef,        90},
};

#ifdef ALAM_LIGHTING
static menuitem_t OGL_LightingMenu[] =
{
	{IT_STRING|IT_CVAR, NULL, "Coronas",          &cv_grcoronas,          0},
	{IT_STRING|IT_CVAR, NULL, "Coronas size",     &cv_grcoronasize,      10},
	{IT_STRING|IT_CVAR, NULL, "Dynamic lighting", &cv_grdynamiclighting, 20},
	{IT_STRING|IT_CVAR, NULL, "Static lighting",  &cv_grstaticlighting,  30},
};
#endif

static menuitem_t OGL_FogMenu[] =
{
	{IT_STRING|IT_CVAR,       NULL, "Fog",         &cv_grfog,         0},
	{IT_STRING|IT_KEYHANDLER, NULL, "Fog color",   M_HandleFogColor, 10},
	{IT_STRING|IT_CVAR,       NULL, "Fog density", &cv_grfogdensity, 20},
	{IT_STRING|IT_CVAR,       NULL, "Software Fog",&cv_grsoftwarefog,30},
};

static menuitem_t OGL_ColorMenu[] =
{
	{IT_STRING|IT_CVAR|IT_CV_SLIDER, NULL, "red",   &cv_grgammared,   10},
	{IT_STRING|IT_CVAR|IT_CV_SLIDER, NULL, "green", &cv_grgammagreen, 20},
	{IT_STRING|IT_CVAR|IT_CV_SLIDER, NULL, "blue",  &cv_grgammablue,  30},
};

menu_t OpenGLOptionDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (OpenGLOptionsMenu)/sizeof (menuitem_t),
	&VideoOptionsDef,
	OpenGLOptionsMenu,
	M_DrawOpenGLMenu,
	30, 40,
	0,
	NULL
};

#ifdef ALAM_LIGHTING
menu_t OGL_LightingDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (OGL_LightingMenu)/sizeof (menuitem_t),
	&OpenGLOptionDef,
	OGL_LightingMenu,
	M_DrawGenericMenu,
	60, 40,
	0,
	NULL
};
#endif

menu_t OGL_FogDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (OGL_FogMenu)/sizeof (menuitem_t),
	&OpenGLOptionDef,
	OGL_FogMenu,
	M_OGL_DrawFogMenu,
	60, 40,
	0,
	NULL
};

menu_t OGL_ColorDef =
{
	"M_OPTTTL",
	"OPTIONS",
	sizeof (OGL_ColorMenu)/sizeof (menuitem_t),
	&OpenGLOptionDef,
	OGL_ColorMenu,
	M_OGL_DrawColorMenu,
	60, 40,
	0,
	NULL
};
//======================================================================
// M_DrawOpenGLMenu()
//======================================================================
static void M_DrawOpenGLMenu(void)
{
	INT32 mx, my;

	mx = OpenGLOptionDef.x;
	my = OpenGLOptionDef.y;
	M_DrawGenericMenu(); // use generic drawer for cursor, items and title
//	V_DrawString(BASEVIDWIDTH - mx - V_StringWidth(cv_scr_depth.string),
//		my + currentMenu->menuitems[2].alphaKey, V_YELLOWMAP, cv_scr_depth.string);
}

#define FOG_COLOR_ITEM  1
//======================================================================
// M_OGL_DrawFogMenu()
//======================================================================
static void M_OGL_DrawFogMenu(void)
{
	INT32 mx, my;

	mx = OGL_FogDef.x;
	my = OGL_FogDef.y;
	M_DrawGenericMenu(); // use generic drawer for cursor, items and title
	V_DrawString(BASEVIDWIDTH - mx - V_StringWidth(cv_grfogcolor.string),
		my + currentMenu->menuitems[FOG_COLOR_ITEM].alphaKey, V_YELLOWMAP, cv_grfogcolor.string);
	// blink cursor on FOG_COLOR_ITEM if selected
	if (itemOn == FOG_COLOR_ITEM && skullAnimCounter < 4)
		V_DrawCharacter(BASEVIDWIDTH - mx,
			my + currentMenu->menuitems[FOG_COLOR_ITEM].alphaKey, '_' | 0x80,false);
}

//======================================================================
// M_OGL_DrawColorMenu()
//======================================================================
static void M_OGL_DrawColorMenu(void)
{
	INT32 mx, my;

	mx = OGL_ColorDef.x;
	my = OGL_ColorDef.y;
	M_DrawGenericMenu(); // use generic drawer for cursor, items and title
	V_DrawString(mx, my + currentMenu->menuitems[0].alphaKey - 10,
		V_YELLOWMAP, "Gamma correction");
}

//======================================================================
// M_OpenGLOption()
//======================================================================
#ifdef SHUFFLE
static void M_OpenGLOption(INT32 choice)
{
	(void)choice;
	if (rendermode != render_soft)
		M_SetupNextMenu(&OpenGLOptionDef);
	else
		M_StartMessage("You are in software mode\nYou can't change the options\n", NULL, MM_NOTHING);
}
#endif
//======================================================================
// M_HandleFogColor()
//======================================================================
static void M_HandleFogColor(INT32 choice)
{
	size_t i, l;
	char temp[8];
	boolean exitmenu = false; // exit to previous menu and send name change

	switch (choice)
	{
		case KEY_DOWNARROW:
			S_StartSound(NULL, sfx_menu1);
			itemOn++;
			break;

		case KEY_UPARROW:
			S_StartSound(NULL, sfx_menu1);
			itemOn--;
			break;

		case KEY_ESCAPE:
			S_StartSound(NULL, sfx_menu1);
			exitmenu = true;
			break;

		case KEY_BACKSPACE:
			S_StartSound(NULL, sfx_menu1);
			strcpy(temp, cv_grfogcolor.string);
			strcpy(cv_grfogcolor.zstring, "000000");
			l = strlen(temp)-1;
			for (i = 0; i < l; i++)
				cv_grfogcolor.zstring[i + 6 - l] = temp[i];
			break;

		default:
			if ((choice >= '0' && choice <= '9') || (choice >= 'a' && choice <= 'f')
				|| (choice >= 'A' && choice <= 'F'))
			{
				S_StartSound(NULL, sfx_menu1);
				strcpy(temp, cv_grfogcolor.string);
				strcpy(cv_grfogcolor.zstring, "000000");
				l = strlen(temp);
				for (i = 0; i < l; i++)
					cv_grfogcolor.zstring[5 - i] = temp[l - i];
					cv_grfogcolor.zstring[5] = (char)choice;
			}
			break;
	}
	if (exitmenu)
	{
		if (currentMenu->prevMenu)
			M_SetupNextMenu(currentMenu->prevMenu);
		else
			M_ClearMenus(true);
	}
}
#endif

// Message responder for turning on
// cheats through the menu system.
void M_CheatActivationResponder(INT32 ch)
{
	if (ch != 'y' && ch != KEY_ENTER)
		return;

	CV_SetValue(&cv_cheats, 1);
}
