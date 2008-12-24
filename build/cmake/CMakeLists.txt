#Copyright (c) 2008, 
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
#    * Neither the name of the <ORGANIZATION> nor the names of its contributors
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
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug FORCE CACHE STRING
      "Build types include: Debug Release None RelWithDebInfo."
      )
ENDIF()

#project global flags

PROJECT(Iridescence CXX)

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


#dependencies
IF(NOT PLATFORM_LIBS)
  SET(PLATFORM_LIBS ${TOP_LEVEL}/../dependencies/)
  GET_FILENAME_COMPONENT(PLATFORM_LIBS ${PLATFORM_LIBS} ABSOLUTE)
  IF(NOT PLATFORM_LIBS)
    MESSAGE(STATUS "Could not find dependencies directory ${TOP_LEVEL}/../dependencies/")
  ENDIF()
ENDIF()

#libraries

#boost > 1.35
IF(NOT BOOST_ROOT)
  IF(WIN32)
    SET(BOOST_ROOT ${PLATFORM_LIBS}/boost_1_35_0)
  ENDIF()
  IF(APPLE)
    SET(BOOST_ROOT ${PLATFORM_LIBS}/boost_1_35_0)
  ENDIF()
ENDIF()

SET(Boost_FIND_REQUIRED TRUE)
FIND_PACKAGE(Boost COMPONENTS system signals program_options thread)
VERIFY_VERSION(Boost 1 35 0)
SET(Boost_LIBRARIES
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_SIGNALS_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY})

#cxx flags
INCLUDE_DIRECTORIES(BEFORE ${SOURCE_DIR} ${TOP_LEVEL} ${TOP_LEVEL}/include ${Boost_INCLUDE_DIRS})

#source files
SET(SOURCE_DIR ${TOP_LEVEL}/src)
SET(IRIDESCENCE_SOURCES ${SOURCE_DIR}/task/EventManager.cpp)
SET(TEST_SOURCES ${SOURCE_DIR}/task/EventManager.cpp)



#linker flags
SET(CMAKE_DEBUG_POSTFIX "_d")
SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} /usr/local)
SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /usr/local)
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
SET(IRIDESCENCE_LIBRARIES
    ${Boost_LIBRARIES})


#binaries
IF(CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
  ADD_EXECUTABLE(iridescence ${IRIDESCENCE_SOURCES})
  TARGET_LINK_LIBRARIES(iridescence ${IRIDESCENCE_LIBRARIES})
  IF(Iridescence_LDFLAGS)
    SET_TARGET_PROPERTIES(iridescence PROPERTIES LINK_FLAGS ${Iridescence_LDFLAGS})
  ENDIF()

  ADD_EXECUTABLE(test EXCLUDE_FROM_ALL ${TEST_SOURCES})
  TARGET_LINK_LIBRARIES(test ${TEST_LIBRARIES})
  IF(Iridescence_LDFLAGS)
    SET_TARGET_PROPERTIES(test PROPERTIES LINK_FLAGS ${Iridescence_LDFLAGS})
  ENDIF()

ELSE()
  ADD_EXECUTABLE(iridescence_release ${IRIDESCENCE_SOURCES})
  TARGET_LINK_LIBRARIES(iridescence_release ${IRIDESCENCE_LIBRARIES})
  IF(Iridescence_LDFLAGS)
    SET_TARGET_PROPERTIES(iridescence_release PROPERTIES LINK_FLAGS ${Iridescence_LDFLAGS})
  ENDIF()

  ADD_EXECUTABLE(test_release EXCLUDE_FROM_ALL ${TEST_SOURCES})
  TARGET_LINK_LIBRARIES(test_release ${TEST_LIBRARIES})
  IF(Iridescence_LDFLAGS)
    SET_TARGET_PROPERTIES(test_release PROPERTIES LINK_FLAGS ${Iridescence_LDFLAGS})
  ENDIF()
ENDIF()
