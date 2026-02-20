#ifndef _WIN32
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#else
#include <SDL.h>
#include <SDL_error.h>
#include <SDL_pixels.h>
#include <SDL_mouse.h>
#include <SDL_render.h>
#include <SDL_surface.h>
#include <SDL_video.h>
#endif

#include "../command.h"
#include "../doomdef.h"
#include "../doomstat.h"
#include "../d_netcmd.h"
#include "../i_system.h"
#include "../i_video.h"
#include "../m_argv.h"
#include "../r_main.h"
#include "../screen.h"
#include "../v_video.h"
#include "../z_zone.h"

#include "i_video.h"

#include <SDL2/SDL_image.h>
#if !(defined (DC) || defined (_WIN32_WCE) || defined (PSP) || defined(GP2X))
#define LOAD_XPM //I want XPM!
#define HAVE_IMAGE //I have SDL_Image, sortof

#include "SDL_icon.xpm"
#endif

static SDL_bool disable_fullscreen = SDL_FALSE;
#define USE_FULLSCREEN (disable_fullscreen||!allow_fullscreen)?0:cv_fullscreen.value

static SDL_bool disable_mouse = SDL_FALSE;
static Uint16 realwidth = BASEVIDWIDTH;
static Uint16 realheight = BASEVIDHEIGHT;
static SDL_bool windownnow = SDL_FALSE;
#define USE_MOUSEINPUT (!disable_mouse && cv_usemouse.value && SDL_GetAppState() & SDL_APPACTIVE)
#define MOUSE_MENU (!disable_mouse && cv_usemouse.value && menuactive && !USE_FULLSCREEN && !windownnow)
#define HalfWarpMouse(win,x,y) SDL_WarpMouseInWindow(win,(Uint16)(x/2),(Uint16)(y/2))

rendermode_t rendermode = render_soft;

boolean highcolor = false;

boolean allow_fullscreen = false;

static SDL_bool mousegrabok = SDL_FALSE;

static SDL_Surface *icoSurface = NULL;

consvar_t cv_vidwait = {"vid_wait", "On", CV_SAVE, CV_OnOff, NULL, 0, "On", NULL, 0, 0, NULL};

// Stores the current mode
int currentmode;

// Various unction protos
void I_ToggleFullscreen(void);
boolean VID_InitConsole(void);
void VID_Command_Vidmode(void);
void VID_Command_Listmodes(void);

SDL_Window* SDL_window = NULL;

SDL_Renderer* SDL_renderer;
SDL_Surface* surface;
SDL_Surface* window_surface;
SDL_Surface* icon_surface;

// todo: un-hardcode this
#define NUM_SDLMODES 11

int i;

