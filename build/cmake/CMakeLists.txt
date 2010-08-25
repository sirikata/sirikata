#Sirikata Build Script
#CMakeLists.txt
#
#Copyright (c) 2008, Patrick Reiter Horn
#Copyright (c) 2009, Ewen Cheslack-Postava
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

IF(APPLE) 
  SET(CMAKE_CXX_COMPILER g++-4.0)
  SET(CMAKE_C_COMPILER gcc-4.0)
ENDIF()


IF(APPLE)
  SET( DYLIB_EXT .dylib)
ELSE()
  IF(WIN32)
    SET( DYLIB_EXT .dll)
  ELSE()
    SET( DYLIB_EXT .so)
  ENDIF()
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
IF(APPLE)
SET(ADDED_DEFINITIONS ${ADDED_DEFINITIONS} -isysroot /Developer/SDKs/MacOSX10.5.sdk -arch i386 -mmacosx-version-min=10.5 )
  SET(sirikata_LDFLAGS "${sirikata_LDFLAGS} -Wl,-syslibroot -Wl,/Developer/SDKs/MacOSX10.5.sdk/")
ENDIF()
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

# Profile build target
SET(CMAKE_CXX_FLAGS_PROFILE "-g2 -O3 -pg -DNDEBUG" CACHE STRING
    "Built cxxflags with default settings."
    FORCE )
SET( CMAKE_C_FLAGS_PROFILE "-g2 -O3 -pg -DNDEBUG" CACHE STRING
    "Built cflags with default settings."
    FORCE )
SET( CMAKE_EXE_LINKER_FLAGS_PROFILE
    "-pg" CACHE STRING
    "Linking binaries with default settings."
    FORCE )
SET( CMAKE_SHARED_LINKER_FLAGS_PROFILE
    "-pg" CACHE STRING
    "Building shared libraries with default settings."
    FORCE )
MARK_AS_ADVANCED(
    CMAKE_CXX_FLAGS_PROFILE
    CMAKE_C_FLAGS_PROFILE
    CMAKE_EXE_LINKER_FLAGS_PROFILE
    CMAKE_SHARED_LINKER_FLAGS_PROFILE )

# Coverage build target
SET(CMAKE_CXX_FLAGS_COVERAGE "-g2 -fprofile-arcs -ftest-coverage" CACHE STRING
    "Built cxxflags with default settings."
    FORCE )
SET( CMAKE_C_FLAGS_COVERAGE "-g2 -fprofile-arcs -ftest-coverage" CACHE STRING
    "Built cflags with default settings."
    FORCE )
SET( CMAKE_EXE_LINKER_FLAGS_COVERAGE
    "-fprofile-arcs -ftest-coverage" CACHE STRING
    "Linking binaries with default settings."
    FORCE )
SET( CMAKE_SHARED_LINKER_FLAGS_COVERAGE
    "-fprofile-arcs -ftest-coverage" CACHE STRING
    "Building shared libraries with default settings."
    FORCE )
MARK_AS_ADVANCED(
    CMAKE_CXX_FLAGS_COVERAGE
    CMAKE_C_FLAGS_COVERAGE
    CMAKE_EXE_LINKER_FLAGS_COVERAGE
    CMAKE_SHARED_LINKER_FLAGS_COVERAGE )

#project global flags

PROJECT(Sirikata C CXX)
ENABLE_LANGUAGE(C)

IF(NOT TOP_LEVEL)
  SET(TOP_LEVEL ${CMAKE_CURRENT_SOURCE_DIR}/../..)
ENDIF()
GET_FILENAME_COMPONENT(TOP_LEVEL ${TOP_LEVEL} ABSOLUTE)

# a bunch of location definitions
IF(NOT DEPENDENCIES_ROOT)
  SET(DEPENDENCIES_ROOT ${TOP_LEVEL}/dependencies)
ENDIF()
#dependencies FIXME duplicated
IF(NOT PLATFORM_LIBS)
  SET(PLATFORM_LIBS ${TOP_LEVEL}/dependencies/)
ENDIF()

GET_FILENAME_COMPONENT(TOOLS_DIR ${TOP_LEVEL}/tools ABSOLUTE)
GET_FILENAME_COMPONENT(OH_TOOLS_DIR ${TOOLS_DIR}/oh ABSOLUTE)
GET_FILENAME_COMPONENT(CDN_TOOLS_DIR ${TOOLS_DIR}/cdn ABSOLUTE)

GET_FILENAME_COMPONENT(DEMO_DIR ${TOP_LEVEL}/demo ABSOLUTE)
GET_FILENAME_COMPONENT(DEMO_SCENES_DIR ${DEMO_DIR}/scenes ABSOLUTE)

SET(BUILD_DIR ${TOP_LEVEL}/build/cmake)
SET(GENERATED_FILES_DIR ${BUILD_DIR}/generated)
SET(ScriptsRoot ${TOP_LEVEL}/liboh/scripts)

SET(SirikataProtocolDirectory ${GENERATED_FILES_DIR})

SET(CSHARP_SCRIPTS_ROOT ${ScriptsRoot}/csharp)
SET(CSHARP_SCRIPTS_PROTOCOL ${ScriptsRoot}/csharp/protocol)
SET(IRONPYTHON_SCRIPTS_ROOT ${ScriptsRoot}/ironpython)
SET(IRONPYTHON_SCRIPTS_PROTOCOL ${ScriptsRoot}/ironpython/protocol)

#add-ins

IF(NOT CMAKE_MODULE_PATH)
  SET(CMAKE_MODULE_PATH ${TOP_LEVEL}/build/modules)
ENDIF()
GET_FILENAME_COMPONENT(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ABSOLUTE)

IF(NOT BUILD_DIR)
  SET(BUILD_DIR ${TOP_LEVEL}/build/cmake)
ENDIF()
GET_FILENAME_COMPONENT(BUILD_DIR ${BUILD_DIR} ABSOLUTE)

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
INCLUDE(AddPluginTarget)
FIND_PACKAGE(PythonInterp)

MACRO(ADD_CXXTEST_CPP_TARGET)
  PARSE_ARGUMENTS(CXXTEST "DEPENDS;OUTPUTDIR;LIBRARYDIR" "" ${ARGN})
  CAR(CXXTEST_NAME ${CXXTEST_DEFAULT_ARGS})
  CDR(CXXTEST_FILES ${CXXTEST_DEFAULT_ARGS})

  SET(CXXTEST_EXEC_TARGET ${CXXTEST_NAME})
  SET(CXXTEST_TEST_FILES)
  SET(CXXTEST_H_FILES)
  SET(CXXTEST_OPTIONS --include=sirikata/core/util/Standard.hh)
  #IF(CXXTEST_OUTPUTDIR)
  #  SET(CXXTEST_OPTIONS ${CXXTEST_OPTIONS} -o ${CXXTEST_OUTPUTDIR})
  #ENDIF(CXXTEST_OUTPUTDIR)
  SET(CXXTEST_CPP_FILE ${CMAKE_CURRENT_BINARY_DIR}/test.cc)

  IF(PYTHON_EXECUTABLE)
    SET(CXXTEST_COMPILER ${PYTHON_EXECUTABLE})
    SET(CXXTEST_GEN ${CXXTEST_LIBRARYDIR}/cxxtestgen.py)
  ELSE()
    FIND_PACKAGE(Perl)
    IF(PERL_EXECUTABLE)
      SET(CXXTEST_CPP_FILE test.cc)      #perl cannot output to a full path.
      SET(CXXTEST_COMPILER ${PERL_EXECUTABLE})
      SET(CXXTEST_GEN ${CXXTEST_LIBRARYDIR}/cxxtestgen.pl)
    ELSE()
      MESSAGE(STATUS "!!! Cannot locate python or perl -- tests will not be compiled.")
    ENDIF()
  ENDIF()
  FOREACH(FILE ${CXXTEST_FILES})
    SET(CXXTEST_H_FILE ${TOP_LEVEL}/${FILE})
    SET(CXXTEST_TEST_FILES ${CXXTEST_TEST_FILES} ${CXXTEST_H_FILE})
  ENDFOREACH()
  IF (CXXTEST_COMPILER)
    SET(FINAL_CXXTEST_COMMAND ${CXXTEST_COMPILER} ${CXXTEST_GEN} ${CXXTEST_OPTIONS} -o ${CXXTEST_CPP_FILE} ${CXXTEST_TEST_FILES})
    ADD_CUSTOM_COMMAND(OUTPUT ${CXXTEST_CPP_FILE}
                       COMMAND ${CXXTEST_COMPILER} ${CXXTEST_GEN} ${CXXTEST_OPTIONS} -o ${CXXTEST_CPP_FILE} ${CXXTEST_TEST_FILES}
                       DEPENDS ${CXXTEST_TEST_FILES} ${CXXTEST_DEPENDS}
                       COMMENT "Building ${CXXTEST_TEST_FILES} -> ${CXXTEST_CPP_FILE}")
  ELSE()
    ADD_CUSTOM_COMMAND(OUTPUT ${CXXTEST_CPP_FILE}
                       COMMAND exit 1
                       COMMENT "Unable to build ${CXXTEST_CPP_FILE} because python and perl were not found.")
  ENDIF()

  SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CXXTEST_CPP_FILE})

ENDMACRO(ADD_CXXTEST_CPP_TARGET)

GET_FILENAME_COMPONENT(PLATFORM_LIBS ${PLATFORM_LIBS} ABSOLUTE)
IF(NOT PLATFORM_LIBS)
  MESSAGE(STATUS "Could not find dependencies directory ${TOP_LEVEL}/dependencies/")
ENDIF()
MESSAGE(STATUS "Using PLATFORM_LIBS=${PLATFORM_LIBS}")
SET(DISABLE_M32 FALSE)
SET(FORCE_32BIT FALSE)
SET(DISABLE_BERKELIUM FALSE)
IF(WIN32)
  IF(sirikata_LDFLAGS)
    SET(sirikata_LDFLAGS ${sirikata_LDFLAGS} /ignore:4099)
  ELSE()
    SET(sirikata_LDFLAGS /ignore:4099)
  ENDIF()
ENDIF()
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
SET(IRON_PYTHON_FOUND FALSE)
IF(MONO_FOUND)
  ADD_DEFINITIONS(-DHAVE_MONO)
  ADD_DEFINITIONS(-DMONO_VERSION_MAJOR=${MONO_MAJOR_VERSION})
  ADD_DEFINITIONS(-DMONO_VERSION_MINOR=${MONO_MINOR_VERSION})
  ADD_DEFINITIONS(-DMONO_VERSION_MICRO=${MONO_SUBMINOR_VERSION})
  IF(EXISTS ${PLATFORM_LIBS}/IronPython)
    SET(IRON_PYTHON_FOUND TRUE)
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
  FIND_PACKAGE(Boost COMPONENTS system program_options thread filesystem date_time regex iostreams)
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

  STRING(REPLACE "boost_system" "boost_date_time" Boost_DATE_TIME_LIBRARY ${Boost_SYSTEM_LIBRARY})
  STRING(REPLACE "boost_system" "boost_date_time" Boost_DATE_TIME_LIBRARY_DEBUG ${Boost_SYSTEM_LIBRARY_DEBUG})
  STRING(REPLACE "boost_system" "boost_date_time" Boost_DATE_TIME_LIBRARY_RELEASE ${Boost_SYSTEM_LIBRARY_RELEASE})
  
  STRING(REPLACE "boost_system" "boost_iostreams" Boost_IOSTREAMS_LIBRARY ${Boost_SYSTEM_LIBRARY})
  STRING(REPLACE "boost_system" "boost_iostreams" Boost_IOSTREAMS_LIBRARY_DEBUG ${Boost_SYSTEM_LIBRARY_DEBUG})
  STRING(REPLACE "boost_system" "boost_iostreams" Boost_IOSTREAMS_LIBRARY_RELEASE ${Boost_SYSTEM_LIBRARY_RELEASE})
