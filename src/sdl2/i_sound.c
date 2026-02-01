/// \file
/// \brief SDL Mixer interface for sound

#include "../doomdef.h"

#if defined(SDL2) && defined(HAVE_MIXER) && SOUND==SOUND_MIXER

#include "../sounds.h"
#include "../s_sound.h"
#include "../i_sound.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../byteptr.h"

#ifdef _MSC_VER
#pragma warning(disable : 4214 4244)
#endif
#include <SDL2/SDL.h>
#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

#include <SDL2/SDL_mixer.h>

/* This is the version number macro for the current SDL_mixer version: */
#ifndef SDL_MIXER_COMPILEDVERSION
#define SDL_MIXER_COMPILEDVERSION \
	SDL_VERSIONNUM(MIX_MAJOR_VERSION, MIX_MINOR_VERSION, MIX_PATCHLEVEL)
#endif

/* This macro will evaluate to true if compiled with SDL_mixer at least X.Y.Z */
#ifndef SDL_MIXER_VERSION_ATLEAST
#define SDL_MIXER_VERSION_ATLEAST(X, Y, Z) \
	(SDL_MIXER_COMPILEDVERSION >= SDL_VERSIONNUM(X, Y, Z))
#endif

#ifdef HAVE_PNG /// TODO: compile with zlib support without libpng

#define HAVE_ZLIB

#ifndef _MSC_VER
#ifndef _WII
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#endif

#ifndef _LFS64_LARGEFILE
#define _LFS64_LARGEFILE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#endif

#include "zlib.h"
#endif
#endif

UINT8 sound_started = false;

static boolean midimode;
static Mix_Music *music;
static UINT8 music_volume, midi_volume, sfx_volume;
static float loop_point;

void I_StartupSound(void)
{
	I_Assert(!sound_started);
	sound_started = true;

	midimode = false;
	music = NULL;
	music_volume = midi_volume = sfx_volume = 0;

	Mix_Init(MIX_INIT_FLAC|MIX_INIT_MOD|MIX_INIT_MP3|MIX_INIT_OGG);
	Mix_OpenAudio(44100, AUDIO_S16LSB, 2, 2048);
	Mix_AllocateChannels(256);
}

void I_ShutdownSound(void)
{
	I_Assert(sound_started);
	sound_started = false;

	Mix_CloseAudio();
	Mix_Quit();
}

void I_UpdateSound(void)
{
}

// this is as fast as I can possibly make it.
// sorry. more asm needed.
static Mix_Chunk *ds2chunk(void *stream)
{
	UINT16 ver,freq;
	UINT32 samples, i, newsamples;
	UINT8 *sound;

	SINT8 *s;
	INT16 *d;
	INT16 o;
	fixed_t step, frac;

	// lump header
	ver = READUINT16(stream); // sound version format?
	if (ver != 3) // It should be 3 if it's a doomsound...
		return NULL; // onos! it's not a doomsound!
	freq = READUINT16(stream);
	samples = READUINT32(stream);

	// convert from signed 8bit ???hz to signed 16bit 44100hz.
	switch(freq)
	{
	case 44100:
		if (samples >= UINT32_MAX>>2)
			return NULL; // would wrap, can't store.
		newsamples = samples;
		break;
	case 22050:
		if (samples >= UINT32_MAX>>3)
			return NULL; // would wrap, can't store.
		newsamples = samples<<1;
		break;
	case 11025:
		if (samples >= UINT32_MAX>>4)
			return NULL; // would wrap, can't store.
		newsamples = samples<<2;
		break;
	default:
		frac = (44100 << FRACBITS) / (UINT32)freq;
		if (!(frac & 0xFFFF)) // other solid multiples (change if FRACBITS != 16)
			newsamples = samples * (frac >> FRACBITS);
		else // strange and unusual fractional frequency steps, plus anything higher than 44100hz.
			newsamples = FixedMul(frac, samples) + 1; // add 1 sample for security! the code below rounds up.
		if (newsamples >= UINT32_MAX>>2)
			return NULL; // would and/or did wrap, can't store.
		break;
	}
	sound = Z_Malloc(newsamples<<2, PU_SOUND, NULL); // samples * frequency shift * bytes per sample * channels

	s = (SINT8 *)stream;
	d = (INT16 *)sound;

	i = 0;
	switch(freq)
	{
	case 44100: // already at the same rate? well that makes it simple.
		while(i++ < samples)
		{
			o = ((INT16)(*s++)+0x80)<<8; // changed signedness and shift up to 16 bits
			*d++ = o; // left channel
			*d++ = o; // right channel
		}
		break;
	case 22050: // unwrap 2x
		while(i++ < samples)
		{
			o = ((INT16)(*s++)+0x80)<<8; // changed signedness and shift up to 16 bits
			*d++ = o; // left channel
			*d++ = o; // right channel
			*d++ = o; // left channel
			*d++ = o; // right channel
		}
		break;
	case 11025: // unwrap 4x
		while(i++ < samples)
		{
			o = ((INT16)(*s++)+0x80)<<8; // changed signedness and shift up to 16 bits
			*d++ = o; // left channel
			*d++ = o; // right channel
			*d++ = o; // left channel
			*d++ = o; // right channel
			*d++ = o; // left channel
			*d++ = o; // right channel
			*d++ = o; // left channel
			*d++ = o; // right channel
		}
		break;
	default: // convert arbitrary hz to 44100.
		step = 0;
		frac = ((UINT32)freq << FRACBITS) / 44100;
		while (i < samples)
		{
			o = (INT16)(*s+0x80)<<8; // changed signedness and shift up to 16 bits
			while (step < FRACUNIT) // this is as fast as I can make it.
			{
				*d++ = o; // left channel
				*d++ = o; // right channel
				step += frac;
			}
			do {
				i++; s++;
				step -= FRACUNIT;
			} while (step >= FRACUNIT);
		}
		break;
	}

	// return Mixer Chunk.
	return Mix_QuickLoad_RAW(sound, (UINT8*)d-sound);
}

