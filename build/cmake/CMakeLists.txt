#Sirikata Build Script
#CMakeLists.txt
#
#Copyright (c) 2008, Patrick Reiter Horn
#All rights reserved.
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions are met:
#
#    * Redistributions of source code must retain the above copyright notice,
#      this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright notice,
#      this list of conditions and the following disclaimer in the documentation
#      and/or other materials provided with the distribution.
#    * Neither the name of the Sirikata nor the names of its contributors
#      may be used to endorse or promote products derived from this software
#      without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
#ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
#ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
#ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#cmake options

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
SET(CMAKE_VERBOSE_MAKEFILE OFF)
SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS TRUE)

IF(COMMAND CMAKE_POLICY)
  # Use new policy for absolute path libraries.  We have no reason to depend on the
  # old version which adds library search paths unnecessarily
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

IF(NOT WIN32 AND NOT APPLE)
  SET(ISLINUX TRUE)
ENDIF()

IF(WIN32)
ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_WIN32_WINNT=0x0500 -D_SCL_SECURE_NO_WARNINGS -EHsc -DBOOST_ALL_DYN_LINK)
SET(CMAKE_CXX_FLAGS "-D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS")
#-fno-rtti
SET(CMAKE_CXX_FLAGS_DEFAULT "-g -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS" CACHE STRING
    "Built cxxflags with default settings."
    FORCE )
SET( CMAKE_C_FLAGS_DEFAULT "-g -Wall -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS" CACHE STRING
    "Built cflags with default settings."
    FORCE )

ELSE()

IF(NOT CMAKE_BUILD_TYPE)
#the below lines work around a bug when cmake first is configured without <CONFIG>
  SET(FORCE_USE_DEBUG_LOCATION "TRUE")
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING
      "Build types include: Default Debug Release None RelWithDebInfo." FORCE
      )
ENDIF()
STRING(TOUPPER ${CMAKE_BUILD_TYPE} UPPERCASE_BUILD_TYPE)
STRING(COMPARE EQUAL ${UPPERCASE_BUILD_TYPE} "DEBUG" ISDEBUG)

SET(ADDED_DEFINITIONS -Wall -Wno-unused-function -Wno-unused-variable -fvisibility=hidden -fPIC -Wnon-virtual-dtor  -Wreorder -Wstrict-null-sentinel -Wsign-promo )

#-Woverloaded_virtual
#-Wabi 
#-Wold-style-cast 
#-fno-rtti
#-fno-rtti
SET(CMAKE_CXX_FLAGS_DEFAULT "-g -Wall -Wno-unused-function -Wno-unused-variable -fvisibility=hidden" CACHE STRING
    "Built cxxflags with default settings."
    FORCE )
SET( CMAKE_C_FLAGS_DEFAULT "-g -Wall -Wno-unused-function -Wno-unused-variable -fvisibility=hidden" CACHE STRING
    "Built cflags with default settings."
    FORCE )
ENDIF()
SET( CMAKE_EXE_LINKER_FLAGS_DEFAULT
    "" CACHE STRING
    "Linking binaries with default settings."
    FORCE )
SET( CMAKE_SHARED_LINKER_FLAGS_DEFAULT
    "" CACHE STRING
    "Building shared libraries with default settings."
    FORCE )
MARK_AS_ADVANCED(
    CMAKE_CXX_FLAGS_DEFAULT
    CMAKE_C_FLAGS_DEFAULT
    CMAKE_EXE_LINKER_FLAGS_DEFAULT
    CMAKE_SHARED_LINKER_FLAGS_DEFAULT )

#project global flags

PROJECT(Sirikata CXX)

IF(NOT TOP_LEVEL)
  SET(TOP_LEVEL ${CMAKE_CURRENT_SOURCE_DIR}/../..)
ENDIF()
GET_FILENAME_COMPONENT(TOP_LEVEL ${TOP_LEVEL} ABSOLUTE)

#add-ins

IF(NOT CMAKE_MODULE_PATH)
  SET(CMAKE_MODULE_PATH ${TOP_LEVEL}/build/modules)
ENDIF()
GET_FILENAME_COMPONENT(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ABSOLUTE)

MACRO(VERIFY_VERSION package_name major minor subminor)
  IF (${package_name}_MAJOR_VERSION LESS ${major})
    MESSAGE(FATAL_ERROR "${package_name} out of date: ${major}.${minor}.${subminor}")
  ELSEIF(${package_name}_MAJOR_VERSION EQUAL ${major})
    IF(${package_name}_MINOR_VERSION LESS ${minor})
       MESSAGE(FATAL_ERROR "${package_name} out of date: ${major}.${minor}.${subminor}")
    ELSEIF(${package_name}_MINOR_VERSION EQUAL ${major})
      IF(${package_name}_SUBMINOR_VERSION LESS ${subminor})
        MESSAGE(FATAL_ERROR "${package_name} out of date: ${major}.${minor}.${subminor}")
      ENDIF()
    ENDIF()
  ENDIF()
ENDMACRO(VERIFY_VERSION)


INCLUDE(ListUtil)
INCLUDE(ParseArguments)
INCLUDE(AddPBJTarget)

MACRO(ADD_CXXTEST_CPP_TARGET)
  PARSE_ARGUMENTS(CXXTEST "DEPENDS;OUTPUTDIR;LIBRARYDIR" "" ${ARGN})
  CAR(CXXTEST_NAME ${CXXTEST_DEFAULT_ARGS})
  CDR(CXXTEST_FILES ${CXXTEST_DEFAULT_ARGS})

  SET(CXXTEST_EXEC_TARGET ${CXXTEST_NAME})

  SET(CXXTEST_TEST_FILES)
  SET(CXXTEST_H_FILES)

  SET(CXXTEST_OPTIONS --runner=StdioPrinter --include=util/Standard.hh)
  #IF(CXXTEST_OUTPUTDIR)
  #  SET(CXXTEST_OPTIONS ${CXXTEST_OPTIONS} -o ${CXXTEST_OUTPUTDIR})
  #ENDIF(CXXTEST_OUTPUTDIR)
  FIND_PACKAGE(Perl)
  IF(PERL_EXECUTABLE)
    SET(CXXTEST_COMPILER ${PERL_EXECUTABLE})
    SET(CXXTEST_GEN ${CXXTEST_LIBRARYDIR}/cxxtestgen.pl)
  ELSE()
    FIND_PACKAGE(PythonInterp)
    IF(PYTHON_EXECUTABLE)
      SET(CXXTEST_COMPILER ${PYTHON_EXECUTABLE})
      SET(CXXTEST_GEN ${CXXTEST_LIBRARYDIR}/cxxtestgen.py)
    ELSE()
      MESSAGE(STATUS "!!! Cannot locate python or perl -- tests will not be compiled.")
    ENDIF()
  ENDIF()
  SET(CXXTEST_CPP_FILE ${CXXTEST_OUTPUTDIR}/test.cc)
  FOREACH(FILE ${CXXTEST_FILES})
    SET(CXXTEST_H_FILE ${FILE})
    SET(CXXTEST_TEST_FILES ${CXXTEST_TEST_FILES} ${CXXTEST_H_FILE})
  ENDFOREACH()
  IF (CXXTEST_COMPILER)
    SET(FINAL_CXXTEST_COMMAND ${CXXTEST_COMPILER} ${CXXTEST_GEN} ${CXXTEST_OPTIONS} -o ${CXXTEST_CPP_FILE} ${CXXTEST_TEST_FILES})
    ADD_CUSTOM_COMMAND(OUTPUT ${CXXTEST_CPP_FILE}
                       COMMAND ${FINAL_CXXTEST_COMMAND}
                       DEPENDS ${CXXTEST_TEST_FILES} ${CXXTEST_DEPENDS}
                       COMMENT "Building ${CXXTEST_TEST_FILES} -> ${CXXTEST_CPP_FILE}")
  ELSE()
    ADD_CUSTOM_COMMAND(OUTPUT ${CXXTEST_CPP_FILE}
                       COMMAND exit 1
                       COMMENT "Unable to build ${CXXTEST_CPP_FILE} because python and perl were not found.")
  ENDIF()

  SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CXXTEST_CPP_FILE})

ENDMACRO(ADD_CXXTEST_CPP_TARGET)

#dependencies
IF(NOT PLATFORM_LIBS)
  SET(PLATFORM_LIBS ${TOP_LEVEL}/dependencies/)
ENDIF()
GET_FILENAME_COMPONENT(PLATFORM_LIBS ${PLATFORM_LIBS} ABSOLUTE)
IF(NOT PLATFORM_LIBS)
  MESSAGE(STATUS "Could not find dependencies directory ${TOP_LEVEL}/dependencies/")
ENDIF()
MESSAGE(STATUS "Using PLATFORM_LIBS=${PLATFORM_LIBS}")
SET(ScriptsRoot ${TOP_LEVEL}/liboh/scripts)
SET(DISABLE_M32 FALSE)
SET(FORCE_32BIT FALSE)
SET(DISABLE_AWESOMIUM FALSE)

IF(ISLINUX)
  IF(FORCE_32BIT OR EXISTS ${PLATFORM_LIBS}/arch-32bit)
    MESSAGE(STATUS "Performing 32-bit build")
    IF(NOT ${DISABLE_M32})
    MESSAGE(STATUS "Really Performing 32-bit build")
      SET(ADDED_DEFINITIONS ${ADDED_DEFINITIONS} -m32)
      IF(sirikata_LDFLAGS)
        SET(sirikata_LDFLAGS ${sirikata_LDFLAGS} -m32)
      ELSE()
        SET(sirikata_LDFLAGS -m32)   
      ENDIF()
    
    ENDIF()
  ELSE()
    MESSAGE(STATUS "Performing 64-bit build")
    # Must be built 32-bit to disable awesomium
    SET(DISABLE_AWESOMIUM TRUE)
  ENDIF()
