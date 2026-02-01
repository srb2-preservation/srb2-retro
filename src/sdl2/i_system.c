#include "../console.h"
#include "../g_input.h"
#include "../g_state.h"
#include "../d_clisrv.h"
#include "../d_main.h"
#include "../d_netcmd.h"
#include "../doomdef.h"
#include "../m_argv.h"
#include "../m_misc.h"
#include "../screen.h"
#include "../keys.h"
#include "../i_sound.h"
#include "../i_system.h"
#include "../i_video.h"

#include "i_main.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_version.h>
#include <SDL2/SDL_keycode.h>

#include <unistd.h>
#include <termios.h>
#include <signal.h>

static struct termios tty_tc;
static struct termios tty_con;
static int tty_erase;
static int tty_eof;
static int consolevent = 1;
static int framebuffer;

UINT8 graphics_started = 0;

UINT8 keyboard_started = 0;

UINT32 I_GetFreeMem(UINT32 *total)
{
	total = NULL;
	return 0;
}

#ifdef _WIN32
static long    hacktics = 0;       //faB: used locally for keyboard repeat keys
static DWORD starttickcount = 0; // hack for win2k time bug
tic_t I_GetTime(void)
{
	int newtics = 0;

	if (!starttickcount) // high precision timer
	{
		LARGE_INTEGER currtime; // use only LowPart if high resolution counter is not available
		static LARGE_INTEGER basetime = { {0, 0} };

		// use this if High Resolution timer is found
		static LARGE_INTEGER frequency;

		if (!basetime.LowPart)
		{
			if (!QueryPerformanceFrequency(&frequency))
				frequency.QuadPart = 0;
			else
				QueryPerformanceCounter(&basetime);
		}

		if (frequency.LowPart && QueryPerformanceCounter(&currtime))
		{
			newtics = (INT32)((currtime.QuadPart - basetime.QuadPart) * TICRATE
				/ frequency.QuadPart);
		}
	}
	else
		newtics = (GetTickCount() - starttickcount) / (1000 / TICRATE);

	return newtics;
}
#else
tic_t I_GetTime(void)
{
	tic_t ticks = SDL_GetTicks();

	ticks = (ticks*TICRATE);

	ticks = (ticks/1000);

	return ticks;
}
#endif

void I_Sleep(void){}

void I_DoStartupMouse(void) {}

