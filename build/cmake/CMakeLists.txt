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

SET( CMAKE_C_FLAGS_RELEASE "-g2 -O3 -Wall -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS" CACHE STRING
    "Built cflags with default settings."
    FORCE )
SET( CMAKE_CXX_FLAGS_RELEASE "-g2 -O3 -Wall -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS" CACHE STRING
    "Built cflags with default settings."
    FORCE )

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
IF(WIN32)
ELSE()
SET(CMAKE_CXX_FLAGS_DEBUG "-g2 -D_DEBUG" CACHE STRING
    "Built cxxflags with default settings."
    FORCE )
SET( CMAKE_C_FLAGS_DEBUG "-g2 -D_DEBUG" CACHE STRING
    "Built cflags with default settings."
    FORCE )
ENDIF()

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

IF(NOT CMAKE_MODULE_PATH)
  SET(CMAKE_MODULE_PATH ${TOP_LEVEL}/build/modules)
ENDIF()
GET_FILENAME_COMPONENT(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ABSOLUTE)

IF(NOT BUILD_DIR)
  SET(BUILD_DIR ${TOP_LEVEL}/build/cmake)
ENDIF()
GET_FILENAME_COMPONENT(BUILD_DIR ${BUILD_DIR} ABSOLUTE)

INCLUDE(GitRevision)

# Set major, minor, and revision numbers. In theory, we should have:
#  * Major number change indicates incompatible ABI changes.
#  * Minor number changes indicate a significant release, but only
#    additions to the API
#  * Revisions also only have additions and are very minor.
SET(SIRIKATA_VERSION_MAJOR "0" CACHE STRING "Sirikata's major version" FORCE)
SET(SIRIKATA_VERSION_MINOR "0" CACHE STRING "Sirikata's minor version" FORCE)
SET(SIRIKATA_VERSION_REVISION "8" CACHE STRING "Sirikata's revision version" FORCE)

# These get translated into two settings: the VERSION and
# SOVERSION. The SOVERSION includes just the major number so anything
# that has support for the same ABI can use it.  The VERSION is the
# full version specifier.
# See http://www.netbsd.org/docs/elf.html#elf-rpath for a more
# detailed explanation.
SET(SIRIKATA_VERSION "${SIRIKATA_VERSION_MAJOR}.${SIRIKATA_VERSION_MINOR}.${SIRIKATA_VERSION_REVISION}"  CACHE STRING "Sirikata's complete version"  FORCE)
SET(SIRIKATA_SOVERSION "${SIRIKATA_VERSION_MAJOR}" CACHE STRING "Sirikata's library (ABI) version" FORCE)
SET(SIRIKATA_VERSION_SETTINGS "VERSION;${SIRIKATA_VERSION};SOVERSION;${SIRIKATA_SOVERSION}" CACHE INTERNAL "Settings to pass to targets to get version info set." FORCE)

# Get the git revision, if we can
GetGitHeadRef(SIRIKATA_GIT_HEAD_REF ${TOP_LEVEL})
GetGitRevision(SIRIKATA_GIT_REVISION_VAL ${TOP_LEVEL} ${SIRIKATA_GIT_HEAD_REF})
SET(SIRIKATA_GIT_REVISION ${SIRIKATA_GIT_REVISION_VAL} CACHE STRING "Sirikata's git hash" FORCE)

# Finally, we generate a configuration file with these settings
CONFIGURE_FILE(${TOP_LEVEL}/libcore/include/sirikata/core/util/Version.hpp.in ${TOP_LEVEL}/libcore/include/sirikata/core/util/Version.hpp)

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
SET(SIRIKATA_OGRE_DATA_DIR ${TOP_LEVEL}/libproxyobject/plugins/ogre/data)
#add-ins

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
INCLUDE(CXXTestUtils)

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

# V8 for JS plugin
IF(NOT V8_ROOT)
  IF(WIN32)
    SET(V8_ROOT ${PLATFORM_LIBS}/v8)
    # TODO: Add path to some prebuilt version of V8 for Win32
  ELSEIF(APPLE)
    SET(V8_ROOT ${PLATFORM_LIBS}/installed-v8)
  ELSE()
    SET(V8_ROOT ${PLATFORM_LIBS}/installed-v8)
  ENDIF()
ENDIF()
FIND_PACKAGE(V8)

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
IF(EXISTS ${PLATFORM_LIBS}/installed-antlr)
  SET(ANTLR_ROOT ${PLATFORM_LIBS}/installed-antlr)
ELSEIF(EXISTS ${PLATFORM_LIBS}/libantlr3c-3.1.3)
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
  ELSEIF(EXISTS ${PLATFORM_LIBS}/ogre-1.7.2)
    SET(OGRE_ROOT ${PLATFORM_LIBS}/ogre-1.7.2)
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
  SET(SQLite3_ROOT ${PLATFORM_LIBS}/installed-sqlite)
ENDIF()
IF(NOT SQLite3_FOUND)
  FIND_PACKAGE(SQLite3)
ENDIF()


IF(NOT HIREDIS_ROOT)
  SET(HIREDIS_ROOT ${PLATFORM_LIBS}/installed-hiredis)
ENDIF()
IF(NOT HIREDIS_FOUND)
  FIND_PACKAGE(HiRedis)
ENDIF()

IF(NOT THRIFT_ROOT)
  SET(THRIFT_ROOT ${PLATFORM_LIBS}/installed-thrift)
ENDIF()
IF(NOT THRIFT_FOUND)
  FIND_PACKAGE(Thrift)
ENDIF()

IF(NOT LIBCASSANDRA_ROOT)
  SET(LIBCASSANDRA_ROOT ${PLATFORM_LIBS}/installed-libcassandra)
ENDIF()
IF(NOT LIBCASSANDRA_FOUND)
  FIND_PACKAGE(Libcassandra)
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
  IF(NOT sdl_ROOT)
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
IF(GSL_FOUND)
  SET(GSL_CXX_FLAGS "-DHAVE_GSL ${GSL_CXX_FLAGS}")
ELSE()
  MESSAGE(STATUS "GSL was not found: Will single sample in center of region to approximate integral.")
  SET(GSL_CXX_FLAGS "")
  SET(GSL_INCLUDE_DIR ${TOP_LEVEL}/externals/berkelium)
  SET(GSL_LINK_DIRECTORIES)
  SET(GSL_LIBRARIES)
ENDIF()

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
  SET(BERKELIUM_TARGET_BINARIES)
ELSE()
  # FIXME real check for berkelium + chrome
  SET(BERKELIUM_FOUND FALSE)
  SET(BERKELIUM_INCLUDE_DIRS)
  SET(BERKELIUM_TARGET_LIBRARIES)
  SET(BERKELIUM_TARGET_BINARIES)
  SET(BERKELIUM_LIBRARIES)
  IF(EXISTS ${TOP_LEVEL}/externals/berkelium/chromium AND EXISTS ${TOP_LEVEL}/externals/berkelium/chromium/chromium)
    SET(SIRIKATA_JQUERY_FILE ${SIRIKATA_OGRE_DATA_DIR}/jquery/js/jquery-1.4.2.min.js)
    IF (EXISTS ${SIRIKATA_JQUERY_FILE})
    ELSE()
      MESSAGE(FATAL_ERROR "Missing jquery symlink in the ogre data directory ${SIRIKATA_JQUERY_FILE} for startup scripts and mouse handling.")
    ENDIF()
    # This name is needed by the ADD_CHROME_APP macro
    SET(BERKELIUM_TOP_LEVEL ${TOP_LEVEL}/externals/berkelium)

    #Prevent submodule from making a CHROME_SYMLINK_TARGET
    SET(CHROME_SYMLINKS_TARGET cppoh)
    SET(CHROME_SYMLINKS_BINARIES ${CHROME_SYMLINKS_BINARIES}
                   ${CMAKE_CURRENT_BINARY_DIR}/cppoh
                   ${CMAKE_CURRENT_BINARY_DIR}/chrome/berkelium
                   ${CMAKE_CURRENT_BINARY_DIR}/chrome/plugin_carbon_interpose.dylib
                   ${CMAKE_CURRENT_BINARY_DIR}/cppoh_d
                   ${CMAKE_CURRENT_BINARY_DIR}/scene.db)

    SET(BERKELIUM_CHROME_ROOT_SEARCH_PATHS ${TOP_LEVEL}/externals/berkelium/chromium/chromium)
    ADD_SUBDIRECTORY(${TOP_LEVEL}/externals/berkelium ${CMAKE_CURRENT_BINARY_DIR}/chrome)

    SET(CMAKE_MODULE_PATH ${TOP_LEVEL}/externals/berkelium ${CMAKE_MODULE_PATH})
    SET(CHROME_ROOT ${BERKELIUM_CHROME_ROOT_SEARCH_PATHS})
    SET(CHROME_FOUND_QUIETLY TRUE)
    FIND_PACKAGE(Chrome)

    IF(CHROME_FOUND)
      SET(BERKELIUM_FOUND TRUE)
      SET(BERKELIUM_INCLUDE_DIRS ${TOP_LEVEL}/externals/berkelium/include)
      SET(BERKELIUM_TARGET_LIBRARIES libberkelium)
      SET(BERKELIUM_TARGET_LIBRARIES berkelium)
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


# FreeImage for image loading and saving.
IF(NOT FREEIMAGE_ROOT)
  IF(WIN32)
    SET(FREEIMAGE_ROOT ${PLATFORM_LIBS}/FreeImage)
  ELSEIF(APPLE)
    SET(FREEIMAGE_ROOT ${PLATFORM_LIBS})
  ELSE()
    SET(FREEIMAGE_ROOT ${PLATFORM_LIBS}/installed-freeimage/Dist)
  ENDIF()
ENDIF()
FIND_PACKAGE(FreeImage)

# NVTT for texture compression.
IF(NOT NVTT_ROOT)
  IF(WIN32)
    SET(NVTT_ROOT ${PLATFORM_LIBS}/nvtt)
  ELSEIF(APPLE)
    SET(NVTT_ROOT ${PLATFORM_LIBS}/installed-nvtt)
  ELSE()
    SET(NVTT_ROOT ${PLATFORM_LIBS}/installed-nvtt)
  ENDIF()
ENDIF()
FIND_PACKAGE(NVTT)

#dependency: cxxtest
SET(CXXTESTRoot ${TOP_LEVEL}/externals/cxxtest)

#dependency: Breakpad
IF(NOT BREAKPAD_ROOT)
  SET(BREAKPAD_ROOT ${PLATFORM_LIBS}/installed-breakpad)
ENDIF()
FIND_PACKAGE(Breakpad)
IF(BREAKPAD_FOUND)
  ADD_DEFINITIONS(-DHAVE_BREAKPAD)
ENDIF()

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
SET(LIBSQLITE_DIR ${TOP_LEVEL}/libsqlite)
SET(LIBOGRE_DIR ${TOP_LEVEL}/libogre)
SET(LIBSPACE_DIR ${TOP_LEVEL}/libspace)
SET(LIBOH_DIR ${TOP_LEVEL}/liboh)
SET(LIBMESH_DIR ${TOP_LEVEL}/libmesh)
SET(LIBPROXYOBJECT_DIR ${TOP_LEVEL}/libproxyobject)
SET(SPACE_DIR ${TOP_LEVEL}/space)
SET(CPPOH_DIR ${TOP_LEVEL}/cppoh)