vmode_t window_modes[NUM_SDLMODES] = {
		// Fallback mode, 320x200 is gross
		{
			NULL,
			"320x200", //faB: W to make sure it's the windowed mode
			320, 200,   //(200.0/320.0)*(320.0/240.0),
			320, 1,     // rowbytes, bytes per pixel
			1, 2,       // windowed (TRUE), numpages
			NULL,
			NULL,
			0          // misc
		},
		// Non-fallback copy of 320x200W, if you WANT to use 320x200W for some reason
		{
			NULL,
			"320x200", //faB: W to make sure it's the windowed mode
			320, 200,   //(200.0/320.0)*(320.0/240.0),
			320, 1,     // rowbytes, bytes per pixel
			1, 2,       // windowed (TRUE), numpages
			NULL,
			NULL,
			0          // misc
		},
		{
			NULL,
			"320x240", //faB: W to make sure it's the windowed mode
			320, 240,   //(200.0/320.0)*(320.0/240.0),
			320, 1,     // rowbytes, bytes per pixel
			1, 2,       // windowed (TRUE), numpages
			NULL,
			NULL,
			0          // misc
		},
		{
			NULL,
			"640x400", //faB: W to make sure it's the windowed mode
			640, 400,   //(200.0/320.0)*(320.0/240.0),
			640, 1,     // rowbytes, bytes per pixel
			1, 2,       // windowed (TRUE), numpages
			NULL,
			NULL,
			0          // misc
		},
		{
			NULL,
			"640x480", //faB: W to make sure it's the windowed mode
			640, 480,   //(200.0/320.0)*(320.0/240.0),
			640, 1,     // rowbytes, bytes per pixel
			1, 2,       // windowed (TRUE), numpages
			NULL,
			NULL,
			0          // misc
		},
		{
			NULL,
			"800x600", //faB: W to make sure it's the windowed mode
			800, 600,   //(200.0/320.0)*(320.0/240.0),
			800, 1,     // rowbytes, bytes per pixel
			1, 2,       // windowed (TRUE), numpages
			NULL,
			NULL,
			0          // misc
		},
		{
			NULL,
			"1024x768", //faB: W to make sure it's the windowed mode
			1024, 768,   //(200.0/320.0)*(320.0/240.0),
			1024, 1,     // rowbytes, bytes per pixel
			1, 2,       // windowed (TRUE), numpages
			NULL,
			NULL,
			0          // misc
		},
		{
			NULL,
			"1280x720", //faB: W to make sure it's the windowed mode
			1280, 720,   //(200.0/320.0)*(320.0/240.0),
			1280, 1,     // rowbytes, bytes per pixel
			1, 2,       // windowed (TRUE), numpages
			NULL,
			NULL,
			0          // misc
		},
		{
			NULL,
			"1280x800", //faB: W to make sure it's the windowed mode
			1280, 800,   //(200.0/320.0)*(320.0/240.0),
			1280, 1,     // rowbytes, bytes per pixel
			1, 2,       // windowed (TRUE), numpages
			NULL,
			NULL,
			0          // misc
		},
		{
			NULL,
			"1920x1080", //faB: W to make sure it's the windowed mode
			1920, 1080,   //(200.0/320.0)*(320.0/240.0),
			1920, 1,     // rowbytes, bytes per pixel
			1, 2,       // windowed (TRUE), numpages
			NULL,
			NULL,
			0          // misc
		},
		{
			NULL,
			"1920x1200", //faB: W to make sure it's the windowed mode
			1920, 1200,   //(200.0/320.0)*(320.0/240.0),
			1920, 1,     // rowbytes, bytes per pixel
			1, 2,       // windowed (TRUE), numpages
			NULL,
			NULL,
			0          // misc
		},
};

void I_ToggleFullscreen(void) {
	VID_SetMode(currentmode);
}

static void SDLdoGrabMouse(void)
{
	if (SDL_FALSE == SDL_SetRelativeMouseMode(SDL_QUERY))
	{
		if (mousegrabok == SDL_TRUE)
			SDL_SetRelativeMouseMode(SDL_TRUE);
	}
}

void SDLdoUngrabMouse(void)
{
	if (SDL_TRUE == SDL_SetRelativeMouseMode(SDL_QUERY))
		SDL_SetRelativeMouseMode(SDL_FALSE);
}

void SDLforceUngrabMouse(void)
{
	if (SDL_WasInit(SDL_INIT_VIDEO)==SDL_INIT_VIDEO)
		SDL_SetRelativeMouseMode(SDL_FALSE);
}

void I_ShutdownGraphics(void){
	CONS_Printf("I_ShutdownGraphics...\n");
	SDL_DestroyRenderer(SDL_renderer);
	SDL_DestroyWindow(SDL_window);
	SDL_FreeSurface(surface);
	free(vid.buffer);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	graphics_started = false;
}

void I_StartupMouse(void)
{
	static SDL_bool firsttimeonmouse = SDL_TRUE;

	if (disable_mouse)
		return;

	if (!firsttimeonmouse)
		HalfWarpMouse(NULL, realwidth, realheight); // warp to center
	else
		firsttimeonmouse = SDL_FALSE;
	
	if (cv_usemouse.value)
		SDLdoGrabMouse();
	else
		SDLdoUngrabMouse();
}

SDL_Color palettebuf[256];

// Translate Doom palette into SDL palette
void I_SetPalette(RGBA_t *palette)
{
	UINT8 color;
	// 256 colors * 3 color channels
	for (i=0; i<256; i++)
	{
		palettebuf[i].r = palette[i].s.red;
		palettebuf[i].g = palette[i].s.green;
		palettebuf[i].b = palette[i].s.blue;
		//if (i == 9)
			//I_Error("byte 1: %d, byte 2: %d, byte 3: %d, byte 4: %d\n",, , , );

		SDL_SetPaletteColors(surface->format->palette, palettebuf, 0, 256);
	}
}

int VID_NumModes(void)
{
	// TODO: Find a way to get length of windowed_modes and return that
	return NUM_SDLMODES;
}

