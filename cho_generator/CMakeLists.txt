cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0074 OLD)
project(cho_generator)

function(ADD_SRC var folder inc src)
	source_group(${folder} FILES ${inc} ${src})
	set(${var} ${${var}} ${inc} ${src} CACHE INTERNAL "")
endfunction(ADD_SRC)

option(CHO_GEN_LIBRARY "Build cho_generator as a library" OFF)

# Local includes
include_directories("inc")
include_directories("src")

# Somehow mstch is not referencing this correctly?
include_directories(${Boost_INCLUDE_DIR})

ADD_SRC(CHO_GEN_SOURCES	"Application"			""					"src/main.cpp")

if (CHO_GEN_LIBRARY)
	add_definitions(-DCHO_GEN_LIBRARY)
	add_library(cho_generator ${CHO_GEN_SOURCES})

else (CHO_GEN_LIBRARY)
	add_definitions(-DCHO_GEN_APPLICATION)
	add_executable(cho_generator ${CHO_GEN_SOURCES})

endif (CHO_GEN_LIBRARY)

set_target_properties(cho_generator PROPERTIES 
	VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	CXX_STANDARD 11
	CXX_EXTENSIONS OFF)
target_link_libraries(cho_generator mstch cho_request)