#include locations
SET(LIBCORE_INCLUDE_DIR ${LIBCORE_DIR}/include)
SET(LIBSQLITE_INCLUDE_DIR ${LIBSQLITE_DIR}/include)
SET(LIBOGRE_INCLUDE_DIR ${LIBOGRE_DIR}/include)
SET(LIBSPACE_INCLUDE_DIR ${LIBSPACE_DIR}/include)
SET(LIBOH_INCLUDE_DIR ${LIBOH_DIR}/include)
SET(LIBMESH_INCLUDE_DIR ${LIBMESH_DIR}/include)
SET(LIBPROXYOBJECT_INCLUDE_DIR ${LIBPROXYOBJECT_DIR}/include)
SET(HTTP_PARSER_INCLUDE_DIR ${TOP_LEVEL}/externals/http-parser)

#source file locations
SET(LIBCORE_SOURCE_DIR ${LIBCORE_DIR}/src)
SET(LIBSQLITE_SOURCE_DIR ${LIBSQLITE_DIR}/src)
SET(LIBOGRE_SOURCE_DIR ${LIBOGRE_DIR}/src)
SET(LIBSPACE_SOURCE_DIR ${LIBSPACE_DIR}/src)
SET(LIBOH_SOURCE_DIR ${LIBOH_DIR}/src)
SET(LIBMESH_SOURCE_DIR ${LIBMESH_DIR}/src)
SET(LIBPROXYOBJECT_SOURCE_DIR ${LIBPROXYOBJECT_DIR}/src)
SET(CRASHREPORTER_SOURCE_DIR ${TOP_LEVEL}/crashreporter/src)
SET(SPACE_SOURCE_DIR ${SPACE_DIR}/src)
SET(CPPOH_SOURCE_DIR ${CPPOH_DIR}/src)
#source file location (cbr)
SET(SIMOH_SOURCE_DIR ${TOP_LEVEL}/simoh/src)
SET(CSEG_SOURCE_DIR ${TOP_LEVEL}/cseg/src)
SET(PINTO_SOURCE_DIR ${TOP_LEVEL}/pinto/src)
SET(CDN_NAMESERVER_SOURCE_DIR ${TOP_LEVEL}/cdn/src/nameserver)
SET(ANALYSIS_SOURCE_DIR ${TOP_LEVEL}/analysis/src)
SET(BENCH_SOURCE_DIR ${TOP_LEVEL}/bench/src)
SET(EMERSON_SOURCE_DIR ${TOP_LEVEL}/liboh/plugins/js/emerson)
SET(STREAM_ECHO_SOURCE_DIR ${TOP_LEVEL}/tools/stream_echo)
SET(MESH_TOOL_SOURCE_DIR ${TOP_LEVEL}/tools/meshtool)
SET(MESH_VIEW_SOURCE_DIR ${TOP_LEVEL}/tools/meshview)

SET(TEST_SOURCE_DIR ${TOP_LEVEL}/test)
SET(TEST_LIBCORE_SOURCE_DIR ${TEST_SOURCE_DIR}/libcore)
SET(TEST_LIBSQLITE_SOURCE_DIR ${TEST_SOURCE_DIR}/libsqlite)
SET(TEST_LIBOH_SOURCE_DIR ${TEST_SOURCE_DIR}/liboh)

#plugins locations
SET(LIBCORE_PLUGIN_DIR ${LIBCORE_DIR}/plugins)
SET(LIBMESH_PLUGIN_DIR ${LIBMESH_DIR}/plugins)
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
  ${LIBSQLITE_INCLUDE_DIR}
  ${LIBOGRE_INCLUDE_DIR}
  ${LIBSPACE_INCLUDE_DIR}
  ${LIBOH_INCLUDE_DIR}
  ${LIBMESH_INCLUDE_DIR}
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
IF(BERKELIUM_FOUND)
SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${BERKELIUM_INCLUDE_DIRS})
ENDIF()
IF(OpenCOLLADA_FOUND)
  SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${OpenCOLLADA_INCLUDE_DIRS})
ENDIF()
IF(bullet_FOUND)
  SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${bullet_INCLUDE_DIRS})
ENDIF()
IF(V8_FOUND)
  SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${V8_INCLUDE_DIRS})
ENDIF()
IF(FREEIMAGE_FOUND)
  SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${FREEIMAGE_INCLUDE_DIRS})
ENDIF()
IF(NVTT_FOUND)
  SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${NVTT_INCLUDE_DIRS})
ENDIF()
IF(HIREDIS_FOUND)
  SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${HIREDIS_INCLUDE_DIRS})
ENDIF()
IF(BREAKPAD_FOUND)
  SET(INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES} ${BREAKPAD_INCLUDE_DIRS})
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
SET(SirikataProtocolRoot ${TOP_LEVEL}/externals/sirikata-protocol)
# protocol files
# FIXME we should jsut change ADD_PBJ_TARGET to take full paths
SET(ProtocolBuffersSourcesCore
  ${SirikataProtocolRoot}/TimedMotionVector
  ${SirikataProtocolRoot}/TimedMotionQuaternion
)
SET(ProtocolBuffersSourcesPublic
  # Core protocol messages
  ${SirikataProtocolRoot}/Empty
  ${SirikataProtocolRoot}/Frame

  ${SirikataProtocolRoot}/TimeSync

  ${SirikataProtocolRoot}/ObjectMessage
  ${SirikataProtocolRoot}/Session
  ${SirikataProtocolRoot}/SSTHeader
  ${SirikataProtocolRoot}/Loc
  ${SirikataProtocolRoot}/Prox
)
SET(ProtocolBuffersSourcesInternal
  # Additional internal stuff
  ${ProtocolBuffersRoot}/Test
  ${ProtocolBuffersRoot}/JSMessage
  ${ProtocolBuffersRoot}/ServerProx
  ${ProtocolBuffersRoot}/MasterPinto
  ${ProtocolBuffersRoot}/CSeg
  ${ProtocolBuffersRoot}/ServerMessage
  ${ProtocolBuffersRoot}/Migration
  ${ProtocolBuffersRoot}/OSeg
  ${ProtocolBuffersRoot}/Forwarder
  ${ProtocolBuffersRoot}/CDN
  )

# Based on dependencies, generate arguments for protocol buffers generation
SET(PROTOCOLBUFFERS_GENS GENERATE_CPP)
SET(SCRIPTING_PROTOCOLBUFFERS_GENS ${PROTOCOLBUFFERS_GENS})


SET(PROTOCOLBUFFERS_CPP_HEADER "#include \"sirikata/core/util/Standard.hh\"\n")


ADD_PBJ_TARGET(
  ${ProtocolBuffersSourcesCore}
  PLUGINNAME "Protocol"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  GENERATED_CPP_FILES ProtocolCore_PBJ_CPP_FILES
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  EXPORTMACRO SIRIKATA_EXPORT
  GENERATE_CPP
)
ADD_PBJ_TARGET(
  ${ProtocolBuffersSourcesPublic}
  DEPENDS ${ProtocolCore_PBJ_CPP_FILES}
  PLUGINNAME "Protocol"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  GENERATED_CPP_FILES ProtocolPublic_PBJ_CPP_FILES
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  EXPORTMACRO SIRIKATA_EXPORT
  ${PROTOCOLBUFFERS_GENS}
)
ADD_PBJ_TARGET(
  ${ProtocolBuffersSourcesInternal}
  DEPENDS ${ProtocolCore_PBJ_CPP_FILES} ${ProtocolPublic_PBJ_CPP_FILES}
  PLUGINNAME "Protocol"
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  GENERATED_CPP_FILES ProtocolInternal_PBJ_CPP_FILES
  CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
  EXPORTMACRO SIRIKATA_EXPORT
  ${PROTOCOLBUFFERS_GENS}
)
SET(Protocol_PBJ_CPP_FILES ${ProtocolPublic_PBJ_CPP_FILES} ${ProtocolInternal_PBJ_CPP_FILES})
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


#IF(V8_FOUND)
  SET(JS_ProtocolBuffersSources JSMessage)
  ADD_PBJ_TARGET(${JS_ProtocolBuffersSources}
    INPUTDIR ${ProtocolBuffersRoot}
    PLUGINNAME "JS"
    IMPORTS ${ProtocolBuffersRoot}
    OUTPUTDIR ${SirikataProtocolDirectory}
    GENERATED_CPP_FILES JS_PBJ_CPP_FILES
    CPP_HEADER ${PROTOCOLBUFFERS_CPP_HEADER}
    ${PROTOCOLBUFFERS_GENS}
    )
#ENDIF(V8_FOUND)

# For scripting
SET(ScriptingProtocolBuffersSources
  Test
  Physics
  )

ADD_PBJ_TARGET(${ScriptingProtocolBuffersSources}
  INPUTDIR ${ProtocolBuffersRoot}
  IMPORTS ${ProtocolBuffersRoot}
  OUTPUTDIR ${SirikataProtocolDirectory}
  ${SCRIPTING_PROTOCOLBUFFERS_GENS}
)
SET(PROTOCOLBUFFERS_SCRIPTING_OUTPUTS ${PROTOCOLBUFFERS_ALL_OUTPUTS})


