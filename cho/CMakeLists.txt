cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0074 OLD)
project(cho)

function(ADD_SRC var folder inc src)
	source_group(${folder} FILES ${inc} ${src})
	set(${var} ${${var}} ${inc} ${src} CACHE INTERNAL "")
endfunction(ADD_SRC)

option(CHO_LIBRARY "Build cho as a library" ON)

# Local includes
include_directories("inc")
include_directories("src")

ADD_SRC(CHO_SOURCES	"RiotAPI\\Generated"	"inc/cho/generated/riot_api_data.hpp"	"src/generated/riot_api_data.cpp")
ADD_SRC(CHO_SOURCES	"RiotAPI"				"inc/cho/riot_api.hpp"					"src/riot_api.cpp")
ADD_SRC(CHO_SOURCES	"RiotAPI\\Request"		"inc/cho/riot_api_request.hpp"			"src/riot_api_request.cpp")
ADD_SRC(CHO_SOURCES	"RateLimit"				"inc/cho/rate_limit.hpp"				"src/rate_limit.cpp")
ADD_SRC(CHO_SOURCES	"Settings"				"inc/cho/settings.hpp"					"src/settings.cpp")
ADD_SRC(CHO_SOURCES	"Cache"					"inc/cho/cache/file_cache.hpp"			"src/cache/file_cache.cpp")
ADD_SRC(CHO_SOURCES	"Cho"					"inc/cho/cho.hpp"						"src/cho.cpp")
ADD_SRC(CHO_SOURCES	"Optional"				"inc/cho/optional.hpp"					"src/optional.cpp")
ADD_SRC(CHO_SOURCES	"Hash"					"inc/cho/hash.hpp"						"src/hash.cpp")
ADD_SRC(CHO_SOURCES	"Test"					""										"src/main.cpp")

if (CHO_LIBRARY)
	add_definitions(-DCHO_LIBRARY)
	add_library(cho ${CHO_SOURCES})

	set_target_properties(cho PROPERTIES
		CXX_STANDARD 11
		CXX_EXTENSIONS OFF)
	target_include_directories(cho PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

else (CHO_LIBRARY)
	add_definitions(-DCHO_APPLICATION)
	add_executable(cho ${CHO_SOURCES})

	set_target_properties(cho PROPERTIES 
		VS_DEBUGGER_WORKING_DIRECTORY "D:/"
		CXX_STANDARD 11
		CXX_EXTENSIONS OFF)

endif (CHO_LIBRARY)

target_link_libraries(cho cho_request)