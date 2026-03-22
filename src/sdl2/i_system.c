#include "../console.h"
#include "../filesrch.h"
#include "../g_input.h"
#include "../g_state.h"
#include "../d_clisrv.h"
#include "../d_main.h"
#include "../d_netcmd.h"
#include "../doomdef.h"
#include "../doomstat.h"
#include "../m_argv.h"
#include "../m_misc.h"
#include "../screen.h"
#include "../keys.h"
#include "../i_sound.h"
#include "../i_system.h"
#include "../i_video.h"
#include "i_video.h"

#include "i_main.h"

#ifndef _WIN32
#include <SDL2/SDL.h>
#include <SDL2/SDL_version.h>
#include <SDL2/SDL_keycode.h>
#else
#include <SDL.h>
#include <SDL_version.h>
#include <SDL_keycode.h>
#endif

// Locations for searching the srb2.srb
#ifdef _arch_dreamcast
#define DEFAULTWADLOCATION1 "/cd"
#define DEFAULTWADLOCATION2 "/pc"
#define DEFAULTWADLOCATION3 "/pc/home/alam/srb2code/data"
#define DEFAULTSEARCHPATH1 "/cd"
#define DEFAULTSEARCHPATH2 "/pc"
//#define DEFAULTSEARCHPATH3 "/pc/home/alam/srb2code/data"
#elif defined (GP2X)
#define DEFAULTWADLOCATION1 "/mnt/sd"
#define DEFAULTWADLOCATION2 "/mnt/sd/SRB2"
#define DEFAULTWADLOCATION3 "/tmp/mnt/sd"
#define DEFAULTWADLOCATION4 "/tmp/mnt/sd/SRB2"
#define DEFAULTSEARCHPATH1 "/mnt/sd"
#define DEFAULTSEARCHPATH2 "/tmp/mnt/sd"
#elif defined (__unix__) || defined(__APPLE__) || defined (UNIXCOMMON)
#define DEFAULTWADLOCATION1 "/usr/local/share/games/srb2"
#define DEFAULTWADLOCATION2 "/usr/local/games/srb2"
#define DEFAULTWADLOCATION3 "/usr/share/games/srb2"
#define DEFAULTWADLOCATION4 "/usr/games/srb2"
#define DEFAULTSEARCHPATH1 "/usr/local/games/"
#define DEFAULTSEARCHPATH2 "/usr/games"
#define DEFAULTSEARCHPATH3 "/usr/local"
#elif defined (_XBOX)
#define NOCWD
#ifdef __GNUC__
#include <openxdk/debug.h>
#endif
#define DEFAULTWADLOCATION1 "c:\\srb2"
#define DEFAULTWADLOCATION2 "d:\\srb2"
#define DEFAULTWADLOCATION3 "e:\\srb2"
#define DEFAULTWADLOCATION4 "f:\\srb2"
#define DEFAULTWADLOCATION5 "g:\\srb2"
#define DEFAULTWADLOCATION6 "h:\\srb2"
#define DEFAULTWADLOCATION7 "i:\\srb2"
#elif defined (_WIN32_WCE)
#define NOCWD
#define NOHOME
#define DEFAULTWADLOCATION1 "\\Storage Card\\SRB2DEMO"
#define DEFAULTSEARCHPATH1 "\\Storage Card"
#elif defined (_WIN32)
#define DEFAULTWADLOCATION1 "c:\\games\\srb2"
#define DEFAULTWADLOCATION2 "\\games\\srb2"
#define DEFAULTSEARCHPATH1 "c:\\games"
#define DEFAULTSEARCHPATH2 "\\games"
#endif

/**	\brief WAD file to look for
*/
#define WADKEYWORD1 "srb2.srb"
#define WADKEYWORD2 "srb2.wad"
/**	\brief holds wad path
*/
static char returnWadPath[256];

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>

static struct termios tty_tc;
static struct termios tty_con;
static int tty_erase;
static int tty_eof;
static int consolevent = 1;
static int framebuffer;
#endif

UINT8 graphics_started = 0;

UINT8 keyboard_started = 0;


#if defined (__unix__) || defined(__APPLE__) || (defined (UNIXCOMMON))
#if defined (__linux__)
#include <sys/vfs.h>
#else
#include <sys/param.h>
#include <sys/mount.h>
/*For meminfo*/
#include <sys/types.h>
#ifdef FREEBSD
#include <kvm.h>
#endif
#include <nlist.h>
#include <sys/vmmeter.h>
#endif
#endif

#ifdef LINUX
#define MEMINFO_FILE "/proc/meminfo"
#define MEMTOTAL "MemTotal:"
#define MEMFREE "MemFree:"
#endif