void I_GetEvent(void){
	event_t e_w;
	SDL_Event e_s;
	// This exists so we can reuse most of the keydown event code for the keyup event
	boolean up = false;
		while (SDL_PollEvent(&e_s)) {
			up = false;
			memset(&e_w,0x00,sizeof(e_w));
			switch (e_s.type) {
				case SDL_QUIT:
					I_Quit();
					break;
				case SDL_KEYUP:
					e_w.type = ev_keyup;
					up = true;
				case SDL_KEYDOWN:
					if (!up)
						e_w.type = ev_keydown;

					switch (e_s.key.keysym.sym) {
						// arrows
						case SDLK_UP:
							e_w.data1 = KEY_UPARROW;
							break;
						case SDLK_DOWN:
							e_w.data1 = KEY_DOWNARROW;
							break;
						case SDLK_RIGHT:
							e_w.data1 = KEY_RIGHTARROW;
							break;
						case SDLK_LEFT:
							e_w.data1 = KEY_LEFTARROW;
							break;
						// modifer keys
						case SDLK_LSHIFT:
						case SDLK_RSHIFT:
							e_w.data1 = KEY_LSHIFT;
							break;
						case SDLK_LCTRL:
						case SDLK_RCTRL:
							e_w.data1 = KEY_RCTRL;
							break;
						case SDLK_LALT:
                        case SDLK_RALT:
							e_w.data1 = KEY_LALT;
							break;
						// function keys
						case SDLK_F1:
							e_w.data1 = KEY_F1;
							break;
						case SDLK_F2:
							e_w.data1 = KEY_F2;
							break;
						case SDLK_F3:
							e_w.data1 = KEY_F3;
							break;
						case SDLK_F4:
							e_w.data1 = KEY_F4;
							break;
						case SDLK_F5:
							e_w.data1 = KEY_F5;
							break;
						case SDLK_F6:
							e_w.data1 = KEY_F6;
							break;
						case SDLK_F7:
							e_w.data1 = KEY_F7;
							break;
						case SDLK_F8:
							e_w.data1 = KEY_F8;
							break;
						case SDLK_F9:
							e_w.data1 = KEY_F9;
							break;
						case SDLK_F10:
							e_w.data1 = KEY_F10;
							break;
						case SDLK_F11:
							e_w.data1 = KEY_F11;
							break;
						case SDLK_F12:
							e_w.data1 = KEY_F12;
							break;
						// Let Snipping Tool/Snip 'n Sketch/Greenshot do whatever! 
						case SDLK_PRINTSCREEN:
							return;
						// those keys to the right of the regular QWERTY layout but to the left of the numpad idk what to call them
						case SDLK_SCROLLLOCK:
							e_w.data1 = KEY_SCROLLLOCK;
							break;
						case SDLK_PAUSE:
							e_w.data1 = KEY_PAUSE;
							break;
						case SDLK_INSERT:
							e_w.data1 = KEY_INS;
							break;
						case SDLK_HOME:
							e_w.data1 = KEY_HOME;
							break;
						case SDLK_PAGEUP:
							e_w.data1 = KEY_PGUP;
							break;
						case SDLK_DELETE:
							e_w.data1 = KEY_DEL;
							break;
						case SDLK_END:
							e_w.data1 = KEY_END;
							break;
						case SDLK_PAGEDOWN:
							e_w.data1 = KEY_PGDN;
							break;
						// numpad keys
						case SDLK_KP_0:
							e_w.data1 = KEY_KEYPAD0;
							break;
						case SDLK_KP_1:
							e_w.data1 = KEY_KEYPAD1;
							break;
						case SDLK_KP_2:
							e_w.data1 = KEY_KEYPAD2;
							break;
						case SDLK_KP_3:
							e_w.data1 = KEY_KEYPAD3;
							break;
						case SDLK_KP_4:
							e_w.data1 = KEY_KEYPAD4;
							break;
						case SDLK_KP_5:
							e_w.data1 = KEY_KEYPAD5;
							break;
						case SDLK_KP_6:
							e_w.data1 = KEY_KEYPAD6;
							break;
						case SDLK_KP_7:
							e_w.data1 = KEY_KEYPAD7;
							break;
						case SDLK_KP_8:
							e_w.data1 = KEY_KEYPAD8;
							break;
						case SDLK_KP_9:
							e_w.data1 = KEY_KEYPAD9;
							break;
						case SDLK_KP_PLUS:
							e_w.data1 = KEY_PLUSPAD;
							break;
						case SDLK_KP_MINUS:
							e_w.data1 = KEY_MINUSPAD;
							break;
						case SDLK_KP_PERIOD:
							e_w.data1 = KEY_KPADDEL;
							break;
						// misc keys
						case SDLK_MENU:
							e_w.data1 = KEY_MENU;
						default:
							e_w.data1 = e_s.key.keysym.sym;
							break;
					}

					// For when I inevitably come back to this
					//CONS_Printf("Virtual key code: 0x%02X (%c)\n", e_s.key.keysym.sym, e_s.key.keysym.sym);
					//CONS_Printf("Physical key code: 0x%02X (%c)\n", e_s.key.keysym.scancode, e_s.key.keysym.scancode);
					D_PostEvent(&e_w);
					break;
				case SDL_MOUSEBUTTONDOWN:
					e_w.type = ev_keydown;
					e_w.data1 = KEY_MOUSE1 + e_s.button.button;
					D_PostEvent(&e_w);
					break;
				case SDL_MOUSEBUTTONUP:
					e_w.type = ev_keyup;
					e_w.data1 = KEY_MOUSE1 + e_s.button.button;
					D_PostEvent(&e_w);
					break;
				case SDL_MOUSEMOTION:
					e_w.type = ev_mouse;
					e_w.data2 = e_s.motion.xrel * 2;
					e_w.data3 = e_s.motion.yrel * -1; // Invert
					D_PostEvent(&e_w);
					break;
				default:
					break;
			}
		}
}

