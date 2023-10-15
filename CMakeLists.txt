cmake_minimum_required(VERSION 3.6)

set(SVEN_ROOT_PATH "C:/Games/Steam/steamapps/common/Sven Co-op/" CACHE FILEPATH "Where your Sven Co-op folder is")
set(DLL_INSTALL_PATH "svencoop/addons/metamod/dlls" CACHE FILEPATH "Where to install the DLL (relative to Sven Co-op folder)")

project(rcbot)

set(SOURCE_FILES
	src/actionutility.h
	src/bak_bot_weapons.h
	src/bits.h src/bits.cpp
	src/bot.h src/bot.cpp
	src/bot_builder.h
	src/bot_chat.h src/bot_chat.cpp
	src/bot_client.h src/bot_client.cpp
	src/bot_combat.cpp
	src/bot_commands.cpp
	src/bot_commands.h
	src/bot_const.h
	src/bot_enemy_cost_ga.h
	src/bot_ga.h src/bot_ga.cpp
	src/bot_learnsys.h
	src/bot_menu.h src/bot_menu.cpp
	src/bot_navigate.cpp
	src/bot_nn.cpp
	src/bot_rep.cpp
	src/bot_texture.h
	src/bot_tick.h
	src/bot_visibles.h src/bot_visibles.cpp
	src/bot_weapons.h src/bot_weapons.cpp
	src/client.cpp
	src/dir.h src/dir.cpp
	src/dll.cpp
	src/engine.cpp
	src/ents
	src/ga.h src/ga.cpp
	src/gannconst.h
	src/generic_class.h
	src/globals.cpp
	src/h_export.cpp
	src/megahal.h src/megahal.cpp
	src/netmessages.cpp
	src/nn.h
	src/perceptron.h src/perceptron.cpp
	src/som.h src/som.cpp
	src/util.cpp
	src/waypoint.h src/waypoint.cpp
	src/main.cpp
)

add_library(${PROJECT_NAME} SHARED)
add_subdirectory(mmlib)
add_compile_definitions(RCBOT_META_BUILD)

if(MSVC)
	source_group("Header Files\\sven" FILES ${SVEN_API_HEADERS})
	source_group("Header Files\\mmlib" FILES ${META_HELPER_HEADERS})
	source_group("Source Files\\mmlib" FILES ${META_HELPER_SOURCES})
	
	set_target_properties(${PROJECT_NAME} PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY_DEBUG "${SVEN_ROOT_PATH}/${DLL_INSTALL_PATH}"
                      RUNTIME_OUTPUT_DIRECTORY_RELEASE "${SVEN_ROOT_PATH}/${DLL_INSTALL_PATH}")
					  
	set_target_properties(${PROJECT_NAME} PROPERTIES
					  VS_STARTUP_PROJECT ${PROJECT_NAME}
					  VS_DEBUGGER_COMMAND "svencoop.exe"
					  VS_DEBUGGER_COMMAND_ARGUMENTS "+map stadium4 -nomsaa -nofbo -noborder -condebug -dll addons/metamod/dlls/metamod.dll"
					  VS_DEBUGGER_WORKING_DIRECTORY "${SVEN_ROOT_PATH}")
					  
	# can't overwrite dll while its loaded but it can be renamed, then you can run "meta reload <plugin>" to update it in-game				
	add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
		COMMAND ${CMAKE_CURRENT_LIST_DIR}/replace_dll.bat
	)
	
	target_link_libraries(${PROJECT_NAME} WS2_32 IPHLPAPI)
	set(CMAKE_CXX_FLAGS "/w /EHsc")
	
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

else()
	set(CMAKE_CXX_FLAGS "-Wall -std=c++11 -static-libstdc++ -static-libgcc -m32 -g -fvisibility=hidden")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0")
	set(CMAKE_CXX_FLAGS_RELEASE "-Os -w -Wfatal-errors")
	target_compile_definitions(${PROJECT_NAME} PUBLIC -Dlinux)
endif()
