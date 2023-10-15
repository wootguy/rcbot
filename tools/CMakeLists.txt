cmake_minimum_required(VERSION 3.6)

project(wptconv)

set(SOURCE_FILES
	src/wptconv.cpp
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

