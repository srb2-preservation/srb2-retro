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
///	This used to be the client/server networking protocol header.
///	The network transport is gone; what's left is the minimum this
///	engine needs to run a solo game, since ticcmd dispatch (SendNetXCmd)
///	and player spawning (SV_SpawnPlayer) were never purely-networked
///	code paths to begin with -- singleplayer always ran through them too.

#ifndef __D_CLISRV__
#define __D_CLISRV__

#include "d_ticcmd.h"
#include "d_netcmd.h" // netxcmd_t
#include "tables.h" // angle_t

#define BACKUPTICS 32
#define MAXTEXTCMD 256
#define MAXFILENEEDED 915 // used only for the add-ons menu's progress-bar math now

// Kick reason codes. Nothing consumes XD_KICK anymore (no local player to
// kick), but the byte values are still passed around by callers.
#define KICK_MSG_GO_AWAY     1
#define KICK_MSG_CON_FAIL    2
#define KICK_MSG_PLAYER_QUIT 3
#define KICK_MSG_TIMEOUT     4
#define KICK_MSG_BANNED      5
#define KICK_MSG_PING_HIGH   6
#define KICK_MSG_CUSTOM_KICK 7
#define KICK_MSG_CUSTOM_BAN  8

// server is always true now -- there is no client mode to flip it off.
extern boolean server;

extern char adminpassword[9], motd[254];

extern ticcmd_t netcmds[BACKUPTICS][MAXPLAYERS];

extern consvar_t cv_playdemospeed;

// Lag-compensation stats, kept only because p_user.c still increments them;
// nothing displays them anymore now that the net-stat HUD is gone.
extern INT32 ticruned, ticmiss;

// ------------------------
// Deferred command dispatch
// ------------------------
// Same interface the old netcode exposed: queue a command by id, and it runs
// against the registered handler. Now it just runs immediately, locally.
void RegisterNetXCmd(netxcmd_t id, void (*cmd_f)(UINT8 **p, INT32 playernum));
void SendNetXCmd(netxcmd_t id, const void *param, size_t nparam);
UINT8 GetFreeXCmdSize(void);

void D_ResetTiccmds(void);

// ------------------------
// Tic loop
// ------------------------
void TryRunTics(tic_t realtics);
void NetUpdate(void);
INT32 D_NumPlayers(void);
boolean Playing(void);

// Demos no longer carry a deferred-XCmd stream (SendNetXCmd runs
// synchronously now, so there's nothing queued to record). Base ticcmd
// demo record/playback in g_game.c is unaffected -- only mid-demo XCmd
// side effects (e.g. a runsoc mid-recording) won't play back exactly.
boolean AddLmpExtradata(UINT8 **demo_point, INT32 playernum);
void ReadLmpExtraData(UINT8 **demo_pointer, INT32 playernum);

// ------------------------
// Player/session bookkeeping that survived the netcode removal
// ------------------------
void SV_SpawnPlayer(INT32 playernum, INT32 x, INT32 y, angle_t angle);
boolean SV_SpawnServer(void);
void SV_StartSinglePlayerServer(void);
void SV_StopServer(void);
void SV_ResetServer(void);
void CL_ClearPlayer(INT32 playernum);

#endif // __D_CLISRV__