int VID_GetModeForSize(int w, int h)
{
	int diffx, diffy;
	int bestdiff, bestmode;

	bestdiff = 9999;

	// Iterate through (accessible) modes
	for (i = 1; i < NUM_SDLMODES; i++) {

		// Get difference between current window mode's width and our requested width
		diffx = window_modes[i].width - w;
		if (diffx < 0) {
			diffx *= -1;
		}

		// Get difference between current window mode's height and our requested height
		diffy = window_modes[i].height - h;
		if (diffy < 0) {
			diffy *= -1;
		}

		// If the mode has the exact coords of our dimensions, use that!
		if (diffx == 0 && diffy == 0)
			return i;

		// If our current mode is the closest found so far to requested dimensions, record it as our best candidate so far
		if (bestdiff > diffx + diffy) {
			bestdiff = diffx + diffy;
			bestmode = i;
		}
	}

	// Well, we didn't find any exact matches. Darn.
	// Oh well, that's what that whole closest match system was for!
	return bestmode;
}

void VID_PrepareModeList(void){
	allow_fullscreen = true;
}

// TODO: Some of the stuff done when we change modes might not be needed. See how much we can keep between mode switches (For example, we might just be able to change the window's dimensions instead of destroying and remaking it)
int VID_SetMode(int modenum)
{
	int flags;

	SDLdoUngrabMouse();

	if (SDL_window != NULL)
		SDL_DestroyWindow(SDL_window);

	if (SDL_renderer != NULL)
		SDL_DestroyRenderer(SDL_renderer);

	if (surface != NULL)
		SDL_FreeSurface(surface);

	vid.modenum = modenum;
	vid.width = window_modes[modenum].width;
	vid.height = window_modes[modenum].height;
	vid.bpp = window_modes[modenum].bytesperpixel;
	vid.rowbytes = window_modes[modenum].rowbytes;
	vid.dupx = vid.width / 320;
	vid.dupy = vid.height / 200;
	vid.recalc = 1;

	if (USE_FULLSCREEN)
		flags = SDL_WINDOW_FULLSCREEN;
	else
		flags = 0;

	// Init window (hardcoded to 640x400 for now) in the center of the screen
	SDL_window = SDL_CreateWindow(va("SRB2 Retro"VERSIONSTRING, "SRB2"), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, vid.width, vid.height, flags);

#ifdef HAVE_IMAGE
	//icoSurface = IMG_ReadXPMFromArray(SDL_icon_xpm);
#endif
	SDL_SetWindowIcon(SDL_window, icoSurface);

	if (!SDL_window)
		I_Error("VID_SetMode(): Could not create window!");

	SDL_renderer = SDL_CreateRenderer(SDL_window, -1, SDL_RENDERER_ACCELERATED);
	if (!SDL_renderer)
		I_Error("VID_SetMode(): Could not create renderer!");

	surface = SDL_CreateRGBSurfaceWithFormat(0, vid.width, vid.height, 8, SDL_PIXELFORMAT_INDEX8);

	window_surface = SDL_GetWindowSurface(SDL_window);

	// allocate buffer
	if (vid.buffer)
		free(vid.buffer);

	vid.buffer = malloc((vid.width * vid.height * vid.bpp * NUMSCREENS) + (vid.width * ST_HEIGHT * vid.bpp));

	// Set mode number accordingly
	currentmode = modenum;

	return 0;
}

void I_StartupGraphics(void) {
	if (M_CheckParm("-win"))
		CV_Set(&cv_fullscreen, "No");
		
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		I_Error("I_StartupGraphics(): Could not initialize SDL2: %s\n"/*, SDL_GetError()*/);
	
	if (VID_InitConsole())
		I_Error("I_StartupGraphics(): Could not initialize commands / console variables!\n");

#include <SDL2/SDL_video.h>
	surface = SDL_CreateRGBSurfaceWithFormat(0, vid.width, vid.height, 8, SDL_PIXELFORMAT_INDEX8);
	graphics_started = true;

	if (M_CheckParm("-nomousegrab"))
	{
		mousegrabok = SDL_FALSE;
		SDLdoUngrabMouse();
	}
	else
		mousegrabok = SDL_TRUE;
}

const char *VID_GetModeName(int modenum)
{
	return window_modes[modenum].name;
}

void I_UpdateNoBlit(void){}

#define SCALE      3
#define PUTDOT(xx,yy,cc) screens[0][((yy)*vid.width+(xx))*vid.bpp]=(cc)

static boolean ticsgraph[TICRATE];