ENDIF(WIN32)
VERIFY_VERSION(Boost 1 35 0)
SET(Boost_LIBRARIES
  debug ${Boost_SYSTEM_LIBRARY_DEBUG} optimized ${Boost_SYSTEM_LIBRARY_RELEASE}
  debug ${Boost_PROGRAM_OPTIONS_LIBRARY_DEBUG} optimized ${Boost_PROGRAM_OPTIONS_LIBRARY_RELEASE}
  debug ${Boost_THREAD_LIBRARY_DEBUG} optimized ${Boost_THREAD_LIBRARY_RELEASE}
  debug ${Boost_FILESYSTEM_LIBRARY_DEBUG} optimized ${Boost_FILESYSTEM_LIBRARY_RELEASE}
  debug ${Boost_DATE_TIME_LIBRARY_DEBUG} optimized ${Boost_DATE_TIME_LIBRARY_RELEASE}
  debug ${Boost_REGEX_LIBRARY_DEBUG} optimized ${Boost_REGEX_LIBRARY_RELEASE}
  debug ${Boost_IOSTREAMS_LIBRARY_DEBUG} optimized ${Boost_IOSTREAMS_LIBRARY_RELEASE}
  )

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

#dependency: Bullet
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
IF(NOT SQLite3_FOUND)
  FIND_PACKAGE(SQLite3)
ENDIF()

#dependency: OpenCOLLADA
IF(WIN32)
  SET(OpenCOLLADA_ROOT ${PLATFORM_LIBS}/installed-opencollada)
ELSE()
  SET(OpenCOLLADA_ROOT ${PLATFORM_LIBS}/installed-opencollada)
ENDIF()

SET(OpenCOLLADA_MINIMUM_VERSION 1.1.0)
FIND_PACKAGE(OpenCOLLADA)

# optional dependency: sdl
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
VERIFY_VERSION(Boost 1 35 0)
FIND_PACKAGE(SDL)

#optional dependency: prox
FIND_PACKAGE(PROX)
IF(NOT PROX_INCLUDE_DIRS)
  MESSAGE(FATAL_ERROR "PROX submodule not found, go to top level and run:
git submodule init
git submodule update
")
ENDIF()

#dependency: glut
#FIND_PACKAGE(GLUT)
#IF(NOT GLUT_FOUND)
#  MESSAGE(SEND_ERROR "Couldn't find GLUT.")
#ENDIF(NOT GLUT_FOUND)
SET(GSL_ROOT ${PLATFORM_LIBS}/installed-gsl)
FIND_PACKAGE(GSL)

IF(WIN32)
#dependency: berkelium
  IF(NOT BERKELIUM_ROOT)
    SET(BERKELIUM_ROOT ${PLATFORM_LIBS}/berkelium)
  ENDIF()
  GET_FILENAME_COMPONENT(BERKELIUM_ROOT ${BERKELIUM_ROOT} ABSOLUTE)
  IF(NOT DISABLE_BERKELIUM)
    FIND_PACKAGE(Berkelium)
    IF(NOT BERKELIUM_FOUND)
      SET(BERKELIUM_ROOT ${TOP_LEVEL}/externals/berkelium)
      FIND_PACKAGE(Berkelium)
    ENDIF()
  ENDIF()
  SET(BERKELIUM_TARGET_LIBRARIES)
ELSE()
  # FIXME real check for berkelium + chrome
  IF(EXISTS ${TOP_LEVEL}/dependencies/chromium AND EXISTS ${TOP_LEVEL}/dependencies/chromium/chromium)
    #Prevent submodule from making a CHROME_SYMLINK_TARGET
    SET(CHROME_SYMLINKS_TARGET cppoh)
    SET(CHROME_SYMLINKS_BINARIES ${CHROME_SYMLINKS_BINARIES}
                   ${CMAKE_CURRENT_BINARY_DIR}/cppoh
                   ${CMAKE_CURRENT_BINARY_DIR}/chrome/berkelium
                   ${CMAKE_CURRENT_BINARY_DIR}/chrome/plugin_carbon_interpose.dylib
                   ${CMAKE_CURRENT_BINARY_DIR}/cppoh_d
                   ${TOP_LEVEL}/cdn.txt ${CMAKE_CURRENT_BINARY_DIR}/scene.db)




    SET(BERKELIUM_CHROME_ROOT_SEARCH_PATHS ${TOP_LEVEL}/dependencies/chromium/chromium)
    ADD_SUBDIRECTORY(${TOP_LEVEL}/externals/berkelium ${CMAKE_CURRENT_BINARY_DIR}/chrome)

    SET(CMAKE_MODULE_PATH ${TOP_LEVEL}/externals/berkelium ${CMAKE_MODULE_PATH})
    SET(CHROME_ROOT ${BERKELIUM_CHROME_ROOT_SEARCH_PATHS})
    SET(CHROME_FOUND_QUIETLY TRUE)
    FIND_PACKAGE(Chrome)   
 
    IF(CHROME_FOUND)

      SET(BERKELIUM_FOUND TRUE)
      SET(BERKELIUM_INCLUDE_DIRS ${TOP_LEVEL}/externals/berkelium/include)
      SET(BERKELIUM_TARGET_LIBRARIES libberkelium)
      SET(BERKELIUM_LIBRARIES)
    ELSE()
      MESSAGE(STATUS "Chrome wasn't found in dependencies/chromium")
      MESSAGE(STATUS "Make sure you ran make install and built chromium.")
    ENDIF()
  ELSE()
    MESSAGE(STATUS "dependencies/chromium not found!")
    MESSAGE(STATUS "Make sure you ran make install and built chromium.")
    SET(BERKELIUM_FOUND FALSE)
  ENDIF()
ENDIF()

#dependency: cxxtest
SET(CXXTESTRoot ${TOP_LEVEL}/externals/cxxtest)

# -- User defined variables that control behavior at compile time
SET(CBR_TIMESTAMP_PACKETS TRUE
  CACHE BOOL "If enabled, detailed timestamps are recorded for each packet, enabling high granularity latency breakdowns."
)
IF(CBR_TIMESTAMP_PACKETS)
  ADD_DEFINITIONS(-DCBR_TIMESTAMP_PACKETS)
ENDIF()

#variable which contains list of plugin targets we want to install
SET(PLUGIN_INSTALL_LIST)

#project directory locations
SET(LIBCORE_DIR ${TOP_LEVEL}/libcore)
SET(LIBSPACE_DIR ${TOP_LEVEL}/libspace)
SET(LIBOH_DIR ${TOP_LEVEL}/liboh)
SET(LIBPROXYOBJECT_DIR ${TOP_LEVEL}/libproxyobject)
SET(SPACE_DIR ${TOP_LEVEL}/space)
SET(CPPOH_DIR ${TOP_LEVEL}/cppoh)

#include locations
SET(LIBCORE_INCLUDE_DIR ${LIBCORE_DIR}/include)
SET(LIBSPACE_INCLUDE_DIR ${LIBSPACE_DIR}/include)
SET(LIBOH_INCLUDE_DIR ${LIBOH_DIR}/include)
SET(LIBPROXYOBJECT_INCLUDE_DIR ${LIBPROXYOBJECT_DIR}/include)
SET(HTTP_PARSER_INCLUDE_DIR ${TOP_LEVEL}/externals/http-parser)

#source file locations
SET(LIBCORE_SOURCE_DIR ${LIBCORE_DIR}/src)
SET(LIBSPACE_SOURCE_DIR ${LIBSPACE_DIR}/src)
SET(LIBOH_SOURCE_DIR ${LIBOH_DIR}/src)
SET(LIBPROXYOBJECT_SOURCE_DIR ${LIBPROXYOBJECT_DIR}/src)
SET(SPACE_SOURCE_DIR ${SPACE_DIR}/src)
SET(CPPOH_SOURCE_DIR ${CPPOH_DIR}/src)
#source file location (cbr)
SET(SIMOH_SOURCE_DIR ${TOP_LEVEL}/simoh/src)
SET(CSEG_SOURCE_DIR ${TOP_LEVEL}/cseg/src)
SET(PINTO_SOURCE_DIR ${TOP_LEVEL}/pinto/src)
SET(ANALYSIS_SOURCE_DIR ${TOP_LEVEL}/analysis/src)
SET(BENCH_SOURCE_DIR ${TOP_LEVEL}/bench/src)
SET(TEST_SOURCE_DIR ${TOP_LEVEL}/test/src)

#plugins locations
SET(LIBCORE_PLUGIN_DIR ${LIBCORE_DIR}/plugins)
SET(LIBPROXYOBJECT_PLUGIN_DIR ${LIBPROXYOBJECT_DIR}/plugins)
SET(LIBOH_PLUGIN_DIR ${LIBOH_DIR}/plugins)
SET(LIBSPACE_PLUGIN_DIR ${LIBSPACE_DIR}/plugins)

#generated source files
SET(CBR_GENERATED_DIR ${BUILD_DIR}/generated)

#cxx flags

# FIXME these include directories should be project specific
SET(INCLUDE_DIRECTORIES
  ${LIBCORE_INCLUDE_DIR}
  ${LIBCORE_SOURCE_DIR} #FIXME
  ${LIBSPACE_INCLUDE_DIR}
  ${LIBOH_INCLUDE_DIR}
  ${LIBPROXYOBJECT_INCLUDE_DIR}/models
  ${LIBPROXYOBJECT_INCLUDE_DIR}
  ${CPPOH_SOURCE_DIR}

  ${SirikataProtocolDirectory}

  ${Boost_INCLUDE_DIRS}
  ${CURL_INCLUDE_DIRS}
  ${PROTOCOLBUFFERS_INCLUDE_DIRS}
  ${ANTLR_INCLUDE_DIRS}
  ${PROX_INCLUDE_DIRS}
  ${CXXTESTRoot}
  ${CMAKE_CURRENT_SOURCE_DIR} #generated files
  ${CBR_GENERATED_DIR} # For generated protocol code
  ${SQLite3_INCLUDE_DIRS}
  #${GLUT_INCLUDE_DIR}
  ${GSL_INCLUDE_DIR}
  ${HTTP_PARSER_INCLUDE_DIR}
  )

IF(OGRE_FOUND AND sdl_FOUND)
SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${OGRE_INCLUDE_DIRS} ${sdl_INCLUDE_DIRS})
ENDIF()
IF(MONO_FOUND)
SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${MONO_INCLUDE_DIRS})
ENDIF()
IF(BERKELIUM_FOUND)
SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${BERKELIUM_INCLUDE_DIRS})
ENDIF()
IF(OpenCOLLADA_FOUND)
  SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${OpenCOLLADA_INCLUDE_DIRS})
ENDIF()
IF(bullet_FOUND)
  SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${bullet_INCLUDE_DIRS})
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
# protocol files
# FIXME we should jsut change ADD_PBJ_TARGET to take full paths
SET(ProtocolBuffersSourcesCore
  TimedMotionVector
  TimedMotionQuaternion
)
SET(ProtocolBuffersSources
  Persistence
  Sirikata
  Time
  Subscription
  Test
  ObjectMessage
  SSTHeader

  Loc
  Prox
  MasterPinto
  Session
  CSeg
  ServerMessage
  Migration
  OSeg
  Forwarder
  )

# Based on dependencies, generate arguments for protocol buffers generation
SET(PROTOCOLBUFFERS_GENS GENERATE_CPP)
SET(SCRIPTING_PROTOCOLBUFFERS_GENS ${PROTOCOLBUFFERS_GENS})
IF(MONO_FOUND AND PROTOCOLBUFFERS_SUPPORTS_CSHARP)
  SET(SCRIPTING_PROTOCOLBUFFERS_GENS ${SCRIPTING_PROTOCOLBUFFERS_GENS} GENERATE_CSHARP CSHARP_OUTPUTDIR ${CSHARP_SCRIPTS_PROTOCOL})
ENDIF()

#needed for scene converter irrespective of IronPython
SET(SCRIPTING_PROTOCOLBUFFERS_GENS ${SCRIPTING_PROTOCOLBUFFERS_GENS} GENERATE_PYTHON PYTHON_OUTPUTDIR ${IRONPYTHON_SCRIPTS_PROTOCOL})

SET(PROTOCOLBUFFERS_CPP_HEADER "#include \"sirikata/core/util/Standard.hh\"\n")