size_t I_GetFreeMem(size_t *total)
{
#if defined (_arch_dreamcast)
	//Dreamcast!
	if (total)
		*total = 16<<20;
	return 8<<20;
#elif defined (_PSP)
	// PSP
	if (total)
		*total = 32<<20;
	return 16<<20;
#elif defined (FREEBSD)
	struct vmmeter sum;
	kvm_t *kd;
	struct nlist namelist[] =
	{
#define X_SUM   0
		{"_cnt"},
		{NULL}
	};
	if ((kd = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvm_open")) == NULL)
	{
		*total = 0L;
		return 0;
	}
	if (kvm_nlist(kd, namelist) != 0)
	{
		kvm_close (kd);
		*total = 0L;
		return 0;
	}
	if (kvm_read(kd, namelist[X_SUM].n_value, &sum,
		sizeof (sum)) != sizeof (sum))
	{
		kvm_close(kd);
		*total = 0L;
		return 0;
	}
	kvm_close(kd);

	if (total)
		*total = sum.v_page_count * sum.v_page_size;
	return sum.v_free_count * sum.v_page_size;
#elif defined (SOLARIS)
	/* Just guess */
	if (total)
		*total = 32 << 20;
	return 32 << 20;
#elif defined (LINUX)
	/* Linux */
	char buf[1024];
	char *memTag;
	size_t freeKBytes;
	size_t totalKBytes;
	INT32 n;
	INT32 meminfo_fd = -1;

	meminfo_fd = open(MEMINFO_FILE, O_RDONLY);
	n = read(meminfo_fd, buf, 1023);
	close(meminfo_fd);

	if (n < 0)
	{
		// Error
		*total = 0L;
		return 0;
	}

	buf[n] = '\0';
	if (NULL == (memTag = strstr(buf, MEMTOTAL)))
	{
		// Error
		*total = 0L;
		return 0;
	}

	memTag += sizeof (MEMTOTAL);
	totalKBytes = (size_t)atoi(memTag);

	if (NULL == (memTag = strstr(buf, MEMFREE)))
	{
		// Error
		*total = 0L;
		return 0;
	}

	memTag += sizeof (MEMFREE);
	freeKBytes = atoi(memTag);

	if (total)
		*total = totalKBytes << 10;
	return freeKBytes << 10;
#elif (defined (_WIN32) || (defined (_WIN32_WCE) && !defined (__GNUC__))) && !defined (_XBOX)
	MEMORYSTATUS info;

	info.dwLength = sizeof (MEMORYSTATUS);
	GlobalMemoryStatus( &info );
	if (total)
		*total = (size_t)info.dwTotalPhys;
	return (size_t)info.dwAvailPhys;
#elif defined (__OS2__)
	UINT32 pr_arena;

	if (total)
		DosQuerySysInfo( QSV_TOTPHYSMEM, QSV_TOTPHYSMEM,
							(PVOID) total, sizeof (UINT32));
	DosQuerySysInfo( QSV_MAXPRMEM, QSV_MAXPRMEM,
				(PVOID) &pr_arena, sizeof (UINT32));

	return pr_arena;
#else
	// Guess 48 MB.
	if (total)
		*total = 48<<20;
	return 48<<20;
#endif /* LINUX */
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

void I_Sleep(void)
{
#if !(defined (_arch_dreamcast) || defined (_XBOX))
	if (cv_sleep.value != -1)
		SDL_Delay(cv_sleep.value);
#endif
}

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

//
//I_OutputMsg
//
void I_OutputMsg(const char *fmt, ...)
{
	size_t len;
#ifdef LOGMESSAGES
	size_t d;
#endif
	XBOXSTATIC char txt[128];
	va_list  argptr;

#ifdef _arch_dreamcast
	if (!keyboard_started) conio_printf(fmt);
#endif

	va_start(argptr,fmt);
	vsprintf(txt, fmt, argptr);
	va_end(argptr);

#if defined (_WIN32) && !defined (_XBOX)
	OutputDebugStringA(txt);
#endif

	len = strlen(txt);

#ifdef LOGMESSAGES
	if (logstream)
	{
		d = SDL_RWwrite(logstream, txt, 1, len);
		SDL_RWseek(logstream, 0, RW_SEEK_CUR);
	}
#endif

#if defined (_WIN32) && !defined (_XBOX) && !defined(_WIN32_WCE)
#ifdef DEBUGFILE
	if (debugfile != stderr)
#endif
	{
		HANDLE co = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD bytesWritten;

		if (co == INVALID_HANDLE_VALUE)
			return;

		if (GetFileType(co) == FILE_TYPE_CHAR && GetConsoleMode(co, &bytesWritten))
		{
			static COORD coordNextWrite = {0,0};
			LPVOID oldLines = NULL;
			INT oldLength;
			CONSOLE_SCREEN_BUFFER_INFO csbi;

			// Save the lines that we're going to obliterate.
			GetConsoleScreenBufferInfo(co, &csbi);
			oldLength = csbi.dwSize.X * (csbi.dwCursorPosition.Y - coordNextWrite.Y) + csbi.dwCursorPosition.X - coordNextWrite.X;

			if (oldLength > 0)
			{
				LPVOID blank = malloc(oldLength);
				if (!blank) return;
				memset(blank, ' ', oldLength); // Blank out.
				oldLines = malloc(oldLength*sizeof(TCHAR));
				if (!oldLines)
				{
					free(blank);
					return;
				}

				ReadConsoleOutputCharacter(co, oldLines, oldLength, coordNextWrite, &bytesWritten);

				// Move to where we what to print - which is where we would've been,
				// had console input not been in the way,
				SetConsoleCursorPosition(co, coordNextWrite);

				WriteConsoleA(co, blank, oldLength, &bytesWritten, NULL);
				free(blank);

				// And back to where we want to print again.
				SetConsoleCursorPosition(co, coordNextWrite);
			}

			// Actually write the string now!
			WriteConsoleA(co, txt, (DWORD)len, &bytesWritten, NULL);

			// Next time, output where we left off.
			GetConsoleScreenBufferInfo(co, &csbi);
			coordNextWrite = csbi.dwCursorPosition;

			// Restore what was overwritten.
			if (oldLines && entering_con_command)
				WriteConsole(co, oldLines, oldLength, &bytesWritten, NULL);
			if (oldLines) free(oldLines);
		}
		else // Redirected to a file.
			WriteFile(co, txt, (DWORD)len, &bytesWritten, NULL);
	}
#else
#ifdef HAVE_TERMIOS
	if (consolevent)
	{
		tty_Hide();
	}
#endif

	if (!framebuffer)
		fprintf(stderr, "%s", txt);
#ifdef HAVE_TERMIOS
	if (consolevent)
	{
		tty_Show();
	}
#endif

	// 2004-03-03 AJR Since not all messages end in newline, some were getting displayed late.
	if (!framebuffer)
		fflush(stderr);

#endif
}

void I_StartupMouse2(void){}

void I_StartupKeyboard(void){}

int I_GetKey(void)
{
	return 0;
}

//
//I_StartupTimer
//
void I_StartupTimer(void)
{
#if (defined (_WIN32) && !defined (_WIN32_WCE)) && !defined (_XBOX)
	// for win2k time bug
	if (M_CheckParm("-gettickcount"))
	{
		starttickcount = GetTickCount();
		CONS_Printf("Using GetTickCount()\n");
	}
	winmm = LoadLibraryA("winmm.dll");
	if (winmm)
	{
		p_timeEndPeriod pfntimeBeginPeriod = (p_timeEndPeriod)GetProcAddress(winmm, "timeBeginPeriod");
		if (pfntimeBeginPeriod)
			pfntimeBeginPeriod(1);
		pfntimeGetTime = (p_timeGetTime)GetProcAddress(winmm, "timeGetTime");
	}
	I_AddExitFunc(I_ShutdownTimer);
#elif 0 //#elif !defined (_arch_dreamcast) && !defined(GP2X) // the DC have it own timer and GP2X have broken pthreads?
	if (SDL_InitSubSystem(SDL_INIT_TIMER) < 0)
		I_Error("SRB2: Needs SDL_Timer, Error: %s", SDL_GetError());
#endif
}

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

static boolean isWadPathOk(const char *path)
{
	char *wad3path = malloc(256);

	if (!wad3path)
		return false;

	sprintf(wad3path, pandf, path, WADKEYWORD1);

	if (FIL_ReadFileOK(wad3path))
	{
		free(wad3path);
		return true;
	}

	sprintf(wad3path, pandf, path, WADKEYWORD2);

	if (FIL_ReadFileOK(wad3path))
	{
		free(wad3path);
		return true;
	}

	free(wad3path);
	return false;
}

static void pathonly(char *s)
{
	size_t j;

	for (j = strlen(s); j != (size_t)-1; j--)
		if ((s[j] == '\\') || (s[j] == ':') || (s[j] == '/'))
		{
			if (s[j] == ':') s[j+1] = 0;
			else s[j] = 0;
			return;
		}
}

static const char *searchWad(const char *searchDir)
{
	static char tempsw[256] = "";
	filestatus_t fstemp;

	strcpy(tempsw, WADKEYWORD1);
	fstemp = filesearch(tempsw,searchDir,NULL,true,20);
	if (fstemp == FS_FOUND)
	{
		pathonly(tempsw);
		return tempsw;
	}

	strcpy(tempsw, WADKEYWORD2);
	fstemp = filesearch(tempsw, searchDir, NULL, true, 20);
	if (fstemp == FS_FOUND)
	{
		pathonly(tempsw);
		return tempsw;
	}
	return NULL;
}

static const char *locateWad(void)
{
	const char *envstr;
	const char *WadPath;

	I_OutputMsg("SRB2WADDIR");
	// does SRB2WADDIR exist?
	if (((envstr = I_GetEnv("SRB220WADDIR")) != NULL) && isWadPathOk(envstr))
		return envstr;

#ifdef _WIN32_WCE
	// examine argv[0]
	strcpy(returnWadPath, myargv[0]);
	pathonly(returnWadPath);
	I_PutEnv(va("HOME=%s",returnWadPath));
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif

#ifndef NOCWD
	I_OutputMsg(",.");
	// examine current dir
	strcpy(returnWadPath, ".");
	if (isWadPathOk(returnWadPath))
		return NULL;
#endif

	// examine default dirs
#ifdef DEFAULTWADLOCATION1
	I_OutputMsg(","DEFAULTWADLOCATION1);
	strcpy(returnWadPath, DEFAULTWADLOCATION1);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION2
	I_OutputMsg(","DEFAULTWADLOCATION2);
	strcpy(returnWadPath, DEFAULTWADLOCATION2);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION3
	I_OutputMsg(","DEFAULTWADLOCATION3);
	strcpy(returnWadPath, DEFAULTWADLOCATION3);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION4
	I_OutputMsg(","DEFAULTWADLOCATION4);
	strcpy(returnWadPath, DEFAULTWADLOCATION4);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION5
	I_OutputMsg(","DEFAULTWADLOCATION5);
	strcpy(returnWadPath, DEFAULTWADLOCATION5);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION6
	I_OutputMsg(","DEFAULTWADLOCATION6);
	strcpy(returnWadPath, DEFAULTWADLOCATION6);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifdef DEFAULTWADLOCATION7
	I_OutputMsg(","DEFAULTWADLOCATION7);
	strcpy(returnWadPath, DEFAULTWADLOCATION7);
	if (isWadPathOk(returnWadPath))
		return returnWadPath;
#endif
#ifndef NOHOME
	// find in $HOME
	I_OutputMsg(",HOME/" DEFAULTDIR);
	if ((envstr = I_GetEnv("HOME")) != NULL)
	{
		char *tmp = malloc(strlen(envstr) + 1 + sizeof(DEFAULTDIR));
		strcpy(tmp, envstr);
		strcat(tmp, "/");
		strcat(tmp, DEFAULTDIR);
		WadPath = searchWad(tmp);
		free(tmp);
		if (WadPath)
			return WadPath;
	}
#endif
#ifdef DEFAULTSEARCHPATH1
	// find in /usr/local
	I_OutputMsg(", in:"DEFAULTSEARCHPATH1);
	WadPath = searchWad(DEFAULTSEARCHPATH1);
	if (WadPath)
		return WadPath;
#endif
#ifdef DEFAULTSEARCHPATH2
	// find in /usr/games
	I_OutputMsg(", in:"DEFAULTSEARCHPATH2);
	WadPath = searchWad(DEFAULTSEARCHPATH2);
	if (WadPath)
		return WadPath;
#endif
#ifdef DEFAULTSEARCHPATH3
	// find in ???
	I_OutputMsg(", in:"DEFAULTSEARCHPATH3);
	WadPath = searchWad(DEFAULTSEARCHPATH3);
	if (WadPath)
		return WadPath;
#endif
	// if nothing was found
	return NULL;
}

const char *I_LocateWad(void)
{
	const char *waddir;

	I_OutputMsg("Looking for WADs in: ");
	waddir = locateWad();
	I_OutputMsg("\n");

	if (waddir)
	{
		// change to the directory where we found srb2.srb
#if (defined (_WIN32) && !defined (_WIN32_WCE)) && !defined (_XBOX)
		SetCurrentDirectoryA(waddir);
#elif !defined (_WIN32_WCE) && !defined (_PS3)
		if (chdir(waddir) == -1)
			I_OutputMsg("Couldn't change working directory\n");
#endif
	}
	return waddir;
}

void I_GetJoystickEvents(void){}

void I_GetJoystick2Events(void){}

void I_GetMouseEvents(void){}


char *I_GetEnv(const char *name)
{
#if defined(_WIN32_WCE)
	(void)name;
	return NULL;
#else
	return getenv(name);
#endif
}

INT32 I_PutEnv(char *variable)
{
#if defined(_WIN32_WCE)
	return ((variable)?-1:0);
#else
	return putenv(variable);
#endif
}