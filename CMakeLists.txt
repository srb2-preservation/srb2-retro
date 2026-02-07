#
# Simple CMakeLists for Sonic Robo Blast 2
#
PROJECT(SRB2)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

#
# Variables
#

set(SRB2_USE_SDL2 ON CACHE BOOL "Use SDL2 instead of SDL1.2")
# use sdl2 by default

#
# Dependencies
#

include(cmake/CPM.cmake)
add_subdirectory(thirdparty)

#
# Common stuff
#

if (${SRB2_USE_SDL2})
	ADD_DEFINITIONS(-DHAVE_ZLIB -DHAVE_PNG -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DDIRECTFULLSCREEN -DSDL2 -DHAVE_MIXER -DNOHW -DNOASM)
else()
	ADD_DEFINITIONS(-DHAVE_ZLIB -DHAVE_PNG -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DDIRECTFULLSCREEN -DSDL -DHAVE_MIXER -DNOHW -DHW3SOUND -DNOASM)
endif()

if (${SRB2_USE_SDL2})
	SET(SDL_DIR sdl2)
else()
	SET(SDL_DIR sdl)
endif()

SET(COMMON_SRCS
	src/${SDL_DIR}/dosstr.c
	src/${SDL_DIR}/i_cdmus.c
	src/${SDL_DIR}/i_main.c
	src/${SDL_DIR}/i_net.c
	src/${SDL_DIR}/i_sound.c
	src/${SDL_DIR}/i_system.c
	src/${SDL_DIR}/i_video.c
	src/am_map.c
	src/command.c
	src/comptime.c
	src/console.c
	src/d_clisrv.c
	src/d_main.c
	src/d_net.c
	src/d_netcmd.c
	src/d_netfil.c
	src/dehacked.c
    src/dstrings.c
	src/f_finale.c
	src/f_wipe.c
	src/filesrch.c
	src/g_game.c
	src/g_input.c
	src/hardware/hw3sound.c
	src/hu_stuff.c
	src/i_tcp.c
	src/info.c
    src/lzf.c
	src/m_argv.c
	src/m_anigif.c
	src/m_bbox.c
	src/m_cheat.c
	src/m_fixed.c
	src/m_menu.c
	src/m_misc.c
	src/m_queue.c
	src/m_random.c
	src/md5.c
	src/mserv.c
	src/p_ceilng.c
	src/p_enemy.c
	src/p_fab.c
	src/p_floor.c
	src/p_inter.c
	src/p_lights.c
	src/p_map.c
	src/p_maputl.c
	src/p_mobj.c
	src/p_polyobj.c
	src/p_saveg.c
	src/p_setup.c
	src/p_sight.c
	src/p_spec.c
	src/p_telept.c
	src/p_tick.c
	src/p_user.c
	src/r_bsp.c
	src/r_data.c
	src/r_draw.c
	src/r_main.c
	src/r_plane.c
	src/r_segs.c
	src/r_sky.c
	src/r_splats.c
	src/r_things.c
	src/s_sound.c
	src/screen.c
	src/sounds.c
	src/st_stuff.c
	src/string.c
	src/tables.c
	src/v_video.c
	src/w_wad.c
	src/y_inter.c
	src/z_zone.c
)

SET(SDL12_SRCS
	src/${SDL_DIR}/hwsym_sdl.c
	src/${SDL_DIR}/endtxt.c
)

#
# Platform-specific stuff
#

MACRO(EXTRALIB NAME)
	FIND_LIBRARY(${NAME}_LIBRARY NAMES ${NAME})
	IF(${NAME}_LIBRARY)
		MESSAGE(STATUS "Found lib${NAME}: ${${NAME}_LIBRARY}")
		SET(EXTRA_LIBRARIES ${EXTRA_LIBRARIES} ${${NAME}_LIBRARY})
	ELSE(${NAME}_LIBRARY)
		MESSAGE(FATAL_ERROR "Could not find lib${NAME}!")
	ENDIF(${NAME}_LIBRARY)
ENDMACRO(EXTRALIB)

IF(${CMAKE_SYSTEM} MATCHES "FreeBSD")
	ADD_DEFINITIONS(-DUNIXCOMMON -DLINUX -DFREEBSD)
	EXTRALIB(kvm)
ELSEIF(${CMAKE_SYSTEM} MATCHES "Linux")
	ADD_DEFINITIONS(-DUNIXCOMMON -DLINUX)
	EXTRALIB(m)
	EXTRALIB(rt)
ELSE(${CMAKE_SYSTEM} MATCHES "Windows")
	ADD_DEFINITIONS(-DWINDOWSHELL -DMINGW)
	MESSAGE(WARNING "No specific settings for your system, it may be not supported!")
ENDIF(${CMAKE_SYSTEM} MATCHES "FreeBSD")

# 3% chance windows actually works, i dont have a device running windows so who knows?

#
# Targets
#

if (NOT (${SRB2_USE_SDL2}))
	message(STATUS "Using SDL1.2")

	INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS} ${SDL_INCLUDE_DIR} ${SDL_MIXER_INCLUDE_DIR})
	ADD_EXECUTABLE(lsdlsrb2 ${COMMON_SRCS} ${SDL12_SRCS})
	LINK_LIBRARIES(lsdlsrb2 ${ZLIB_LIBRARIES} ${PNG_LIBRARIES} ${SDL_LIBRARY} ${SDL_MIXER_LIBRARY} ${EXTRA_LIBRARIES})
else()
	message(STATUS "Using SDL2")

	INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} ${SDL2_mixer_INCLUDE_DIRS})
	ADD_EXECUTABLE(lsdl2srb2 ${COMMON_SRCS})
	TARGET_LINK_LIBRARIES(lsdl2srb2 ZLIB png SDL2 SDL2_mixer ${EXTRA_LIBRARIES})
endif()
