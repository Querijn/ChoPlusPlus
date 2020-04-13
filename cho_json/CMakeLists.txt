cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0074 OLD)
project(cho_json)

# Local includes
include_directories("inc")
include_directories("src")

ADD_SRC(CHO_JSON_SOURCES "json"			"inc/cho_json/json.hpp"	"src/json.cpp")

add_library(cho_json STATIC ${CHO_JSON_SOURCES})

target_include_directories(cho_json PUBLIC ${PROJECT_SOURCE_DIR}/inc)

set_target_properties(cho_json PROPERTIES 
	VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	CXX_STANDARD 11
	CXX_EXTENSIONS OFF)
