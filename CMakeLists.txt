cmake_minimum_required(VERSION 3.10)

option(HLCOOP_BUILD "Use the hl co-op headers rather than the hlsdk + sven headers" ON)

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
	src/rcbot_vector.h
	src/hlcoop.h
)

if (HLCOOP_BUILD)
	add_library(${PROJECT_NAME} SHARED
		${SOURCE_FILES}
	)
	
	hlcoop_setup_plugin("plugins/server/")
	
	target_compile_definitions(${PROJECT_NAME} PRIVATE -DHLCOOP_BUILD)
else()
	add_library(${PROJECT_NAME} SHARED)
	add_subdirectory(mmlib)
	mmlib_default_setup()
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE -DRCBOT_META_BUILD)
