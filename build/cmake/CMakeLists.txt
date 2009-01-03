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
  SET(CMAKE_BUILD_TYPE Default CACHE STRING
      "Build types include: Default Debug Release None RelWithDebInfo." FORCE
      )
ENDIF()
#-fno-rtti
SET(CMAKE_CXX_FLAGS "-Wall -Wno-unused-function -Wno-unused-variable -fvisibility=hidden")
#-fno-rtti
SET(CMAKE_CXX_FLAGS_DEFAULT "-O0 -g2 -Wall -Wno-unused-function -Wno-unused-variable -fvisibility=hidden" CACHE STRING
    "Built cxxflags with default settings."
    FORCE )
SET( CMAKE_C_FLAGS_DEFAULT "-g2 -Wall -Wno-unused-function -Wno-unused-variable -fvisibility=hidden" CACHE STRING
    "Built cflags with default settings."
    FORCE )
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


MACRO(CAR var)
  SET(${var} ${ARGV1})
ENDMACRO(CAR)
MACRO(CDR var junk)
  SET(${var} ${ARGN})
ENDMACRO(CDR)

MACRO(LIST_CONTAINS var value)
  SET(${var})
  FOREACH (value2 ${ARGN})
    IF (${value} STREQUAL ${value2})
      SET(${var} TRUE)
    ENDIF (${value} STREQUAL ${value2})
  ENDFOREACH (value2)
ENDMACRO(LIST_CONTAINS)

MACRO(PARSE_ARGUMENTS prefix arg_names option_names)
  SET(DEFAULT_ARGS)
  FOREACH(arg_name ${arg_names})
    SET(${prefix}_${arg_name})
  ENDFOREACH(arg_name)
  FOREACH(option ${option_names})
    SET(${prefix}_${option} FALSE)
  ENDFOREACH(option)

  SET(current_arg_name DEFAULT_ARGS)
  SET(current_arg_list)
  FOREACH(arg ${ARGN})
    LIST_CONTAINS(is_arg_name ${arg} ${arg_names})
    IF (is_arg_name)
      SET(${prefix}_${current_arg_name} ${current_arg_list})
      SET(current_arg_name ${arg})
      SET(current_arg_list)
    ELSE (is_arg_name)
      LIST_CONTAINS(is_option ${arg} ${option_names})
      IF (is_option)
	SET(${prefix}_${arg} TRUE)
      ELSE (is_option)
	SET(current_arg_list ${current_arg_list} ${arg})
      ENDIF (is_option)
    ENDIF (is_arg_name)
  ENDFOREACH(arg)
  SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO(PARSE_ARGUMENTS)

MACRO(ADD_CXXTEST_CPP_TARGET)
  PARSE_ARGUMENTS(CXXTEST "DEPENDS;OUTPUTDIR;LIBRARYDIR" "" ${ARGN})
  CAR(CXXTEST_NAME ${CXXTEST_DEFAULT_ARGS})
  CDR(CXXTEST_FILES ${CXXTEST_DEFAULT_ARGS})

  SET(CXXTEST_EXEC_TARGET ${CXXTEST_NAME})

  SET(CXXTEST_TEST_FILES)
  SET(CXXTEST_H_FILES)

  SET(CXXTEST_OPTIONS --runner=StdioPrinter)
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
      MESSAGE("!!! Cannot locate python or perl -- tests will not be compiled.")
    ENDIF()
  ENDIF()
  SET(CXXTEST_CPP_FILE ${CXXTEST_OUTPUTDIR}/test.cc)
  FOREACH(FILE ${CXXTEST_FILES})
    SET(CXXTEST_H_FILE ${FILE})

    SET(CXXTEST_TEST_FILES ${CXXTEST_TEST_FILES} ${CXXTEST_H_FILE})
  ENDFOREACH(FILE)
  IF (CXXTEST_COMPILER)
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

SET(CXXTESTSources
  ${TOP_LEVEL}/test/AtomicTest.hpp
  ${TOP_LEVEL}/test/Vector3Test.hpp 
  ${TOP_LEVEL}/test/EventTest.hpp )
SET(CXXTESTRoot ${TOP_LEVEL}/externals/cxxtest)

ADD_CXXTEST_CPP_TARGET(CXXTEST ${CXXTESTSources}
	LIBRARYDIR ${CXXTESTRoot}
	OUTPUTDIR ${CMAKE_CURRENT_BINARY_DIR})

SET(CxxTestLibSources ${CXXTEST_CPP_FILE})

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
    SET(BOOST_ROOT ${PLATFORM_LIBS})
  ENDIF()
  IF(APPLE)
    SET(BOOST_ROOT ${PLATFORM_LIBS})
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
INCLUDE_DIRECTORIES(BEFORE ${SOURCE_DIR} ${TOP_LEVEL} ${TOP_LEVEL}/src ${TOP_LEVEL}/include ${Boost_INCLUDE_DIRS} ${CXXTESTRoot})

#source files
SET(SOURCE_DIR ${TOP_LEVEL}/src)
SET(IRIDESCENCE_SOURCES
	${SOURCE_DIR}/task/EventManager.cpp
	${SOURCE_DIR}/task/Event.cpp
	${SOURCE_DIR}/task/UniqueId.cpp
	${SOURCE_DIR}/task/Time.cpp
	${SOURCE_DIR}/util/internal_sha2.cpp
	${SOURCE_DIR}/util/Sha256.cpp
	${SOURCE_DIR}/util/UUID.cpp
)
SET(TEST_SOURCES ${CxxTestLibSources} ) #${TOP_LEVEL}/test/main.cpp



#linker flags
SET(CMAKE_DEBUG_POSTFIX "_d")
SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} /usr/local)
SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /usr/local)
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
SET(IRIDESCENCE_LIBRARIES
    ${Boost_LIBRARIES})


#libraries
ADD_LIBRARY(iridium STATIC ${IRIDESCENCE_SOURCES})
TARGET_LINK_LIBRARIES(iridium ${IRIDESCENCE_LIBRARIES})
IF(Iridescence_LDFLAGS)
  SET_TARGET_PROPERTIES(iridium PROPERTIES LINK_FLAGS ${Iridescence_LDFLAGS})
ENDIF()


#binaries
IF(CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
  ADD_EXECUTABLE(test_debug EXCLUDE_FROM_ALL ${TEST_SOURCES})
  TARGET_LINK_LIBRARIES(test_debug iridium ${TEST_LIBRARIES})
  IF(Iridescence_LDFLAGS)
    SET_TARGET_PROPERTIES(test_debug PROPERTIES LINK_FLAGS ${Iridescence_LDFLAGS})
  ENDIF()

  SET(TEST_BINARY test_debug)
ELSE()
  ADD_EXECUTABLE(test_release EXCLUDE_FROM_ALL ${TEST_SOURCES})
  TARGET_LINK_LIBRARIES(test_release iridium ${TEST_LIBRARIES})
  IF(Iridescence_LDFLAGS)
    SET_TARGET_PROPERTIES(test_release PROPERTIES LINK_FLAGS ${Iridescence_LDFLAGS})
  ENDIF()

  SET(TEST_BINARY test_release)
ENDIF()

#ENABLE_TESTING()
#ADD_TEST(test ${EXECUTABLE_OUTPUT_PATH}/${TEST_BINARY}
#  DEPENDS ${EXECUTABLE_OUTPUT_PATH}/${TEST_BINARY}) 

ADD_CUSTOM_TARGET(test
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${TEST_BINARY}
	COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${TEST_BINARY})