void I_OsPolling(void){
	event_t e_w;

	I_GetEvent();
}

static ticcmd_t emptycmd;
ticcmd_t *I_BaseTiccmd(void)
{
	return &emptycmd;
}

static ticcmd_t emptycmd2;
ticcmd_t *I_BaseTiccmd2(void)
{
	return &emptycmd2;
}

// never exit without calling this, or your terminal will be left in a pretty bad state
static void I_ShutdownConsole(void)
{
	if (consolevent)
	{
		I_OutputMsg("Shutdown tty console\n");
		consolevent = SDL_FALSE;
		tcsetattr (STDIN_FILENO, TCSADRAIN, &tty_tc);
	}
}

static void I_StartupConsole(void)
{
	struct termios tc;

	// TTimo
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=390 (404)
	// then SIGTTIN or SIGTOU is emitted, if not catched, turns into a SIGSTP
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

#if !defined(GP2X) //read is bad on GP2X
	consolevent = !M_CheckParm("-noconsole");
#endif
	framebuffer = M_CheckParm("-framebuffer");

	if (framebuffer)
		consolevent = SDL_FALSE;

	if (!consolevent) return;

	if (isatty(STDIN_FILENO)!=1)
	{
		I_OutputMsg("stdin is not a tty, tty console mode failed\n");
		consolevent = SDL_FALSE;
		return;
	}
	memset(&tty_con, 0x00, sizeof(tty_con));
	tcgetattr (0, &tty_tc);
	tty_erase = tty_tc.c_cc[VERASE];
	tty_eof = tty_tc.c_cc[VEOF];
	tc = tty_tc;
	/*
	 ECHO: don't echo input characters
	 ICANON: enable canonical mode.  This  enables  the  special
	  characters  EOF,  EOL,  EOL2, ERASE, KILL, REPRINT,
	  STATUS, and WERASE, and buffers by lines.
	 ISIG: when any of the characters  INTR,  QUIT,  SUSP,  or
	  DSUSP are received, generate the corresponding signal
	*/
	tc.c_lflag &= ~(ECHO | ICANON);
	/*
	 ISTRIP strip off bit 8
	 INPCK enable input parity checking
	 */
	tc.c_iflag &= ~(ISTRIP | INPCK);
	tc.c_cc[VMIN] = 0; //1?
	tc.c_cc[VTIME] = 0;
	tcsetattr (0, TCSADRAIN, &tc);
}

void I_Quit(void)
{
	exit(0);
}

void I_Error(const char *error, ...)
{
	int len;
	char* buffer;

	va_list args;
	va_start(args, error);

	len = vsnprintf(NULL, 0, error, args);
	va_end(args);

	buffer = (char*)malloc(len + 1);

	va_start(args, error);
	vsnprintf(buffer, len + 1, error, args);
	va_end(args);

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SRB2 Error", buffer, NULL);

	M_SaveConfig(NULL);
	D_QuitNetGame();
	I_ShutdownGraphics();
	I_ShutdownSound();
	I_ShutdownMusic();
	I_ShutdownConsole();
	I_ShutdownSystem();
	exit(-1);
}

UINT8 *I_AllocLow(int length)
{
	length = 0;
	return NULL;
}

void I_Tactile(FFType Type, const JoyFF_t *Effect)
{
	Type = 0;
	Effect = NULL;
}

void I_Tactile2(FFType Type, const JoyFF_t *Effect)
{
	Type = 0;
	Effect = NULL;
}

void I_JoyScale(void){}

void I_JoyScale2(void){}

void I_InitJoystick(void){}

void I_InitJoystick2(void){}

int I_NumJoys(void)
{
	return 0;
}

const char *I_GetJoyName(int joyindex)
{
	joyindex = 0;
	return NULL;
}