ADD_PBJ_TARGET(
  ${ProtocolBuffersSourcesCore}
  INPUTDIR ${ProtocolBuffersRoot}
  PLUGINNAME "Protocol"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  GENERATED_CPP_FILES ProtocolCore_PBJ_CPP_FILES
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  EXPORTMACRO SIRIKATA_EXPORT
  GENERATE_CPP
)
ADD_PBJ_TARGET(
  MessageHeader ${ProtocolBuffersSources}
  DEPENDS ${ProtocolCore_PBJ_CPP_FILES}
  INPUTDIR ${ProtocolBuffersRoot}
  PLUGINNAME "Protocol"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  GENERATED_CPP_FILES Protocol_PBJ_CPP_FILES
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  EXPORTMACRO SIRIKATA_EXPORT
  ${PROTOCOLBUFFERS_GENS}
)
#Aggregate core and normal
SET(Protocol_PBJ_CPP_FILES ${ProtocolCore_PBJ_CPP_FILES} ${Protocol_PBJ_CPP_FILES})

# For Traces
# Trace protocol buffers are handled separately to handle dependencies and their slightly different location
SET(TRACE_PROTOCOLS_CORE
  Geometry.pbj
)
SET(TRACE_OBJECT_PROTOCOLS
  ObjectTrace.pbj
  OSegTrace.pbj
  MigrationTrace.pbj
  PingTrace.pbj
  DatagramTrace.pbj
  LocProxTrace.pbj
  CSegTrace.pbj
)
ADD_PBJ_TARGET(
  ${TRACE_PROTOCOLS_CORE}
  PLUGINNAME "Protocol"
  INPUTDIR ${ProtocolBuffersRoot}/trace
  IMPORTS ${ProtocolBuffersRoot}/trace
  OUTPUTDIR ${SirikataProtocolDirectory}
  GENERATED_CPP_FILES TRACE_CORE_PBJ_CPP_FILES
  CPP_HEADER ${PBJ_CPP_HEADER}
  EXPORTMACRO SIRIKATA_EXPORT
  GENERATE_CPP
)
ADD_PBJ_TARGET(
  ${TRACE_OBJECT_PROTOCOLS}
  DEPENDS ${TRACE_CORE_PBJ_CPP_FILES}
  PLUGINNAME "Protocol"
  INPUTDIR ${ProtocolBuffersRoot}/trace
  IMPORTS ${ProtocolBuffersRoot}/trace
  OUTPUTDIR ${SirikataProtocolDirectory}
  GENERATED_CPP_FILES TRACE_PBJ_CPP_FILES
  CPP_HEADER ${PBJ_CPP_HEADER}
  EXPORTMACRO SIRIKATA_EXPORT
  GENERATE_CPP
)
# Aggregate core and full pbj files
SET(TRACE_PBJ_CPP_FILES ${TRACE_PBJ_CPP_FILES} ${TRACE_CORE_PBJ_CPP_FILES})


# For scripting
SET(ScriptingProtocolBuffersSources
  Persistence
  Sirikata
  Time
  Subscription
  Test
  MessageHeader
  Physics
  )

ADD_PBJ_TARGET(${ScriptingProtocolBuffersSources}
  INPUTDIR ${ProtocolBuffersRoot}
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  ${SCRIPTING_PROTOCOLBUFFERS_GENS}
)
SET(PROTOCOLBUFFERS_SCRIPTING_OUTPUTS ${PROTOCOLBUFFERS_ALL_OUTPUTS})


ADD_PBJ_TARGET(Physics
  INPUTDIR ${ProtocolBuffersRoot}
  PLUGINNAME "Protocol"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  GENERATED_CPP_FILES Bullet_PBJ_CPP_FILES
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  EXPORTMACRO SIRIKATA_PROXYOBJECT_EXPORT
  ${PROTOCOLBUFFERS_GENS}
  )


#binary files to be linked into sirikata
SET(BINARY_TO_CPP_SOURCE_FILE ${TOP_LEVEL}/externals/binarytocpp/binarytocpp.cpp)
SET(BINARY_TO_CPP_BINARY binarytocpp)
ADD_EXECUTABLE(${BINARY_TO_CPP_BINARY} ${BINARY_TO_CPP_SOURCE_FILE} )
GET_TARGET_PROPERTY(BINARY_TO_CPP_RUNABLE ${BINARY_TO_CPP_BINARY} LOCATION)

SET(HTTP_PARSER_SOURCE_FILE ${TOP_LEVEL}/externals/http-parser/http_parser.c)
IF(WIN32)
  SET_SOURCE_FILES_PROPERTIES(${HTTP_PARSER_SOURCE_FILE} PROPERTIES LANGUAGE CXX)
ENDIF()
ADD_LIBRARY(http-parser ${HTTP_PARSER_SOURCE_FILE} )

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
   SET(AdditionalProtoDepends ${AdditionalProtoDepends} ${IRONPYTHON_SCRIPTS_PROTOCOL}/${FILE}_pb2.py)
ENDFOREACH()
ADD_CUSTOM_TARGET(SirikataPythonProtocol ALL
           DEPENDS ${AdditionalProtoDepends}
           COMMENT "Checking ${AdditionalProtoDepends}")

IF(MONO_FOUND)
  SET(SirikataProtoScriptSources ${SirikataProtoScriptSources})
  FOREACH(FILE ${ScriptingProtocolBuffersSources})
     SET(SirikataProtoScriptSources ${SirikataProtoScriptSources}
  ${CSHARP_SCRIPTS_PROTOCOL}/${FILE}.pbj.cs ${CSHARP_SCRIPTS_PROTOCOL}/${FILE}.cs )
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
                                     -warn:2 -nowarn:3021
                                     -r:${PROTOCOLBUFFERS_BIN_DIRS}/Google.ProtocolBuffers.dll
                                     ${SirikataProtoScriptSources}
                   DEPENDS ${SirikataProtoScriptSources}
                   COMMENT "Building Sirikata.Protocol.dll")
  ENDIF()
  SET(SirikataProtoScriptRuntimeBuildOutputs ${SirikataProtoScriptRuntime_LIBRARY} ${SirikataProtoScriptRuntime_LIBRARY}.mdb)
  SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${SirikataProtoScriptRuntimeBuildOutputs}")


  # Note: Can't use GLOB here because of Python objects
  SET(SirikataScriptSources
       ${CSHARP_SCRIPTS_ROOT}/HostedObject.cs
       ${CSHARP_SCRIPTS_ROOT}/Time.cs
     )
  IF(IRON_PYTHON_FOUND)
    SET(SirikataScriptSources ${SirikataScriptSources} ${CSHARP_SCRIPTS_ROOT}/PythonObject.cs)
  ENDIF()

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

#IF(WIN32)
#ADD_DEFINITIONS(/FI\"${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Platform.hpp\")
#ELSE()
#ADD_DEFINITIONS(-include \"${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Platform.hpp\")
#ENDIF()


SET(SIRIKATA_CORE_SOURCES
	${PROTOCOLBUFFERS_SCRIPTING_OUTPUTS}
        ${Protocol_PBJ_CPP_FILES}
        ${TRACE_PBJ_CPP_FILES}
	${LIBCORE_SOURCE_DIR}/transfer/HTTPRequest.cpp
	${LIBCORE_SOURCE_DIR}/transfer/TransferMediator.cpp
	${LIBCORE_SOURCE_DIR}/transfer/FileProtocolHandler.cpp
	${LIBCORE_SOURCE_DIR}/transfer/DiskCacheLayer.cpp
	${LIBCORE_SOURCE_DIR}/transfer/TransferHandlers.cpp
	${LIBCORE_SOURCE_DIR}/transfer/HttpManager.cpp
	${LIBCORE_SOURCE_DIR}/persistence/ObjectStorage.cpp
	${LIBCORE_SOURCE_DIR}/persistence/ReadWriteHandlerFactory.cpp
	${LIBCORE_SOURCE_DIR}/persistence/MinitransactionHandlerFactory.cpp
	${LIBCORE_SOURCE_DIR}/task/DependencyTask.cpp
	${LIBCORE_SOURCE_DIR}/task/EventManager.cpp
	${LIBCORE_SOURCE_DIR}/task/WorkQueue.cpp
	${LIBCORE_SOURCE_DIR}/task/Event.cpp
	${LIBCORE_SOURCE_DIR}/task/UniqueId.cpp
	${LIBCORE_SOURCE_DIR}/task/Time.cpp
	${LIBCORE_SOURCE_DIR}/network/Asio.cpp
        ${LIBCORE_SOURCE_DIR}/odp/Defs.cpp
        ${LIBCORE_SOURCE_DIR}/odp/DelegateService.cpp
        ${LIBCORE_SOURCE_DIR}/odp/DelegatePort.cpp
   	${LIBCORE_SOURCE_DIR}/options/Options.cpp
   	${LIBCORE_SOURCE_DIR}/options/Config.cpp
   	${LIBCORE_SOURCE_DIR}/options/CDNConfig.cpp
   	${LIBCORE_SOURCE_DIR}/options/CommonOptions.cpp
        ${LIBCORE_SOURCE_DIR}/network/Address4.cpp
	${LIBCORE_SOURCE_DIR}/network/IOService.cpp
	${LIBCORE_SOURCE_DIR}/network/IOServiceFactory.cpp
	${LIBCORE_SOURCE_DIR}/network/IOServicePool.cpp
	${LIBCORE_SOURCE_DIR}/network/IOWork.cpp
	${LIBCORE_SOURCE_DIR}/network/IOStrand.cpp
	${LIBCORE_SOURCE_DIR}/network/IOTimer.cpp
	${LIBCORE_SOURCE_DIR}/network/Stream.cpp
	${LIBCORE_SOURCE_DIR}/network/StreamListener.cpp
	${LIBCORE_SOURCE_DIR}/network/StreamFactory.cpp
	${LIBCORE_SOURCE_DIR}/network/StreamListenerFactory.cpp
        ${LIBCORE_SOURCE_DIR}/network/NTPTimeSync.cpp
        ${LIBCORE_SOURCE_DIR}/network/ServerIDMap.cpp
        ${LIBCORE_SOURCE_DIR}/network/ObjectMessage.cpp
        ${LIBCORE_SOURCE_DIR}/network/SSTImpl.cpp
        ${LIBCORE_SOURCE_DIR}/service/Context.cpp
        ${LIBCORE_SOURCE_DIR}/service/Poller.cpp
        ${LIBCORE_SOURCE_DIR}/service/PollingService.cpp
        ${LIBCORE_SOURCE_DIR}/service/TimeProfiler.cpp
	${LIBCORE_SOURCE_DIR}/util/DynamicLibrary.cpp
	${LIBCORE_SOURCE_DIR}/util/internal_sha2.cpp
	${LIBCORE_SOURCE_DIR}/util/Logging.cpp
	${LIBCORE_SOURCE_DIR}/util/Plugin.cpp
	${LIBCORE_SOURCE_DIR}/util/PluginManager.cpp
	${LIBCORE_SOURCE_DIR}/util/Sha256.cpp
	${LIBCORE_SOURCE_DIR}/util/SolidAngle.cpp
	${LIBCORE_SOURCE_DIR}/queue/ThreadSafeQueue.cpp
        ${LIBCORE_SOURCE_DIR}/util/Platform.cpp
	${LIBCORE_SOURCE_DIR}/util/UUID.cpp
        ${LIBCORE_SOURCE_DIR}/util/ThreadId.cpp
	${LIBCORE_SOURCE_DIR}/util/BoundingInfo.cpp
        ${LIBCORE_SOURCE_DIR}/util/SentMessage.cpp
        ${LIBCORE_SOURCE_DIR}/util/QueryTracker.cpp
        ${LIBCORE_SOURCE_DIR}/util/Hash.cpp
        ${LIBCORE_SOURCE_DIR}/util/Timer.cpp
        ${LIBCORE_SOURCE_DIR}/util/RegionWeightCalculator.cpp
        ${LIBCORE_SOURCE_DIR}/trace/BatchedBuffer.cpp
        ${LIBCORE_SOURCE_DIR}/trace/Trace.cpp
)

#precompiled header
SET(LIBCORE_STANDARD_HH ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Standard.hh)
IF(WIN32)
 SET(LIBCORE_STANDARD_CPP ${LIBCORE_SOURCE_DIR}/util/Standard.cpp)
 # Add Standard.cpp, which generates the PCH. Other settings are created farther down when the necessary targets have been created
 SET(SIRIKATA_CORE_SOURCES
     ${SIRIKATA_CORE_SOURCES}
     ${LIBCORE_STANDARD_CPP}
 )
