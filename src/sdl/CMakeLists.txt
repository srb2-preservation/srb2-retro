# Declare SDL2 interface sources

set(SRB2_CONFIG_SDL2_USEMIXER ON CACHE BOOL "Use SDL2_mixer or regular sdl sound")

if(${SRB2_CONFIG_SDL2_USEMIXER})
	if(${CMAKE_SYSTEM} MATCHES Emscripten)
		set(SDL2_MIXER_FOUND  ON)
		set(SDL2_MIXER_LIBRARIES "${CMAKE_SOURCE_DIR}/emlibs/libSDL2_mixer.a")
	else()
		find_package(SDL2_mixer)
	endif()
	if(${SDL2_MIXER_FOUND})
		set(SRB2_HAVE_MIXER ON)
		set(SRB2_SDL2_SOUNDIMPL mixer_sound.c)
	else()
		message(WARNING "You specified that SDL2_mixer is available, but it was not found. Falling back to sdl sound.")
		set(SRB2_SDL2_SOUNDIMPL sdl_sound.c)
	endif()
else()
	set(SRB2_SDL2_SOUNDIMPL sdl_sound.c)
endif()


set(SRB2_SDL2_SOURCES
	dosstr.c
    endtxt.c
    hwsym_sdl.c
	i_cdmus.c
	i_main.c
    i_net.c
	i_system.c
	i_video.c
	ogl_sdl.c

	${SRB2_SDL2_SOUNDIMPL}
)

set(SRB2_SDL2_HEADERS
    hwsym_sdl.h
	ogl_sdl.h
)

source_group("Interface Code" FILES ${SRB2_SDL2_SOURCES} ${SRB2_SDL2_HEADERS})

# Dependency
if(${CMAKE_SYSTEM} MATCHES Emscripten)
	set(SDL2_FOUND ON)
else()
	find_package(SDL2)
endif()

