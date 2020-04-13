cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0074 OLD)
project(cho)

function(ADD_SRC var folder inc src)
	source_group(${folder} FILES ${inc} ${src})
	set(${var} ${${var}} ${inc} ${src} CACHE INTERNAL "")
endfunction(ADD_SRC)

# Forced settings
set(HTTP_ONLY ON CACHE BOOL "" FORCE) # Just doing HTTP requests here
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE) # Just using libcurl
set(BUILD_TESTING OFF CACHE BOOL "" FORCE) # No tests
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) # Build into cho++
set(ENABLE_MANUAL OFF CACHE BOOL "" FORCE) # No manuals

# OpenSSL
if(WIN32)
	set(CMAKE_USE_WINSSL ON CACHE BOOL "" FORCE) # Windows doesn't need SSL for Curl, avoid
	set(CMAKE_USE_OPENSSL OFF CACHE BOOL "" FORCE)
else(WIN32)
	set(CMAKE_USE_WINSSL OFF CACHE BOOL "" FORCE)
	set(CMAKE_USE_OPENSSL ON CACHE BOOL "" FORCE)

	find_package(OpenSSL)
endif(WIN32)

# Mstch
add_subdirectory(external/mstch)
include_directories(${mstch_INCLUDE_DIR})
set_target_properties(mstch PROPERTIES FOLDER external)

# CURL
add_subdirectory(external/curl)
set(CURL_FOUND TRUE)
set(CURL_LIBRARIES libcurl)
set(CURL_INCLUDE_DIRS ${CURL_SOURCE_DIR}/include ${CURL_BINARY_DIR}/include/curl)
set_target_properties(libcurl PROPERTIES FOLDER external)

# General properties
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)

add_subdirectory(cho_json)
add_subdirectory(cho_request)
add_subdirectory(cho_generator)
add_subdirectory(cho)