ENDIF()
IF(APPLE)
  IF(CMAKE_SYSTEM_VERSION)
    IF(CMAKE_SYSTEM_VERSION MATCHES "[12345678]\\.[0-9]*\\.[0-9]*")
      SET(DISABLE_AWESOMIUM TRUE)
    ENDIF()
  ENDIF()
ENDIF()

#libraries
SET(MONO_MINIMUM_VERSION 2.0)
#dependency: mono
IF(NOT MONO_ROOT)
  IF(WIN32)
    SET(MONO_ROOT ${PLATFORM_LIBS}/mono-2.4)
  ELSEIF(APPLE)
    # TODO: Add path to some prebuilt version of Mono for Macs
  ELSE()
    SET(MONO_ROOT ${PLATFORM_LIBS}/installed-mono)
  ENDIF()
ENDIF()
FIND_PACKAGE(Mono)
IF(MONO_FOUND)
  ADD_DEFINITIONS(-DHAVE_MONO)
  ADD_DEFINITIONS(-DMONO_VERSION_MAJOR=${MONO_MAJOR_VERSION})
  ADD_DEFINITIONS(-DMONO_VERSION_MINOR=${MONO_MINOR_VERSION})
  ADD_DEFINITIONS(-DMONO_VERSION_MICRO=${MONO_SUBMINOR_VERSION})
  IF(EXISTS ${PLATFORM_LIBS}/IronPython)
    SET(IRON_PYTHON_ROOT ${PLATFORM_LIBS}/IronPython)
    SET(IRON_PYTHON_LIBRARIES
      -r:${IRON_PYTHON_ROOT}/IronPython.dll 
      -r:${IRON_PYTHON_ROOT}/IronPython.Modules.dll
      -r:${IRON_PYTHON_ROOT}/Microsoft.Scripting.dll
      -r:${IRON_PYTHON_ROOT}/Microsoft.Scripting.Core.dll
#      -r:${IRON_PYTHON_ROOT}/Microsoft.Scripting.ExtensionAttribute.dll
       )
  ENDIF(EXISTS ${PLATFORM_LIBS}/IronPython)
  FIND_PACKAGE(Cecil)
ENDIF(MONO_FOUND)

#dependency: protocolbuffers
IF(EXISTS ${PLATFORM_LIBS}/installed-protobufs/bin)
  SET(PROTOCOLBUFFERS_ROOT ${PLATFORM_LIBS}/installed-protobufs)
ELSEIF(EXISTS ${PLATFORM_LIBS}/protobufs/bin)
  SET(PROTOCOLBUFFERS_ROOT ${PLATFORM_LIBS}/protobufs)
ELSE()
  SET(PROTOCOLBUFFERS_ROOT ${PLATFORM_LIBS})