#binary files to be linked into sirikata
SET(BINARY_TO_CPP_SOURCE_FILE ${TOP_LEVEL}/externals/binarytocpp/binarytocpp.cpp)
SET(BINARY_TO_CPP_BINARY binarytocpp)
ADD_EXECUTABLE(${BINARY_TO_CPP_BINARY} ${BINARY_TO_CPP_SOURCE_FILE} )
SET_TARGET_PROPERTIES(${BINARY_TO_CPP_BINARY} PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
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


#IF(WIN32)
#ADD_DEFINITIONS(/FI\"${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Platform.hpp\")
#ELSE()
#ADD_DEFINITIONS(-include \"${LIBCORE_INCLUDE_DIR}/sirikata/core/util/Platform.hpp\")
#ENDIF()


SET(SIRIKATA_CORE_SOURCES
	${PROTOCOLBUFFERS_SCRIPTING_OUTPUTS}
        ${Protocol_PBJ_CPP_FILES}
        ${TRACE_PBJ_CPP_FILES}
	${LIBCORE_SOURCE_DIR}/transfer/TransferMediator.cpp
	${LIBCORE_SOURCE_DIR}/transfer/DiskCacheLayer.cpp
	${LIBCORE_SOURCE_DIR}/transfer/DiskManager.cpp
	${LIBCORE_SOURCE_DIR}/transfer/TransferHandlers.cpp
	${LIBCORE_SOURCE_DIR}/transfer/MeerkatTransferHandler.cpp
	${LIBCORE_SOURCE_DIR}/transfer/FileTransferHandler.cpp
	${LIBCORE_SOURCE_DIR}/transfer/TransferPool.cpp
	${LIBCORE_SOURCE_DIR}/transfer/HttpManager.cpp
        ${LIBCORE_SOURCE_DIR}/transfer/ResourceDownloadTask.cpp
	${LIBCORE_SOURCE_DIR}/task/DependencyTask.cpp
	${LIBCORE_SOURCE_DIR}/task/WorkQueue.cpp
	${LIBCORE_SOURCE_DIR}/task/Time.cpp
	${LIBCORE_SOURCE_DIR}/network/Asio.cpp
        ${LIBCORE_SOURCE_DIR}/odp/Defs.cpp
        ${LIBCORE_SOURCE_DIR}/odp/DelegateService.cpp
        ${LIBCORE_SOURCE_DIR}/odp/DelegatePort.cpp
   	${LIBCORE_SOURCE_DIR}/options/Options.cpp
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
        ${LIBCORE_SOURCE_DIR}/network/PBJDebug.cpp
        ${LIBCORE_SOURCE_DIR}/network/Frame.cpp
        ${LIBCORE_SOURCE_DIR}/service/Signal.cpp
        ${LIBCORE_SOURCE_DIR}/service/Breakpad.cpp
        ${LIBCORE_SOURCE_DIR}/service/Context.cpp
        ${LIBCORE_SOURCE_DIR}/service/Poller.cpp
        ${LIBCORE_SOURCE_DIR}/service/PollingService.cpp
        ${LIBCORE_SOURCE_DIR}/service/TimeProfiler.cpp
	${LIBCORE_SOURCE_DIR}/util/DynamicLibrary.cpp
	${LIBCORE_SOURCE_DIR}/util/SpaceID.cpp
	${LIBCORE_SOURCE_DIR}/util/ObjectReference.cpp
	${LIBCORE_SOURCE_DIR}/util/SpaceObjectReference.cpp
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
        ${LIBCORE_SOURCE_DIR}/util/Hash.cpp
        ${LIBCORE_SOURCE_DIR}/util/Timer.cpp
        ${LIBCORE_SOURCE_DIR}/util/RegionWeightCalculator.cpp
        ${LIBCORE_SOURCE_DIR}/util/Liveness.cpp
        ${LIBCORE_SOURCE_DIR}/trace/BatchedBuffer.cpp
        ${LIBCORE_SOURCE_DIR}/trace/Trace.cpp
        ${LIBCORE_SOURCE_DIR}/trace/TimeSeries.cpp
	${LIBCORE_SOURCE_DIR}/sync/TimeSyncServer.cpp
	${LIBCORE_SOURCE_DIR}/sync/TimeSyncClient.cpp
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

SET(LIBSQLITE_SOURCES
  ${LIBSQLITE_SOURCE_DIR}/SQLite.cpp
  )

SET(LIBOGRE_SOURCES
  ${LIBOGRE_SOURCE_DIR}/task/EventManager.cpp
  ${LIBOGRE_SOURCE_DIR}/task/Event.cpp
  ${LIBOGRE_SOURCE_DIR}/task/UniqueId.cpp
  ${LIBOGRE_SOURCE_DIR}/input/InputEvents.cpp
  ${LIBOGRE_SOURCE_DIR}/input/InputDevice.cpp
  ${LIBOGRE_SOURCE_DIR}/input/SDLInputDevice.cpp
  ${LIBOGRE_SOURCE_DIR}/input/SDLInputManager.cpp
  ${LIBOGRE_SOURCE_DIR}/resourceManager/AssetDownloadTask.cpp
  ${LIBOGRE_SOURCE_DIR}/resourceManager/CDNArchive.cpp
  ${LIBOGRE_SOURCE_DIR}/resourceManager/CDNArchiveFactory.cpp
  ${LIBOGRE_SOURCE_DIR}/resourceManager/CDNArchivePlugin.cpp
  ${LIBOGRE_SOURCE_DIR}/resourceManager/ReplacingDataStream.cpp
  ${LIBOGRE_SOURCE_DIR}/Camera.cpp
  ${LIBOGRE_SOURCE_DIR}/Entity.cpp
  ${LIBOGRE_SOURCE_DIR}/OgreRenderer.cpp
  ${LIBOGRE_SOURCE_DIR}/OverlayPosition.cpp
  ${LIBOGRE_SOURCE_DIR}/ViewportOverlay.cpp
  ${LIBOGRE_SOURCE_DIR}/WebView.cpp
  ${LIBOGRE_SOURCE_DIR}/WebViewManager.cpp
  )

SET(LIBSPACE_SOURCES
  ${LIBSPACE_SOURCE_DIR}/Authenticator.cpp
  ${LIBSPACE_SOURCE_DIR}/CoordinateSegmentation.cpp
  ${LIBSPACE_SOURCE_DIR}/LoadMonitor.cpp
  ${LIBSPACE_SOURCE_DIR}/ObjectSegmentation.cpp
  ${LIBSPACE_SOURCE_DIR}/OSegLookupTraceToken.cpp
  ${LIBSPACE_SOURCE_DIR}/ServerMessage.cpp
  ${LIBSPACE_SOURCE_DIR}/SpaceContext.cpp
  ${LIBSPACE_SOURCE_DIR}/SpaceNetwork.cpp
  ${LIBSPACE_SOURCE_DIR}/Trace.cpp
  ${LIBSPACE_SOURCE_DIR}/PintoServerQuerier.cpp
  ${LIBSPACE_SOURCE_DIR}/ProxSimulationTraits.cpp
  ${LIBSPACE_SOURCE_DIR}/LocationService.cpp
  )

SET(LIBMESH_SOURCES
  ${LIBMESH_SOURCE_DIR}/Meshdata.cpp
  ${LIBMESH_SOURCE_DIR}/AnyModelsSystem.cpp
  ${LIBMESH_SOURCE_DIR}/ModelsSystemFactory.cpp
  ${LIBMESH_SOURCE_DIR}/Filter.cpp
  ${LIBMESH_SOURCE_DIR}/CompositeFilter.cpp
  ${LIBMESH_SOURCE_DIR}/MeshSimplifier.cpp

  )

SET(LIBPROXYOBJECT_SOURCES
  ${Bullet_PBJ_CPP_FILES}
                  ${LIBPROXYOBJECT_SOURCE_DIR}/Invokable.cpp
                  ${LIBPROXYOBJECT_SOURCE_DIR}/ProxyObject.cpp
                  ${LIBPROXYOBJECT_SOURCE_DIR}/VWObject.cpp
                  ${LIBPROXYOBJECT_SOURCE_DIR}/SimulationFactory.cpp
                  ${LIBPROXYOBJECT_SOURCE_DIR}/OrphanLocUpdateManager.cpp
    )
SET(LIBOH_SOURCES
                  ${LIBOH_SOURCE_DIR}/SpaceNodeConnection.cpp
                  ${LIBOH_SOURCE_DIR}/SessionManager.cpp
                  ${LIBOH_SOURCE_DIR}/ObjectHost.cpp
                  ${LIBOH_SOURCE_DIR}/ObjectFactory.cpp
                  ${LIBOH_SOURCE_DIR}/HostedObject.cpp
                  ${LIBOH_SOURCE_DIR}/ObjectHostProxyManager.cpp
                  ${LIBOH_SOURCE_DIR}/ObjectScriptManagerFactory.cpp
                  ${LIBOH_SOURCE_DIR}/ObjectHostContext.cpp
                  ${LIBOH_SOURCE_DIR}/PerPresenceData.cpp
                  ${LIBOH_SOURCE_DIR}/Trace.cpp
                  ${LIBOH_SOURCE_DIR}/Storage.cpp
                  ${LIBOH_SOURCE_DIR}/PersistedObjectSet.cpp
                   )

SET(LIBMESH_PLUGIN_COLLADAMODELS_DIR ${LIBMESH_PLUGIN_DIR}/collada)
SET(LIBMESH_PLUGIN_COMMONFILTERS_DIR ${LIBMESH_PLUGIN_DIR}/common-filters)

SET(LIBPROXYOBJECT_PLUGIN_OGREGRAPHICS_DIR ${LIBPROXYOBJECT_PLUGIN_DIR}/ogre)


SET(CPPOH_SOURCES
  ${CPPOH_SOURCE_DIR}/main.cpp
  ${CPPOH_SOURCE_DIR}/Options.cpp
  ${CPPOH_SOURCE_DIR}/ObjectHost.cpp
  ${ObjectHostBinary_PBJ_CPP_FILES}
 )

# plugins sources

SET(LIBCORE_PLUGIN_SKELETON_DIR ${LIBCORE_PLUGIN_DIR}/skeleton)
SET(LIBCORE_PLUGIN_SKELETON_SOURCES ${LIBCORE_PLUGIN_SKELETON_DIR}/PluginInterface.cpp)

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

SET(LIBCORE_PLUGIN_WEIGHTCONST_DIR ${LIBCORE_PLUGIN_DIR}/weightconst)
SET(LIBCORE_PLUGIN_WEIGHTCONST_SOURCES
  ${LIBCORE_PLUGIN_WEIGHTCONST_DIR}/PluginInterface.cpp
  )

SET(LIBCORE_PLUGIN_GRAPHITE_DIR ${LIBCORE_PLUGIN_DIR}/graphite)
SET(LIBCORE_PLUGIN_GRAPHITE_SOURCES
  ${LIBCORE_PLUGIN_GRAPHITE_DIR}/PluginInterface.cpp
  ${LIBCORE_PLUGIN_GRAPHITE_DIR}/GraphiteTimeSeries.cpp
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
SET(LIBPROXYOBJECT_PLUGIN_OGREGRAPHICS_SOURCES
  ${Ogre_PBJ_CPP_FILES}
  ${SirikataOgreCoreZipCpp}
  ${GFX}/OgreSystem.cpp
  ${GFX}/OgreMeshRaytrace.cpp
  ${GFX}/CubeMap.cpp
  ${GFX}/OgreSystemMouseHandler.cpp
  ${GFX}/ProxyEntity.cpp
  ${GFX}/ProxyCamera.cpp
  ${GFX}/OgrePlugin.cpp
${GFX}/resourceManager/ResourceDownloadPlanner.cpp
${GFX}/resourceManager/DistanceDownloadPlanner.cpp
${GFX}/resourceManager/SAngleDownloadPlanner.cpp
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

SET(LIBSPACE_PLUGIN_NULL_DIR ${LIBSPACE_PLUGIN_DIR}/null)
SET(LIBSPACE_PLUGIN_NULL_SOURCES
  ${LIBSPACE_PLUGIN_NULL_DIR}/PluginInterface.cpp
  ${LIBSPACE_PLUGIN_NULL_DIR}/NullAuthenticator.cpp
)

SET(LIBSPACE_PLUGIN_LOCAL_DIR ${LIBSPACE_PLUGIN_DIR}/local)
SET(LIBSPACE_PLUGIN_LOCAL_SOURCES
  ${LIBSPACE_PLUGIN_LOCAL_DIR}/PluginInterface.cpp
  ${LIBSPACE_PLUGIN_LOCAL_DIR}/LocalObjectSegmentation.cpp
)

SET(LIBSPACE_PLUGIN_REDIS_DIR ${LIBSPACE_PLUGIN_DIR}/redis)
SET(LIBSPACE_PLUGIN_REDIS_SOURCES
  ${LIBSPACE_PLUGIN_REDIS_DIR}/PluginInterface.cpp
  ${LIBSPACE_PLUGIN_REDIS_DIR}/RedisObjectSegmentation.cpp
)

SET(LIBSPACE_PLUGIN_SQLITE_DIR ${LIBSPACE_PLUGIN_DIR}/sqlite)
SET(LIBSPACE_PLUGIN_SQLITE_SOURCES
  ${LIBSPACE_PLUGIN_SQLITE_DIR}/PluginInterface.cpp
  ${LIBSPACE_PLUGIN_SQLITE_DIR}/SQLiteAuthenticator.cpp
)

SET(LIBSPACE_PLUGIN_MASTER_PINTO_DIR ${LIBSPACE_PLUGIN_DIR}/master-pinto)
SET(LIBSPACE_PLUGIN_MASTER_PINTO_SOURCES
  ${LIBSPACE_PLUGIN_MASTER_PINTO_DIR}/PluginInterface.cpp
  ${LIBSPACE_PLUGIN_MASTER_PINTO_DIR}/MasterPintoServerQuerier.cpp
)

SET(LIBSPACE_PLUGIN_STANDARD_DIR ${LIBSPACE_PLUGIN_DIR}/standard)
SET(LIBSPACE_PLUGIN_STANDARD_SOURCES
  ${LIBSPACE_PLUGIN_STANDARD_DIR}/PluginInterface.cpp
  ${LIBSPACE_PLUGIN_STANDARD_DIR}/StandardLocationService.cpp
  ${LIBSPACE_PLUGIN_STANDARD_DIR}/AlwaysLocationUpdatePolicy.cpp
)

SET(LIBSPACE_PLUGIN_BULLETPHYSICS_DIR ${LIBSPACE_PLUGIN_DIR}/physics)
SET(LIBSPACE_PLUGIN_BULLETPHYSICS_SOURCES
  ${LIBSPACE_PLUGIN_BULLETPHYSICS_DIR}/PluginInterface.cpp
  ${LIBSPACE_PLUGIN_BULLETPHYSICS_DIR}/BulletPhysicsService.cpp
)

SET(CRASHREPORTER_SOURCES
  ${CRASHREPORTER_SOURCE_DIR}/main.cpp
)

SET(SPACE_SOURCES
  ${SPACE_SOURCE_DIR}/AggregateManager.cpp
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
  ${SPACE_SOURCE_DIR}/MigrationMonitor.cpp
  ${SPACE_SOURCE_DIR}/ObjectConnection.cpp
  ${SPACE_SOURCE_DIR}/ObjectHostConnectionManager.cpp
  ${SPACE_SOURCE_DIR}/Options.cpp
  ${SPACE_SOURCE_DIR}/OSegHasher.cpp
  ${SPACE_SOURCE_DIR}/OSegLookupQueue.cpp
  ${SPACE_SOURCE_DIR}/Proximity.cpp
  ${SPACE_SOURCE_DIR}/Server.cpp
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
  ${SIMOH_SOURCE_DIR}/HitPointScenario.cpp
  ${SIMOH_SOURCE_DIR}/AirTrafficControllerScenario.cpp
  ${SIMOH_SOURCE_DIR}/UnreliableHitPointScenario.cpp
  ${SIMOH_SOURCE_DIR}/LoadPacketTrace.cpp
  ${SIMOH_SOURCE_DIR}/DelugePairScenario.cpp
  ${SIMOH_SOURCE_DIR}/OSegScenario.cpp
  ${SIMOH_SOURCE_DIR}/ByteTransferScenario.cpp
  ${SIMOH_SOURCE_DIR}/NullScenario.cpp
  ${SIMOH_SOURCE_DIR}/ObjectHost.cpp
  ${SIMOH_SOURCE_DIR}/Options.cpp
  ${SIMOH_SOURCE_DIR}/main.cpp
  )

SET(GENPACK_SOURCES
  ${SIMOH_SOURCE_DIR}/RandomMotionPath.cpp
  ${SIMOH_SOURCE_DIR}/QuakeMotionPath.cpp
  ${SIMOH_SOURCE_DIR}/StaticMotionPath.cpp
  ${SIMOH_SOURCE_DIR}/Object.cpp
  ${SIMOH_SOURCE_DIR}/ObjectFactory.cpp
  ${SIMOH_SOURCE_DIR}/ObjectHost.cpp # for a few symbols
  ${SIMOH_SOURCE_DIR}/OSegTestMotionPath.cpp # for a few symbols
  ${SIMOH_SOURCE_DIR}/Options.cpp
  ${SIMOH_SOURCE_DIR}/GenPack.cpp
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


SET(EMERSON_SOURCES
  ${EMERSON_SOURCE_DIR}/EmersonLexer.cpp
  ${EMERSON_SOURCE_DIR}/EmersonParser.cpp
  ${EMERSON_SOURCE_DIR}/EmersonTree.cpp
  ${EMERSON_SOURCE_DIR}/EmersonUtil.cpp
  ${EMERSON_SOURCE_DIR}/Util.cpp
  )


SET(EMERSON_BIN_SOURCES
  ${EMERSON_SOURCES}
  ${EMERSON_SOURCE_DIR}/main.cpp
  )

SET(JSCOMPILE_SOURCES
   ${EMERSON_SOURCE_DIR}/jsutil/JSCompile.cpp
   )




SET(CDN_NAMESERVER_SOURCES
  ${CDN_NAMESERVER_SOURCE_DIR}/main.cpp
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

SET(STREAM_ECHO_SOURCES
  ${STREAM_ECHO_SOURCE_DIR}/StreamEcho.cpp
)

SET(MESH_TOOL_SOURCES
  ${MESH_TOOL_SOURCE_DIR}/main.cpp
)

SET(MESH_VIEW_SOURCES
  ${MESH_VIEW_SOURCE_DIR}/MeshView.cpp
)

SET(BENCH_SOURCES
  ${BENCH_SOURCE_DIR}/BenchmarkRunner.cpp
  ${BENCH_SOURCE_DIR}/TimerSpeedBenchmark.cpp
  ${BENCH_SOURCE_DIR}/TimerJitterBenchmark.cpp
  ${BENCH_SOURCE_DIR}/TimerMonotonicityBenchmark.cpp
  ${BENCH_SOURCE_DIR}/TCPSSTBenchmark.cpp
  ${BENCH_SOURCE_DIR}/main.cpp
)

#test source files
SET(CXXTESTSources
${TEST_LIBCORE_SOURCE_DIR}/TransferTest.hpp
${TEST_LIBCORE_SOURCE_DIR}/AnyTest.hpp
${TEST_LIBCORE_SOURCE_DIR}/AtomicTest.hpp
#${TEST_LIBCORE_SOURCE_DIR}/CacheLayerTest.hpp
${TEST_LIBCORE_SOURCE_DIR}/ExtrapolationTest.hpp
${TEST_LIBCORE_SOURCE_DIR}/FactoryTest.hpp
${TEST_LIBCORE_SOURCE_DIR}/FairQueueTest.hpp
${TEST_LIBCORE_SOURCE_DIR}/ListenerTest.hpp
${TEST_LIBCORE_SOURCE_DIR}/Matrix3Test.hpp
${TEST_LIBCORE_SOURCE_DIR}/OptionValueListTest.hpp
${TEST_LIBCORE_SOURCE_DIR}/OptionTest.hpp
${TEST_LIBCORE_SOURCE_DIR}/QuaternionTest.hpp
${TEST_LIBCORE_SOURCE_DIR}/TCPSSTCloseTest.hpp
${TEST_LIBCORE_SOURCE_DIR}/TCPSSTConnectTest.hpp
#${TEST_LIBCORE_SOURCE_DIR}/ThreadSafeQueueTest.hpp
${TEST_LIBCORE_SOURCE_DIR}/TR1Test.hpp
${TEST_LIBCORE_SOURCE_DIR}/Vector3Test.hpp
${TEST_LIBCORE_SOURCE_DIR}/BoundingBoxTest.hpp
 )
IF(SQLite3_FOUND)
  SET(CXXTESTSources
    ${CXXTESTSources}
    ${TEST_LIBSQLITE_SOURCE_DIR}/ThreadingTest.hpp
    ${TEST_LIBOH_SOURCE_DIR}/SQLiteStorageTest.hpp)
ENDIF()
ADD_CXXTEST_CPP_TARGET(CXXTEST ${CXXTESTSources}
	LIBRARYDIR ${CXXTESTRoot})

SET(TEST_SOURCES
  ${TEST_SOURCE_DIR}/Test.cpp
  ${CXXTEST_CPP_FILE}
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
    ${BREAKPAD_LIBRARIES}
    http-parser )

IF(BERKELIUM_FOUND)
  SET(BERKELIUM_DEFS HAVE_BERKELIUM)
  SET(BERKELIUM_CFLAGS -DHAVE_BERKELIUM)
ELSE()
  SET(BERKELIUM_DEFS)
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
IF(bullet_FOUND)
SET(FINAL_LINK_DIRS ${FINAL_LINK_DIRS} ${bullet_LIBRARY_DIRS})
ENDIF()
IF(SQLite3_FOUND)
SET(FINAL_LINK_DIRS ${FINAL_LINK_DIRS} ${SQLite3_LIBRARY_DIRS})
ENDIF()
IF(HIREDIS_FOUND)
SET(FINAL_LINK_DIRS ${FINAL_LINK_DIRS} ${HIREDIS_LIBRARY_DIRS})
ENDIF()
IF(BREAKPAD_FOUND)
  SET(FINAL_LINK_DIRS ${FINAL_LINK_DIRS} ${BREAKPAD_LIBRARY_DIRS})
ENDIF()

LINK_DIRECTORIES(${FINAL_LINK_DIRS})

SET(SIRIKATA_CORE_LIB sirikata-core)
SET(SIRIKATA_SQLITE_LIB sirikata-sqlite)
SET(SIRIKATA_OGRE_LIB sirikata-ogre)
SET(SIRIKATA_SPACE_LIB sirikata-space)
SET(SIRIKATA_OH_LIB sirikata-oh)
SET(SIRIKATA_MESH_LIB sirikata-mesh)
SET(SIRIKATA_PROXYOBJECT_LIB sirikata-proxyobject)
SET(CRASHREPORTER_BINARY crashreporter)
SET(SPACE_BINARY space)
SET(CPPOH_BINARY cppoh)
SET(TEST_BINARY tests)
SET(STREAM_ECHO_BINARY stream_echo)
SET(MESH_TOOL_BINARY meshtool)
SET(MESH_VIEW_BINARY meshview)
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
TARGET_LINK_LIBRARIES(${SIRIKATA_CORE_LIB} ${SIRIKATA_CORE_LIBRARIES} ${PROTOCOLBUFFERS_LIBRARIES} )
IF(sirikata_core_LDFLAGS OR sirikata_LDFLAGS)
  SET(sirikata_core_LDFLAGS ${sirikata_core_LDFLAGS} ${sirikata_LDFLAGS})
  SET_TARGET_PROPERTIES(${SIRIKATA_CORE_LIB} PROPERTIES LINK_FLAGS ${sirikata_core_LDFLAGS})
ENDIF()
SET_TARGET_PROPERTIES(${SIRIKATA_CORE_LIB} PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
#ADD_DEPENDENCIES(${SIRIKATA_CORE_LIB})

ADD_LIBRARY(${SIRIKATA_SPACE_LIB} SHARED ${LIBSPACE_SOURCES})
SET_TARGET_PROPERTIES(${SIRIKATA_SPACE_LIB} PROPERTIES ${COMPILE_DEFS_OPT} COMPILE_DEFINITIONS SIRIKATA_SPACE_BUILD)
ADD_DEPENDENCIES(${SIRIKATA_SPACE_LIB} ${SIRIKATA_MESH_LIB} ${SIRIKATA_PROXYOBJECT_LIB}  ${SIRIKATA_CORE_LIB})
IF(sirikata_core_LDFLAGS)# use the core ldflags--these guys act similarly to core for their respective modules
  SET_TARGET_PROPERTIES(${SIRIKATA_SPACE_LIB} PROPERTIES LINK_FLAGS ${sirikata_core_LDFLAGS})
ENDIF()
TARGET_LINK_LIBRARIES(${SIRIKATA_SPACE_LIB} ${SIRIKATA_MESH_LIB} ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB} ${PROTOCOLBUFFERS_LIBRARIES})
SET_TARGET_PROPERTIES(${SIRIKATA_SPACE_LIB} PROPERTIES ${SIRIKATA_VERSION_SETTINGS})


ADD_LIBRARY(${SIRIKATA_MESH_LIB} SHARED ${LIBMESH_SOURCES})
SET_TARGET_PROPERTIES(${SIRIKATA_MESH_LIB} PROPERTIES ${COMPILE_DEFS_OPT} COMPILE_DEFINITIONS SIRIKATA_MESH_BUILD)
ADD_DEPENDENCIES(${SIRIKATA_MESH_LIB} ${SIRIKATA_CORE_LIB})
IF(sirikata_core_LDFLAGS)# use the core ldflags--these guys act similarly to core for their respective modules
  SET_TARGET_PROPERTIES(${SIRIKATA_MESH_LIB} PROPERTIES LINK_FLAGS ${sirikata_core_LDFLAGS})
ENDIF()
TARGET_LINK_LIBRARIES(${SIRIKATA_MESH_LIB} ${SIRIKATA_CORE_LIB})
SET_TARGET_PROPERTIES(${SIRIKATA_MESH_LIB} PROPERTIES ${SIRIKATA_VERSION_SETTINGS})


ADD_LIBRARY(${SIRIKATA_PROXYOBJECT_LIB} SHARED ${LIBPROXYOBJECT_SOURCES})
SET_TARGET_PROPERTIES(${SIRIKATA_PROXYOBJECT_LIB} PROPERTIES ${COMPILE_DEFS_OPT} COMPILE_DEFINITIONS SIRIKATA_PROXYOBJECT_BUILD)
ADD_DEPENDENCIES(${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB} ${SIRIKATA_MESH_LIB})
IF(sirikata_core_LDFLAGS)# use the core ldflags--these guys act similarly to core for their respective modules
  SET_TARGET_PROPERTIES(${SIRIKATA_PROXYOBJECT_LIB} PROPERTIES LINK_FLAGS ${sirikata_core_LDFLAGS})
ENDIF()
TARGET_LINK_LIBRARIES(${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB} ${SIRIKATA_MESH_LIB})
SET_TARGET_PROPERTIES(${SIRIKATA_PROXYOBJECT_LIB} PROPERTIES ${SIRIKATA_VERSION_SETTINGS})


ADD_LIBRARY(${SIRIKATA_OH_LIB} SHARED ${LIBOH_SOURCES})
SET_TARGET_PROPERTIES(${SIRIKATA_OH_LIB} PROPERTIES ${COMPILE_DEFS_OPT} COMPILE_DEFINITIONS SIRIKATA_OH_BUILD)
ADD_DEPENDENCIES(${SIRIKATA_OH_LIB} ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB})
IF(sirikata_core_LDFLAGS)# use the core ldflags--these guys act similarly to core for their respective modules
  SET_TARGET_PROPERTIES(${SIRIKATA_OH_LIB} PROPERTIES LINK_FLAGS ${sirikata_core_LDFLAGS})
ENDIF()
TARGET_LINK_LIBRARIES(${SIRIKATA_OH_LIB} ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB} ${PROTOCOLBUFFERS_LIBRARIES})
SET_TARGET_PROPERTIES(${SIRIKATA_OH_LIB} PROPERTIES ${SIRIKATA_VERSION_SETTINGS})


#plugins
ADD_PLUGIN_TARGET(skeleton
                    SOURCES ${LIBCORE_PLUGIN_SKELETON_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )

IF(SQLite3_FOUND)

ADD_LIBRARY(${SIRIKATA_SQLITE_LIB} SHARED ${LIBSQLITE_SOURCES})
SET_TARGET_PROPERTIES(${SIRIKATA_SQLITE_LIB} PROPERTIES ${COMPILE_DEFS_OPT} COMPILE_DEFINITIONS SIRIKATA_SQLITE_BUILD)
ADD_DEPENDENCIES(${SIRIKATA_SQLITE_LIB} ${SIRIKATA_CORE_LIB})
IF(sirikata_core_LDFLAGS)# use the core ldflags--these guys act similarly to core for their respective modules
  SET_TARGET_PROPERTIES(${SIRIKATA_SQLITE_LIB} PROPERTIES LINK_FLAGS ${sirikata_core_LDFLAGS})
ENDIF()
TARGET_LINK_LIBRARIES(${SIRIKATA_SQLITE_LIB} ${SIRIKATA_CORE_LIB} ${SQLite3_LIBRARIES})
SET_TARGET_PROPERTIES(${SIRIKATA_SQLITE_LIB} PROPERTIES ${SIRIKATA_VERSION_SETTINGS})

ENDIF()

ADD_PLUGIN_TARGET(tcpsst
                    SOURCES ${LIBCORE_PLUGIN_TCPSST_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} tcpsst)

ADD_PLUGIN_TARGET(weight-exp
                    SOURCES ${LIBCORE_PLUGIN_WEIGHTEXP_SOURCES}
                    TARGET_CXXFLAGS ${GSL_CXX_FLAGS}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} weight-exp)

ADD_PLUGIN_TARGET(weight-sqr
                    SOURCES ${LIBCORE_PLUGIN_WEIGHTSQR_SOURCES}
                    TARGET_CXXFLAGS ${GSL_CXX_FLAGS}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB} ${GSL_LIBRARIES}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} weight-sqr)

ADD_PLUGIN_TARGET(weight-const
                    SOURCES ${LIBCORE_PLUGIN_WEIGHTCONST_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} weight-const)

ADD_PLUGIN_TARGET(graphite
                    SOURCES ${LIBCORE_PLUGIN_GRAPHITE_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} graphite)

ADD_PLUGIN_TARGET(servermap-tabular
                    SOURCES ${LIBCORE_PLUGIN_SERVERMAP_TABULAR_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} servermap-tabular)

ADD_PLUGIN_TARGET(core-local
                    SOURCES ${LIBCORE_PLUGIN_LOCAL_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} core-local)

ADD_PLUGIN_TARGET(space-craq
                    SOURCES ${LIBSPACE_PLUGIN_CRAQ_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} space-craq)

ADD_PLUGIN_TARGET(space-null
                    SOURCES ${LIBSPACE_PLUGIN_NULL_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} space-null)

ADD_PLUGIN_TARGET(space-local
                    SOURCES ${LIBSPACE_PLUGIN_LOCAL_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} space-local)

IF(HIREDIS_FOUND)
    ADD_PLUGIN_TARGET(space-redis
      SOURCES ${LIBSPACE_PLUGIN_REDIS_SOURCES}
      TARGET_LDFLAGS ${sirikata_LDFLAGS}
      LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB} ${HIREDIS_LIBRARIES}
      TARGET_LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB}
      VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
      )
    SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} space-redis)