ELSE()
 IF(${CMAKE_COMPILER_IS_GNUCXX})
  SET(LIBCORE_STANDARD_HH_GCH ${LIBCORE_STANDARD_HH}.gch)
  SET(SIRIKATA_CORE_SOURCES
     ${LIBCORE_STANDARD_HH_GCH} ${SIRIKATA_CORE_SOURCES})
  SET_DIRECTORY_PROPERTIES(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
    ${LIBCORE_STANDARD_HH_GCH})

  IF(ISDEBUG)
    SET(PCH_CXX_FLAGS  ${CMAKE_CXX_FLAGS_DEBUG} )
  ELSE()
    SET(PCH_CXX_FLAGS  ${CMAKE_CXX_FLAGS_RELEASE} )
  ENDIF()
  SEPARATE_ARGUMENTS(PCH_CXX_FLAGS)
  # FINAL_COMMAND doesn't like it if a 2 part compiler command has spaces on the second one.
  STRING(REPLACE " " "" CXX_COMPILER_ARG1_WITHOUT_SPACES "${CMAKE_CXX_COMPILER_ARG1}")
  SET(FINAL_COMMAND ${CMAKE_CXX_COMPILER} ${CXX_COMPILER_ARG1_WITHOUT_SPACES} ${PCH_CXX_FLAGS} ${INCLUDE_COMMAND} ${ADDED_DEFINITIONS} ${LIBCORE_STANDARD_HH})
  ADD_CUSTOM_COMMAND(OUTPUT ${LIBCORE_STANDARD_HH_GCH}
                     COMMAND ${FINAL_COMMAND}
                     DEPENDS ${LIBCORE_STANDARD_HH}
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/options/OptionValue.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Any.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Array.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/BoundingBox.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/BoundingSphere.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Factory.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Location.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Logging.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Matrix3x3.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Noncopyable.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Platform.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Quaternion.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/SelfWeakPtr.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Singleton.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Standard.hh
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/TotallyOrdered.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Transform.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/BoundingInfo.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/UUID.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/ThreadId.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Vector3.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Vector4.hpp
 ${LIBCORE_INCLUDE_DIR}/sirikata/core/util/SolidAngle.hpp
 ${LIBCORE_SOURCE_DIR}/util/boost_sha1.hpp
 ${LIBCORE_SOURCE_DIR}/util/boost_uuid.hpp
 ${LIBCORE_SOURCE_DIR}/util/valgrind.h
                     COMMENT "${FINAL_COMMAND}")
#  ADD_DEPENDENCIES(${SIRIKATA_CORE_LIB} ${LIBCORE_STANDARD_HH_GCH})
 ENDIF()
ENDIF()

SET(LIBSPACE_SOURCES
  ${LIBSPACE_SOURCE_DIR}/CoordinateSegmentation.cpp
  ${LIBSPACE_SOURCE_DIR}/LoadMonitor.cpp
  ${LIBSPACE_SOURCE_DIR}/ObjectSegmentation.cpp
  ${LIBSPACE_SOURCE_DIR}/OSegLookupTraceToken.cpp
  ${LIBSPACE_SOURCE_DIR}/ServerMessage.cpp
  ${LIBSPACE_SOURCE_DIR}/SpaceContext.cpp
  ${LIBSPACE_SOURCE_DIR}/Trace.cpp
  ${LIBSPACE_SOURCE_DIR}/PintoServerQuerier.cpp
  ${LIBSPACE_SOURCE_DIR}/ProxSimulationTraits.cpp
  )

SET(LIBPROXYOBJECT_SOURCES
  ${Bullet_PBJ_CPP_FILES}
                  ${LIBPROXYOBJECT_SOURCE_DIR}/ProxyObject.cpp
                  ${LIBPROXYOBJECT_SOURCE_DIR}/VWObject.cpp
                  ${LIBPROXYOBJECT_SOURCE_DIR}/ProxyMeshObject.cpp
                  ${LIBPROXYOBJECT_SOURCE_DIR}/models/MeshObject.cpp
                  ${LIBPROXYOBJECT_SOURCE_DIR}/ProxyLightObject.cpp
                  ${LIBPROXYOBJECT_SOURCE_DIR}/ProxyCameraObject.cpp
                  ${LIBPROXYOBJECT_SOURCE_DIR}/ProxyWebViewObject.cpp
                  ${LIBPROXYOBJECT_SOURCE_DIR}/ModelsSystemFactory.cpp
                  ${LIBPROXYOBJECT_SOURCE_DIR}/LightInfo.cpp
                  ${LIBPROXYOBJECT_SOURCE_DIR}/SimulationFactory.cpp
    )
SET(LIBOH_SOURCES
                  ${LIBOH_SOURCE_DIR}/SpaceNodeConnection.cpp
                  ${LIBOH_SOURCE_DIR}/SessionManager.cpp
                  ${LIBOH_SOURCE_DIR}/ObjectHost.cpp
                  ${LIBOH_SOURCE_DIR}/ObjectFactory.cpp
                  ${LIBOH_SOURCE_DIR}/SpaceIDMap.cpp
                  ${LIBOH_SOURCE_DIR}/HostedObject.cpp
                  ${LIBOH_SOURCE_DIR}/ObjectHostProxyManager.cpp
                  ${LIBOH_SOURCE_DIR}/SpaceTimeOffsetManager.cpp
                  ${LIBOH_SOURCE_DIR}/ObjectHostTimeOffsetManager.cpp
                  ${LIBOH_SOURCE_DIR}/ObjectScriptManagerFactory.cpp
                  ${LIBOH_SOURCE_DIR}/ObjectHostContext.cpp
                  ${LIBOH_SOURCE_DIR}/Trace.cpp
                   )

SET(CPPOH_SOURCES
  ${CPPOH_SOURCE_DIR}/main.cpp
  ${CPPOH_SOURCE_DIR}/Options.cpp
  ${ObjectHostBinary_PBJ_CPP_FILES}
 )

# plugins sources

SET(LIBCORE_PLUGIN_SKELETON_DIR ${LIBCORE_PLUGIN_DIR}/skeleton)
SET(LIBCORE_PLUGIN_SKELETON_SOURCES ${LIBCORE_PLUGIN_SKELETON_DIR}/PluginInterface.cpp)

