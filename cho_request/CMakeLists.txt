cmake_minimum_required(VERSION 3.5)
cmake_policy(SET CMP0074 OLD)
project(cho_request)

# Local includes
include_directories("inc")
include_directories("src")
include_directories(${CURL_INCLUDE_DIRS})

ADD_SRC(CHO_REQ_SOURCES "Request"			"inc/cho_request/request.hpp"			"src/request.cpp")
ADD_SRC(CHO_REQ_SOURCES "Request\\Base"		"inc/cho_request/base_request.hpp"		"src/base_request.cpp")
ADD_SRC(CHO_REQ_SOURCES "Request\\CURL"		"inc/cho_request/curl_request.hpp"		"src/curl_request.cpp")

add_library(cho_request STATIC ${CHO_REQ_SOURCES})

target_include_directories(cho_request PUBLIC ${PROJECT_SOURCE_DIR}/inc)

set_target_properties(cho_request PROPERTIES 
	VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	CXX_STANDARD 11
	CXX_EXTENSIONS OFF)
target_link_libraries(cho_request ${CURL_LIBRARIES} cho_json)
