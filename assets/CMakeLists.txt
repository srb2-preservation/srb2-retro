## Assets Target Configuration ##

# MD5 generation
set(SRB2_ASSET_ALL
	${CMAKE_CURRENT_SOURCE_DIR}/srb2.srb
	${CMAKE_CURRENT_SOURCE_DIR}/sonic.plr
    ${CMAKE_CURRENT_SOURCE_DIR}/tails.plr
    ${CMAKE_CURRENT_SOURCE_DIR}/knux.plr
	${CMAKE_CURRENT_SOURCE_DIR}/rings.wpn
	${CMAKE_CURRENT_SOURCE_DIR}/zones.dta
	${CMAKE_CURRENT_SOURCE_DIR}/drill.dta
    ${CMAKE_CURRENT_SOURCE_DIR}/soar.dta
	${CMAKE_CURRENT_SOURCE_DIR}/music.dta
)

set(SRB2_ASSET_HASHED
	srb2.srb
	sonic.plr
    tails.plr
    knux.plr
	rings.wpn
	zones.dta
	drill.dta
    soar.dta
	music.dta
)

foreach(SRB2_ASSET ${SRB2_ASSET_HASHED})
	file(MD5 ${CMAKE_CURRENT_SOURCE_DIR}/${SRB2_ASSET} "SRB2_ASSET_${SRB2_ASSET}_HASH")
	set(SRB2_ASSET_${SRB2_ASSET}_HASH ${SRB2_ASSET_${SRB2_ASSET}_HASH} PARENT_SCOPE)
endforeach()

# Installation

if(${CMAKE_SYSTEM} MATCHES Darwin)
	get_target_property(outname SRB2SDL2 OUTPUT_NAME)
	install(FILES ${SRB2_ASSET_ALL}
		DESTINATION "${outname}.app/Contents/Resources"
	)
else()
	install(FILES ${SRB2_ASSET_ALL}
		DESTINATION .
	)
endif()