ENDIF()
FIND_PACKAGE(ProtocolBuffers)
IF(NOT PROTOCOLBUFFERS_FOUND)
  MESSAGE(FATAL_ERROR "Cannot find Protocol Buffers
Did you install the dependencies?
To install dependencies go to the top level and run:

make depends
")
ENDIF()

#dependency: ANTLR
IF(EXISTS ${PLATFORM_LIBS}/libantlr3c-3.1.3)
  SET(ANTLR_ROOT ${PLATFORM_LIBS}/libantlr3c-3.1.3)
ELSEIF(EXISTS ${PLATFORM_LIBS}/antlr3c-3.1.3)
  SET(ANTLR_ROOT ${PLATFORM_LIBS}/antlr3c-3.1.3)
ELSEIF(EXISTS ${PLATFORM_LIBS}/libantlr3c)
  SET(ANTLR_ROOT ${PLATFORM_LIBS}/libantlr3c)
ELSEIF(EXISTS ${PLATFORM_LIBS}/antlr3c)
  SET(ANTLR_ROOT ${PLATFORM_LIBS}/antlr3c)
ELSE()
  SET(ANTLR_ROOT ${PLATFORM_LIBS})
ENDIF()
IF(NOT WIN32)
IF(NOT APPLE)
  SET(ANTLR_ROOT ${PLATFORM_LIBS})
ENDIF()
ENDIF()
FIND_PACKAGE(ANTLR)
IF(NOT ANTLR_FOUND)
  MESSAGE(FATAL_ERROR "Cannot find antlr")
ENDIF()


#dependency: boost > 1.35
IF(NOT BOOST_ROOT)
  # IF(WIN32)
  #   SET(Boost_USE_STATIC_LIBS ON)
  # ENDIF()
  IF(EXISTS ${PLATFORM_LIBS}/installed-boost/lib)
    SET(BOOST_ROOT ${PLATFORM_LIBS}/installed-boost)
  ELSEIF(EXISTS ${PLATFORM_LIBS}/boost_1_35_0/lib)
    SET(BOOST_ROOT ${PLATFORM_LIBS}/boost_1_35_0)
  ELSE()
    SET(BOOST_ROOT ${PLATFORM_LIBS})
  ENDIF()
ENDIF()
SET(Boost_FIND_REQUIRED TRUE)
IF(WIN32)
  FIND_PACKAGE(Boost COMPONENTS system program_options thread filesystem)
ELSE(WIN32)
  FIND_PACKAGE(Boost COMPONENTS system)

  STRING(REPLACE "boost_system" "boost_program_options" Boost_PROGRAM_OPTIONS_LIBRARY ${Boost_SYSTEM_LIBRARY})
  STRING(REPLACE "boost_system" "boost_program_options" Boost_PROGRAM_OPTIONS_LIBRARY_DEBUG ${Boost_SYSTEM_LIBRARY_DEBUG})
  STRING(REPLACE "boost_system" "boost_program_options" Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE ${Boost_SYSTEM_LIBRARY_RELEASE})

  STRING(REPLACE "boost_system" "boost_thread" Boost_THREAD_LIBRARY ${Boost_SYSTEM_LIBRARY})
  STRING(REPLACE "boost_system" "boost_thread" Boost_THREAD_LIBRARY_DEBUG ${Boost_SYSTEM_LIBRARY_DEBUG})
  STRING(REPLACE "boost_system" "boost_thread" Boost_THREAD_LIBRARY_RELEASE ${Boost_SYSTEM_LIBRARY_RELEASE})

  STRING(REPLACE "boost_system" "boost_filesystem" Boost_FILESYSTEM_LIBRARY ${Boost_SYSTEM_LIBRARY})
  STRING(REPLACE "boost_system" "boost_filesystem" Boost_FILESYSTEM_LIBRARY_DEBUG ${Boost_SYSTEM_LIBRARY_DEBUG})
  STRING(REPLACE "boost_system" "boost_filesystem" Boost_FILESYSTEM_LIBRARY_RELEASE ${Boost_SYSTEM_LIBRARY_RELEASE})

  STRING(REPLACE "boost_system" "boost_regex" Boost_REGEX_LIBRARY ${Boost_SYSTEM_LIBRARY})
  STRING(REPLACE "boost_system" "boost_regex" Boost_REGEX_LIBRARY_DEBUG ${Boost_SYSTEM_LIBRARY_DEBUG})
  STRING(REPLACE "boost_system" "boost_regex" Boost_REGEX_LIBRARY_RELEASE ${Boost_SYSTEM_LIBRARY_RELEASE})
ENDIF(WIN32)
VERIFY_VERSION(Boost 1 35 0)

SET(Boost_LIBRARIES
    ${Boost_SYSTEM_LIBRARY_${UPPERCASE_BUILD_TYPE}}
    ${Boost_PROGRAM_OPTIONS_LIBRARY_${UPPERCASE_BUILD_TYPE}}
    ${Boost_THREAD_LIBRARY_${UPPERCASE_BUILD_TYPE}}
    ${Boost_FILESYSTEM_LIBRARY_${UPPERCASE_BUILD_TYPE}})

#dependency: curl
IF(NOT CURL_ROOT)
  IF(EXISTS ${PLATFORM_LIBS}/installed-curl)
    SET(CURL_ROOT ${PLATFORM_LIBS}/installed-curl)
  ELSEIF(EXISTS ${PLATFORM_LIBS}/libcurl_7_18_0)
    SET(CURL_ROOT ${PLATFORM_LIBS}/libcurl_7_18_0)
  ELSEIF(EXISTS ${PLATFORM_LIBS}/libcurl_7_15_5)
    SET(CURL_ROOT ${PLATFORM_LIBS}/libcurl_7_15_5)
  ENDIF()
  IF(WIN32)
    GET_FILENAME_COMPONENT(CURL_ROOT ${CURL_ROOT} ABSOLUTE)
  ENDIF()
  IF(NOT CURL_ROOT)
    SET(CURL_ROOT ${PLATFORM_LIBS}/libcurl_7_15_5)
  ENDIF()
ENDIF()

FIND_PACKAGE(CURL)
SET(TEST_LIBRARIES ${CURL_LIBRARIES})

#dependency: ois
IF(NOT OIS_ROOT)
  IF(EXISTS ${PLATFORM_LIBS}/installed-ois)
    SET(OIS_ROOT ${PLATFORM_LIBS}/installed-ois)
  ELSEIF(EXISTS ${PLATFORM_LIBS}/ois-1.2)
    SET(OIS_ROOT ${PLATFORM_LIBS}/ois-1.2)
  ELSEIF(EXISTS ${PLATFORM_LIBS}/ois-1.0)
    SET(OIS_ROOT ${PLATFORM_LIBS}/ois-1.0)
  ENDIF()
  IF(WIN32 AND OIS_ROOT)
    GET_FILENAME_COMPONENT(OIS_ROOT ${OIS_ROOT} ABSOLUTE)
  ENDIF()
  IF(NOT OIS_ROOT)
    SET(OIS_ROOT ${PLATFORM_LIBS})
  ENDIF()
ENDIF()

FIND_PACKAGE(OIS)

#dependency: ogre
IF(NOT OGRE_ROOT)
  IF(EXISTS ${PLATFORM_LIBS}/installed-ogre)
    SET(OGRE_ROOT ${PLATFORM_LIBS}/installed-ogre)
  ELSEIF(EXISTS ${PLATFORM_LIBS}/ogre-1.6.1)
    SET(OGRE_ROOT ${PLATFORM_LIBS}/ogre-1.6.1)
  ELSEIF(EXISTS ${PLATFORM_LIBS}/ogre-1.6.x)
    SET(OGRE_ROOT ${PLATFORM_LIBS}/ogre-1.6.x)
  ELSEIF(EXISTS ${PLATFORM_LIBS}/ogre)
    SET(OGRE_ROOT ${PLATFORM_LIBS}/ogre)
  ENDIF()
  IF(WIN32 AND OGRE_ROOT)
    GET_FILENAME_COMPONENT(OGRE_ROOT ${OGRE_ROOT} ABSOLUTE)
  ENDIF()
  IF(NOT OGRE_ROOT)
    SET(OGRE_ROOT ${PLATFORM_LIBS})
  ENDIF()
ENDIF()

FIND_PACKAGE(OGRE)
IF(WIN32)
SET(bullet_ROOT ${PLATFORM_LIBS}/bullet-2.74)
ELSE()
SET(bullet_ROOT ${PLATFORM_LIBS}/installed-bullet)
ENDIF()
SET(bullet_MINIMUM_VERSION 2.74)
FIND_PACKAGE(Bullet)



IF(NOT SQLite3_ROOT)
  IF(WIN32)
    SET(SQLite3_ROOT ${PLATFORM_LIBS}/sqlite-3_6_16)
  ELSE()
    IF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
      SET(SQLite3_ROOT ${PLATFORM_LIBS})
    ELSE()
      SET(SQLite3_ROOT ${PLATFORM_LIBS}/installed-sqlite)     
    ENDIF()
  ENDIF()
ENDIF()

SET(SQLite3_FIND_REQUIRED TRUE)
FIND_PACKAGE(SQLite3)



SET(sdl_MINIMUM_VERSION 1.3.0)
IF(NOT sdl_ROOT)
  IF(EXISTS ${PLATFORM_LIBS}/installed-sdl)
    SET(sdl_ROOT ${PLATFORM_LIBS}/installed-sdl)
  ELSEIF(EXISTS ${PLATFORM_LIBS}/sdl-1.3)
    SET(sdl_ROOT ${PLATFORM_LIBS}/sdl-1.3)
  ELSEIF(EXISTS ${PLATFORM_LIBS}/sdl-1.3.0)
    SET(sdl_ROOT ${PLATFORM_LIBS}/sdl-1.3.0)
  ELSEIF(EXISTS ${PLATFORM_LIBS}/sdl)
    SET(sdl_ROOT ${PLATFORM_LIBS}/sdl)
  ENDIF()
  IF(WIN32 AND sdl_ROOT)
    GET_FILENAME_COMPONENT(sdl_ROOT ${sdl_ROOT} ABSOLUTE)
  ENDIF()
  IF(APPLE OR NOT sdl_ROOT)
    SET(sdl_ROOT ${PLATFORM_LIBS})
  ENDIF()
ENDIF()

FIND_PACKAGE(SDL)

#optional dependency: prox
FIND_PACKAGE(PROX)
IF(NOT PROX_INCLUDE_DIRS)
  MESSAGE(FATAL_ERROR "PROX submodule not found, go to top level and run:

git submodule init
git submodule update
")
ENDIF()
#dependency: awesomium
IF(NOT AWESOMIUM_ROOT)
  IF(EXISTS ${PLATFORM_LIBS}/awesomium_v1.01_sdk_msvc8)
    SET(AWESOMIUM_ROOT ${PLATFORM_LIBS}/awesomium_v1.01_sdk_msvc8)
  ELSEIF(EXISTS ${PLATFORM_LIBS}/awesomium_v1.01_sdk_msvc9)
    SET(AWESOMIUM_ROOT ${PLATFORM_LIBS}/awesomium_v1.01_sdk_msvc9)
  ELSEIF(EXISTS ${PLATFORM_LIBS}/installed-awesomium)
    SET(AWESOMIUM_ROOT ${PLATFORM_LIBS}/installed-awesomium)
  ELSEIF(EXISTS ${PLATFORM_LIBS}/awesomium)
    SET(AWESOMIUM_ROOT ${PLATFORM_LIBS}/awesomium)
  ENDIF()
  IF(WIN32 AND AWESOMIUM_ROOT)
    GET_FILENAME_COMPONENT(AWESOMIUM_ROOT ${AWESOMIUM_ROOT} ABSOLUTE)
  ENDIF()
  IF(NOT AWESOMIUM_ROOT)
    SET(AWESOMIUM_ROOT ${PLATFORM_LIBS})
  ENDIF()
ENDIF()

IF(NOT DISABLE_AWESOMIUM)
  FIND_PACKAGE(AWESOMIUM)
ENDIF()

#dependency: cxxtest
SET(CXXTESTRoot ${TOP_LEVEL}/externals/cxxtest)


# macro for adding a plugin
MACRO(ADD_PLUGIN_TARGET)
  PARSE_ARGUMENTS(PLUGIN "SOURCES;TARGET_CXXFLAGS;TARGET_LDFLAGS;TARGET_LIBRARIES;LIBRARIES" "" ${ARGN})

  CAR(PLUGIN_NAME ${PLUGIN_DEFAULT_ARGS})

  ADD_LIBRARY(${PLUGIN_NAME} SHARED ${PLUGIN_SOURCES})
  IF(PLUGIN_TARGET_LIBRARIES)
    ADD_DEPENDENCIES(${PLUGIN_NAME} ${PLUGIN_TARGET_LIBRARIES})
  ENDIF()
  TARGET_LINK_LIBRARIES(${PLUGIN_NAME} ${PLUGIN_TARGET_LIBRARIES} ${PLUGIN_LIBRARIES})
  IF(PLUGIN_TARGET_LDFLAGS)
    STRING(REGEX REPLACE ";" " " PLUGIN_TARGET_LDFLAGS  "${PLUGIN_TARGET_LDFLAGS}")  #FIXME does not deal with space-having ldflags
    SET_TARGET_PROPERTIES(${PLUGIN_NAME} PROPERTIES LINK_FLAGS ${PLUGIN_TARGET_LDFLAGS})
  ENDIF()
  IF(PLUGIN_TARGET_CXXFLAGS)
    STRING(REGEX REPLACE ";" " " PLUGIN_TARGET_CXXFLAGS  "${PLUGIN_TARGET_CXXFLAGS}")  #FIXME does not deal with space-having ldflags
    SET_TARGET_PROPERTIES(${PLUGIN_NAME} PROPERTIES COMPILE_FLAGS ${PLUGIN_TARGET_CXXFLAGS})
  ENDIF()
ENDMACRO(ADD_PLUGIN_TARGET)


#project directory locations
SET(LIBCORE_DIR ${TOP_LEVEL}/libcore)
SET(LIBSPACE_DIR ${TOP_LEVEL}/libspace)
SET(LIBPROXIMITY_DIR ${TOP_LEVEL}/libproximity)
SET(LIBSUBSCRIPTION_DIR ${TOP_LEVEL}/libsubscription)
SET(LIBOH_DIR ${TOP_LEVEL}/liboh)
SET(SPACE_DIR ${TOP_LEVEL}/space)
SET(SUBSCRIPTION_DIR ${TOP_LEVEL}/subscription)
SET(PROXIMITY_DIR ${TOP_LEVEL}/proximity)
SET(CPPOH_DIR ${TOP_LEVEL}/cppoh)

#include locations
SET(LIBSPACE_INCLUDE_DIR ${LIBSPACE_DIR}/include)
SET(LIBPROXIMITY_INCLUDE_DIR ${LIBPROXIMITY_DIR}/include)
SET(LIBSUBSCRIPTION_INCLUDE_DIR ${LIBSUBSCRIPTION_DIR}/include)
SET(LIBOH_INCLUDE_DIR ${LIBOH_DIR}/include)

#source file locations
SET(LIBCORE_SOURCE_DIR ${LIBCORE_DIR}/src)
SET(LIBSPACE_SOURCE_DIR ${LIBSPACE_DIR}/src)
SET(LIBPROXIMITY_SOURCE_DIR ${LIBPROXIMITY_DIR}/src)
SET(LIBSUBSCRIPTION_SOURCE_DIR ${LIBSUBSCRIPTION_DIR}/src)
SET(LIBOH_SOURCE_DIR ${LIBOH_DIR}/src)
SET(SPACE_SOURCE_DIR ${SPACE_DIR}/src)
SET(PROXIMITY_SOURCE_DIR ${PROXIMITY_DIR}/src)
SET(SUBSCRIPTION_SOURCE_DIR ${SUBSCRIPTION_DIR}/src)
SET(CPPOH_SOURCE_DIR ${CPPOH_DIR}/src)

#plugins locations
SET(LIBCORE_PLUGIN_DIR ${LIBCORE_DIR}/plugins)
SET(LIBPROXIMITY_PLUGIN_DIR ${LIBPROXIMITY_DIR}/plugins)
#SET(LIBSUBSCRIPTION_PLUGIN_DIR ${LIBSUBSCRIPTION_DIR}/plugins)
SET(LIBOH_PLUGIN_DIR ${LIBOH_DIR}/plugins)

#cxx flags
# FIXME these include directories should be project specific



SET(INCLUDE_DIRECTORIES
  ${LIBCORE_SOURCE_DIR}
  ${LIBSPACE_INCLUDE_DIR}
  ${LIBPROXIMITY_INCLUDE_DIR}
  ${LIBSUBSCRIPTION_INCLUDE_DIR}
  ${LIBOH_INCLUDE_DIR}
  ${LIBOH_DIR}/plugins/ogre/meruCompat #temporary hack
  ${SPACE_SOURCE_DIR}
  ${PROXIMITY_SOURCE_DIR}
  ${SUBSCRIPTION_SOURCE_DIR}
  ${CPPOH_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${CURL_INCLUDE_DIRS}
  ${PROTOCOLBUFFERS_INCLUDE_DIRS}
  ${ANTLR_INCLUDE_DIRS}
  ${PROX_INCLUDE_DIRS}
  ${CXXTESTRoot}
  ${CMAKE_CURRENT_SOURCE_DIR} #generated files
  ${bullet_INCLUDE_DIRS}
  ${SQLite3_INCLUDE_DIRS}
  )
IF(OGRE_FOUND AND sdl_FOUND)
SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${OGRE_INCLUDE_DIRS} ${sdl_INCLUDE_DIRS})
ENDIF()
IF(MONO_FOUND)
SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${MONO_INCLUDE_DIRS})
ENDIF()
IF(AWESOMIUM_FOUND)
SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${AWESOMIUM_INCLUDE_DIRS})
ENDIF()

SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${TOP_LEVEL}/externals/pbj/)

INCLUDE_DIRECTORIES(BEFORE ${INCLUDE_DIRECTORIES})
ADD_DEFINITIONS(${ADDED_DEFINITIONS})

SET(PBJ_SOURCE_FILES 
  ${TOP_LEVEL}/externals/pbj/PBJParseUtil.cpp
  ${TOP_LEVEL}/externals/pbj/PBJLexer.cpp
  ${TOP_LEVEL}/externals/pbj/PBJParser.cpp 
  ${TOP_LEVEL}/externals/pbj/main.cpp)
SET(PBJ_BINARY pbj)
ADD_EXECUTABLE(${PBJ_BINARY} ${PBJ_SOURCE_FILES} )
TARGET_LINK_LIBRARIES(${PBJ_BINARY} ${ANTLR_LIBRARIES})
IF(NOT WIN32)
IF(NOT APPLE)
#SET_TARGET_PROPERTIES(${PBJ_BINARY} PROPERTIES COMPILE_FLAGS -m32)
#SET_TARGET_PROPERTIES(${PBJ_BINARY} PROPERTIES LINK_FLAGS -m32)
ENDIF()
ENDIF()
GET_TARGET_PROPERTY(PBJ_RUNABLE ${PBJ_BINARY} LOCATION)

SET(INCLUDE_COMMAND)
FOREACH(INCLUDE_NONCE ${INCLUDE_DIRECTORIES})
  SET(INCLUDE_COMMAND  ${INCLUDE_COMMAND} -I${INCLUDE_NONCE})
ENDFOREACH()

#protocol buffers preprocessing
SET(ProtocolBuffersRoot ${LIBCORE_DIR}/protocol)
SET(ProtocolBuffersSources
  Persistence Sirikata Subscription Test
)


SET(SirikataProtocolDirectory ${LIBCORE_SOURCE_DIR})

# Based on dependencies, generate arguments for protocol buffers generation
SET(PROTOCOLBUFFERS_GENS GENERATE_CPP)
SET(SCRIPTING_PROTOCOLBUFFERS_GENS ${PROTOCOLBUFFERS_GENS})
IF(MONO_FOUND AND PROTOCOLBUFFERS_SUPPORTS_CSHARP)
  SET(SCRIPTING_PROTOCOLBUFFERS_GENS ${SCRIPTING_PROTOCOLBUFFERS_GENS} GENERATE_CSHARP)
  # FIXME there should probably be another check for IronPython
  SET(SCRIPTING_PROTOCOLBUFFERS_GENS ${SCRIPTING_PROTOCOLBUFFERS_GENS} GENERATE_PYTHON)
ENDIF()


SET(PROTOCOLBUFFERS_CPP_HEADER "#include \"util/Standard.hh\"\n")

ADD_PBJ_TARGET(MessageHeader
  INPUTDIR ${ProtocolBuffersRoot}
  PLUGINNAME "Protocol"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  OUTPUTCPPFILE ${SirikataProtocolDirectory}/MessageHeader_protobuf.cc
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  ${PROTOCOLBUFFERS_GENS}
)

ADD_PBJ_TARGET(${ProtocolBuffersSources}
  INPUTDIR ${ProtocolBuffersRoot}
  PLUGINNAME "Prox"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  OUTPUTCPPFILE ${SirikataProtocolDirectory}/Prox_protobuf.cc
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  ${PROTOCOLBUFFERS_GENS}
)

# For scripting
SET(ScriptingProtocolBuffersSources ${ProtocolBuffersSources} MessageHeader Physics)

ADD_PBJ_TARGET(${ScriptingProtocolBuffersSources}
  INPUTDIR ${ProtocolBuffersRoot}
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  GENERATE_CSHARP GENERATE_PYTHON
  ${SCRIPTING_PROTOCOLBUFFERS_GENS}
)
SET(PROTOCOLBUFFERS_SCRIPTING_OUTPUTS ${PROTOCOLBUFFERS_ALL_OUTPUTS})

ADD_PBJ_TARGET(${ProtocolBuffersSources}
  INPUTDIR ${ProtocolBuffersRoot}
  PLUGINNAME "ObjectHost"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  OUTPUTCPPFILE ${SirikataProtocolDirectory}/ObjectHost_protobuf.cc
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  ${PROTOCOLBUFFERS_GENS}
)

ADD_PBJ_TARGET(${ProtocolBuffersSources}
  INPUTDIR ${ProtocolBuffersRoot}
  PLUGINNAME "Ogre"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  OUTPUTCPPFILE ${SirikataProtocolDirectory}/Ogre_protobuf.cc
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  ${PROTOCOLBUFFERS_GENS}
)

ADD_PBJ_TARGET(Sirikata Physics
  INPUTDIR ${ProtocolBuffersRoot}
  PLUGINNAME "Bullet"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  OUTPUTCPPFILE ${SirikataProtocolDirectory}/Bullet_protobuf.cc
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  ${PROTOCOLBUFFERS_GENS}
)

ADD_PBJ_TARGET(${ProtocolBuffersSources}
  INPUTDIR ${ProtocolBuffersRoot}
  PLUGINNAME "ObjectHostBinary"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  OUTPUTCPPFILE ${SirikataProtocolDirectory}/ObjectHostBinary_protobuf.cc
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  ${PROTOCOLBUFFERS_GENS}
)

ADD_PBJ_TARGET(${ProtocolBuffersSources}
  INPUTDIR ${ProtocolBuffersRoot}
  PLUGINNAME "Space"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  OUTPUTCPPFILE ${SirikataProtocolDirectory}/Space_protobuf.cc
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  ${PROTOCOLBUFFERS_GENS}
)

ADD_PBJ_TARGET(${ProtocolBuffersSources}
  INPUTDIR ${ProtocolBuffersRoot}
  PLUGINNAME "Test"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  OUTPUTCPPFILE ${SirikataProtocolDirectory}/Test_protobuf.cc
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  ${PROTOCOLBUFFERS_GENS}
)

ADD_PBJ_TARGET(Persistence
  INPUTDIR ${ProtocolBuffersRoot}
  PLUGINNAME "SQLite"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  OUTPUTCPPFILE ${SirikataProtocolDirectory}/SQLite_protobuf.cc
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  ${PROTOCOLBUFFERS_GENS}
)

ADD_PBJ_TARGET(${ProtocolBuffersSources}
  INPUTDIR ${ProtocolBuffersRoot}
  PLUGINNAME "Proximity"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  OUTPUTCPPFILE ${SirikataProtocolDirectory}/Proximity_protobuf.cc
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  ${PROTOCOLBUFFERS_GENS}
)

ADD_PBJ_TARGET(Subscription
  INPUTDIR
  ${ProtocolBuffersRoot}
  PLUGINNAME "Subscription"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  OUTPUTCPPFILE ${SirikataProtocolDirectory}/Subscription_protobuf.cc
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  ${PROTOCOLBUFFERS_GENS}
)


#binary files to be linked into sirikata
SET(BINARY_TO_CPP_SOURCE_FILE ${TOP_LEVEL}/externals/binarytocpp/binarytocpp.cpp)
SET(BINARY_TO_CPP_BINARY binarytocpp)
ADD_EXECUTABLE(${BINARY_TO_CPP_BINARY} ${BINARY_TO_CPP_SOURCE_FILE} )
GET_TARGET_PROPERTY(BINARY_TO_CPP_RUNABLE ${BINARY_TO_CPP_BINARY} LOCATION)

IF(OGRE_ZIP_PATH)
    SET(SirikataOgreCoreZipCpp ${CMAKE_CURRENT_SOURCE_DIR}/ogreCoreZip.hpp)
    ADD_CUSTOM_COMMAND(OUTPUT ${SirikataOgreCoreZipCpp}
                       COMMAND ${BINARY_TO_CPP_RUNABLE} -namespace=Sirikata ogreCoreZip ${OGRE_ZIP_PATH} ${SirikataOgreCoreZipCpp}
                       DEPENDS ${BINARY_TO_CPP_BINARY}
                       COMMENT "Creating OgreZip cpp file")
  SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${SirikataOgreCoreZipCpp})
ENDIF()
#source files

SET(AdditionalProtoDepends)
FOREACH(FILE ${ScriptingProtocolBuffersSources})
   SET(AdditionalProtoDepends ${AdditionalProtoDepends} ${ScriptsRoot}/ironpython/protocol/${FILE}_pb2.py)
ENDFOREACH()
ADD_CUSTOM_TARGET(SirikataPythonProtocol ALL
              DEPENDS ${AdditionalProtoDepends}
              COMMENT "Checking ${AdditionalProtoDepends}")

