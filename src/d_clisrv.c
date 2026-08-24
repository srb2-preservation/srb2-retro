// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
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
/// \brief Single-player tic loop.
///
/// This engine never had a "local-only" code path: even solo play built
/// its ticcmd, dropped it in netcmds[][], and ran it a tic later through
/// the same deferred-command queue (SendNetXCmd/RegisterNetXCmd) that a
/// networked game used. Removing the netcode meant losing that plumbing,
/// so this file replaces it with the minimum needed to keep a solo game
/// running: build the local ticcmd every frame, advance gametic, and run
/// deferred commands synchronously instead of one tic later over a socket.

#include "doomdef.h"
#include "doomstat.h"
#include "d_clisrv.h"
#include "d_main.h"
#include "command.h"
#include "console.h"
#include "g_game.h"
#include "g_input.h"
#include "m_menu.h"
#include "i_system.h"
#include "y_inter.h"
#include "z_zone.h"

boolean server = true;
INT32 serverplayer = 0;

// DEBFILE() debug logging sink (doomstat.h); this engine build never had a
// non-networking definition for it, so it lives here now.
FILE *debugfile = NULL;

char adminpassword[9], motd[254];

ticcmd_t netcmds[BACKUPTICS][MAXPLAYERS];

consvar_t cv_playdemospeed = {"playdemospeed", "0", 0, CV_Unsigned, NULL, 0, NULL, NULL, 0, 0, NULL};

INT32 ticruned, ticmiss;

static tic_t maketic = 1, neededtic = 1;
static ticcmd_t localcmds, localcmds2;

void D_ResetTiccmds(void)
{
	memset(&localcmds, 0, sizeof(ticcmd_t));
	memset(&localcmds2, 0, sizeof(ticcmd_t));
}

// -----------------------------------------------------------------
// Deferred command dispatch. Same shape as the old net XCmd system,
// but there's nobody to send this to anymore -- run it now, locally.
// -----------------------------------------------------------------
static void (*listnetxcmd[MAXNETXCMD])(UINT8 **p, INT32 playernum);

void RegisterNetXCmd(netxcmd_t id, void (*cmd_f)(UINT8 **p, INT32 playernum))
{
	listnetxcmd[id] = cmd_f;
}

static void RunNetXCmd(netxcmd_t id, const void *param, size_t nparam, INT32 playernum)
{
	UINT8 buf[256];
	UINT8 *p = buf;

	if (!listnetxcmd[id])
		return;

	if (param && nparam)
		M_Memcpy(buf, param, nparam);

	listnetxcmd[id](&p, playernum);
}

void SendNetXCmd(netxcmd_t id, const void *param, size_t nparam)
{
	if (demoplayback)
		return;

	RunNetXCmd(id, param, nparam, consoleplayer);
}

// splitscreen player
void SendNetXCmd2(netxcmd_t id, const void *param, size_t nparam)
{
	if (demoplayback)
		return;

	RunNetXCmd(id, param, nparam, secondarydisplayplayer);
}

UINT8 GetFreeXCmdSize(void)
{
	return 255;
}

// -----------------------------------------------------------------
// Player/session bookkeeping
// -----------------------------------------------------------------
static boolean serverrunning = false;

boolean Playing(void)
{
	return serverrunning;
}

void SV_SpawnPlayer(INT32 playernum, INT32 x, INT32 y, angle_t angle)
{
	(void)x;
	(void)y;
	netcmds[maketic%BACKUPTICS][playernum].angleturn = (INT16)((INT16)(angle>>16) | TICCMD_RECEIVED);
}

boolean SV_SpawnServer(void)
{
	if (demoplayback)
		G_StopDemo();

	if (!serverrunning)
	{
		serverrunning = true;
		SV_ResetServer();
		return true;
	}

	return false;
}

void SV_StopServer(void)
{
	if (gamestate == GS_INTERMISSION)
		Y_EndIntermission();
	gamestate = wipegamestate = GS_NULL;
	maketic = gametic + 1;
	neededtic = maketic;
	serverrunning = false;
}

void SV_ResetServer(void)
{
	INT32 i;

	maketic = gametic + 1;
	neededtic = maketic;

	for (i = 1; i < MAXPLAYERS; i++)
		playeringame[i] = false;
	playeringame[0] = true;
	consoleplayer = 0;
	serverplayer = consoleplayer;
}

void SV_StartSinglePlayerServer(void)
{
	server = true;
	netgame = false;
	multiplayer = splitscreen;
	gametype = GT_COOP;
	SV_StopServer();
}

void CL_ClearPlayer(INT32 playernum)
{
	memset(&players[playernum], 0, sizeof (player_t));
}

void CL_AddSplitscreenPlayer(void)
{
}

void CL_RemoveSplitscreenPlayer(void)
{
}

INT32 D_NumPlayers(void)
{
	INT32 num = 0, i;
	for (i = 0; i < MAXPLAYERS; i++)
		if (playeringame[i])
			num++;
	return num;
}

boolean AddLmpExtradata(UINT8 **demo_point, INT32 playernum)
{
	(void)demo_point;
	(void)playernum;
	return false;
}

void ReadLmpExtraData(UINT8 **demo_pointer, INT32 playernum)
{
	(void)demo_pointer;
	(void)playernum;
}

// -----------------------------------------------------------------
// Tic loop
// -----------------------------------------------------------------
void NetUpdate(void)
{
	static tic_t gametime = 0;
	tic_t nowtime, realtics;

	nowtime = I_GetTime();
	realtics = nowtime - gametime;

	if (realtics <= 0)
		return;
	if (realtics > 5)
		realtics = 1;

	gametime = nowtime;

	I_OsPolling();
	D_ProcessEvents();

	if (!dedicated)
		rendergametic = gametic;

	G_BuildTiccmd(&localcmds, (INT32)realtics);
	if (splitscreen)
		G_BuildTiccmd2(&localcmds2, (INT32)realtics);

	netcmds[maketic%BACKUPTICS][consoleplayer] = localcmds;
	if (splitscreen)
		netcmds[maketic%BACKUPTICS][secondarydisplayplayer] = localcmds2;
	maketic++;
	neededtic = maketic;

	M_Ticker();
	CON_Ticker();
}

void TryRunTics(tic_t realtics)
{
	if (realtics > TICRATE/7)
		realtics = 1;

	if (singletics)
		realtics = 1;

	if (realtics >= 1)
	{
		COM_BufExecute();
		if (mapchangepending)
			D_MapChange(-1, 0, ultimatemode, 0, 2, false, fromlevelselect);
	}

	NetUpdate();

	if (demoplayback)
	{
		neededtic = gametic + realtics + cv_playdemospeed.value;
		maketic += realtics;
	}

	if (neededtic > gametic)
	{
		if (advancedemo)
			D_StartTitle();
		else
			while (neededtic > gametic)
			{
				G_Ticker();
				gametic++;

				if (demoplayback && paused)
					neededtic++;
			}
	}
}