static UINT32 fpstime = 0;
static UINT32 lastupdatetime = 0;

#define FPSUPDATERATE 1/20 
#define FPSMAXSAMPLES 16

static UINT32 fpssamples[FPSMAXSAMPLES];
static UINT32 fpssampleslen = 0;
static UINT32 fpssum = 0;
static double aproxfps = 0.0f;

void SCR_CalcAproxFps(void)
{
	tic_t i = 0;
	if (I_PreciseToMicros(fpstime - lastupdatetime) > 1000000 * FPSUPDATERATE)
	{
		if (fpssampleslen == FPSMAXSAMPLES)
		{
			fpssum -= fpssamples[0];

			for (i = 1; i < fpssampleslen; i++)
				fpssamples[i-1] = fpssamples[i];
		}
		else
			fpssampleslen++;

		fpssamples[fpssampleslen-1] = I_GetPreciseTime() - fpstime;
		fpssum += fpssamples[fpssampleslen-1];

		aproxfps = 1000000 / (I_PreciseToMicros(fpssum) / (double)fpssampleslen);

		lastupdatetime = I_GetPreciseTime();
	}

	fpstime = I_GetPreciseTime();
}

static void displayticrate(fixed_t value)
{
	int j,l,i;
	static tic_t lasttic;
	tic_t tics,t;
	tic_t ontic = I_GetTime();
	tic_t totaltics = 0;
	const INT32 h = vid.height-(8*vid.dupy);

	if (gamestate == GS_NULL)
		return;

	if (ticsgraph[i])
		++totaltics;

	t = I_GetTime();
	tics = (t - lasttic)/NEWTICRATERATIO;
	lasttic = t;
	if (tics > OLDTICRATE) tics = OLDTICRATE;

	for (i = lasttic + 1; i < TICRATE+lasttic && i < ontic; ++i)
	ticsgraph[i % TICRATE] = false;

	ticsgraph[ontic % TICRATE] = true;

	for (i=0;i<OLDTICRATE-1;i++)
		ticsgraph[i]=ticsgraph[i+1];
	ticsgraph[OLDTICRATE-1]=OLDTICRATE-tics;

	if (value == 1 || value == 3)
	{

		V_DrawString(vid.width- (80 * vid.dupx), h, V_NOSCALESTART,
			va("FPS:% 02.2f", aproxfps));
	}
	if (value == 1)
		return;

	lasttic = ontic;
}
#undef SCALE
#undef PUTDOT

void I_FinishUpdate(void)
{
	SCR_CalcAproxFps();

	if (cv_ticrate.value) // FPS counter
		displayticrate(cv_ticrate.value);

	UINT8 *pixels = surface->pixels;
	// Copy vid.buffer to our surface
	for (i = 0; i < vid.width * vid.height; i++) {
		pixels[i] = vid.buffer[i];
	}

	SDL_BlitSurface(surface, NULL, window_surface, NULL);
	SDL_UpdateWindowSurface(SDL_window);
}

void I_WaitVBL(int count)
{
	count = 0;
}

void I_ReadScreen(UINT8 *scr)
{
	SDL_memcpy(scr, vid.buffer, vid.width * vid.height * vid.bpp);
}

void I_BeginRead(void){}

void I_EndRead(void){}


// COMMAND STUFF

boolean VID_InitConsole(void) {

	// Register commands
	COM_AddCommand("videomode", VID_Command_Vidmode);
	COM_AddCommand("listmodes", VID_Command_Listmodes);

	return 0;
}

void VID_Command_Vidmode(void) {

	int modenum;

	if (COM_Argc() != 2) {
		CONS_Printf("videomode <mode number>: Changes the video mode to the specified one. Number must be between 1 and %d.\n", NUM_SDLMODES - 1);
		return;
	}

	modenum = atoi(COM_Argv(1));

	if (modenum < 1 || modenum > NUM_SDLMODES - 1) {
		CONS_Printf("Invalid video mode \"%d\"! Must be between 1 and %d.\n", modenum, NUM_SDLMODES - 1);
		return;
	} else {
		VID_SetMode(modenum);
	}

	I_StartupMouse();

	CONS_Printf("Changed to video mode %s (index %d).\n", VID_GetModeName(modenum), modenum);
}

void VID_Command_Listmodes(void) {
	for (i = 1; i < NUM_SDLMODES; i++) {
		CONS_Printf("Video mode %d: %s.\n", i, VID_GetModeName(i));
	}
}