IF(MONO_FOUND)
  SET(SirikataProtoScriptSources ${SirikataProtoScriptSources})
  FOREACH(FILE ${ScriptingProtocolBuffersSources})
     SET(SirikataProtoScriptSources ${SirikataProtoScriptSources}
  ${ScriptsRoot}/csharp/protocol/${FILE}.pbj.cs ${ScriptsRoot}/csharp/protocol/${FILE}.cs )
  ENDFOREACH()  

  SET(SirikataProtoScriptSources ${SirikataProtoScriptSources} ${TOP_LEVEL}/externals/pbj/PBJ.cs)
  SET(SirikataProtoScriptRuntime_LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
  SET(SirikataProtoScriptRuntime_LIBRARY ${SirikataProtoScriptRuntime_LIBRARY_DIR}/Sirikata.Protocol.dll)
  IF(PROTOCOLBUFFERS_SUPPORTS_CSHARP)
    ADD_CUSTOM_TARGET(SirikataScriptProtocol ALL
                  DEPENDS ${SirikataProtoScriptSources} ${SirikataProtoScriptRuntime_LIBRARY}
                  COMMENT "Checking Sirikata.Protocol.dll")
    ADD_CUSTOM_COMMAND(OUTPUT ${SirikataProtoScriptRuntime_LIBRARY}
                   COMMAND ${GMCS_EXECUTABLE} -debug
                                     -target:library
                                     -out:${SirikataProtoScriptRuntime_LIBRARY}
                                     -warn:2
                                     -r:${PROTOCOLBUFFERS_BIN_DIRS}/Google.ProtocolBuffers.dll
                                     ${SirikataProtoScriptSources}
                   DEPENDS ${SirikataProtoScriptSources}
                   COMMENT "Building Sirikata.Protocol.dll")
  ENDIF()
  SET(SirikataProtoScriptRuntimeBuildOutputs ${SirikataProtoScriptRuntime_LIBRARY} ${SirikataProtoScriptRuntime_LIBRARY}.mdb)
  SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${SirikataProtoScriptRuntimeBuildOutputs}")


  FILE(GLOB SirikataScriptSources ${ScriptsRoot}/csharp/*.cs )
  SET(SirikataScriptRuntime_LIBRARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
  SET(SirikataScriptRuntime_LIBRARY ${SirikataScriptRuntime_LIBRARY_DIR}/Sirikata.Runtime.dll)
  ADD_CUSTOM_TARGET(SirikataScriptRuntime ALL
                  DEPENDS ${SirikataScriptSources} ${SirikataScriptRuntime_LIBRARY}
                  COMMENT "Checking Sirikata.Runtime.dll")
  ADD_CUSTOM_COMMAND(OUTPUT ${SirikataScriptRuntime_LIBRARY}
                   COMMAND ${GMCS_EXECUTABLE} -debug
                                     -target:library
                                     -out:${SirikataScriptRuntime_LIBRARY}
                                     -warn:2
                                     ${CECIL_LIBRARIES}
                                     ${IRON_PYTHON_LIBRARIES}
                                     ${SirikataScriptSources}
                   DEPENDS ${SirikataScriptSources}
                   COMMENT "Building Sirikata.Runtime.dll")
  SET(SirikataScriptRuntimeBuildOutputs ${SirikataScriptRuntime_LIBRARY} ${SirikataScriptRuntime_LIBRARY}.mdb)
  SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${SirikataScriptRuntimeBuildOutputs}")

ENDIF()

SET(SIRIKATA_CORE_SOURCES
	${PROTOCOLBUFFERS_SCRIPTING_OUTPUTS}
	${SirikataProtocolDirectory}/MessageHeader_protobuf.cc
	${LIBCORE_SOURCE_DIR}/transfer/HTTPRequest.cpp
	${LIBCORE_SOURCE_DIR}/transfer/FileProtocolHandler.cpp
	${LIBCORE_SOURCE_DIR}/transfer/DiskCacheLayer.cpp
	${LIBCORE_SOURCE_DIR}/persistence/ObjectStorage.cpp
	${LIBCORE_SOURCE_DIR}/persistence/ReadWriteHandlerFactory.cpp
	${LIBCORE_SOURCE_DIR}/persistence/MinitransactionHandlerFactory.cpp
	${LIBCORE_SOURCE_DIR}/task/DependencyTask.cpp
	${LIBCORE_SOURCE_DIR}/task/EventManager.cpp
	${LIBCORE_SOURCE_DIR}/task/WorkQueue.cpp
	${LIBCORE_SOURCE_DIR}/task/Event.cpp
	${LIBCORE_SOURCE_DIR}/task/UniqueId.cpp
	${LIBCORE_SOURCE_DIR}/task/Time.cpp
   	${LIBCORE_SOURCE_DIR}/options/Options.cpp
	${LIBCORE_SOURCE_DIR}/network/IOServiceFactory.cpp
	${LIBCORE_SOURCE_DIR}/network/TCPDefinitions.cpp
	${LIBCORE_SOURCE_DIR}/network/Stream.cpp
	${LIBCORE_SOURCE_DIR}/network/StreamListener.cpp
	${LIBCORE_SOURCE_DIR}/network/StreamFactory.cpp
	${LIBCORE_SOURCE_DIR}/network/StreamListenerFactory.cpp
	${LIBCORE_SOURCE_DIR}/util/DynamicLibrary.cpp
	${LIBCORE_SOURCE_DIR}/util/internal_sha2.cpp
	${LIBCORE_SOURCE_DIR}/util/Logging.cpp
	${LIBCORE_SOURCE_DIR}/util/Plugin.cpp
	${LIBCORE_SOURCE_DIR}/util/PluginManager.cpp
	${LIBCORE_SOURCE_DIR}/util/Sha256.cpp
	${LIBCORE_SOURCE_DIR}/util/ThreadSafeQueue.cpp
	${LIBCORE_SOURCE_DIR}/util/UUID.cpp
    ${LIBCORE_SOURCE_DIR}/util/ThreadId.cpp
	${LIBCORE_SOURCE_DIR}/util/BoundingInfo.cpp
        ${LIBCORE_SOURCE_DIR}/util/SentMessage.cpp
        ${LIBCORE_SOURCE_DIR}/util/QueryTracker.cpp
)

#precompiled header
IF(NOT WIN32)
 IF(${CMAKE_COMPILER_IS_GNUCXX})
  SET(SIRIKATA_CORE_SOURCES
     ${LIBCORE_SOURCE_DIR}/util/Standard.hh.gch ${SIRIKATA_CORE_SOURCES})
  SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
    ${LIBCORE_SOURCE_DIR}/util/Standard.hh.gch)

  SET(PCH_INPUT ${LIBCORE_SOURCE_DIR}/util/Standard.hh)
  SET(PCH_OUTPUT ${LIBCORE_SOURCE_DIR}/util/Standard.hh.gch)
  IF(ISDEBUG)
    SET(PCH_CXX_FLAGS  ${CMAKE_CXX_FLAGS_DEBUG} )
  ELSE()
    SET(PCH_CXX_FLAGS  ${CMAKE_CXX_FLAGS_RELEASE} )
  ENDIF()
  SEPARATE_ARGUMENTS(PCH_CXX_FLAGS)
  SET(FINAL_COMMAND ${CMAKE_CXX_COMPILER} ${PCH_CXX_FLAGS} ${INCLUDE_COMMAND} ${ADDED_DEFINITIONS} ${PCH_INPUT})
  ADD_CUSTOM_COMMAND(OUTPUT ${PCH_OUTPUT}
                     COMMAND ${FINAL_COMMAND}
                     DEPENDS ${PCH_INPUT}
 ${LIBCORE_SOURCE_DIR}/options/OptionValue.hpp
 ${LIBCORE_SOURCE_DIR}/util/Any.hpp
 ${LIBCORE_SOURCE_DIR}/util/Array.hpp
 ${LIBCORE_SOURCE_DIR}/util/BoundingBox.hpp
 ${LIBCORE_SOURCE_DIR}/util/BoundingSphere.hpp
 ${LIBCORE_SOURCE_DIR}/util/Factory.hpp
 ${LIBCORE_SOURCE_DIR}/util/Location.hpp
 ${LIBCORE_SOURCE_DIR}/util/Logging.hpp
 ${LIBCORE_SOURCE_DIR}/util/Matrix3x3.hpp
 ${LIBCORE_SOURCE_DIR}/util/Noncopyable.hpp
 ${LIBCORE_SOURCE_DIR}/util/Platform.hpp
 ${LIBCORE_SOURCE_DIR}/util/Quaternion.hpp
 ${LIBCORE_SOURCE_DIR}/util/SelfWeakPtr.hpp
 ${LIBCORE_SOURCE_DIR}/util/Singleton.hpp
 ${LIBCORE_SOURCE_DIR}/util/Standard.hh
 ${LIBCORE_SOURCE_DIR}/util/TotallyOrdered.hpp
 ${LIBCORE_SOURCE_DIR}/util/Transform.hpp
 ${LIBCORE_SOURCE_DIR}/util/BoundingInfo.hpp
 ${LIBCORE_SOURCE_DIR}/util/UUID.hpp
 ${LIBCORE_SOURCE_DIR}/util/ThreadId.hpp
 ${LIBCORE_SOURCE_DIR}/util/Vector3.hpp
 ${LIBCORE_SOURCE_DIR}/util/Vector4.hpp
 ${LIBCORE_SOURCE_DIR}/util/boost_sha1.hpp
 ${LIBCORE_SOURCE_DIR}/util/boost_uuid.hpp
 ${LIBCORE_SOURCE_DIR}/util/valgrind.h
                     COMMENT "${FINAL_COMMAND}")
#  ADD_DEPENDENCIES(${SIRIKATA_CORE_LIB} ${PCH_OUTPUT})
 ENDIF()
ENDIF()

SET(LIBSPACE_SOURCES 
  ${SirikataProtocolDirectory}/Space_protobuf.cc
                     ${LIBSPACE_SOURCE_DIR}/Space.cpp
                     ${LIBSPACE_SOURCE_DIR}/ObjectConnections.cpp
                     ${LIBSPACE_SOURCE_DIR}/Loc.cpp
                     ${LIBSPACE_SOURCE_DIR}/Registration.cpp
                      )
SET(LIBPROXIMITY_SOURCES 
                  ${SirikataProtocolDirectory}/Proximity_protobuf.cc
                  ${LIBPROXIMITY_SOURCE_DIR}/ProximityConnectionFactory.cpp 
                  ${LIBPROXIMITY_SOURCE_DIR}/ProximitySystem.cpp 
                  ${LIBPROXIMITY_SOURCE_DIR}/ProximitySystemFactory.cpp 
                  ${LIBPROXIMITY_SOURCE_DIR}/SingleStreamProximityConnection.cpp )

SET(LIBSUBSCRIPTION_SOURCES 
                  ${SirikataProtocolDirectory}/Subscription_protobuf.cc
                  ${LIBSUBSCRIPTION_SOURCE_DIR}/SubscriptionClient.cpp 
                  ${LIBSUBSCRIPTION_SOURCE_DIR}/Server.cpp 
                  ${LIBSUBSCRIPTION_SOURCE_DIR}/Broadcast.cpp 
                  ${LIBSUBSCRIPTION_SOURCE_DIR}/SubscriptionState.cpp
 )

SET(LIBOH_SOURCES
                  ${SirikataProtocolDirectory}/ObjectHost_protobuf.cc
                  ${LIBOH_SOURCE_DIR}/ObjectHost.cpp
                  ${LIBOH_SOURCE_DIR}/SpaceIDMap.cpp
                  ${LIBOH_SOURCE_DIR}/HostedObject.cpp
                  ${LIBOH_SOURCE_DIR}/ObjectHostProxyManager.cpp
                  ${LIBOH_SOURCE_DIR}/TopLevelSpaceConnection.cpp
                  ${LIBOH_SOURCE_DIR}/SpaceConnection.cpp
                  ${LIBOH_SOURCE_DIR}/LightInfo.cpp
                  ${LIBOH_SOURCE_DIR}/ProxyMeshObject.cpp
                  ${LIBOH_SOURCE_DIR}/ProxyLightObject.cpp
                  ${LIBOH_SOURCE_DIR}/ProxyObject.cpp
                  ${LIBOH_SOURCE_DIR}/ProxyCameraObject.cpp
                  ${LIBOH_SOURCE_DIR}/ProxyWebViewObject.cpp
                  ${LIBOH_SOURCE_DIR}/SimulationFactory.cpp
                  ${LIBOH_SOURCE_DIR}/ObjectScriptManagerFactory.cpp )
SET(SPACE_SOURCES ${SPACE_SOURCE_DIR}/main.cpp )
SET(PROXIMITY_SOURCES ${PROXIMITY_SOURCE_DIR}/main.cpp )
SET(SUBSCRIPTION_SOURCES ${SUBSCRIPTION_SOURCE_DIR}/main.cpp )
SET(CPPOH_SOURCES ${CPPOH_SOURCE_DIR}/main.cpp
${CPPOH_SOURCE_DIR}/Config.cpp
${CPPOH_SOURCE_DIR}/CDNConfig.cpp
  ${SirikataProtocolDirectory}/ObjectHostBinary_protobuf.cc
 )

# plugins sources

SET(LIBPROXIMITY_PLUGIN_PROX_DIR ${LIBPROXIMITY_PLUGIN_DIR}/prox)
SET(LIBPROXIMITY_PLUGIN_PROX_SOURCES
  ${SirikataProtocolDirectory}/Prox_protobuf.cc
  ${LIBPROXIMITY_PLUGIN_PROX_DIR}/ProxPlugin.cpp
  ${LIBPROXIMITY_PLUGIN_PROX_DIR}/ProxBridge.cpp
  ${LIBPROXIMITY_PLUGIN_PROX_DIR}/BruteForceProx.cpp
  ${PROX_SOURCE_FILES})


SET(LIBCORE_PLUGIN_SKELETON_DIR ${LIBCORE_PLUGIN_DIR}/skeleton)
SET(LIBCORE_PLUGIN_SKELETON_SOURCES ${LIBCORE_PLUGIN_SKELETON_DIR}/PluginInterface.cpp)

SET(LIBCORE_PLUGIN_SQLITE_DIR ${LIBCORE_PLUGIN_DIR}/sqlite)
SET(LIBCORE_PLUGIN_SQLITE_SOURCES
        ${SirikataProtocolDirectory}/SQLite_protobuf.cc
        ${LIBCORE_PLUGIN_SQLITE_DIR}/SQLitePlugin.cpp
        ${LIBCORE_PLUGIN_SQLITE_DIR}/SQLite.cpp
        ${LIBCORE_PLUGIN_SQLITE_DIR}/SQLiteObjectStorage.cpp)


SET(LIBCORE_PLUGIN_TCPSST_DIR ${LIBCORE_PLUGIN_DIR}/tcpsst)
SET(LIBCORE_PLUGIN_TCPSST_SOURCES
        ${LIBCORE_PLUGIN_TCPSST_DIR}/TcpsstPlugin.cpp
        ${LIBCORE_PLUGIN_TCPSST_DIR}/TCPStream.cpp
        ${LIBCORE_PLUGIN_TCPSST_DIR}/TCPStreamListener.cpp
        ${LIBCORE_PLUGIN_TCPSST_DIR}/MultiplexedSocket.cpp
        ${LIBCORE_PLUGIN_TCPSST_DIR}/ASIOConnectAndHandshake.cpp
        ${LIBCORE_PLUGIN_TCPSST_DIR}/ASIOReadBuffer.cpp
        ${LIBCORE_PLUGIN_TCPSST_DIR}/ASIOSocketWrapper.cpp
        ${LIBCORE_PLUGIN_TCPSST_DIR}/ASIOStreamBuilder.cpp)


SET(LIBOH_PLUGIN_OGREGRAPHICS_DIR ${LIBOH_PLUGIN_DIR}/ogre)
SET(GFX ${LIBOH_PLUGIN_OGREGRAPHICS_DIR})
SET(INPUT ${GFX}/input)
SET(LIBOH_PLUGIN_OGREGRAPHICS_SOURCES
  ${SirikataProtocolDirectory}/Ogre_protobuf.cc
  ${SirikataOgreCoreZipCpp}
  ${GFX}/OgreSystem.cpp
  ${GFX}/OgreMeshRaytrace.cpp
  ${GFX}/CubeMap.cpp
  ${GFX}/OgreSystemMouseHandler.cpp
  ${GFX}/DragActions.cpp
  ${GFX}/InputBinding.cpp
  ${GFX}/InputBindingEvent.cpp
  ${GFX}/InputResponse.cpp
  ${INPUT}/SDLInputManager.cpp
  ${INPUT}/InputEventDescriptor.cpp
  ${INPUT}/InputEvents.cpp
  ${INPUT}/InputDevice.cpp
  ${INPUT}/SDLInputDevice.cpp
  ${GFX}/Entity.cpp
  ${GFX}/LightEntity.cpp
  ${GFX}/MeshEntity.cpp
  ${GFX}/CameraEntity.cpp
  ${GFX}/OgrePlugin.cpp
  ${GFX}/CameraPath.cpp
${GFX}/meruCompat/Event.cpp
${GFX}/meruCompat/EventSource.cpp
${GFX}/meruCompat/MeruDefs.cpp
${GFX}/meruCompat/SequentialWorkQueue.cpp
${GFX}/resourceManager/CDNArchive.cpp
${GFX}/resourceManager/CDNArchiveFactory.cpp
${GFX}/resourceManager/CDNArchivePlugin.cpp
${GFX}/resourceManager/GraphicsCamera.cpp
${GFX}/resourceManager/GraphicsLight.cpp
${GFX}/resourceManager/GraphicsObject.cpp
${GFX}/resourceManager/GraphicsRenderSettings.cpp
${GFX}/resourceManager/GraphicsResource.cpp
${GFX}/resourceManager/GraphicsResourceAsset.cpp
${GFX}/resourceManager/GraphicsResourceEntity.cpp
${GFX}/resourceManager/GraphicsResourceManager.cpp
${GFX}/resourceManager/GraphicsResourceMaterial.cpp
${GFX}/resourceManager/GraphicsResourceMesh.cpp
${GFX}/resourceManager/GraphicsResourceName.cpp
${GFX}/resourceManager/GraphicsResourceShader.cpp
${GFX}/resourceManager/GraphicsResourceSkeleton.cpp
${GFX}/resourceManager/GraphicsResourceTexture.cpp
${GFX}/resourceManager/GraphicsSystem.cpp
${GFX}/resourceManager/ManualMaterialLoader.cpp
${GFX}/resourceManager/ReplacingDataStream.cpp
${GFX}/resourceManager/ResourceDependencyTask.cpp
${GFX}/resourceManager/ResourceDownloadTask.cpp
${GFX}/resourceManager/ResourceLoadTask.cpp
${GFX}/resourceManager/ResourceLoadingQueue.cpp
${GFX}/resourceManager/ResourceManager.cpp
${GFX}/resourceManager/ResourceTransfer.cpp
${GFX}/resourceManager/ResourceUnloadTask.cpp
${GFX}/resourceManager/UploadTool.cpp
${GFX}/ViewportOverlay.cpp
${GFX}/WebView.cpp
${GFX}/WebViewManager.cpp
        )

#test source files
SET(CXXTESTSources
${LIBCORE_DIR}/test/AnyTest.hpp
${LIBCORE_DIR}/test/AtomicTest.hpp
#${LIBCORE_DIR}/test/CacheLayerTest.hpp
${LIBCORE_DIR}/test/DownloadTest.hpp
${LIBCORE_DIR}/test/EventTest.hpp
${LIBCORE_DIR}/test/ExtrapolationTest.hpp
${LIBCORE_DIR}/test/FactoryTest.hpp
${LIBCORE_DIR}/test/ListenerTest.hpp
${LIBCORE_DIR}/test/Matrix3Test.hpp
${LIBCORE_DIR}/test/MinitransactionHandlerTest.hpp
${LIBCORE_DIR}/test/NameLookupTest.hpp
${LIBCORE_DIR}/test/ObjectStorageTest.hpp
${LIBCORE_DIR}/test/OptionTest.hpp
#${LIBCORE_DIR}/test/ProxTest.hpp
${LIBCORE_DIR}/test/QuaternionTest.hpp
${LIBCORE_DIR}/test/ReadWriteHandlerTest.hpp
${LIBCORE_DIR}/test/RoutableMessageTest.hpp
${LIBCORE_DIR}/test/SQLiteMinitransactionTest.hpp
${LIBCORE_DIR}/test/SQLiteReadWriteTest.hpp
${LIBCORE_DIR}/test/SstTest.hpp
${LIBCORE_DIR}/test/SubscriptionTest.hpp
#${LIBCORE_DIR}/test/ThreadSafeQueueTest.hpp
${LIBCORE_DIR}/test/TR1Test.hpp
#${LIBCORE_DIR}/test/UploadTest.hpp
${LIBCORE_DIR}/test/Vector3Test.hpp
 )
#  ${LIBCORE_DIR}/test/ThreadSafeQueueTest.hpp
ADD_CXXTEST_CPP_TARGET(CXXTEST ${CXXTESTSources}
	LIBRARYDIR ${CXXTESTRoot}
	OUTPUTDIR ${CMAKE_CURRENT_BINARY_DIR})

SET(TEST_SOURCES ${CXXTEST_CPP_FILE}
                 ${SirikataProtocolDirectory}/Test_protobuf.cc 
                 ${LIBCORE_DIR}/test/SQLiteMinitransactionTest.cpp
                 ${LIBCORE_DIR}/test/ObjectStorageTest.cpp
                 ${LIBCORE_DIR}/test/MinitransactionHandlerTest.cpp
                 ${LIBCORE_DIR}/test/SQLiteReadWriteTest.cpp
                 ${LIBCORE_DIR}/test/ReadWriteHandlerTest.cpp

)


#linker flags
SET(CMAKE_DEBUG_POSTFIX "_d")

IF(WIN32)
  SET(SYSTEM_DL_LIBRARY "wsock32")
ELSE()
  SET(SYSTEM_DL_LIBRARY "dl")
ENDIF()

SET(SIRIKATA_CORE_LIBRARIES
    ${SYSTEM_DL_LIBRARY}
    
    ${CURL_LIBRARIES}
    ${Boost_LIBRARIES} )

IF(AWESOMIUM_FOUND)
  SET(AWESOMIUM_CFLAGS -DHAVE_AWESOMIUM)
ELSE()
  SET(AWESOMIUM_CFLAGS "")
  SET(AWESOMIUM_LIBRARIES "")
  SET(AWESOMIUM_LDFLAGS "")
  SET(AWESOMIUM_LIBRARY_DIRS "")
ENDIF()

SET(FINAL_LINK_DIRS ${ANTLR_LIBRARY_DIRS} ${Boost_LIBRARY_DIRS})
IF(OGRE_FOUND AND sdl_FOUND)
SET(FINAL_LINK_DIRS ${FINAL_LINK_DIRS} ${OGRE_LIBRARY_DIRS} ${sdl_LIBRARY_DIRS} ${AWESOMIUM_LIBRARY_DIRS})
ENDIF()
IF(MONO_FOUND)
SET(FINAL_LINK_DIRS ${FINAL_LINK_DIRS} ${MONO_LIBRARY_DIRS})
ENDIF()
IF(bullet_FOUND)
SET(FINAL_LINK_DIRS ${FINAL_LINK_DIRS} ${bullet_LIBRARY_DIRS})
ENDIF()
IF(SQLite3_FOUND)
SET(FINAL_LINK_DIRS ${FINAL_LINK_DIRS} ${SQLite3_LIBRARY_DIRS})
ENDIF()

LINK_DIRECTORIES(${FINAL_LINK_DIRS})

SET(SIRIKATA_CORE_LIB sirikata-core)
SET(SIRIKATA_SPACE_LIB sirikata-space)
SET(SIRIKATA_PROXIMITY_LIB sirikata-proximity)
SET(SIRIKATA_SUBSCRIPTION_LIB sirikata-subscription)
SET(SIRIKATA_OH_LIB sirikata-oh)
SET(SPACE_BINARY space)
SET(PROXIMITY_BINARY proximity)
SET(SUBSCRIPTION_BINARY subscription)
SET(CPPOH_BINARY cppoh)
SET(TEST_BINARY tests)


# FIXME we're doing static linking now and need this to get the export/import
# macros working properly.
# ADD_DEFINITIONS(-DSTATIC_LINKED)

#libraries
ADD_LIBRARY(${SIRIKATA_CORE_LIB} SHARED ${SIRIKATA_CORE_SOURCES})
SET_TARGET_PROPERTIES(${SIRIKATA_CORE_LIB} PROPERTIES COMPILE_DEFINITIONS SIRIKATA_BUILD)
TARGET_LINK_LIBRARIES(${SIRIKATA_CORE_LIB} ${SIRIKATA_CORE_LIBRARIES} ${PROTOCOLBUFFERS_LIBRARIES})
IF(sirikata_core_LDFLAGS OR sirikata_LDFLAGS)
  SET(sirikata_core_LDFLAGS ${sirikata_core_LDFLAGS} ${sirikata_LDFLAGS})
  SET_TARGET_PROPERTIES(${SIRIKATA_CORE_LIB} PROPERTIES LINK_FLAGS ${sirikata_core_LDFLAGS})
ENDIF()
#ADD_DEPENDENCIES(${SIRIKATA_CORE_LIB})

ADD_LIBRARY(${SIRIKATA_PROXIMITY_LIB} SHARED ${LIBPROXIMITY_SOURCES})
SET_TARGET_PROPERTIES(${SIRIKATA_PROXIMITY_LIB} PROPERTIES COMPILE_DEFINITIONS SIRIKATA_PROXIMITY_BUILD)
ADD_DEPENDENCIES(${SIRIKATA_PROXIMITY_LIB} ${SIRIKATA_CORE_LIB})
IF(sirikata_core_LDFLAGS)# use the core ldflags--these guys act similarly to core for their respective modules
  SET_TARGET_PROPERTIES(${SIRIKATA_PROXIMITY_LIB} PROPERTIES LINK_FLAGS ${sirikata_core_LDFLAGS})
ENDIF()
TARGET_LINK_LIBRARIES(${SIRIKATA_PROXIMITY_LIB} ${SIRIKATA_CORE_LIB} ${PROTOCOLBUFFERS_LIBRARIES})

ADD_LIBRARY(${SIRIKATA_SUBSCRIPTION_LIB} SHARED ${LIBSUBSCRIPTION_SOURCES})
SET_TARGET_PROPERTIES(${SIRIKATA_SUBSCRIPTION_LIB} PROPERTIES COMPILE_DEFINITIONS SIRIKATA_SUBSCRIPTION_BUILD)
ADD_DEPENDENCIES(${SIRIKATA_SUBSCRIPTION_LIB} ${SIRIKATA_CORE_LIB})
IF(sirikata_core_LDFLAGS)# use the core ldflags--these guys act similarly to core for their respective modules
  SET_TARGET_PROPERTIES(${SIRIKATA_SUBSCRIPTION_LIB} PROPERTIES LINK_FLAGS ${sirikata_core_LDFLAGS})
ENDIF()
TARGET_LINK_LIBRARIES(${SIRIKATA_SUBSCRIPTION_LIB} ${SIRIKATA_CORE_LIB} ${PROTOCOLBUFFERS_LIBRARIES})

ADD_LIBRARY(${SIRIKATA_SPACE_LIB} SHARED ${LIBSPACE_SOURCES})
SET_TARGET_PROPERTIES(${SIRIKATA_SPACE_LIB} PROPERTIES COMPILE_DEFINITIONS SIRIKATA_SPACE_BUILD)
ADD_DEPENDENCIES(${SIRIKATA_SPACE_LIB} ${SIRIKATA_CORE_LIB} ${SIRIKATA_PROXIMITY_LIB})
IF(sirikata_core_LDFLAGS)# use the core ldflags--these guys act similarly to core for their respective modules
  SET_TARGET_PROPERTIES(${SIRIKATA_SPACE_LIB} PROPERTIES LINK_FLAGS ${sirikata_core_LDFLAGS})
ENDIF()
TARGET_LINK_LIBRARIES(${SIRIKATA_SPACE_LIB} ${SIRIKATA_CORE_LIB} ${SIRIKATA_PROXIMITY_LIB} ${PROTOCOLBUFFERS_LIBRARIES})

ADD_LIBRARY(${SIRIKATA_OH_LIB} SHARED ${LIBOH_SOURCES})
SET_TARGET_PROPERTIES(${SIRIKATA_OH_LIB} PROPERTIES COMPILE_DEFINITIONS SIRIKATA_OH_BUILD)
ADD_DEPENDENCIES(${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB})
IF(sirikata_core_LDFLAGS)# use the core ldflags--these guys act similarly to core for their respective modules
  SET_TARGET_PROPERTIES(${SIRIKATA_OH_LIB} PROPERTIES LINK_FLAGS ${sirikata_core_LDFLAGS})
ENDIF()
TARGET_LINK_LIBRARIES(${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB} ${PROTOCOLBUFFERS_LIBRARIES})

#plugins
ADD_PLUGIN_TARGET(skeleton
                    SOURCES ${LIBCORE_PLUGIN_SKELETON_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB})

IF(SQLite3_FOUND)
ADD_PLUGIN_TARGET(sqlite
                    SOURCES ${LIBCORE_PLUGIN_SQLITE_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB}
                    LIBRARIES ${SQLite3_LIBRARIES} ${PROTOCOLBUFFERS_LIBRARIES})
ENDIF()

ADD_PLUGIN_TARGET(tcpsst
                    SOURCES ${LIBCORE_PLUGIN_TCPSST_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB})


IF(OGRE_FOUND AND sdl_FOUND)
ADD_PLUGIN_TARGET(ogregraphics
                    SOURCES ${LIBOH_PLUGIN_OGREGRAPHICS_SOURCES}
                    TARGET_CXXFLAGS ${OGRE_CFLAGS} ${sdl_CFLAGS} ${AWESOMIUM_CFLAGS}
                    TARGET_LDFLAGS ${OGRE_LDFLAGS} ${sdl_LDFLAGS} ${AWESOMIUM_LDFLAGS} ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB} 
                    LIBRARIES ${OGRE_LIBRARIES} ${sdl_LIBRARIES} ${AWESOMIUM_LIBRARIES}
                              ${Boost_REGEX_LIBRARY_${UPPERCASE_BUILD_TYPE}}
                               ${PROTOCOLBUFFERS_LIBRARIES})
ENDIF(OGRE_FOUND AND sdl_FOUND)


IF(PROX_FOUND)
ADD_PLUGIN_TARGET(prox
                    SOURCES ${LIBPROXIMITY_PLUGIN_PROX_SOURCES}
                    TARGET_CXXFLAGS ${PROX_CFLAGS}
                    TARGET_LDFLAGS ${PROX_LDFLAGS} ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_PROXIMITY_LIB} ${SIRIKATA_CORE_LIB}
                    LIBRARIES ${PROX_LIBRARIES} ${PROTOCOLBUFFERS_LIBRARIES})
ENDIF(PROX_FOUND)

SET(LIBOH_PLUGIN_BULLETPHYSICS_DIR ${LIBOH_PLUGIN_DIR}/bullet)
SET(LIBOH_PLUGIN_BULLETPHYSICS_SOURCES
 ${SirikataProtocolDirectory}/Bullet_protobuf.cc
 ${LIBOH_PLUGIN_BULLETPHYSICS_DIR}/BulletSystem.cpp
    )
IF(bullet_FOUND)
ADD_PLUGIN_TARGET(bulletphysics
                    SOURCES ${LIBOH_PLUGIN_BULLETPHYSICS_SOURCES}
                    TARGET_CXXFLAGS ${bullet_CFLAGS}
                    TARGET_LDFLAGS ${bullet_LDFLAGS} ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB}
                    LIBRARIES ${bullet_LIBRARIES})
ENDIF()



SET(LIBOH_PLUGIN_MONO_DIR ${LIBOH_PLUGIN_DIR}/monoscript)
SET(LIBOH_PLUGIN_MONO_SOURCES
 ${LIBOH_PLUGIN_MONO_DIR}/MonoArray.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoPlugin.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoSystem.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoClass.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoAssembly.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoContext.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoConvert.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoDelegate.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoDomain.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoException.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoHostedObjectExports.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoIDictionary.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoIList.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoMethodLookupCache.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoObject.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoVWObjectScriptManager.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoVWObjectScript.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoPropertyLookupCache.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoThread.cpp
 ${LIBOH_PLUGIN_MONO_DIR}/MonoUtil.cpp
    )
IF(MONO_FOUND)
ADD_PLUGIN_TARGET(monoscript
                    SOURCES ${LIBOH_PLUGIN_MONO_SOURCES}
                    TARGET_CXXFLAGS ${MONO_CFLAGS}
                    TARGET_LDFLAGS ${MONO_LDFLAGS} ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB}
                    LIBRARIES ${MONO_LIBRARIES})
ENDIF()


#binaries
ADD_EXECUTABLE(${TEST_BINARY} ${TEST_SOURCES})# EXCLUDE_FROM_ALL
ADD_EXECUTABLE(${SPACE_BINARY} ${SPACE_SOURCES})
ADD_EXECUTABLE(${PROXIMITY_BINARY} ${PROXIMITY_SOURCES})
ADD_EXECUTABLE(${SUBSCRIPTION_BINARY} ${SUBSCRIPTION_SOURCES})
ADD_EXECUTABLE(${CPPOH_BINARY} ${CPPOH_SOURCES})

ADD_DEPENDENCIES(${TEST_BINARY} ${SIRIKATA_CORE_LIB})
ADD_DEPENDENCIES(${SPACE_BINARY} ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB})
ADD_DEPENDENCIES(${PROXIMITY_BINARY} ${SIRIKATA_PROXIMITY_LIB} ${SIRIKATA_CORE_LIB})
ADD_DEPENDENCIES(${SUBSCRIPTION_BINARY} ${SIRIKATA_SUBSCRIPTION_LIB} ${SIRIKATA_CORE_LIB})
ADD_DEPENDENCIES(${CPPOH_BINARY} ${SIRIKATA_CORE_LIB} ${SIRIKATA_OH_LIB})

SET_TARGET_PROPERTIES(${SPACE_BINARY} ${PROXIMITY_BINARY} ${SUBSCRIPTION_BINARY} ${CPPOH_BINARY} ${TEST_BINARY}
                      PROPERTIES
                      DEBUG_POSTFIX "_d" )
TARGET_LINK_LIBRARIES(${TEST_BINARY} ${SIRIKATA_CORE_LIB}
                      ${TEST_LIBRARIES} ${PROTOCOLBUFFERS_LIBRARIES} ${SIRIKATA_PROXIMITY_LIB} ${SIRIKATA_SUBSCRIPTION_LIB})
TARGET_LINK_LIBRARIES(${SPACE_BINARY} ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB})
TARGET_LINK_LIBRARIES(${PROXIMITY_BINARY} ${SIRIKATA_CORE_LIB} ${SIRIKATA_PROXIMITY_LIB})
TARGET_LINK_LIBRARIES(${SUBSCRIPTION_BINARY} ${SUBSCRIPTION_CORE_LIB} ${SIRIKATA_SUBSCRIPTION_LIB})
SET(CPPOH_LINK_LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_OH_LIB})
IF(OGRE_FOUND AND sdl_FOUND)
  SET(CPPOH_LINK_LIBRARIES ${CPPOH_LINK_LIBRARIES} ogregraphics)
ENDIF(OGRE_FOUND AND sdl_FOUND)
IF(bullet_FOUND)
  SET(CPPOH_LINK_LIBRARIES ${CPPOH_LINK_LIBRARIES} bulletphysics)
ENDIF(bullet_FOUND)
TARGET_LINK_LIBRARIES(${CPPOH_BINARY} ${CPPOH_LINK_LIBRARIES})

IF(sirikata_LDFLAGS)
  SET_TARGET_PROPERTIES(${TEST_BINARY} PROPERTIES LINK_FLAGS ${sirikata_LDFLAGS})
  SET_TARGET_PROPERTIES(${SPACE_BINARY} PROPERTIES LINK_FLAGS ${sirikata_LDFLAGS})
  SET_TARGET_PROPERTIES(${PROXIMITY_BINARY} PROPERTIES LINK_FLAGS ${sirikata_LDFLAGS})
  SET_TARGET_PROPERTIES(${SUBSCRIPTION_BINARY} PROPERTIES LINK_FLAGS ${sirikata_LDFLAGS})
  SET_TARGET_PROPERTIES(${CPPOH_BINARY} PROPERTIES LINK_FLAGS ${sirikata_LDFLAGS})
  SET_TARGET_PROPERTIES(${BINARY_TO_CPP_BINARY} PROPERTIES LINK_FLAGS ${sirikata_LDFLAGS})
  SET_TARGET_PROPERTIES(${PBJ_BINARY} PROPERTIES LINK_FLAGS ${sirikata_LDFLAGS})
ENDIF()


#precompiled headers
IF(WIN32)
  SET_TARGET_PROPERTIES(${SIRIKATA_CORE_LIB} ${TEST_BINARY} PROPERTIES COMPILE_FLAGS "-Ycutil/Standard.hh")
ENDIF()


#installation information
INSTALL(TARGETS
          ${PBJ_BINARY}
          ${SIRIKATA_CORE_LIB}
          ${SIRIKATA_SPACE_LIB}
          ${SIRIKATA_PROXIMITY_LIB}
          ${SIRIKATA_SUBSCRIPTION_LIB}
          ${SIRIKATA_OH_LIB}
          ${SPACE_BINARY}
          ${PROXIMITY_BINARY}
          ${SUBSCRIPTION_BINARY}
          ${CPPOH_BINARY}
        RUNTIME
          DESTINATION bin
        LIBRARY
          DESTINATION lib
        ARCHIVE
          DESTINATION lib
)


INSTALL(DIRECTORY
          # note: the trailing slashes avoid prepending the directory name
          # itself to the installed files
          ${LIBCORE_SOURCE_DIR}/
          ${LIBSPACE_INCLUDE_DIR}/
          ${LIBPROXIMITY_INCLUDE_DIR}/
          ${LIBSUBSCRIPTION_INCLUDE_DIR}/
          ${LIBOH_INCLUDE_DIR}/
        DESTINATION include/sirikata
        PATTERN "*.cpp" EXCLUDE
        PATTERN "*.cc" EXCLUDE
        PATTERN "*.hh" EXCLUDE
)

# get the name of the binaries for running tests
IF(WIN32)
  GET_TARGET_PROPERTY(TEST_RUNABLE ${TEST_BINARY} LOCATION)
ELSE()
  IF(ISDEBUG)
#some CRAZY bug with cmake-2.4 does not bake the _d into LOCATION
    GET_TARGET_PROPERTY(TEST_RUNABLE ${TEST_BINARY} DEBUG_LOCATION)
  ELSE()
    GET_TARGET_PROPERTY(TEST_RUNABLE ${TEST_BINARY} LOCATION)
  ENDIF()
ENDIF()

ADD_CUSTOM_TARGET(test
  DEPENDS tests
  COMMAND ${TEST_RUNABLE})
