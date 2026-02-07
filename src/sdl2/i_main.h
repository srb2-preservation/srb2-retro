#ifndef _WIN32
#include <SDL2/SDL_rwops.h>
#else
#include <SDL_rwops.h>
#endif
extern SDL_RWops* logstream;