ENDIF()

IF(SQLite3_FOUND)
  ADD_PLUGIN_TARGET(space-sqlite
                    SOURCES ${LIBSPACE_PLUGIN_SQLITE_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB} ${SIRIKATA_SQLITE_LIB} ${SQLite3_LIBRARIES}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB} ${SIRIKATA_SQLITE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
  SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} space-sqlite)
ENDIF()

ADD_PLUGIN_TARGET(space-master-pinto
                    SOURCES ${LIBSPACE_PLUGIN_MASTER_PINTO_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} space-master-pinto)

ADD_PLUGIN_TARGET(space-standard
                    SOURCES ${LIBSPACE_PLUGIN_STANDARD_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} space-standard)

IF(OGRE_FOUND AND sdl_FOUND )
  IF(WIN32)#not sure why this needs to happen again
    STRING(REPLACE "boost_system" "boost_regex" Boost_REGEX_LIBRARY ${Boost_SYSTEM_LIBRARY})
    STRING(REPLACE "boost_system" "boost_regex" Boost_REGEX_LIBRARY_DEBUG ${Boost_SYSTEM_LIBRARY_DEBUG})
    STRING(REPLACE "boost_system" "boost_regex" Boost_REGEX_LIBRARY_RELEASE ${Boost_SYSTEM_LIBRARY_RELEASE})
  ENDIF()

  ADD_LIBRARY(${SIRIKATA_OGRE_LIB} SHARED ${LIBOGRE_SOURCES})
  SET_TARGET_PROPERTIES(${SIRIKATA_OGRE_LIB} PROPERTIES ${COMPILE_DEFS_OPT})
  SET(SIRIKATA_OGRE_LIB_COMPILE_DEFS ${BERKELIUM_DEFS} SIRIKATA_OGRE_BUILD)
  SET_TARGET_PROPERTIES(${SIRIKATA_OGRE_LIB} PROPERTIES COMPILE_DEFINITIONS "${SIRIKATA_OGRE_LIB_COMPILE_DEFS}")
  ADD_DEPENDENCIES(${SIRIKATA_OGRE_LIB} ${SIRIKATA_CORE_LIB} ${SIRIKATA_MESH_LIB} ${SIRIKATA_PROXYOBJECT_LIB} ${BERKELIUM_TARGET_LIBRARIES})
  IF(sirikata_core_LDFLAGS)# use the core ldflags--these guys act similarly to core for their respective modules
    SET_TARGET_PROPERTIES(${SIRIKATA_OGRE_LIB} PROPERTIES LINK_FLAGS ${sirikata_core_LDFLAGS})
  ENDIF()
  STRING(REGEX REPLACE ";" " " SIRIKATA_OGRE_LIB_TARGET_LDFLAGS "${OGRE_LDFLAGS} ${sdl_LDFLAGS} ${BERKELIUM_LDFLAGS}") #FIXME does not deal with space-having ldflags
  SET_TARGET_PROPERTIES(${SIRIKATA_OGRE_LIB} PROPERTIES LINK_FLAGS ${SIRIKATA_OGRE_LIB_TARGET_LDFLAGS})
  TARGET_LINK_LIBRARIES(${SIRIKATA_OGRE_LIB}
    ${SIRIKATA_CORE_LIB} ${SIRIKATA_MESH_LIB} ${SIRIKATA_PROXYOBJECT_LIB}
    ${OGRE_LIBRARIES} ${sdl_LIBRARIES} ${BERKELIUM_LIBRARIES}
    debug ${Boost_REGEX_LIBRARY_DEBUG} optimized ${Boost_REGEX_LIBRARY_RELEASE}
    ${PROTOCOLBUFFERS_LIBRARIES}
    )
  SET_TARGET_PROPERTIES(${SIRIKATA_OGRE_LIB} PROPERTIES ${SIRIKATA_VERSION_SETTINGS})


  ADD_PLUGIN_TARGET(ogregraphics
                    SOURCES ${LIBPROXYOBJECT_PLUGIN_OGREGRAPHICS_SOURCES}
                    TARGET_CXXFLAGS ${OGRE_CFLAGS} ${sdl_CFLAGS} ${BERKELIUM_CFLAGS}
                    TARGET_LDFLAGS ${OGRE_LDFLAGS} ${sdl_LDFLAGS} ${BERKELIUM_LDFLAGS} ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_OGRE_LIB} ${SIRIKATA_MESH_LIB} ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB} ${BERKELIUM_TARGET_LIBRARIES}
                    LIBRARIES ${SIRIKATA_OGRE_LIB} ${SIRIKATA_MESH_LIB} ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_CORE_LIB}
                              ${OGRE_LIBRARIES} ${sdl_LIBRARIES} ${BERKELIUM_LIBRARIES}
                              debug ${Boost_REGEX_LIBRARY_DEBUG} optimized ${Boost_REGEX_LIBRARY_RELEASE}
                              ${PROTOCOLBUFFERS_LIBRARIES}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
  IF(NOT WIN32)
    ADD_DEPENDENCIES(${SIRIKATA_OGRE_LIB} berkelium)
    ADD_DEPENDENCIES(ogregraphics berkelium)
    IF(BERKELIUM_FOUND)
      TARGET_LINK_LIBRARIES(ogregraphics -Lchrome debug liblibberkelium_d${DYLIB_EXT} optimized liblibberkelium${DYLIB_EXT} )
      TARGET_LINK_LIBRARIES(${SIRIKATA_OGRE_LIB} -Lchrome debug liblibberkelium_d${DYLIB_EXT} optimized liblibberkelium${DYLIB_EXT} )
    ENDIF()
  ENDIF()
  #IF(ISDEBUG)
  #  SET(BERKELIUM_LDFLAGS -llibberkelium_d)
  #ELSE()
  #  SET(BERKELIUM_LDFLAGS -llibberkelium)
  #ENDIF()

SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} ogregraphics)
ENDIF(OGRE_FOUND AND sdl_FOUND)


IF(bullet_FOUND)
ADD_PLUGIN_TARGET(space-bulletphysics
                    SOURCES ${LIBSPACE_PLUGIN_BULLETPHYSICS_SOURCES}
		    TARGET_CXXFLAGS ${bullet_CFLAGS}
                    TARGET_LDFLAGS ${bullet_LDFLAGS} ${sirikata_LDFLAGS}
                    LIBRARIES ${SIRIKATA_CORE_LIB} ${bullet_LIBRARIES} ${SIRIKATA_SPACE_LIB}
                    TARGET_LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} space-bulletphysics)
ENDIF()





SET(LIBOH_PLUGIN_JS_DIR ${LIBOH_PLUGIN_DIR}/js)
SET(LIBOH_PLUGIN_JS_SOURCES
  ${JS_PBJ_CPP_FILES}
  ${LIBOH_PLUGIN_JS_DIR}/JSPlugin.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjectScriptManager.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjectScript.cpp
  ${LIBOH_PLUGIN_JS_DIR}/EmersonScript.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSUtil.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjects/JSVec3.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjects/JSQuaternion.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjects/JSUtilObj.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjects/JSObjectsUtils.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjects/JSHandler.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjects/JSTimer.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjects/JSGlobal.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSPattern.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSSerializer.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSVisibleManager.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjectStructs/JSContextStruct.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjectStructs/JSSystemStruct.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjectStructs/JSUtilStruct.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjectStructs/JSVisibleStruct.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjectStructs/JSPresenceStruct.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjectStructs/JSPositionListener.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjectStructs/JSSuspendable.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjectStructs/JSTimerStruct.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjectStructs/JSEventHandlerStruct.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjects/JSContext.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjects/JSVisible.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjects/JSPresence.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjects/JSInvokableObject.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjects/JSSystem.cpp
  ${LIBOH_PLUGIN_JS_DIR}/JSObjects/JSFunctionInvokable.cpp
  ${EMERSON_SOURCES}
  )