if(${SDL2_FOUND})
	set(SRB2_SDL2_TOTAL_SOURCES
		${SRB2_SDL2_SOURCES}
		${SRB2_SDL2_HEADERS}
	)

	if(${SRB2_CONFIG_HWRENDER})
		set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES}
			${SRB2_HWRENDER_SOURCES}
			${SRB2_HWRENDER_HEADERS}
			${SRB2_R_OPENGL_SOURCES}
			${SRB2_R_OPENGL_HEADERS}
		)

		source_group("Hardware" FILES ${SRB2_HWRENDER_SOURCES} ${SRB2_HWRENDER_HEADERS})
		source_group("Hardware\\OpenGL Renderer" FILES ${SRB2_R_OPENGL_SOURCES} ${SRB2_R_OPENGL_HEADERS})
    endif()

	if(${CMAKE_SYSTEM} MATCHES Windows)
		set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES}
			${CMAKE_SOURCE_DIR}/src/win32/win_dbg.c
            ${CMAKE_SOURCE_DIR}/src/win32/Srb2win.rc
		)
	endif()

	if(${CMAKE_SYSTEM} MATCHES Darwin)
		set(MACOSX_BUNDLE_ICON_FILE Srb2mac.icns)
		set_source_files_properties(macosx/Srb2mac.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
		set(SRB2_SDL2_MAC_SOURCES
			macosx/mac_alert.c
			macosx/mac_alert.h
			macosx/mac_resources.c
			macosx/mac_resources.h
			macosx/Srb2mac.icns
		)
		source_group("Interface Code\\OSX Compatibility" FILES ${SRB2_SDL2_MAC_SOURCES})
		set(SRB2_SDL2_TOTAL_SOURCES ${SRB2_SDL2_TOTAL_SOURCES} ${SRB2_SDL2_MAC_SOURCES})
	endif()

	add_executable(SRB2SDL2 MACOSX_BUNDLE WIN32 ${SRB2_SDL2_TOTAL_SOURCES})
	if(${CMAKE_SYSTEM} MATCHES Windows)
		set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME srb2win)
	elseif(${CMAKE_SYSTEM} MATCHES Linux)
		set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME lsdlsrb2)
	else()
		set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME srb2)
	endif()

	if(${CMAKE_SYSTEM} MATCHES Darwin)
		add_framework(CoreFoundation SRB2SDL2)
		add_framework(SDL2 SRB2SDL2)
		add_framework(SDL2_mixer SRB2SDL2)
		target_link_libraries(SRB2SDL2 PRIVATE
			${PNG_LIBRARIES}
			${ZLIB_LIBRARIES}
			${OPENGL_LIBRARIES}
		)
		set_target_properties(SRB2SDL2 PROPERTIES OUTPUT_NAME "Sonic Robo Blast 2")
	else()
        target_link_libraries(SRB2SDL2 PRIVATE
			${SDL2_LIBRARIES}
            ${SDL2_MIXER_LIBRARIES}
			${PNG_LIBRARIES}
			${ZLIB_LIBRARIES}
			${OPENGL_LIBRARIES}
		)

		if(${CMAKE_SYSTEM} MATCHES Linux)
			target_link_libraries(SRB2SDL2 PRIVATE
				m
				rt
			)
		endif()
	endif()

	target_link_libraries(SRB2SDL2 PRIVATE SRB2Core)

	set_target_properties(SRB2SDL2 PROPERTIES VERSION ${SRB2_VERSION})

	if(${CMAKE_SYSTEM} MATCHES Windows)
		target_link_libraries(SRB2SDL2 PRIVATE
			ws2_32
		)
		target_compile_options(SRB2SDL2 PRIVATE
			-U_WINDOWS
		)
	endif()

	if(MSVC)
        find_package(SDL2_MAIN REQUIRED)
		target_link_libraries(SRB2SDL2 PRIVATE
			${SDL2_MAIN_LIBRARIES}
		)
		target_compile_options(SRB2SDL2 PRIVATE
			/Umain
			/D_CRT_SECURE_NO_WARNINGS # something about string functions.
			/D_CRT_NONSTDC_NO_DEPRECATE
			/DSDLMAIN
			/D_WINSOCK_DEPRECATED_NO_WARNINGS # Don't care
		)
	endif()

	target_include_directories(SRB2SDL2 PRIVATE
		${SDL2_INCLUDE_DIRS}
		${SDL2_MIXER_INCLUDE_DIRS}
		${PNG_INCLUDE_DIRS}
		${ZLIB_INCLUDE_DIRS}
		${OPENGL_INCLUDE_DIRS}
	)

	if(${SRB2_HAVE_MIXER})
		target_compile_definitions(SRB2SDL2 PRIVATE -DHAVE_MIXER -DSOUND=SOUND_MIXER)
	endif()

	target_compile_definitions(SRB2SDL2 PRIVATE
		-DDIRECTFULLSCREEN -DSDL
	)

	## strip debug symbols into separate file when using gcc
	if(CMAKE_COMPILER_IS_GNUCC)
		if(${CMAKE_BUILD_TYPE} MATCHES Debug)
			message(STATUS "Will make separate debug symbols in *.debug")
			add_custom_command(TARGET SRB2SDL2 POST_BUILD
				COMMAND ${OBJCOPY} --only-keep-debug $<TARGET_FILE:SRB2SDL2> $<TARGET_FILE:SRB2SDL2>.debug
				COMMAND ${OBJCOPY} --strip-debug $<TARGET_FILE:SRB2SDL2>
				COMMAND ${OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE_NAME:SRB2SDL2>.debug $<TARGET_FILE:SRB2SDL2>
			)
		endif()
	endif()

	#### Installation ####
	if(${CMAKE_SYSTEM} MATCHES Darwin)
		install(TARGETS SRB2SDL2
			BUNDLE DESTINATION .
		)
	else()
		install(TARGETS SRB2SDL2 SRB2SDL2
			RUNTIME DESTINATION .
		)
	endif()

	if(${CMAKE_SYSTEM} MATCHES Windows)
		set(win_extra_dll_list "")
		macro(getwinlib dllname defaultname)
			find_library(SRB2_SDL2_DLL_${dllname} "${defaultname}")
			list(APPEND win_extra_dll_list ${SRB2_SDL2_DLL_${dllname}})
		endmacro()
		getwinlib(SDL2 "SDL2.dll")
		if(${SRB2_CONFIG_SDL2_USEMIXER})
			getwinlib(SDL2_mixer "SDL2_mixer.dll")
			getwinlib(libmikmod-2 "libmikmod-2.dll")
			getwinlib(libogg_0 "libogg-0.dll")
			getwinlib(libvorbis_0 "libvorbis-0.dll")
			getwinlib(libvorbisfile_3 "libvorbisfile-3.dll")
			getwinlib(smpeg2 "smpeg2.dll")
		endif()

		install(PROGRAMS
			${win_extra_dll_list}
			DESTINATION .
		)

		# We also want to copy those DLLs to build directories on MSVC.
		# So we'll add a post_build step.
		copy_files_to_build_dir(SRB2SDL2 win_extra_dll_list)
	endif()

	# Mac bundle fixup
	if(${CMAKE_SYSTEM} MATCHES Darwin)
		install(CODE "
			include(BundleUtilities)
			fixup_bundle(\"${CMAKE_INSTALL_PREFIX}/Sonic Robo Blast 2.app\"
				\"\"
				/Library/Frameworks
			)"
		)
	endif()

	set(SRB2_SDL2_AVAILABLE YES PARENT_SCOPE)

else()
	message(WARNING "SDL2 was not found, so the SDL2 target will not be available.")
	set(SRB2_SDL2_AVAILABLE NO PARENT_SCOPE)
endif()

if(${CMAKE_SYSTEM} MATCHES Emscripten)
	add_subdirectory(${PROJECT_SOURCE_DIR}/src/web SRB2SDL2)
endif()