SET(LIBCORE_PLUGIN_SQLITE_DIR ${LIBCORE_PLUGIN_DIR}/sqlite)
SET(LIBCORE_PLUGIN_SQLITE_SOURCES
  ${SQLite_PBJ_CPP_FILES}
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

SET(LIBCORE_PLUGIN_WEIGHTEXP_DIR ${LIBCORE_PLUGIN_DIR}/weightexp)
SET(LIBCORE_PLUGIN_WEIGHTEXP_SOURCES
  ${LIBCORE_PLUGIN_WEIGHTEXP_DIR}/PluginInterface.cpp
  ${LIBCORE_PLUGIN_WEIGHTEXP_DIR}/ExpIntegral.cpp
  )

SET(LIBCORE_PLUGIN_WEIGHTSQR_DIR ${LIBCORE_PLUGIN_DIR}/weightsqr)
SET(LIBCORE_PLUGIN_WEIGHTSQR_SOURCES
  ${LIBCORE_PLUGIN_WEIGHTSQR_DIR}/PluginInterface.cpp
  ${LIBCORE_PLUGIN_WEIGHTSQR_DIR}/SqrIntegral.cpp
  )

SET(LIBCORE_PLUGIN_SERVERMAP_TABULAR_DIR ${LIBCORE_PLUGIN_DIR}/servermap_tabular)
SET(LIBCORE_PLUGIN_SERVERMAP_TABULAR_SOURCES
  ${LIBCORE_PLUGIN_SERVERMAP_TABULAR_DIR}/PluginInterface.cpp
  ${LIBCORE_PLUGIN_SERVERMAP_TABULAR_DIR}/TabularServerIDMap.cpp
  )

SET(LIBCORE_PLUGIN_LOCAL_DIR ${LIBCORE_PLUGIN_DIR}/local)
SET(LIBCORE_PLUGIN_LOCAL_SOURCES
  ${LIBCORE_PLUGIN_LOCAL_DIR}/PluginInterface.cpp
  ${LIBCORE_PLUGIN_LOCAL_DIR}/LocalServerIDMap.cpp
  )


SET(LIBPROXYOBJECT_PLUGIN_OGREGRAPHICS_DIR ${LIBPROXYOBJECT_PLUGIN_DIR}/ogre)
SET(GFX ${LIBPROXYOBJECT_PLUGIN_OGREGRAPHICS_DIR})
SET(INPUT ${GFX}/input)
SET(LIBPROXYOBJECT_PLUGIN_OGREGRAPHICS_SOURCES
  ${Ogre_PBJ_CPP_FILES}
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
${GFX}/resourceManager/GraphicsResource.cpp
${GFX}/resourceManager/GraphicsResourceAsset.cpp
${GFX}/resourceManager/GraphicsResourceEntity.cpp
${GFX}/resourceManager/GraphicsResourceManager.cpp
${GFX}/resourceManager/GraphicsResourceMaterial.cpp
${GFX}/resourceManager/GraphicsResourceMesh.cpp
${GFX}/resourceManager/GraphicsResourceModel.cpp
${GFX}/resourceManager/GraphicsResourceName.cpp
${GFX}/resourceManager/GraphicsResourceShader.cpp
${GFX}/resourceManager/GraphicsResourceSkeleton.cpp
${GFX}/resourceManager/GraphicsResourceTexture.cpp
${GFX}/resourceManager/ManualMaterialLoader.cpp
${GFX}/resourceManager/ReplacingDataStream.cpp
${GFX}/resourceManager/ResourceDependencyTask.cpp
${GFX}/resourceManager/ResourceDownloadTask.cpp
${GFX}/resourceManager/ResourceDownloadPlanner.cpp
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

SET(LIBSPACE_PLUGIN_CRAQ_DIR ${LIBSPACE_PLUGIN_DIR}/craq)
SET(LIBSPACE_PLUGIN_CRAQ_SOURCES
  ${LIBSPACE_PLUGIN_CRAQ_DIR}/PluginInterface.cpp
  ${LIBSPACE_PLUGIN_CRAQ_DIR}/CraqObjectSegmentation.cpp
  ${LIBSPACE_PLUGIN_CRAQ_DIR}/craq_hybrid/asyncCraqHybrid.cpp
  ${LIBSPACE_PLUGIN_CRAQ_DIR}/craq_hybrid/asyncCraqUtil.cpp
  ${LIBSPACE_PLUGIN_CRAQ_DIR}/craq_hybrid/craq_gets/asyncConnectionGet.cpp
  ${LIBSPACE_PLUGIN_CRAQ_DIR}/craq_hybrid/craq_gets/asyncCraqGet.cpp
  ${LIBSPACE_PLUGIN_CRAQ_DIR}/craq_hybrid/craq_sets/asyncConnectionSet.cpp
  ${LIBSPACE_PLUGIN_CRAQ_DIR}/craq_hybrid/craq_sets/asyncCraqSet.cpp
  ${LIBSPACE_PLUGIN_CRAQ_DIR}/craq_oseg/asyncCraq.cpp
  ${LIBSPACE_PLUGIN_CRAQ_DIR}/craq_oseg/CraqEntry.cpp
  ${LIBSPACE_PLUGIN_CRAQ_DIR}/craq_oseg/Base64.cpp
  ${LIBSPACE_PLUGIN_CRAQ_DIR}/craq_oseg/asyncConnection.cpp
  ${LIBSPACE_PLUGIN_CRAQ_DIR}/craq_oseg/asyncUtil.cpp
  )

SET(LIBSPACE_PLUGIN_LOCAL_DIR ${LIBSPACE_PLUGIN_DIR}/local)
SET(LIBSPACE_PLUGIN_LOCAL_SOURCES
  ${LIBSPACE_PLUGIN_LOCAL_DIR}/PluginInterface.cpp
  ${LIBSPACE_PLUGIN_LOCAL_DIR}/LocalObjectSegmentation.cpp
)

SET(LIBSPACE_PLUGIN_MASTER_PINTO_DIR ${LIBSPACE_PLUGIN_DIR}/master-pinto)
SET(LIBSPACE_PLUGIN_MASTER_PINTO_SOURCES
  ${LIBSPACE_PLUGIN_MASTER_PINTO_DIR}/PluginInterface.cpp
  ${LIBSPACE_PLUGIN_MASTER_PINTO_DIR}/MasterPintoServerQuerier.cpp
)

SET(SPACE_SOURCES
  ${SPACE_SOURCE_DIR}/AlwaysLocationUpdatePolicy.cpp
  ${SPACE_SOURCE_DIR}/CBRLocationServiceCache.cpp
  ${SPACE_SOURCE_DIR}/CoordinateSegmentationClient.cpp
  ${SPACE_SOURCE_DIR}/caches/Complete_Cache.cpp
  ${SPACE_SOURCE_DIR}/caches/CacheRecords.cpp
  ${SPACE_SOURCE_DIR}/caches/FCache.cpp
  ${SPACE_SOURCE_DIR}/caches/CommunicationCache.cpp
  ${SPACE_SOURCE_DIR}/caches/CacheLRUOriginal.cpp
  ${SPACE_SOURCE_DIR}/RegionODPFlowScheduler.cpp
  ${SPACE_SOURCE_DIR}/CSFQODPFlowScheduler.cpp
  ${SPACE_SOURCE_DIR}/ServerMessageReceiver.cpp
  ${SPACE_SOURCE_DIR}/FairServerMessageReceiver.cpp
  ${SPACE_SOURCE_DIR}/ServerMessageQueue.cpp
  ${SPACE_SOURCE_DIR}/FairServerMessageQueue.cpp
  ${SPACE_SOURCE_DIR}/Forwarder.cpp
  ${SPACE_SOURCE_DIR}/ForwarderServiceQueue.cpp
  ${SPACE_SOURCE_DIR}/LocalForwarder.cpp
  ${SPACE_SOURCE_DIR}/LocationService.cpp
  ${SPACE_SOURCE_DIR}/MigrationMonitor.cpp
  ${SPACE_SOURCE_DIR}/ObjectConnection.cpp
  ${SPACE_SOURCE_DIR}/ObjectHostConnectionManager.cpp
  ${SPACE_SOURCE_DIR}/Options.cpp
  ${SPACE_SOURCE_DIR}/OSegHasher.cpp
  ${SPACE_SOURCE_DIR}/OSegLookupQueue.cpp
  ${SPACE_SOURCE_DIR}/SpaceNetwork.cpp
  ${SPACE_SOURCE_DIR}/Proximity.cpp
  ${SPACE_SOURCE_DIR}/Server.cpp
  ${SPACE_SOURCE_DIR}/StandardLocationService.cpp
  ${SPACE_SOURCE_DIR}/TCPSpaceNetwork.cpp
#  ${SPACE_SOURCE_DIR}/Test.cpp
  ${SPACE_SOURCE_DIR}/UniformCoordinateSegmentation.cpp
  ${SPACE_SOURCE_DIR}/main.cpp
)

SET(SIMOH_SOURCES
  ${SIMOH_SOURCE_DIR}/RandomMotionPath.cpp
  ${SIMOH_SOURCE_DIR}/QuakeMotionPath.cpp
  ${SIMOH_SOURCE_DIR}/StaticMotionPath.cpp
  ${SIMOH_SOURCE_DIR}/Object.cpp
  ${SIMOH_SOURCE_DIR}/ObjectFactory.cpp
  ${SIMOH_SOURCE_DIR}/OSegTestMotionPath.cpp
  ${SIMOH_SOURCE_DIR}/ScenarioFactory.cpp
  ${SIMOH_SOURCE_DIR}/ConnectedObjectTracker.cpp
  ${SIMOH_SOURCE_DIR}/DistributionPingScenario.cpp
  ${SIMOH_SOURCE_DIR}/PingDelugeScenario.cpp
  ${SIMOH_SOURCE_DIR}/LoadPacketTrace.cpp
  ${SIMOH_SOURCE_DIR}/DelugePairScenario.cpp
  ${SIMOH_SOURCE_DIR}/ByteTransferScenario.cpp
  ${SIMOH_SOURCE_DIR}/NullScenario.cpp
  ${SIMOH_SOURCE_DIR}/ObjectHost.cpp
  ${SIMOH_SOURCE_DIR}/Options.cpp
  ${SIMOH_SOURCE_DIR}/main.cpp
  )

SET(CSEG_SOURCES
  ${CSEG_SOURCE_DIR}/DistributedCoordinateSegmentation.cpp
  ${CSEG_SOURCE_DIR}/Options.cpp
  ${CSEG_SOURCE_DIR}/WorldPopulationBSPTree.cpp
  ${CSEG_SOURCE_DIR}/main.cpp
  ${CSEG_SOURCE_DIR}/LoadBalancer.cpp

  )

SET(PINTO_SOURCES
  ${PINTO_SOURCE_DIR}/PintoManager.cpp
  ${PINTO_SOURCE_DIR}/PintoManagerLocationServiceCache.cpp
  ${PINTO_SOURCE_DIR}/Options.cpp
  ${PINTO_SOURCE_DIR}/main.cpp
  )

SET(ANALYSIS_SOURCES
  ${ANALYSIS_SOURCE_DIR}/Analysis.cpp
  ${ANALYSIS_SOURCE_DIR}/FlowStats.cpp
  ${ANALYSIS_SOURCE_DIR}/RecordedMotionPath.cpp
  ${ANALYSIS_SOURCE_DIR}/MessageLatency.cpp
  ${ANALYSIS_SOURCE_DIR}/ObjectLatency.cpp
  ${ANALYSIS_SOURCE_DIR}/Options.cpp
  #${ANALYSIS_SOURCE_DIR}/Visualization.cpp
  ${ANALYSIS_SOURCE_DIR}/main.cpp
)

SET(BENCH_SOURCES
  ${BENCH_SOURCE_DIR}/BenchmarkRunner.cpp
  ${BENCH_SOURCE_DIR}/TimerSpeedBenchmark.cpp
  ${BENCH_SOURCE_DIR}/TimerJitterBenchmark.cpp
  ${BENCH_SOURCE_DIR}/TimerMonotonicityBenchmark.cpp
  ${BENCH_SOURCE_DIR}/SSTBenchmark.cpp
  ${BENCH_SOURCE_DIR}/main.cpp
)

#test source files
SET(CXXTESTSources
libcore/test/TransferTest.hpp
libcore/test/AnyTest.hpp
libcore/test/AtomicTest.hpp
#libcore/test/CacheLayerTest.hpp
libcore/test/DownloadTest.hpp
libcore/test/EventTest.hpp
libcore/test/ExtrapolationTest.hpp
libcore/test/FactoryTest.hpp
libcore/test/FairQueueTest.hpp
libcore/test/ListenerTest.hpp
libcore/test/Matrix3Test.hpp
libcore/test/MinitransactionHandlerTest.hpp
libcore/test/NameLookupTest.hpp
libcore/test/ObjectStorageTest.hpp
libcore/test/OptionTest.hpp
#libcore/test/ProxTest.hpp
libcore/test/QuaternionTest.hpp
libcore/test/RoutableMessageTest.hpp
libcore/test/SstCloseTest.hpp
libcore/test/SstTest.hpp
libcore/test/SubscriptionTest.hpp
#libcore/test/ThreadSafeQueueTest.hpp
libcore/test/TR1Test.hpp
#libcore/test/UploadTest.hpp
libcore/test/Vector3Test.hpp
 )
IF(NOT WIN32)
SET(CXXTESTSources ${CXXTESTSources} libcore/test/ReadWriteHandlerTest.hpp libcore/test/SQLiteMinitransactionTest.hpp libcore/test/SQLiteReadWriteTest.hpp)
ENDIF()
#  libcore/test/ThreadSafeQueueTest.hpp
ADD_CXXTEST_CPP_TARGET(CXXTEST ${CXXTESTSources}
	LIBRARYDIR ${CXXTESTRoot})





SET(TEST_SOURCES
  ${LIBCORE_DIR}/test/Test.cpp
  ${CXXTEST_CPP_FILE}
  ${Test_PBJ_CPP_FILES}
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
    ${Boost_LIBRARIES}
    http-parser )

IF(BERKELIUM_FOUND)
  SET(BERKELIUM_CFLAGS -DHAVE_BERKELIUM)
ELSE()
  SET(BERKELIUM_CFLAGS "")
ENDIF()

SET(FINAL_LINK_DIRS
  ${ANTLR_LIBRARY_DIRS}
  ${Boost_LIBRARY_DIRS}
  ${GSL_LINK_DIRECTORIES}
)

IF(OGRE_FOUND AND sdl_FOUND)
SET(FINAL_LINK_DIRS ${FINAL_LINK_DIRS} ${OGRE_LIBRARY_DIRS} ${sdl_LIBRARY_DIRS} ${BERKELIUM_LIBRARY_DIRS} ${CMAKE_CURRENT_BINARY_DIR}/chrome)
ENDIF()
IF(OpenCOLLADA_FOUND)
SET(FINAL_LINK_DIRS ${FINAL_LINK_DIRS} ${OpenCOLLADA_LIBRARY_DIRS})
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
SET(SIRIKATA_OH_LIB sirikata-oh)
SET(SIRIKATA_PROXYOBJECT_LIB sirikata-proxyobject)
SET(SPACE_BINARY space)
SET(CPPOH_BINARY cppoh)
SET(TEST_BINARY tests)
SET(BENCH_BINARY bench)


# FIXME we're doing static linking now and need this to get the export/import
# macros working properly.
# ADD_DEFINITIONS(-DSTATIC_LINKED)

# Debug compile flags. On windows this can't be done globally -- we need to specify this
# only on the debug build because ISDEBUG isn't picked up since there is not build type on
# Windows, all are generated.  Instead we just set up a variable to apply to all targets.
SET(COMPILE_DEFS_OPT COMPILE_DEFINITIONS_DEBUG SIRIKATA_DEBUG_BUILD)
# Unfortunately, CMake 2.4 doesn't handle the above flag
# properly. This should only be an issue on Mac and Linux, and on
# those platforms ISDEBUG should work fine.
IF (${CMAKE_MAJOR_VERSION} LESS 3)
  IF(${CMAKE_MINOR_VERSION} LESS 6)
    IF(ISDEBUG)
      ADD_DEFINITIONS(-DSIRIKATA_DEBUG_BUILD)
    ENDIF()
  ENDIF()
ENDIF()

#libraries
ADD_LIBRARY(${SIRIKATA_CORE_LIB} SHARED ${SIRIKATA_CORE_SOURCES})
SET_TARGET_PROPERTIES(${SIRIKATA_CORE_LIB} PROPERTIES ${COMPILE_DEFS_OPT} COMPILE_DEFINITIONS SIRIKATA_BUILD)
TARGET_LINK_LIBRARIES(${SIRIKATA_CORE_LIB} ${SIRIKATA_CORE_LIBRARIES} ${PROTOCOLBUFFERS_LIBRARIES})
IF(sirikata_core_LDFLAGS OR sirikata_LDFLAGS)
  SET(sirikata_core_LDFLAGS ${sirikata_core_LDFLAGS} ${sirikata_LDFLAGS})
  SET_TARGET_PROPERTIES(${SIRIKATA_CORE_LIB} PROPERTIES LINK_FLAGS ${sirikata_core_LDFLAGS})
ENDIF()
#ADD_DEPENDENCIES(${SIRIKATA_CORE_LIB})

ADD_LIBRARY(${SIRIKATA_SPACE_LIB} SHARED ${LIBSPACE_SOURCES})
SET_TARGET_PROPERTIES(${SIRIKATA_SPACE_LIB} PROPERTIES ${COMPILE_DEFS_OPT} COMPILE_DEFINITIONS SIRIKATA_SPACE_BUILD)
ADD_DEPENDENCIES(${SIRIKATA_SPACE_LIB} ${SIRIKATA_PROXYOBJECT_LIB}  ${SIRIKATA_CORE_LIB})
IF(sirikata_core_LDFLAGS)# use the core ldflags--these guys act similarly to core for their respective modules
  SET_TARGET_PROPERTIES(${SIRIKATA_SPACE_LIB} PROPERTIES LINK_FLAGS ${sirikata_core_LDFLAGS})
ENDIF()
TARGET_LINK_LIBRARIES(${SIRIKATA_SPACE_LIB} ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB} ${PROTOCOLBUFFERS_LIBRARIES})


ADD_LIBRARY(${SIRIKATA_PROXYOBJECT_LIB} SHARED ${LIBPROXYOBJECT_SOURCES})
SET_TARGET_PROPERTIES(${SIRIKATA_PROXYOBJECT_LIB} PROPERTIES ${COMPILE_DEFS_OPT} COMPILE_DEFINITIONS SIRIKATA_PROXYOBJECT_BUILD)
ADD_DEPENDENCIES(${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB})
IF(sirikata_core_LDFLAGS)# use the core ldflags--these guys act similarly to core for their respective modules
  SET_TARGET_PROPERTIES(${SIRIKATA_PROXYOBJECT_LIB} PROPERTIES LINK_FLAGS ${sirikata_core_LDFLAGS})
ENDIF()
TARGET_LINK_LIBRARIES(${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB})

ADD_LIBRARY(${SIRIKATA_OH_LIB} SHARED ${LIBOH_SOURCES})
SET_TARGET_PROPERTIES(${SIRIKATA_OH_LIB} PROPERTIES ${COMPILE_DEFS_OPT} COMPILE_DEFINITIONS SIRIKATA_OH_BUILD)
ADD_DEPENDENCIES(${SIRIKATA_OH_LIB} ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB})
IF(sirikata_core_LDFLAGS)# use the core ldflags--these guys act similarly to core for their respective modules
  SET_TARGET_PROPERTIES(${SIRIKATA_OH_LIB} PROPERTIES LINK_FLAGS ${sirikata_core_LDFLAGS})