IF(V8_FOUND)
SET(BUILDING_JS_PLUGIN TRUE)
ADD_DEFINITIONS(-DHAVE_JS_SCRIPTING)
ADD_PLUGIN_TARGET(scripting-js
                    SOURCES ${LIBOH_PLUGIN_JS_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS} ${BERKELIUM_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB}
                    TARGET_CXXFLAGS  ${BERKELIUM_CFLAGS}
                    LIBRARIES ${V8_LIBRARIES} ${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB} ${ANTLR_LIBRARIES}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
                    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} scripting-js)
ENDIF()


SET(EMHEADLESS_SOURCES
  ${LIBOH_PLUGIN_JS_SOURCES}
  ${LIBOH_PLUGIN_JS_DIR}/headless/EMHeadless.cpp
  )
 


SET(LIBOH_PLUGIN_CSVFACTORY_DIR ${LIBOH_PLUGIN_DIR}/csvfactory)
SET(LIBOH_PLUGIN_CSVFACTORY_SOURCES
 ${LIBOH_PLUGIN_CSVFACTORY_DIR}/CSVObjectFactory.cpp
 ${LIBOH_PLUGIN_CSVFACTORY_DIR}/PluginInterface.cpp
    )
ADD_PLUGIN_TARGET(csvfactory
                    SOURCES ${LIBOH_PLUGIN_CSVFACTORY_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB}
                    LIBRARIES ${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} csvfactory)


SET(LIBOH_PLUGIN_FILE_DIR ${LIBOH_PLUGIN_DIR}/file)
SET(LIBOH_PLUGIN_FILE_SOURCES
 ${LIBOH_PLUGIN_FILE_DIR}/FileStorage.cpp
 ${LIBOH_PLUGIN_FILE_DIR}/PluginInterface.cpp
    )
ADD_PLUGIN_TARGET(oh-file
                    SOURCES ${LIBOH_PLUGIN_FILE_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB}
                    LIBRARIES ${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} oh-file)

IF(SQLite3_FOUND)
  SET(LIBOH_PLUGIN_SQLITE_DIR ${LIBOH_PLUGIN_DIR}/sqlite)
  SET(LIBOH_PLUGIN_SQLITE_SOURCES
    ${LIBOH_PLUGIN_SQLITE_DIR}/SQLiteStorage.cpp
    ${LIBOH_PLUGIN_SQLITE_DIR}/SQLitePersistedObjectSet.cpp
    ${LIBOH_PLUGIN_SQLITE_DIR}/SQLiteObjectFactory.cpp
    ${LIBOH_PLUGIN_SQLITE_DIR}/PluginInterface.cpp
    )
  ADD_PLUGIN_TARGET(oh-sqlite
    SOURCES ${LIBOH_PLUGIN_SQLITE_SOURCES}
    TARGET_LDFLAGS ${sirikata_LDFLAGS}
    TARGET_LIBRARIES ${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB} ${SIRIKATA_SQLITE_LIB}
    LIBRARIES ${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB} ${SIRIKATA_SQLITE_LIB}
    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
    )
  SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} oh-sqlite)
ENDIF()



# simplecamera plugin
SET(LIBOH_PLUGIN_SIMPLECAMERA_DIR ${LIBOH_PLUGIN_DIR}/simplecamera)
SET(LIBOH_PLUGIN_SIMPLECAMERA_SOURCES
 ${LIBOH_PLUGIN_SIMPLECAMERA_DIR}/PluginInterface.cpp
 ${LIBOH_PLUGIN_SIMPLECAMERA_DIR}/SimpleCameraObjectScript.cpp
 ${LIBOH_PLUGIN_SIMPLECAMERA_DIR}/SimpleCameraObjectScriptManager.cpp
  ${LIBOH_PLUGIN_SIMPLECAMERA_DIR}/InputBinding.cpp
  ${LIBOH_PLUGIN_SIMPLECAMERA_DIR}/InputBindingEvent.cpp
  ${LIBOH_PLUGIN_SIMPLECAMERA_DIR}/InputResponse.cpp
    )
ADD_PLUGIN_TARGET(simplecamera
                    SOURCES ${LIBOH_PLUGIN_SIMPLECAMERA_SOURCES}
                    TARGET_LDFLAGS ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB}
                    LIBRARIES ${SIRIKATA_OH_LIB} ${SIRIKATA_CORE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} simplecamera)



SET(LIBMESH_PLUGIN_COMMONFILTERS_DIR ${LIBMESH_PLUGIN_DIR}/common-filters)
SET(LIBMESH_PLUGIN_COMMONFILTERS_SOURCES
 ${LIBMESH_PLUGIN_COMMONFILTERS_DIR}/PluginInterface.cpp
 ${LIBMESH_PLUGIN_COMMONFILTERS_DIR}/LoadFilter.cpp
 ${LIBMESH_PLUGIN_COMMONFILTERS_DIR}/SaveFilter.cpp
 ${LIBMESH_PLUGIN_COMMONFILTERS_DIR}/PrintFilter.cpp
 ${LIBMESH_PLUGIN_COMMONFILTERS_DIR}/ComputeBoundsFilter.cpp
 ${LIBMESH_PLUGIN_COMMONFILTERS_DIR}/SquashPrimitivesFilter.cpp
 ${LIBMESH_PLUGIN_COMMONFILTERS_DIR}/SquashMaterialsFilter.cpp
 ${LIBMESH_PLUGIN_COMMONFILTERS_DIR}/SquashInstancedGeometryFilter.cpp
 ${LIBMESH_PLUGIN_COMMONFILTERS_DIR}/SingleMaterialGeometryFilter.cpp
 ${LIBMESH_PLUGIN_COMMONFILTERS_DIR}/TransformFilter.cpp
 ${LIBMESH_PLUGIN_COMMONFILTERS_DIR}/CenterFilter.cpp
 ${LIBMESH_PLUGIN_COMMONFILTERS_DIR}/TriangulateFilter.cpp
 )
ADD_PLUGIN_TARGET(common-filters
  SOURCES ${LIBMESH_PLUGIN_COMMONFILTERS_SOURCES}
  TARGET_LDFLAGS ${sirikata_LDFLAGS}
  TARGET_LIBRARIES ${SIRIKATA_MESH_LIB} ${SIRIKATA_CORE_LIB}
  LIBRARIES ${SIRIKATA_MESH_LIB} ${SIRIKATA_CORE_LIB}
  VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
  )
SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} common-filters)

IF(NVTT_FOUND AND FREEIMAGE_FOUND)
  SET(LIBMESH_PLUGIN_NVTT_DIR ${LIBMESH_PLUGIN_DIR}/nvtt)
  SET(LIBMESH_PLUGIN_NVTT_SOURCES
    ${LIBMESH_PLUGIN_NVTT_DIR}/PluginInterface.cpp
    ${LIBMESH_PLUGIN_NVTT_DIR}/FreeImage.cpp
    ${LIBMESH_PLUGIN_NVTT_DIR}/CompressTexturesFilter.cpp
    ${LIBMESH_PLUGIN_NVTT_DIR}/TextureAtlasFilter.cpp
    )
  ADD_PLUGIN_TARGET(nvtt
    SOURCES ${LIBMESH_PLUGIN_NVTT_SOURCES}
    TARGET_LIBRARIES ${SIRIKATA_MESH_LIB} ${SIRIKATA_CORE_LIB} ${NVTT_LIBRARIES} ${FREEIMAGE_LIBRARIES}
    LIBRARIES ${SIRIKATA_MESH_LIB} ${SIRIKATA_CORE_LIB} ${NVTT_LIBRARIES} ${FREEIMAGE_LIBRARIES}
    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
    )
  SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} nvtt)
ENDIF(NVTT_FOUND AND FREEIMAGE_FOUND)

