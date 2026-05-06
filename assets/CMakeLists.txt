## Assets Target Configuration ##

# MD5 generation
set(SRB2_ASSET_ALL
	${CMAKE_CURRENT_SOURCE_DIR}/installer/srb2.srb
	${CMAKE_CURRENT_SOURCE_DIR}/installer/sonic.plr
    ${CMAKE_CURRENT_SOURCE_DIR}/installer/tails.plr
    ${CMAKE_CURRENT_SOURCE_DIR}/installer/knux.plr
	${CMAKE_CURRENT_SOURCE_DIR}/installer/rings.wpn
	${CMAKE_CURRENT_SOURCE_DIR}/installer/zones.dta
	${CMAKE_CURRENT_SOURCE_DIR}/installer/drill.dta
    ${CMAKE_CURRENT_SOURCE_DIR}/installer/soar.dta
	${CMAKE_CURRENT_SOURCE_DIR}/installer/music.dta
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
	file(MD5 ${CMAKE_CURRENT_SOURCE_DIR}/installer/${SRB2_ASSET} "SRB2_ASSET_${SRB2_ASSET}_HASH")
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