void I_OutputMsg(const char *error, ...)
{
	int len;
	char* buffer;

	va_list args;
	va_start(args, error);

	len = vsnprintf(NULL, 0, error, args);
	va_end(args);

	buffer = (char*)malloc(len + 1);

	va_start(args, error);
	vsnprintf(buffer, len + 1, error, args);
	va_end(args);

	//SDL_Log(buffer);
}
void I_StartupMouse(void){
	if (cv_usemouse.value != 0) {
		//SDL_SetRelativeMouseMode(SDL_TRUE);
	} else {
		//SDL_SetRelativeMouseMode(SDL_FALSE);
	}
}

void I_StartupMouse2(void){}

void I_StartupKeyboard(void){}

int I_GetKey(void)
{
	return 0;
}

void I_LoadingScreen(const char* msg)
{
	//SDL_Log(msg);
}


void I_StartupTimer(void){}

void I_AddExitFunc(void (*func)())
{
	func = NULL;
}

void I_RemoveExitFunc(void (*func)())
{
	func = NULL;
}

INT32 I_StartupSystem(void)
{
	SDL_version SDLcompiled;
	const SDL_version *SDLlinked;
#ifdef _XBOX
#ifdef __GNUC__
	char DP[] ="      Sonic Robo Blast 2!\n";
	debugPrint(DP);
#endif
	unlink("e:/Games/SRB2/stdout.txt");
	freopen("e:/Games/SRB2/stdout.txt", "w+", stdout);
	unlink("e:/Games/SRB2/stderr.txt");
	freopen("e:/Games/SRB2/stderr.txt", "w+", stderr);
#endif
#ifdef _arch_dreamcast
#ifdef _DEBUG
	//gdb_init();
#endif
	printf(__FILE__":%i\n",__LINE__);
#ifdef _DEBUG
	//gdb_breakpoint();
#endif
#endif
	SDL_VERSION(&SDLcompiled)
	I_StartupConsole();
	CONS_Printf("Compiled for SDL version: %d.%d.%d\n",
	 SDLcompiled.major, SDLcompiled.minor, SDLcompiled.patch);
	CONS_Printf("Linked with SDL version: %d.%d.%d\n",
	 SDLlinked->major, SDLlinked->minor, SDLlinked->patch);
#if 0 //#ifdef GP2X //start up everything
	if (SDL_Init(SDL_INIT_NOPARACHUTE|SDL_INIT_EVERYTHING) < 0)
#else
	if (SDL_Init(SDL_INIT_NOPARACHUTE) < 0)
#endif
		I_Error("SRB2: SDL System Error: %s", SDL_GetError()); //Alam: Oh no....
	return 0;
}

void I_ShutdownSystem(void){
	SDL_RWclose(logstream);
}

void I_GetDiskFreeSpace(INT64* freespace)
{
	freespace = NULL;
}

char *I_GetUserName(void)
{
	return NULL;
}

// stolen from sdl1
int I_mkdir(const char *dirname, int unixright)
{
//[segabor]
#if defined (UNIXLIKE) || defined (__CYGWIN__) || defined (__OS2__) || (defined (_XBOX) && defined (__GNUC__))
	return mkdir(dirname, unixright);
#elif (defined (_WIN32) || (defined (_WIN32_WCE) && !defined (__GNUC__)) || defined (_WIN64)) && !defined (_XBOX)
	unixright = 0; /// \todo should implement ntright under nt...
	return CreateDirectoryA(dirname, NULL);
#else
	dirname = NULL;
	unixright = 0;
	return false;
#endif
}

UINT64 I_FileSize(const char *filename)
{
	filename = NULL;
	return (UINT64)-1;
}

const CPUInfoFlags *I_CPUInfo(void)
{
	return NULL;
}

const char *I_LocateWad(void)
{
	return NULL;
}

void I_GetJoystickEvents(void){}

void I_GetJoystick2Events(void){}

void I_GetMouseEvents(void){}

char *I_GetEnv(const char *name) // todo: make this work
{
	(void)name;
	return NULL;
}

int I_PutEnv(char *variable)
{
	variable = NULL;
	return -1;
}