CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(Lsd)

# define default build configuration options
IF(NOT DEFINED BUILD_TESTS)
	SET(BUILD_TESTS OFF)
ENDIF()

# display status message for important variables
MESSAGE(STATUS "Build configuration:")
MESSAGE(STATUS "-----------------------------------------------------")
MESSAGE(STATUS "BUILD_TESTS = ${BUILD_TESTS}")
MESSAGE(STATUS "Change a value with: cmake -D<Variable>=<Value>")
MESSAGE(STATUS "-----------------------------------------------------")

SET( BUILD_TESTS "${BUILD_TESTS}" CACHE BOOL "Set to OFF to skip building tests." FORCE )

FIND_PACKAGE(Boost 1.40.0 REQUIRED
    COMPONENTS
        thread-mt
        unit_test_framework-mt
        program_options-mt)

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

FILE(GLOB lsd_SRC
    "library/*.cpp"
)

ADD_LIBRARY(lsd SHARED
    ${lsd_SRC})

TARGET_LINK_LIBRARIES(lsd
    boost_thread-mt
    crypto
    curl
    json
    msgpack
    ${LIBUUID_LIBRARIES})

#build tests if defined by config
IF(BUILD_TESTS)
    # unit tests
	ADD_EXECUTABLE(lsd-tests
        tests/main.cpp
        tests/tests.cpp
        library/time_value.cpp)

    TARGET_LINK_LIBRARIES(lsd-tests
        boost_unit_test_framework-mt
        lsd
        zmq)
   
    ADD_CUSTOM_TARGET(check COMMAND make COMMAND ./lsd-tests)
    
    # test client
	ADD_EXECUTABLE(lsd-client
        tests/test_client_app.cpp)

    TARGET_LINK_LIBRARIES(lsd-client
        boost_unit_test_framework-mt
        boost_program_options-mt
        lsd
        msgpack
        zmq)
ENDIF()

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