void *I_GetSfx(sfxinfo_t *sfx)
{
	void *lump;
	Mix_Chunk *chunk;

	if (sfx->lumpnum == LUMPERROR)
		sfx->lumpnum = S_GetSfxLumpNum(sfx);
	sfx->length = W_LumpLength(sfx->lumpnum);

	lump = W_CacheLumpNum(sfx->lumpnum, PU_SOUND);

	// convert from standard DoomSound format.
	chunk = ds2chunk(lump);
	if (chunk)
	{
		Z_Free(lump);
		return chunk;
	}

	// Try to load it as a WAVE or OGG using Mixer.
	return Mix_LoadWAV_RW(SDL_RWFromMem(lump, sfx->length), 1);
}

void I_FreeSfx(sfxinfo_t *sfx)
{
	if (sfx->data)
		Mix_FreeChunk(sfx->data);
	sfx->data = NULL;
}

INT32 I_StartSound(sfxenum_t id, INT32 vol, INT32 sep, INT32 pitch, INT32 priority)
{
	UINT8 volume = (((UINT16)vol + 1) * (UINT16)sfx_volume) / 62; // (256 * 31) / 62 == 127
	INT32 handle = Mix_PlayChannel(-1, S_sfx[id].data, 0);
	Mix_Volume(handle, volume);
	Mix_SetPanning(handle, min((UINT16)sep<<1, 0xff), min((UINT16)(0xff-sep)<<1, 0xff));
	(void)pitch; // Mixer can't handle pitch
	(void)priority; // priority and channel management is handled by SRB2...
	return handle;
}

void I_StopSound(INT32 handle)
{
	Mix_HaltChannel(handle);
}

INT32 I_SoundIsPlaying(INT32 handle)
{
	return Mix_Playing(handle);
}

void I_UpdateSoundParams(INT32 handle, INT32 vol, INT32 sep, INT32 pitch)
{
	UINT8 volume = (((UINT16)vol + 1) * (UINT16)sfx_volume) / 62; // (256 * 31) / 62 == 127
	Mix_Volume(handle, volume);
	Mix_SetPanning(handle, min((UINT16)sep<<1, 0xff), min((UINT16)(0xff-sep)<<1, 0xff));
	(void)pitch;
}

void I_SetSfxVolume(INT32 volume)
{
	sfx_volume = volume;
}

//
// Music
//

// Music hooks
static void music_loop(void)
{
	Mix_PlayMusic(music, 0);
	Mix_SetMusicPosition(loop_point);
}

void I_InitMusic(void)
{
}

void I_ShutdownMusic(void)
{
	I_ShutdownDigMusic();
	I_ShutdownMIDIMusic();
}

void I_PauseSong(INT32 handle)
{
	(void)handle;
	Mix_PauseMusic();
}

void I_ResumeSong(INT32 handle)
{
	(void)handle;
	Mix_ResumeMusic();
}

//
// Digital Music
//

void I_InitDigMusic(void)
{
}

void I_ShutdownDigMusic(void)
{
	if (midimode)
		return;
	if (!music)
		return;
	Mix_HookMusicFinished(NULL);
	Mix_FreeMusic(music);
	music = NULL;
}