SET(LIBMESH_PLUGIN_COLLADAMODELS_DIR ${LIBMESH_PLUGIN_DIR}/collada)
SET(LIBMESH_PLUGIN_COLLADAMODELS_SOURCES
 ${LIBMESH_PLUGIN_COLLADAMODELS_DIR}/ColladaPlugin.cpp
 ${LIBMESH_PLUGIN_COLLADAMODELS_DIR}/ColladaSystem.cpp
 ${LIBMESH_PLUGIN_COLLADAMODELS_DIR}/ColladaDocumentImporter.cpp
 ${LIBMESH_PLUGIN_COLLADAMODELS_DIR}/ColladaDocumentLoader.cpp
 ${LIBMESH_PLUGIN_COLLADAMODELS_DIR}/ColladaErrorHandler.cpp
 ${LIBMESH_PLUGIN_COLLADAMODELS_DIR}/ColladaAsset.cpp
 ${LIBMESH_PLUGIN_COLLADAMODELS_DIR}/ColladaDocument.cpp
 ${LIBMESH_PLUGIN_COLLADAMODELS_DIR}/MeshdataToCollada.cpp
    )

IF(OpenCOLLADA_FOUND)
  ADD_PLUGIN_TARGET(colladamodels
                    SOURCES ${LIBMESH_PLUGIN_COLLADAMODELS_SOURCES}
                    TARGET_CXXFLAGS ${OpenCOLLADA_CFLAGS}
                    TARGET_LDFLAGS ${OpenCOLLADA_LDFLAGS} ${sirikata_LDFLAGS}
                    TARGET_LIBRARIES ${SIRIKATA_MESH_LIB} ${SIRIKATA_CORE_LIB}
                    LIBRARIES ${OpenCOLLADA_LIBRARIES} ${SIRIKATA_MESH_LIB} ${SIRIKATA_CORE_LIB}
		    VERSION_INFO ${SIRIKATA_VERSION_SETTINGS}
		    )
  SET(PLUGIN_INSTALL_LIST ${PLUGIN_INSTALL_LIST} colladamodels)
ENDIF(OpenCOLLADA_FOUND)

