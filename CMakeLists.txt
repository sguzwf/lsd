CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(Lsd)

FIND_PACKAGE(Boost 1.40.0 REQUIRED 
    COMPONENTS
        thread)

FIND_PACKAGE(OpenSSL REQUIRED)
FIND_PACKAGE(CURL REQUIRED)

INCLUDE(cmake/LocateLibrary.cmake)

LOCATE_LIBRARY(LIBMSGPACK "msgpack.hpp" "msgpack")
LOCATE_LIBRARY(LIBZMQ "zmq.hpp" "zmq")

IF(NOT APPLE)
    LOCATE_LIBRARY(LIBUUID "uuid/uuid.h" "uuid")
ENDIF()

INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${LIBMSGPACK_INCLUDE_DIRS}
    ${LIBZMQ_INCLUDE_DIRS})

LINK_DIRECTORIES(
    ${Boost_LIBRARY_DIRS}
    ${LIBMSGPACK_LIBRARY_DIRS}
    ${LIBZMQ_LIBRARY_DIRS})

INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/foreign/jsoncpp-0.5.0/include)
INCLUDE_DIRECTORIES(BEFORE ${PROJECT_SOURCE_DIR}/include)

ADD_LIBRARY(json
    foreign/jsoncpp-0.5.0/src/lib_json/json_value
    foreign/jsoncpp-0.5.0/src/lib_json/json_reader
    foreign/jsoncpp-0.5.0/src/lib_json/json_writer)

SET_TARGET_PROPERTIES(json PROPERTIES
    COMPILE_FLAGS -fPIC)

ADD_LIBRARY(lsd SHARED
    library/cached_message.cpp
    library/cached_response.cpp
    library/client_impl.cpp
    library/client.cpp
    library/configuration.cpp
    library/context.cpp
    library/curl_hosts_fetcher.cpp
    library/data_container.cpp
    library/error.cpp
    library/http_heartbeats_collector.cpp
    library/message_cache.cpp
    library/progress_timer.cpp
    library/refresher.cpp)

TARGET_LINK_LIBRARIES(lsd
    boost_thread-mt
    crypto
    curl
    json
    ${LIBUUID_LIBRARIES})

SET_TARGET_PROPERTIES(lsd PROPERTIES
    VERSION 1)

INSTALL(
    TARGETS
        lsd
    LIBRARY DESTINATION lib COMPONENT runtime)

INSTALL(DIRECTORY
        include/lsd
    DESTINATION include
    COMPONENT development)