boolean I_StartDigSong(const char *musicname, INT32 looping)
{
	char *data;
	size_t len;
	lumpnum_t lumpnum = W_CheckNumForName(va("O_%s",musicname));

	I_Assert(!music);

	if (lumpnum == LUMPERROR)
	{
		lumpnum = W_CheckNumForName(va("D_%s",musicname));
		if (lumpnum == LUMPERROR)
			return false;
		midimode = true;
	}
	else
		midimode = false;

	data = (char *)W_CacheLumpNum(lumpnum, PU_MUSIC);
	len = W_LumpLength(lumpnum);

	SDL_RWops *rw = SDL_RWFromMem(data, (int)len);
	music = Mix_LoadMUS_RW(rw, 1);
	if (!music)
	{
		CONS_Printf("Mix_LoadMUS_RW: %s\n", Mix_GetError());
		return true;
	}

	// Find the OGG loop point.
	loop_point = 0.0f;
	if (looping)
	{
		const char *key1 = "LOOP";
		const char *key2 = "POINT=";
		const char *key3 = "MS=";
		const UINT8 key1len = strlen(key1);
		const UINT8 key2len = strlen(key2);
		const UINT8 key3len = strlen(key3);
		char *p = data;
		while ((UINT32)(p - data) < len)
		{
			if (strncmp(p++, key1, key1len))
				continue;
			p += key1len-1; // skip OOP (the L was skipped in strncmp)
			if (!strncmp(p, key2, key2len)) // is it LOOPPOINT=?
			{
				p += key2len; // skip POINT=
				loop_point = (float)((44.1L+atoi(p)) / 44100.0L); // LOOPPOINT works by sample count.
				// because SDL_Mixer is USELESS and can't even tell us
				// something simple like the frequency of the streaming music,
				// we are unfortunately forced to assume that ALL MUSIC is 44100hz.
				// This means a lot of tracks that are only 22050hz for a reasonable downloadable file size will loop VERY badly.
			}
			else if (!strncmp(p, key3, key3len)) // is it LOOPMS=?
			{
				p += key3len; // skip MS=
				loop_point = atoi(p) / 1000.0L; // LOOPMS works by real time, as miliseconds.
				// Everything that uses LOOPMS will work perfectly with SDL_Mixer.
			}
			// Neither?! Continue searching.
		}
	}

	if (Mix_PlayMusic(music, looping && loop_point == 0.0f ? -1 : 0) == -1)
	{
		CONS_Printf("Mix_PlayMusic: %s\n", Mix_GetError());
		return true;
	}
	if (midimode)
		Mix_VolumeMusic((UINT32)midi_volume*128/31);
	else
		Mix_VolumeMusic((UINT32)music_volume*128/31);

	if (loop_point != 0.0f)
		Mix_HookMusicFinished(music_loop);
	return true;
}

void I_StopDigSong(void)
{
	if (midimode)
		return;
	if (!music)
		return;
	Mix_HookMusicFinished(NULL);
	Mix_FreeMusic(music);
	music = NULL;
}

void I_SetDigMusicVolume(INT32 volume)
{
	music_volume = volume;
	if (midimode || !music)
		return;
	Mix_VolumeMusic((UINT32)volume*128/31);
}

boolean I_SetSongSpeed(float speed)
{
	if (speed > 250.0f)
		speed = 250.0f; //limit speed up to 250x
	(void)speed;
	return false;
}

//
// MIDI Music
//

void I_InitMIDIMusic(void)
{
}

void I_ShutdownMIDIMusic(void)
{
	if (!midimode || !music)
		return;
	Mix_FreeMusic(music);
	music = NULL;
}

void I_SetMIDIMusicVolume(INT32 volume)
{
	midi_volume = volume;
	if (!midimode || !music)
		return;
	Mix_VolumeMusic((UINT32)volume*128/31);
}

INT32 I_RegisterSong(void *data, size_t len)
{
	SDL_RWops *rw = SDL_RWFromMem(data, (int)len);
	music = Mix_LoadMUS_RW(rw, 1);
	if (!music)
	{
		CONS_Printf("Mix_LoadMUS_RW: %s\n", Mix_GetError());
		return -1;
	}
	return 1337;
}

boolean I_PlaySong(INT32 handle, INT32 looping)
{
	(void)handle;

	midimode = true;

	if (Mix_PlayMusic(music, looping ? -1 : 0) == -1)
	{
		CONS_Printf("Mix_PlayMusic: %s\n", Mix_GetError());
		return false;
	}
	Mix_VolumeMusic((UINT32)music_volume*128/31);
	return true;
}

void I_StopSong(INT32 handle)
{
	if (!midimode || !music)
		return;

	(void)handle;
	Mix_HaltMusic();
}

void I_UnRegisterSong(INT32 handle)
{
	if (!midimode || !music)
		return;

	(void)handle;
	Mix_FreeMusic(music);
	music = NULL;
}

#endif