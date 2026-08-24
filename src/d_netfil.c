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
/// \brief Local WAD/add-on file lookup.
///	The network file-transfer half of this used to live here too;
///	that's gone along with the rest of the netcode. This keeps only
///	the local filesystem search/MD5-check helpers that filesrch.c,
///	w_wad.c, and the add-ons menu still call directly.

#include <stdio.h>
#include <string.h>

#include "doomdef.h"
#include "doomstat.h"
#include "d_main.h"
#include "d_netfil.h"
#include "filesrch.h"
#include "md5.h"

INT32 fileneedednum;
fileneeded_t fileneeded[MAX_WADFILES];
char downloaddir[256] = "DOWNLOAD";

void nameonly(char *s)
{
	size_t j, len;
	void *ns;

	for (j = strlen(s); j != (size_t)-1; j--)
		if ((s[j] == '\\') || (s[j] == ':') || (s[j] == '/'))
		{
			ns = &(s[j+1]);
			len = strlen(ns);
			memmove(s, ns, len+1);
			return;
		}
}

// Returns the length in characters of the last element of a path.
size_t nameonlylength(const char *s)
{
	size_t j, len = strlen(s);

	for (j = len; j != (size_t)-1; j--)
		if ((s[j] == '\\') || (s[j] == ':') || (s[j] == '/'))
			return len - j - 1;

	return len;
}

filestatus_t checkfilemd5(char *filename, const UINT8 *wantedmd5sum)
{
#if defined (NOMD5) || defined (_arch_dreamcast)
	(void)wantedmd5sum;
	(void)filename;
#else
	FILE *fhandle;
	UINT8 md5sum[16];

	if (!wantedmd5sum)
		return FS_FOUND;

	fhandle = fopen(filename, "rb");
	if (fhandle)
	{
		md5_stream(fhandle,md5sum);
		fclose(fhandle);
		if (!memcmp(wantedmd5sum, md5sum, 16))
			return FS_FOUND;
		return FS_MD5SUMBAD;
	}

	I_Error("Couldn't open %s for md5 check", filename);
#endif
	return FS_FOUND; // will never happen, but makes the compiler shut up
}

filestatus_t findfile(char *filename, const UINT8 *wantedmd5sum, boolean completepath)
{
	filestatus_t homecheck = filesearch(filename, srb2home, wantedmd5sum, false, 10);
	if (homecheck == FS_FOUND)
		return filesearch(filename, srb2home, wantedmd5sum, completepath, 10);

	homecheck = filesearch(filename, srb2path, wantedmd5sum, false, 10);
	if (homecheck == FS_FOUND)
		return filesearch(filename, srb2path, wantedmd5sum, completepath, 10);

#ifdef _arch_dreamcast
	return filesearch(filename, "/cd", wantedmd5sum, completepath, 10);
#else
	return filesearch(filename, ".", wantedmd5sum, completepath, 10);
#endif
}