ENDIF()
TARGET_LINK_LIBRARIES(${SIRIKATA_OH_LIB} ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB} ${PROTOCOLBUFFERS_LIBRARIES})

#plugins
ADD_PLUGIN_TARGET(skeleton
                    SOURCES ${LIBCORE_PLUGIN_SKELETON_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB})

IF(SQLite3_FOUND)
ADD_PLUGIN_TARGET(sqlite
                    SOURCES ${LIBCORE_PLUGIN_SQLITE_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB}
                    LIBRARIES ${SIRIKATA_CORE_LIB} ${SQLite3_LIBRARIES} ${PROTOCOLBUFFERS_LIBRARIES})
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} sqlite)
ENDIF()

ADD_PLUGIN_TARGET(tcpsst
                    SOURCES ${LIBCORE_PLUGIN_TCPSST_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB})
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} tcpsst)

ADD_PLUGIN_TARGET(weight-exp
                    SOURCES ${LIBCORE_PLUGIN_WEIGHTEXP_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB})
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} weight-exp)

ADD_PLUGIN_TARGET(weight-sqr
                    SOURCES ${LIBCORE_PLUGIN_WEIGHTSQR_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB} ${GSL_LIBRARIES}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB})
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} weight-sqr)

ADD_PLUGIN_TARGET(servermap-tabular
                    SOURCES ${LIBCORE_PLUGIN_SERVERMAP_TABULAR_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB})
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} servermap-tabular)

ADD_PLUGIN_TARGET(core-local
                    SOURCES ${LIBCORE_PLUGIN_LOCAL_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB})
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} core-local)

ADD_PLUGIN_TARGET(space-craq
                    SOURCES ${LIBSPACE_PLUGIN_CRAQ_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB})
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} space-craq)

ADD_PLUGIN_TARGET(space-local
                    SOURCES ${LIBSPACE_PLUGIN_LOCAL_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB})
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} space-local)

ADD_PLUGIN_TARGET(space-master-pinto
                    SOURCES ${LIBSPACE_PLUGIN_MASTER_PINTO_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB})
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} space-master-pinto)

IF(OGRE_FOUND AND sdl_FOUND )
  IF(WIN32)#not sure why this needs to happen again
    STRING(REPLACE "boost_system" "boost_regex" Boost_REGEX_LIBRARY ${Boost_SYSTEM_LIBRARY})
    STRING(REPLACE "boost_system" "boost_regex" Boost_REGEX_LIBRARY_DEBUG ${Boost_SYSTEM_LIBRARY_DEBUG})
    STRING(REPLACE "boost_system" "boost_regex" Boost_REGEX_LIBRARY_RELEASE ${Boost_SYSTEM_LIBRARY_RELEASE})
  ENDIF()
  ADD_PLUGIN_TARGET(ogregraphics
                    SOURCES ${LIBPROXYOBJECT_PLUGIN_OGREGRAPHICS_SOURCES}
                    TARGET_CXXFLAGS ${OGRE_CFLAGS} ${sdl_CFLAGS} ${BERKELIUM_CFLAGS}
                    TARGET_LDFLAGS ${OGRE_LDFLAGS} ${sdl_LDFLAGS} ${BERKELIUM_LDFLAGS} ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB} ${BERKELIUM_TARGET_LIBRARIES}
                    LIBRARIES ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB}
                              ${OGRE_LIBRARIES} ${sdl_LIBRARIES} ${BERKELIUM_LIBRARIES}
                              debug ${Boost_REGEX_LIBRARY_DEBUG} optimized ${Boost_REGEX_LIBRARY_RELEASE}
                               ${PROTOCOLBUFFERS_LIBRARIES})
  IF(NOT WIN32)
    ADD_DEPENDENCIES(ogregraphics berkelium)
    IF(BERKELIUM_FOUND)
      TARGET_LINK_LIBRARIES(ogregraphics -Lchrome debug liblibberkelium_d${DYLIB_EXT} optimized liblibberkelium${DYLIB_EXT} )
    ENDIF()
  ENDIF()
  #IF(ISDEBUG)
  #  SET(BERKELIUM_LDFLAGS -llibberkelium_d)
  #ELSE()
  #  SET(BERKELIUM_LDFLAGS -llibberkelium)
  #ENDIF()

SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} ogregraphics)
ENDIF(OGRE_FOUND AND sdl_FOUND)


SET(LIBPROXYOBJECT_PLUGIN_BULLETPHYSICS_DIR ${LIBPROXYOBJECT_PLUGIN_DIR}/bullet)
SET(LIBPROXYOBJECT_PLUGIN_BULLETPHYSICS_SOURCES
 ${LIBPROXYOBJECT_PLUGIN_BULLETPHYSICS_DIR}/BulletSystem.cpp
    )
IF(bullet_FOUND)
ADD_PLUGIN_TARGET(bulletphysics
                    SOURCES ${LIBPROXYOBJECT_PLUGIN_BULLETPHYSICS_SOURCES}
                    TARGET_CXXFLAGS ${bullet_CFLAGS}
                    TARGET_LDFLAGS ${bullet_LDFLAGS} ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB}
                    LIBRARIES ${bullet_LIBRARIES} ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB})
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} bulletphysics)
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
                    LIBRARIES ${MONO_LIBRARIES} ${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB})
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} monoscript)
ENDIF()


SET(LIBOH_PLUGIN_CSVFACTORY_DIR ${LIBOH_PLUGIN_DIR}/csvfactory)
SET(LIBOH_PLUGIN_CSVFACTORY_SOURCES
 ${LIBOH_PLUGIN_CSVFACTORY_DIR}/CSVObjectFactory.cpp
 ${LIBOH_PLUGIN_CSVFACTORY_DIR}/PluginInterface.cpp
    )
ADD_PLUGIN_TARGET(csvfactory
                    SOURCES ${LIBOH_PLUGIN_CSVFACTORY_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB}
                    LIBRARIES ${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB})
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} csvfactory)


SET(LIBPROXYOBJECT_PLUGIN_COLLADAMODELS_DIR ${LIBPROXYOBJECT_PLUGIN_DIR}/collada)
SET(LIBPROXYOBJECT_PLUGIN_COLLADAMODELS_SOURCES
 ${LIBPROXYOBJECT_PLUGIN_COLLADAMODELS_DIR}/ColladaPlugin.cpp
 ${LIBPROXYOBJECT_PLUGIN_COLLADAMODELS_DIR}/ColladaSystem.cpp
 ${LIBPROXYOBJECT_PLUGIN_COLLADAMODELS_DIR}/ColladaMeshObject.cpp
 ${LIBPROXYOBJECT_PLUGIN_COLLADAMODELS_DIR}/ColladaDocumentImporter.cpp
 ${LIBPROXYOBJECT_PLUGIN_COLLADAMODELS_DIR}/ColladaDocumentLoader.cpp
 ${LIBPROXYOBJECT_PLUGIN_COLLADAMODELS_DIR}/ColladaErrorHandler.cpp
 ${LIBPROXYOBJECT_PLUGIN_COLLADAMODELS_DIR}/ColladaAsset.cpp
 ${LIBPROXYOBJECT_PLUGIN_COLLADAMODELS_DIR}/ColladaDocument.cpp
    )

IF(OpenCOLLADA_FOUND)
  ADD_PLUGIN_TARGET(colladamodels
                    SOURCES ${LIBPROXYOBJECT_PLUGIN_COLLADAMODELS_SOURCES}
                    TARGET_CXXFLAGS ${OpenCOLLADA_CFLAGS}
                    TARGET_LDFLAGS ${OpenCOLLADA_LDFLAGS} ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB}
                    LIBRARIES ${OpenCOLLADA_LIBRARIES} ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB})
  SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} colladamodels)
ENDIF(OpenCOLLADA_FOUND)

#binaries
ADD_EXECUTABLE(${TEST_BINARY} ${TEST_SOURCES})# EXCLUDE_FROM_ALL
SET_TARGET_PROPERTIES(${TEST_BINARY} PROPERTIES ${COMPILE_DEFS_OPT})
ADD_DEPENDENCIES(${TEST_BINARY} tcpsst)

ADD_EXECUTABLE(${SPACE_BINARY} ${SPACE_SOURCES})
SET_TARGET_PROPERTIES(${SPACE_BINARY} PROPERTIES ${COMPILE_DEFS_OPT})
TARGET_LINK_LIBRARIES(${SPACE_BINARY}
        ${Boost_LIBRARIES}
        ${SIRIKATA_CORE_LIB}
        ${SIRIKATA_SPACE_LIB}
        ${PROTOCOLBUFFERS_LIBRARIES}
        )

ADD_EXECUTABLE(${CPPOH_BINARY} ${CPPOH_SOURCES})
SET_TARGET_PROPERTIES(${CPPOH_BINARY} PROPERTIES ${COMPILE_DEFS_OPT})

ADD_EXECUTABLE(simoh ${SIMOH_SOURCES})
SET_TARGET_PROPERTIES(simoh PROPERTIES ${COMPILE_DEFS_OPT})
TARGET_LINK_LIBRARIES(simoh
        ${Boost_LIBRARIES}
        ${SIRIKATA_CORE_LIB}
        ${SIRIKATA_OH_LIB}
        ${PROTOCOLBUFFERS_LIBRARIES}
        )

ADD_EXECUTABLE(cseg ${CSEG_SOURCES})
SET_TARGET_PROPERTIES(cseg PROPERTIES ${COMPILE_DEFS_OPT})
TARGET_LINK_LIBRARIES(cseg
        ${Boost_LIBRARIES}
        ${SIRIKATA_CORE_LIB}
        ${PROTOCOLBUFFERS_LIBRARIES}
        )

ADD_EXECUTABLE(pinto ${PINTO_SOURCES})
SET_TARGET_PROPERTIES(pinto PROPERTIES ${COMPILE_DEFS_OPT})
TARGET_LINK_LIBRARIES(pinto
        ${Boost_LIBRARIES}
        ${SIRIKATA_CORE_LIB}
        ${SIRIKATA_SPACE_LIB}
        ${PROTOCOLBUFFERS_LIBRARIES}
        )

