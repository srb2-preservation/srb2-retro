#include "../doomdef.h"
#include "../d_main.h"
#include "../m_argv.h"
#ifndef _WIN32
#include <SDL2/SDL.h>
#include <SDL2/SDL_rwops.h>
#else
#include <SDL.h>
#include <SDL_rwops.h>
#endif

SDL_RWops* logstream = NULL;

int SDL_main(int argc, char **argv)
{
	myargc = argc;
	myargv = argv; /// \todo pull out path to exe from this string

	//int logstream



	logstream = SDL_RWFromFile("sdllog.txt", "a");
	if (!logstream)
		I_Error("Unable to get write access for log file. \n Are you running multiple instances of SRB2?");

	// startup SRB2
	CONS_Printf ("Setting up SRB2...\n");
	D_SRB2Main();
	CONS_Printf ("Entering main game loop...\n");
	// never return
	D_SRB2Loop();

	// return to OS
#ifndef __GNUC__
	return 0;
#endif
}

int main(int argc, char **argv)
{
    return SDL_main(argc, argv);
}