cmake_minimum_required(VERSION 3.6)

project(wptconv)

set(SOURCE_FILES
	src/wptconv.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if(MSVC)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()