ADD_EXECUTABLE(analysis ${ANALYSIS_SOURCES})
SET_TARGET_PROPERTIES(analysis PROPERTIES ${COMPILE_DEFS_OPT})
TARGET_LINK_LIBRARIES(analysis
        ${Boost_LIBRARIES}
        ${SIRIKATA_CORE_LIB}
        ${PROTOCOLBUFFERS_LIBRARIES}
        #${GLUT_LIBRARIES}
        )

ADD_EXECUTABLE(${BENCH_BINARY} ${BENCH_SOURCES})
SET_TARGET_PROPERTIES(${BENCH_BINARY} PROPERTIES ${COMPILE_DEFS_OPT})
TARGET_LINK_LIBRARIES(${BENCH_BINARY}
  ${Boost_LIBRARIES}
  ${SIRIKATA_CORE_LIB}
  ${PROTOCOLBUFFERS_LIBRARIES}
  )
# Fixme incorporate old benchmark
#ADD_EXECUTABLE(${BENCH_BINARY} ${TOP_LEVEL}/libcore/test/SSTBenchmark.cpp)

IF(CHROME_FOUND)
  IF(APPLE)
    # FIXME GET_TARGET_PROPERTY doesn't seem to have anything that gives us this without a full path
    IF(ISDEBUG)
      SET(CPPOH_BINARY_FILENAME "${CPPOH_BINARY}_d")
    ELSE()
      SET(CPPOH_BINARY_FILENAME "${CPPOH_BINARY}")
    ENDIF()

    ADD_CHROME_APP(
      APP ${CPPOH_BINARY_FILENAME}
      DEPENDS ${CPPOH_BINARY}
      LINKS
      ${CMAKE_CURRENT_BINARY_DIR}/chrome/libplugin_carbon_interpose.dylib
      ${CMAKE_CURRENT_BINARY_DIR}/chrome/berkelium
      
      ${CMAKE_CURRENT_BINARY_DIR}/*.dylib
      
      ${TOP_LEVEL}/cdn.txt
      ${CMAKE_CURRENT_BINARY_DIR}/scene.db
      )
    
    # FIXME for some reason, the links inside the .app aren't working
    # here, even though they seem to work fine for Berkelium
    ADD_CUSTOM_TARGET(cppoh_app_help
      ALL COMMAND
      ln -sf chrome/berkelium ${CMAKE_CURRENT_BINARY_DIR}/berkelium &&
      ln -sf ${CHROME_APP_NAME}/Contents/Resources/chrome.pak ${CMAKE_CURRENT_BINARY_DIR}/chrome.pak &&
      ln -sf ${CHROME_APP_NAME}/Contents/Resources/theme.pak ${CMAKE_CURRENT_BINARY_DIR}/theme.pak &&
      ln -sf ${CHROME_APP_NAME}/Contents/Resources/linkCursor.png ${CMAKE_CURRENT_BINARY_DIR}/linkCursor.png &&
      ln -sf ${CHROME_APP_NAME}/Contents/Resources/renderer.sb ${CMAKE_CURRENT_BINARY_DIR}/renderer.sb &&
      ln -sf ${CHROME_APP_NAME}/Contents/Resources/en_US.lproj ${CMAKE_CURRENT_BINARY_DIR}/en_US.lproj
      DEPENDS ${CPPOH_BINARY} ${CPPOH_BINARY_FILENAME}.app
      )
  ELSEIF(ISLINUX)
    ADD_CHROME_SYMLINK_TARGET(TARGETNAME chrome_symlinks_for_sirikata DEPENDS libberkelium)
  ENDIF()
ENDIF()

ADD_DEPENDENCIES(${TEST_BINARY} ${SIRIKATA_CORE_LIB})
ADD_DEPENDENCIES(${SPACE_BINARY} ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB})
ADD_DEPENDENCIES(${CPPOH_BINARY} ${SIRIKATA_CORE_LIB} ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_OH_LIB})

SET_TARGET_PROPERTIES(${SPACE_BINARY} ${CPPOH_BINARY} ${TEST_BINARY} simoh cseg pinto analysis ${BENCH_BINARY}
                      PROPERTIES
                      DEBUG_POSTFIX "_d" )
TARGET_LINK_LIBRARIES(${TEST_BINARY} ${SIRIKATA_CORE_LIB}
                      ${TEST_LIBRARIES} ${PROTOCOLBUFFERS_LIBRARIES})
TARGET_LINK_LIBRARIES(${SPACE_BINARY} ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB})
SET(CPPOH_LINK_LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_OH_LIB})
IF(OGRE_FOUND AND sdl_FOUND)
  SET(CPPOH_LINK_LIBRARIES ${CPPOH_LINK_LIBRARIES} ogregraphics)
ENDIF(OGRE_FOUND AND sdl_FOUND)
IF(bullet_FOUND)
  SET(CPPOH_LINK_LIBRARIES ${CPPOH_LINK_LIBRARIES} bulletphysics)
ENDIF(bullet_FOUND)
IF(OpenCOLLADA_FOUND)
  SET(CPPOH_LINK_LIBRARIES ${CPPOH_LINK_LIBRARIES} colladamodels)
ENDIF(OpenCOLLADA_FOUND)
TARGET_LINK_LIBRARIES(${CPPOH_BINARY} ${CPPOH_LINK_LIBRARIES})

IF(sirikata_LDFLAGS)
  SET_TARGET_PROPERTIES(${TEST_BINARY} PROPERTIES LINK_FLAGS ${sirikata_LDFLAGS})
  SET_TARGET_PROPERTIES(${SPACE_BINARY} PROPERTIES LINK_FLAGS ${sirikata_LDFLAGS})
  SET_TARGET_PROPERTIES(${CPPOH_BINARY} PROPERTIES LINK_FLAGS ${sirikata_LDFLAGS})
  SET_TARGET_PROPERTIES(${BINARY_TO_CPP_BINARY} PROPERTIES LINK_FLAGS ${sirikata_LDFLAGS})
  SET_TARGET_PROPERTIES(${PBJ_BINARY} PROPERTIES LINK_FLAGS ${sirikata_LDFLAGS})
ENDIF()


#precompiled headers
IF(WIN32)
  # Creation is turned on only for libcore/util/Standard.cpp
  SET_SOURCE_FILES_PROPERTIES(${LIBCORE_STANDARD_CPP} PROPERTIES COMPILE_FLAGS "-FIsirikata/core/util/Standard.hh -Ycsirikata/core/util/Standard.hh")

  # Everything uses the PCH
  SET_TARGET_PROPERTIES(${SIRIKATA_CORE_LIB} PROPERTIES COMPILE_FLAGS "-FIsirikata/core/util/Standard.hh -Yusirikata/core/util/Standard.hh")

  SET_TARGET_PROPERTIES(${SIRIKATA_PROXYOBJECT_LIB} PROPERTIES COMPILE_FLAGS "-FIsirikata/proxyobject/Platform.hpp")
ELSE()
  SET_TARGET_PROPERTIES(${SIRIKATA_CORE_LIB} PROPERTIES COMPILE_FLAGS "-include sirikata/core/util/Standard.hh")
  SET_TARGET_PROPERTIES(${SIRIKATA_PROXYOBJECT_LIB} PROPERTIES COMPILE_FLAGS "-include sirikata/proxyobject/Platform.hpp")

ENDIF()


IF(PYTHON_EXECUTABLE) #FIXME should work if we have dotnet protobufs (if we can generate pbj.py files)
  SET(CSV_CONVERTER_BINARY ${OH_TOOLS_DIR}/csv_converter.py)
  SET(INPUT_BLANK_CSV ${DEMO_SCENES_DIR}/blank.csv)
  SET(INPUT_SCENE_CSV ${DEMO_SCENES_DIR}/colladasmall.csv)

  IF(EXISTS ${CSV_CONVERTER_BINARY} AND EXISTS ${INPUT_SCENE_CSV})
    SET(GENERATED_BLANK_DB ${CMAKE_CURRENT_BINARY_DIR}/blank.db)
    SET(GENERATED_SCENE_DB ${CMAKE_CURRENT_BINARY_DIR}/scene.db)

    ADD_CUSTOM_TARGET(SCENEDB ALL
      DEPENDS ${AdditionalProtoDepends} ${GENERATED_BLANK_DB} ${GENERATED_SCENE_DB}
      COMMENT "Checking databases...")

    ADD_CUSTOM_COMMAND(OUTPUT ${GENERATED_SCENE_DB}
      COMMAND ${PYTHON_EXECUTABLE} ${CSV_CONVERTER_BINARY} ${INPUT_SCENE_CSV} ${GENERATED_SCENE_DB}
      DEPENDS ${AdditionalProtoDepends} ${INPUT_SCENE_CSV}
      COMMENT "Making example scene database from  ${GENERATED_SCENE_DB}"
      )
    ADD_CUSTOM_COMMAND(OUTPUT ${GENERATED_BLANK_DB}
      COMMAND ${PYTHON_EXECUTABLE} ${CSV_CONVERTER_BINARY} ${INPUT_BLANK_CSV} ${GENERATED_BLANK_DB}
      DEPENDS ${AdditionalProtoDepends} ${INPUT_BLANK_CSV}
      COMMENT "Making example scene database from  ${GENERATED_BLANK_DB}"
      )
    INSTALL(FILES ${GENERATED_SCENE_DB} DESTINATION bin)
    MESSAGE(STATUS "Added ${GENERATED_SCENE_DB} target.")
  ENDIF()
ENDIF()

SET(EXEC_PERMS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)
#installation information
INSTALL(TARGETS
          ${PBJ_BINARY}
          ${SIRIKATA_CORE_LIB}
          ${SIRIKATA_SPACE_LIB}
          ${SIRIKATA_PROXYOBJECT_LIB}
          ${SIRIKATA_OH_LIB}
          ${PLUGIN_INSTALL_LIST}
          ${SPACE_BINARY}
          ${CPPOH_BINARY}
        PERMISSIONS ${EXEC_PERMS}
        RUNTIME
          DESTINATION bin
        LIBRARY
          DESTINATION lib
        ARCHIVE
          DESTINATION lib
)

#install protobufs
IF(PROTOCOLBUFFERS_ROOT)
  IF(WIN32)
    INSTALL(DIRECTORY ${PROTOCOLBUFFERS_ROOT}/bin/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION bin/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.exe" EXCLUDE)
  ELSE()
    INSTALL(DIRECTORY ${PROTOCOLBUFFERS_ROOT}/lib/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION lib/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.exe" EXCLUDE)
  ENDIF()
ENDIF()


#install ois
IF(OIS_ROOT AND FOUND_OIS)
  IF(WIN32)
    INSTALL(DIRECTORY ${OIS_ROOT}/bin/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION bin/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.exe" EXCLUDE)
  ELSE()
    INSTALL(DIRECTORY ${OIS_ROOT}/lib/
            FILE_PERMISSIONS ${EXEC_PERMS}
            PATTERN "*.a" EXCLUDE
            PATTERN "*.la" EXCLUDE
            DESTINATION lib/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.exe" EXCLUDE)
  ENDIF()
ENDIF()


#install sql
IF(SQLite3_ROOT)
  IF(WIN32)
    INSTALL(DIRECTORY ${SQLite3_ROOT}/lib/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION bin/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.exe" EXCLUDE
            PATTERN "*.lib" EXCLUDE)
  ELSE()
    INSTALL(DIRECTORY ${SQLite3_ROOT}/lib/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION lib/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.a" EXCLUDE
            PATTERN "*.la" EXCLUDE
            PATTERN "*.exe" EXCLUDE)
  ENDIF()
ENDIF()

#install SDL
IF(sdl_FOUND AND sdl_ROOT)
  IF(WIN32)
    INSTALL(DIRECTORY ${sdl_ROOT}/bin/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION bin/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.exe" EXCLUDE
            PATTERN "*.lib" EXCLUDE)
  ELSEIF(APPLE)
    INSTALL(DIRECTORY ${sdl_ROOT}/lib/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION lib/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.a" EXCLUDE
            PATTERN "*.la" EXCLUDE
            PATTERN "*.exe" EXCLUDE)
  ELSE()
    INSTALL(DIRECTORY ${sdl_LIBRARY_DIRS}/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION lib/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.a" EXCLUDE
            PATTERN "*.la" EXCLUDE
            PATTERN "*.exe" EXCLUDE)
  ENDIF()
ENDIF()

IF(bullet_FOUND AND bullet_ROOT)
  IF(WIN32)
    INSTALL(DIRECTORY ${bullet_ROOT}/bin/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION bin/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.exe" EXCLUDE)
  ELSE()
    INSTALL(DIRECTORY ${bullet_ROOT}/lib/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION lib/
            PATTERN "*.a" EXCLUDE
            PATTERN "*.la" EXCLUDE
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.exe" EXCLUDE)
  ENDIF()
ENDIF()

IF(OGRE_FOUND)
  IF(WIN32)
    INSTALL(DIRECTORY ${OGRE_ROOT}/bin/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION bin/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.exe" EXCLUDE)
  ELSE()
    FOREACH(OGRE_LIBRARY_DIR ${OGRE_LIBRARY_DIRS})
      INSTALL(DIRECTORY ${OGRE_LIBRARY_DIR}/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION lib/
            #PATTERN "*.so"<-- if only we could just INCLUDE a pattern
            PATTERN "*.la" EXCLUDE
            PATTERN "*.a" EXCLUDE
            PATTERN "*.me" EXCLUDE
            PATTERN "*.h" EXCLUDE
            )
     ENDFOREACH()

  ENDIF()

  # data for ogre, including web views
  INSTALL(DIRECTORY ${TOP_LEVEL}/libproxyobject/plugins/ogre/data/
    DESTINATION share/ogre/data/
    )
ENDIF()

#install boost
IF(BOOST_ROOT)
  IF(WIN32)
    FOREACH(Boost_LIBRARY_DIR ${PLATFORM_LIBS}/boost_1_35_0/lib ${PLATFORM_LIBS}/boost_1_37_0/lib ${PLATFORM_LIBS}/boost_1_37_0/bin ${PLATFORM_LIBS}/installed-boost/lib)
      IF(EXISTS ${Boost_LIBRARY_DIR})
      INSTALL(DIRECTORY ${Boost_LIBRARY_DIR}/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION bin/
            #PATTERN "*.so"<-- if only we could just INCLUDE a pattern
            PATTERN "*.la" EXCLUDE
            PATTERN "*.a" EXCLUDE
            PATTERN "*.me" EXCLUDE
            PATTERN "*.hpp" EXCLUDE
	    PATTERN "*.lib" EXCLUDE
            PATTERN "*wave-vc90-mt-1_37.dll" EXCLUDE
            PATTERN "*wave-vc90-mt-gd-1_37.dll" EXCLUDE
            PATTERN "*wave-vc80-mt-1_35.dll" EXCLUDE
            PATTERN "*wave-vc80-mt-gd-1_35.dll" EXCLUDE
            )
     ENDIF()
     ENDFOREACH()
  ELSE()
    INSTALL(DIRECTORY ${BOOST_ROOT}/lib/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION lib/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.exe" EXCLUDE)
  ENDIF()
ENDIF()

#install berkelium
IF(BERKELIUM_FOUND)
IF(WIN32)  
  INSTALL(DIRECTORY ${BERKELIUM_ROOT}/bin/ 
          FILE_PERMISSIONS ${EXEC_PERMS} 
          DESTINATION bin/ )
ELSE()
IF(APPLE)
    IF(NOT CHROME_ROOT)
      SET(CHROME_ROOT ${PLATFORM_LIBS}/ch/c)
    ENDIF()
    SET(CHROMIUM_FRAMEWORK Chromium\ Framework.framework)
    SET(APPLE_PLUGIN_COMMANDS)
    FOREACH (APPLE_PLUGIN ${ENABLED_PLUGINS})
      SET(APPLE_PLUGIN_COMMANDS && ln -sf ${CMAKE_CURRENT_BINARY_DIR}/lib${APPLE_PLUGIN}_d${DYLIB_EXT} ${CMAKE_CURRENT_BINARY_DIR}/${CHROME_SYMLINKS_COMMAND_TARGET}/Contents/MacOS/lib${APPLE_PLUGIN}_d${DYLIB_EXT} && ln -sf ${CMAKE_CURRENT_BINARY_DIR}/lib${APPLE_PLUGIN}${DYLIB_EXT} ${CMAKE_CURRENT_BINARY_DIR}/${CHROME_SYMLINKS_COMMAND_TARGET}/Contents/MacOS/lib${APPLE_PLUGIN}${DYLIB_EXT} ${APPLE_PLUGIN_COMMANDS})

    ENDFOREACH()
    IF(CHROME_FOUND)
      ADD_CUSTOM_TARGET(${CHROME_SYMLINKS_COMMAND_TARGET} ALL COMMAND
      ${CHROME_SYMLINKS_COMMAND} && ln -sf ${TOP_LEVEL}/libproxyobject
      ${CMAKE_CURRENT_BINARY_DIR}/${CHROME_SYMLINKS_COMMAND_TARGET}/ ${APPLE_PLUGIN_COMMANDS})
    ENDIF()
#    ADD_CUSTOM_TARGET(sirikata.app ALL COMMAND mkdir -p sirikata.app && mkdir -p sirikata.app/Contents && mkdir -p sirikata.app/Contents/Resources && mkdir -p sirikata.app/Contents/Frameworks && ln -sf ${CMAKE_CURRENT_BINARY_DIR}/sirikata.app sirikata.app/Contents/Resources/Berkelium\ Helper.app && ln -sf ${CMAKE_CURRENT_BINARY_DIR}/sirikata.app/Contents sirikata.app/Contents/Frameworks/${CHROMIUM_FRAMEWORK} && mkdir -p sirikata.app/Contents/MacOS && ln -sf ${CMAKE_CURRENT_BINARY_DIR}/cppoh_d sirikata.app/Contents/MacOS/cppoh_d&& ln -sf ${CMAKE_CURRENT_BINARY_DIR}/cppoh sirikata.app/Contents/MacOS/cppoh && ln -sf ${CMAKE_CURRENT_BINARY_DIR}/chrome/berkelium sirikata.app/Contents/MacOS/berkelium && ln -sf ${CHROME_ROOT}/src/xcodebuild/Release/${CHROMIUM_FRAMEWORK}/Resources/chrome.pak sirikata.app/Contents/Resources/chrome.pak && ln -sf ${CHROME_ROOT}/src/xcodebuild/Release/${CHROMIUM_FRAMEWORK}/Resources/theme.pak sirikata.app/Contents/Resources/theme.pak && ln -sf ${CHROME_ROOT}/src/xcodebuild/Release/${CHROMIUM_FRAMEWORK}/Resources/linkCursor.png sirikata.app/Contents/Resources/linkCursor.png  && ln -sf ${CHROME_ROOT}/src/xcodebuild/Release/${CHROMIUM_FRAMEWORK}/Resources/renderer.sb sirikata.app/Contents/Resources/renderer.sb && ln -sf ${CHROME_ROOT}/src/xcodebuild/Release/${CHROMIUM_FRAMEWORK}/Resources/en_US.lproj sirikata.app/Contents/Resources/en_US.lproj && ln -sf ${TOP_LEVEL}/cdn.txt sirikata.app/Contents/MacOS/cdn.txt && ln -sf ${GENERATED_SCENE_DB} sirikata.app/Contents/MacOS/scene.db ${APPLE_PLUGIN_COMMANDS})

ELSE()
IF(NOT WIN32)
ADD_CUSTOM_TARGET(berkelium.bin ALL COMMAND ln -sf chrome/berkelium && ln -sf chrome/berkelium berkelium.bin)
#ADD_CUSTOM_TARGET(${CHROME_SYMLINKS_COMMAND_TARGET}.install ALL COMMAND ${CHROME_SYMLINKS_COMMAND} DEPENDS ${CHROME_SYMLINKS_COMMAND_TARGET})
ENDIF()
ENDIF()
ENDIF()
          
ENDIF()

IF(MONO_FOUND)
  INSTALL(PROGRAMS
          ${SirikataScriptRuntime}
        DESTINATION lib/)
  INSTALL(PROGRAMS
        ${SirikataScriptRuntime_LIBRARY}
        DESTINATION lib/)

  IF(PROTOCOLBUFFERS_SUPPORTS_CSHARP)
    INSTALL(PROGRAMS
              ${SirikataScriptProtocol}
            DESTINATION lib/)
    INSTALL(PROGRAMS
              ${SirikataProtoScriptRuntime_LIBRARY}
            DESTINATION lib/)
  ENDIF()

  INSTALL(DIRECTORY
          ${IRON_PYTHON_ROOT}/
          FILE_PERMISSIONS ${EXEC_PERMS}
          DESTINATION lib/)
  IF(MONO_ROOT AND EXISTS ${MONO_ROOT})
    INSTALL(PROGRAMS
        ${MONO_ROOT}/lib/mono/2.0/System.Xml.dll
        DESTINATION lib/)
    INSTALL(DIRECTORY
          ${MONO_ROOT}/lib/
          FILE_PERMISSIONS ${EXEC_PERMS}
          DESTINATION lib/
          #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
	  PATTERN "*zlib1.dll" EXCLUDE
          PATTERN "*.xsd" EXCLUDE
          PATTERN "*.config" EXCLUDE
          PATTERN "*.rsp" EXCLUDE
          PATTERN "*.targets" EXCLUDE
          PATTERN "*.zip" EXCLUDE
          PATTERN "*.cs" EXCLUDE
          PATTERN "*.tree" EXCLUDE
          PATTERN "*.mdb" EXCLUDE
          PATTERN "*.exe" EXCLUDE
          PATTERN "*.la" EXCLUDE
          PATTERN "*.a" EXCLUDE
          PATTERN "*.xml*" EXCLUDE
#         PATTERN "*.pc" EXCLUDE
#          PATTERN "*.so*" EXCLUDE
          PERMISSIONS ${EXEC_PERMS})
  ENDIF()
  IF(WIN32)
    INSTALL(DIRECTORY ${MONO_ROOT}/bin/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION bin/
  	    PATTERN "*zlib1.dll" EXCLUDE
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.exe" EXCLUDE)
  ENDIF()
ENDIF()

#install curl #MUST BE DONE AFTER MONO to get correct libz
IF(CURL_ROOT)
  IF(WIN32)
    INSTALL(DIRECTORY ${CURL_ROOT}/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION bin/
            PATTERN "*.lib" EXCLUDE
            PATTERN "*README" EXCLUDE
            PATTERN "*COPYING" EXCLUDE
            PATTERN "*Makefile" EXCLUDE
            PATTERN "*.am" EXCLUDE
            PATTERN "*.in" EXCLUDE
            PATTERN "*CHANGES" EXCLUDE
            PATTERN "*.exp" EXCLUDE
            PATTERN "*.h" EXCLUDE
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.exe" EXCLUDE)
  ELSE()
    INSTALL(DIRECTORY ${CURL_ROOT}/lib/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION lib/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.exe" EXCLUDE)
  ENDIF()
ENDIF()

INSTALL(DIRECTORY
          # note: the trailing slashes avoid prepending the directory name
          # itself to the installed files
          ${LIBCORE_INCLUDE_DIR}/
          ${LIBSPACE_INCLUDE_DIR}/
          ${LIBOH_INCLUDE_DIR}/
          ${LIBPROXYOBJECT_INCLUDE_DIR}/
        DESTINATION include
        PATTERN "*.cpp" EXCLUDE
        PATTERN "*.cc" EXCLUDE
        PATTERN "*.hh" EXCLUDE
)


INSTALL(DIRECTORY
          # note: the trailing slashes avoid prepending the directory name
          # itself to the installed files
          ${ScriptsRoot}
        DESTINATION lib/
        PATTERN "*.cs" EXCLUDE
        PATTERN "*.pyc" EXCLUDE)


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

INSTALL(FILES ${TOP_LEVEL}/cdn.txt
        DESTINATION bin/)

IF(PYTHON_EXECUTABLE)
  IF(${GENERATED_SCENE_DB})
    INSTALL(FILES ${GENERATED_SCENE_DB}
      DESTINATION bin/
      OPTIONAL )
  ENDIF()
ENDIF()

ADD_CUSTOM_TARGET(test
  DEPENDS tests
  COMMAND ${TEST_RUNABLE} $ENV{SINGLE_SUITE})
