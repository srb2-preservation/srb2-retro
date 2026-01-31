#
# Simple CMakeLists for Sonic Robo Blast 2
#
PROJECT(SRB2)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

#
# Dependencies
#
FIND_PACKAGE(SDL)
FIND_PACKAGE(SDL_mixer)
FIND_PACKAGE(PNG)

#
# Common stuff
#

# not added defines:
# -DHAVE_PNG - does not build (incorrect use of PNG_EXPORT etc.)
ADD_DEFINITIONS(-DDIRECTFULLSCREEN -DSDL -DHAVE_MIXER -DNOHW -DHW3SOUND -DNOASM)

SET(SDL_DIR sdl)

SET(COMMON_SRCS
	src/${SDL_DIR}/dosstr.c
	src/${SDL_DIR}/endtxt.c
	src/${SDL_DIR}/hwsym_sdl.c
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
	ADD_DEFINITIONS(-DWINDOWSHELL -MINGW)
	MESSAGE(WARNING "No specific settings for you system, it may be not supported!")
ENDIF(${CMAKE_SYSTEM} MATCHES "Windows")

# 3% chance windows actually works, i dont have a device running windows so who knows?

#
# Targets
#
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIRS} ${SDL_INCLUDE_DIR} ${SDL_MIXER_INCLUDE_DIRS})
ADD_EXECUTABLE(SRB2 ${COMMON_SRCS})
TARGET_LINK_LIBRARIES(SRB2 ${SDL_LIBRARY} ${SDL_MIXER_LIBRARIES} ${EXTRA_LIBRARIES})