#binaries
SET(BUILDING_CRASHREPORTER FALSE)
IF(CURL_FOUND AND NOT APPLE)
  SET(BUILDING_CRASHREPORTER TRUE)
  ADD_EXECUTABLE(${CRASHREPORTER_BINARY} ${CRASHREPORTER_SOURCES})
  SET_TARGET_PROPERTIES(${CRASHREPORTER_BINARY} PROPERTIES ${COMPILE_DEFS_OPT})
  SET_TARGET_PROPERTIES(${CRASHREPORTER_BINARY} PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
  TARGET_LINK_LIBRARIES(${CRASHREPORTER_BINARY}
    ${Boost_LIBRARIES}
    ${CURL_LIBRARIES}
    )
ENDIF()

ADD_EXECUTABLE(${TEST_BINARY} ${TEST_SOURCES})# EXCLUDE_FROM_ALL
SET_TARGET_PROPERTIES(${TEST_BINARY} PROPERTIES ${COMPILE_DEFS_OPT})
SET_TARGET_PROPERTIES(${TEST_BINARY} PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
ADD_DEPENDENCIES(${TEST_BINARY} tcpsst sqlite ${SIRIKATA_SQLITE_LIB} ${SIRIKATA_OH_LIB} oh-file oh-sqlite)

ADD_EXECUTABLE(${SPACE_BINARY} ${SPACE_SOURCES})
SET_TARGET_PROPERTIES(${SPACE_BINARY} PROPERTIES ${COMPILE_DEFS_OPT})
SET_TARGET_PROPERTIES(${SPACE_BINARY} PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
TARGET_LINK_LIBRARIES(${SPACE_BINARY}
        ${Boost_LIBRARIES}
        ${SIRIKATA_CORE_LIB}
        ${SIRIKATA_SPACE_LIB}
        ${PROTOCOLBUFFERS_LIBRARIES}
        )

ADD_EXECUTABLE(${CPPOH_BINARY} ${CPPOH_SOURCES})
SET_TARGET_PROPERTIES(${CPPOH_BINARY} PROPERTIES ${COMPILE_DEFS_OPT})
SET_TARGET_PROPERTIES(${CPPOH_BINARY} PROPERTIES ${SIRIKATA_VERSION_SETTINGS})

ADD_EXECUTABLE(simoh ${SIMOH_SOURCES})
SET_TARGET_PROPERTIES(simoh PROPERTIES ${COMPILE_DEFS_OPT})
SET_TARGET_PROPERTIES(simoh PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
TARGET_LINK_LIBRARIES(simoh
        ${Boost_LIBRARIES}
        ${SIRIKATA_CORE_LIB}
        ${SIRIKATA_OH_LIB}
        ${PROTOCOLBUFFERS_LIBRARIES}
        )

ADD_EXECUTABLE(genpack ${GENPACK_SOURCES})
SET_TARGET_PROPERTIES(genpack PROPERTIES ${COMPILE_DEFS_OPT})
SET_TARGET_PROPERTIES(genpack PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
TARGET_LINK_LIBRARIES(genpack
        ${Boost_LIBRARIES}
        ${SIRIKATA_CORE_LIB}
        ${SIRIKATA_OH_LIB}
        ${PROTOCOLBUFFERS_LIBRARIES}
        )

ADD_EXECUTABLE(cseg ${CSEG_SOURCES})
SET_TARGET_PROPERTIES(cseg PROPERTIES ${COMPILE_DEFS_OPT})
SET_TARGET_PROPERTIES(cseg PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
TARGET_LINK_LIBRARIES(cseg
        ${Boost_LIBRARIES}
        ${SIRIKATA_CORE_LIB}
        ${PROTOCOLBUFFERS_LIBRARIES}
        )



ADD_DEFINITIONS(-DEMERSON_COMPILE)
ADD_EXECUTABLE(emerson ${EMERSON_BIN_SOURCES})
SET_TARGET_PROPERTIES(emerson PROPERTIES ${COMPILE_DEFS_OPT})
SET_TARGET_PROPERTIES(emerson PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
TARGET_LINK_LIBRARIES(emerson
        ${Boost_LIBRARIES}
        ${ANTLR_LIBRARIES}
        )


IF(V8_FOUND)
  ADD_EXECUTABLE(jscompile ${JSCOMPILE_SOURCES})
  SET_TARGET_PROPERTIES(jscompile PROPERTIES ${COMPILE_DEFS_OPT})
  SET_TARGET_PROPERTIES(jscompile PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
  TARGET_LINK_LIBRARIES(jscompile
    ${Boost_LIBRARIES}
    ${V8_LIBRARIES}
    )
ENDIF(V8_FOUND)



IF(V8_FOUND)
  ADD_EXECUTABLE(emheadless ${EMHEADLESS_SOURCES})
  SET_TARGET_PROPERTIES(emheadless PROPERTIES ${COMPILE_DEFS_OPT})
  SET_TARGET_PROPERTIES(emheadless PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
  TARGET_LINK_LIBRARIES(emheadless
    ${Boost_LIBRARIES}
    ${V8_LIBRARIES}
    ${SIRIKATA_OH_LIB}
    ${SIRIKATA_CORE_LIB}
    ${ANTLR_LIBRARIES}
    )
ENDIF(V8_FOUND)





ADD_EXECUTABLE(pinto ${PINTO_SOURCES})
SET_TARGET_PROPERTIES(pinto PROPERTIES ${COMPILE_DEFS_OPT})
SET_TARGET_PROPERTIES(pinto PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
TARGET_LINK_LIBRARIES(pinto
        ${Boost_LIBRARIES}
        ${SIRIKATA_CORE_LIB}
        ${SIRIKATA_SPACE_LIB}
        ${PROTOCOLBUFFERS_LIBRARIES}
        )

ADD_EXECUTABLE(cdn_nameserver ${CDN_NAMESERVER_SOURCES})
SET_TARGET_PROPERTIES(cdn_nameserver PROPERTIES ${COMPILE_DEFS_OPT})
SET_TARGET_PROPERTIES(cdn_nameserver PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
TARGET_LINK_LIBRARIES(cdn_nameserver
        ${Boost_LIBRARIES}
        ${SIRIKATA_CORE_LIB}
        ${PROTOCOLBUFFERS_LIBRARIES}
        )

ADD_EXECUTABLE(analysis ${ANALYSIS_SOURCES})
SET_TARGET_PROPERTIES(analysis PROPERTIES ${COMPILE_DEFS_OPT})
SET_TARGET_PROPERTIES(analysis PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
TARGET_LINK_LIBRARIES(analysis
        ${Boost_LIBRARIES}
        ${SIRIKATA_CORE_LIB}
        ${PROTOCOLBUFFERS_LIBRARIES}
        #${GLUT_LIBRARIES}
        )

ADD_EXECUTABLE(${STREAM_ECHO_BINARY} ${STREAM_ECHO_SOURCES})
SET_TARGET_PROPERTIES(${STREAM_ECHO_BINARY} PROPERTIES ${COMPILE_DEFS_OPT})
SET_TARGET_PROPERTIES(${STREAM_ECHO_BINARY} PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
TARGET_LINK_LIBRARIES(${STREAM_ECHO_BINARY}
  ${Boost_LIBRARIES}
  ${SIRIKATA_CORE_LIB}
  ${PROTOCOLBUFFERS_LIBRARIES}
  )

ADD_EXECUTABLE(${MESH_TOOL_BINARY} ${MESH_TOOL_SOURCES})
SET_TARGET_PROPERTIES(${MESH_TOOL_BINARY} PROPERTIES ${COMPILE_DEFS_OPT})
SET_TARGET_PROPERTIES(${MESH_TOOL_BINARY} PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
TARGET_LINK_LIBRARIES(${MESH_TOOL_BINARY}
  ${Boost_LIBRARIES}
  ${SIRIKATA_CORE_LIB}
  ${SIRIKATA_MESH_LIB}
  )

IF(OGRE_FOUND AND sdl_FOUND)
  ADD_EXECUTABLE(${MESH_VIEW_BINARY} ${MESH_VIEW_SOURCES})
  SET_TARGET_PROPERTIES(${MESH_VIEW_BINARY} PROPERTIES ${COMPILE_DEFS_OPT})
  IF(BERKELIUM_FOUND)
    SET_TARGET_PROPERTIES(${MESH_VIEW_BINARY} PROPERTIES COMPILE_DEFINITIONS ${BERKELIUM_DEFS})
  ENDIF()
  SET_TARGET_PROPERTIES(${MESH_VIEW_BINARY} PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
  TARGET_LINK_LIBRARIES(${MESH_VIEW_BINARY}
    ${Boost_LIBRARIES}
    ${SIRIKATA_CORE_LIB}
    ${SIRIKATA_MESH_LIB}
    ${SIRIKATA_OGRE_LIB}
    )
ENDIF()

ADD_EXECUTABLE(${BENCH_BINARY} ${BENCH_SOURCES})
SET_TARGET_PROPERTIES(${BENCH_BINARY} PROPERTIES ${COMPILE_DEFS_OPT})
SET_TARGET_PROPERTIES(${BENCH_BINARY} PROPERTIES ${SIRIKATA_VERSION_SETTINGS})
TARGET_LINK_LIBRARIES(${BENCH_BINARY}
  ${Boost_LIBRARIES}
  ${SIRIKATA_CORE_LIB}
  ${PROTOCOLBUFFERS_LIBRARIES}
  )

IF(CHROME_FOUND)
  IF(APPLE)
  ELSE()
    MACRO(ADD_CHROME_SYMLINK_TARGET)
      PARSE_ARGUMENTS(CHROME_SYMLINKS_ARG "TARGETNAME;DEPENDS" "" ${ARGN})

      SET(CHROME_SYMLINKS_COMMAND_TARGET chrome_symlinks)
      IF(CHROME_SYMLINKS_ARG_TARGETNAME)
        SET(CHROME_SYMLINKS_COMMAND_TARGET ${CHROME_SYMLINKS_ARG_TARGETNAME})
      ENDIF()

      SET(CHROME_SYMLINKS_COMMAND
        ln -sf ${CHROMIUM_DATADIR}/locales &&
        ln -sf ${CHROMIUM_DATADIR}/resources.pak &&
        ln -sf ${CHROMIUM_DATADIR}/chrome.pak
        )

      SET(DEPENDS_ARGS)
      IF(CHROME_SYMLINKS_ARG_DEPENDS)
        SET(DEPENDS_ARGS DEPENDS ${CHROME_SYMLINKS_ARG_DEPENDS})
      ENDIF()

      ADD_CUSTOM_TARGET(${CHROME_SYMLINKS_COMMAND_TARGET} ALL
        COMMAND ${CHROME_SYMLINKS_COMMAND}
        ${DEPENDS_ARGS}
        )
    ENDMACRO()
  ENDIF()

  IF(APPLE)
    # FIXME GET_TARGET_PROPERTY doesn't seem to have anything that gives us this without a full path
    IF(ISDEBUG)
      SET(CPPOH_BINARY_FILENAME "${CPPOH_BINARY}_d")
    ELSE()
      SET(CPPOH_BINARY_FILENAME "${CPPOH_BINARY}")
    ENDIF()

    ADD_CHROME_APP(
      APP ${CPPOH_BINARY_FILENAME}
      DEPENDS ${CPPOH_BINARY} ${BERKELIUM_TARGET_LIBRARIES} ${BERKELIUM_TARGET_BINARIES}
      COPY
      ${CMAKE_CURRENT_BINARY_DIR}/chrome/berkelium
      ${CMAKE_CURRENT_BINARY_DIR}/scene.db
      )

  ELSEIF(ISLINUX)
    ADD_CHROME_SYMLINK_TARGET(TARGETNAME chrome_symlinks_for_sirikata DEPENDS ${BERKELIUM_TARGET_LIBRARIES})
  ENDIF()
ENDIF()

ADD_DEPENDENCIES(${TEST_BINARY} ${SIRIKATA_CORE_LIB} ${SIRIKATA_SQLITE_LIB} ${SIRIKATA_OH_LIB})
ADD_DEPENDENCIES(${SPACE_BINARY} ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB} )
ADD_DEPENDENCIES(${CPPOH_BINARY} ${SIRIKATA_CORE_LIB} ${SIRIKATA_MESH_LIB} ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_OH_LIB})

SET(ALL_BINARIES ${SPACE_BINARY} ${CPPOH_BINARY} ${TEST_BINARY} simoh genpack cseg pinto analysis ${STREAM_ECHO_BINARY} ${MESH_TOOL_BINARY} ${BENCH_BINARY})
IF(BUILDING_CRASHREPORTER)
  SET(ALL_BINARIES ${ALL_BINARIES} ${CRASHREPORTER_BINARY})
ENDIF()
IF(OGRE_FOUND AND sdl_FOUND)
  SET(ALL_BINARIES ${ALL_BINARIES} ${MESH_VIEW_BINARY})
ENDIF()
SET_TARGET_PROPERTIES(${ALL_BINARIES}
                      PROPERTIES
                      DEBUG_POSTFIX "_d" )
TARGET_LINK_LIBRARIES(${TEST_BINARY} ${SIRIKATA_CORE_LIB} ${SIRIKATA_SQLITE_LIB} ${SIRIKATA_OH_LIB}
                      ${TEST_LIBRARIES} ${PROTOCOLBUFFERS_LIBRARIES})
TARGET_LINK_LIBRARIES(${SPACE_BINARY} ${SIRIKATA_CORE_LIB} ${SIRIKATA_SPACE_LIB})
SET(CPPOH_LINK_LIBRARIES ${SIRIKATA_CORE_LIB} ${SIRIKATA_MESH_LIB} ${SIRIKATA_PROXYOBJECT_LIB} ${SIRIKATA_OH_LIB})
IF(OGRE_FOUND AND sdl_FOUND)
  SET(CPPOH_LINK_LIBRARIES ${CPPOH_LINK_LIBRARIES} ogregraphics)
ENDIF(OGRE_FOUND AND sdl_FOUND)
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
  SET(INPUT_SCENE_CSV ${DEMO_SCENES_DIR}/boxes.db)

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
SET(SIRIKATA_BINARY_INSTALL_TARGETS
          ${PBJ_BINARY}
          ${SIRIKATA_CORE_LIB}
          ${SIRIKATA_SQLITE_LIB}
          ${SIRIKATA_SPACE_LIB}
          ${SIRIKATA_MESH_LIB}
          ${SIRIKATA_PROXYOBJECT_LIB}
          ${SIRIKATA_OH_LIB}
          ${PLUGIN_INSTALL_LIST}
          ${MESH_TOOL_BINARY}
          ${SPACE_BINARY}
          ${CPPOH_BINARY}
          cseg
          emerson
          pinto
          )
IF(OGRE_FOUND AND sdl_FOUND)
  SET(SIRIKATA_BINARY_INSTALL_TARGETS
    ${SIRIKATA_BINARY_INSTALL_TARGETS}
    ${SIRIKATA_OGRE_LIB}
    ${MESH_VIEW_BINARY}
)
ENDIF()


INSTALL(TARGETS
  ${SIRIKATA_BINARY_INSTALL_TARGETS}
        PERMISSIONS ${EXEC_PERMS}
        RUNTIME
          DESTINATION bin
        LIBRARY
          DESTINATION lib
        ARCHIVE
          DESTINATION lib
)

IF(BUILDING_CRASHREPORTER)
  INSTALL(TARGETS
          ${CRASHREPORTER_BINARY}
        PERMISSIONS ${EXEC_PERMS}
        RUNTIME
          DESTINATION bin
        LIBRARY
          DESTINATION lib
        ARCHIVE
          DESTINATION lib
          )
ENDIF()

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

#install 
IF(THRIFT_FOUND AND LIBCASSANDRA_FOUND)
  IF(WIN32)
    INSTALL(DIRECTORY ${THRIFT_ROOT}/lib/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION bin/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.exe" EXCLUDE
            PATTERN "*.lib" EXCLUDE)
    INSTALL(DIRECTORY ${LIBCASSANDRA_ROOT}/lib/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION bin/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.exe" EXCLUDE
            PATTERN "*.lib" EXCLUDE)
  ELSE()
    INSTALL(DIRECTORY ${THRIFT_ROOT}/lib/
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION lib/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.a" EXCLUDE
            PATTERN "*.la" EXCLUDE
            PATTERN "*.exe" EXCLUDE)
    INSTALL(DIRECTORY ${LIBCASSANDRA_ROOT}/lib/
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
    IF(ISDEBUG)
      SET(OGRE_BIN_SUBFOLDER "debug/")
    ELSE()
      SET(OGRE_BIN_SUBFOLDER "release/")
    ENDIF()
    INSTALL(DIRECTORY ${OGRE_ROOT}/bin/${OGRE_BIN_SUBFOLDER}
            FILE_PERMISSIONS ${EXEC_PERMS}
            DESTINATION bin/
            #PATTERN "*.dll"<-- if only we could just INCLUDE a pattern
            PATTERN "*.exe" EXCLUDE)
  ELSEIF(APPLE)
    INSTALL(DIRECTORY ${OGRE_FRAMEWORKS}
      FILE_PERMISSIONS ${EXEC_PERMS}
      DESTINATION Frameworks/
    )
    INSTALL(DIRECTORY ${OGRE_LIBRARY_DIRS}/
      FILE_PERMISSIONS ${EXEC_PERMS}
      DESTINATION lib/
      PATTERN "*.framework" EXCLUDE
    )
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
  IF(APPLE)
    INSTALL(DIRECTORY ${SIRIKATA_OGRE_DATA_DIR}/
      DESTINATION share/ogre/data/
      PATTERN "*/jquery" EXCLUDE
      )
    #and jquery is a link, so we need to handle it carefully
    INSTALL(DIRECTORY ${TOP_LEVEL}/dependencies/jquery/
      DESTINATION share/ogre/data/jquery
      )
  ELSE()
    INSTALL(DIRECTORY ${TOP_LEVEL}/libproxyobject/plugins/ogre/data/
      DESTINATION share/ogre/data/
      )
  ENDIF()
ENDIF()

#install boost
IF(BOOST_ROOT)
  IF(WIN32)
    FOREACH(Boost_LIBRARY_DIR ${PLATFORM_LIBS}/boost_1_35_0/lib ${PLATFORM_LIBS}/boost_1_37_0/lib ${PLATFORM_LIBS}/boost_1_37_0/bin ${PLATFORM_LIBS}/boost_1_44_0/lib ${PLATFORM_LIBS}/installed-boost/lib)
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

#install gsl
IF(GSL_FOUND)
  IF(WIN32)
    INSTALL(DIRECTORY ${GSL_ROOT}/bin/
      FILE_PERMISSIONS ${EXEC_PERMS}
      DESTINATION bin/
      PATTERN "*.exe" EXCLUDE
      )
  ELSEIF(APPLE)
    INSTALL(DIRECTORY ${GSL_ROOT}/lib/
      FILE_PERMISSIONS ${EXEC_PERMS}
      DESTINATION lib/
      PATTERN "*.exe" EXCLUDE
      )
  ELSE()
    # System provided dependency, nothing to install.
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


    # The install process just runs a script that copies all the files into the right place.
    SET(CPPOH_DISTRIBUTABLE_DEBUG_OPTS)
    IF(ISDEBUG)
      SET(CPPOH_DISTRIBUTABLE_DEBUG_OPTS "debug")
    ENDIF()
    INSTALL(CODE "EXECUTE_PROCESS(COMMAND ./mac-app-install.sh cppoh ${CMAKE_INSTALL_PREFIX} ${CPPOH_DISTRIBUTABLE_DEBUG_OPTS})")
ELSE()
IF(NOT WIN32)
ADD_CUSTOM_TARGET(berkelium.bin ALL COMMAND ln -sf chrome/berkelium && ln -sf chrome/berkelium berkelium.bin)
ENDIF()
ENDIF()
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

# If V8 was found, we're buildin
IF(BUILDING_JS_PLUGIN)
  # data for ogre, including web views
  INSTALL(DIRECTORY ${TOP_LEVEL}/liboh/plugins/js/scripts/
    DESTINATION share/js/scripts/
    )
ENDIF()

INSTALL(DIRECTORY
          # note: the trailing slashes avoid prepending the directory name
          # itself to the installed files
          ${LIBCORE_INCLUDE_DIR}/
          ${LIBSPACE_INCLUDE_DIR}/
          ${LIBOH_INCLUDE_DIR}/
          ${LIBMESH_INCLUDE_DIR}/
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
