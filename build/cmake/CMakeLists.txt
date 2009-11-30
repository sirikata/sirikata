#CMakeLists.txt
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
#    * Neither the name of cbr nor the names of its contributors
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
      "Build types include: Default Debug Release None RelWithDebInfo Profile Coverage." FORCE
      )
ENDIF()

SET(CBR_DEFAULT_COMPILE_FLAGS "-Wall -Wno-unused-function -Wno-unused-variable -fvisibility=hidden")
SET(CMAKE_CXX_FLAGS ${CBR_DEFAULT_COMPILE_FLAGS})

# Default build target
SET(CMAKE_CXX_FLAGS_DEFAULT "-O0 -g2 ${CBR_DEFAULT_COMPILE_FLAGS}" CACHE STRING
    "Built cxxflags with default settings."
    FORCE )
SET( CMAKE_C_FLAGS_DEFAULT "-g2 ${CBR_DEFAULT_COMPILE_FLAGS}" CACHE STRING
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

# Profile build target
SET(CMAKE_CXX_FLAGS_PROFILE "-g2 -O3 -pg ${CBR_DEFAULT_COMPILE_FLAGS}" CACHE STRING
    "Built cxxflags with default settings."
    FORCE )
SET( CMAKE_C_FLAGS_PROFILE "-g2 -O3 -pg ${CBR_DEFAULT_COMPILE_FLAGS}" CACHE STRING
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
SET(CMAKE_CXX_FLAGS_COVERAGE "-g2 -fprofile-arcs -ftest-coverage ${CBR_DEFAULT_COMPILE_FLAGS}" CACHE STRING
    "Built cxxflags with default settings."
    FORCE )
SET( CMAKE_C_FLAGS_COVERAGE "-g2 -fprofile-arcs -ftest-coverage ${CBR_DEFAULT_COMPILE_FLAGS}" CACHE STRING
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

PROJECT(cbr C CXX)

IF(NOT TOP_LEVEL)
  SET(TOP_LEVEL ${CMAKE_CURRENT_SOURCE_DIR}/../..)
ENDIF()
GET_FILENAME_COMPONENT(TOP_LEVEL ${TOP_LEVEL} ABSOLUTE)

IF(NOT DEPENDENCIES_ROOT)
  SET(DEPENDENCIES_ROOT ${TOP_LEVEL}/dependencies)
ENDIF()

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


#libraries

# first we find/verify sirikata and installed-sirikata, which will
# provide a lot of useful tools
SET(SIRIKATA_CODE_ROOT)
IF(EXISTS ${DEPENDENCIES_ROOT}/sirikata)
  SET(SIRIKATA_CODE_ROOT ${DEPENDENCIES_ROOT}/sirikata)
ENDIF()

SET(SIRIKATA_INSTALL_ROOT)
IF(EXISTS ${DEPENDENCIES_ROOT}/installed-sirikata)
  SET(SIRIKATA_INSTALL_ROOT ${DEPENDENCIES_ROOT}/installed-sirikata)
ENDIF()


#dependency: sirikata
FIND_PACKAGE(Sirikata REQUIRED)


# add sirikata's cmake modules
IF(Sirikata_CMAKE_MODULES)
  SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${Sirikata_CMAKE_MODULES})
ENDIF()

#dependency: protocolbuffers
IF(NOT PROTOCOLBUFFERS_ROOT)
  IF(Sirikata_PROTOBUFS_ROOT)
    SET(PROTOCOLBUFFERS_ROOT ${Sirikata_PROTOBUFS_ROOT})
  ENDIF()
ENDIF()
FIND_PACKAGE(ProtocolBuffers)
IF(NOT PROTOCOLBUFFERS_FOUND)
  MESSAGE(FATAL_ERROR "Cannot find Protocol Buffers")
ENDIF()


#dependency: boost > 1.35
IF(NOT BOOST_ROOT)
  IF(Sirikata_BOOST_ROOT)
    SET(BOOST_ROOT ${Sirikata_BOOST_ROOT})
  ENDIF()
ENDIF()
SET(Boost_FIND_REQUIRED TRUE)
IF(WIN32)
  FIND_PACKAGE(Boost COMPONENTS system thread date_time program_options)
ELSE()
  FIND_PACKAGE(Boost COMPONENTS system)
  STRING(REPLACE "boost_system" "boost_thread" Boost_THREAD_LIBRARY ${Boost_SYSTEM_LIBRARY})
  STRING(REPLACE "boost_system" "boost_regex" Boost_REGEX_LIBRARY ${Boost_SYSTEM_LIBRARY})
  STRING(REPLACE "boost_system" "boost_date_time" Boost_DATE_TIME_LIBRARY ${Boost_SYSTEM_LIBRARY})
  STRING(REPLACE "boost_system" "boost_program_options" Boost_PROGRAM_OPTIONS_LIBRARY ${Boost_SYSTEM_LIBRARY})
ENDIF()
VERIFY_VERSION(Boost 1 35 0)


#dependency: glut
FIND_PACKAGE(GLUT)

FIND_PACKAGE(GSL)
IF(NOT GLUT_FOUND)
  MESSAGE(SEND_ERROR "Couldn't find GLUT.")
ENDIF(NOT GLUT_FOUND)

#dependency: qt
IF(EXISTS ${DEPENDENCIES_ROOT}/qt/qt)
  SET(Qt4_ROOT ${DEPENDENCIES_ROOT}/qt/qt)
ELSE()
  SET(Qt4_ROOT ${DEPENDENCIES_ROOT})
ENDIF()
FIND_PACKAGE(Qt4 REQUIRED)
ADD_DEFINITIONS(${QT_DEFINITIONS})

#dependency: sst
IF(EXISTS ${DEPENDENCIES_ROOT}/installed-sst)
  SET(SST_ROOT ${DEPENDENCIES_ROOT}/installed-sst)
ELSE()
  SET(SST_ROOT ${DEPENDENCIES_ROOT})
ENDIF()
FIND_PACKAGE(SST REQUIRED)

#dependency: prox
IF(EXISTS ${DEPENDENCIES_ROOT}/installed-prox)
  SET(PROX_ROOT ${DEPENDENCIES_ROOT}/installed-prox)
ELSE()
  SET(PROX_ROOT ${DEPENDENCIES_ROOT})
ENDIF()
FIND_PACKAGE(Prox REQUIRED)

#dependency: gcrypt
FIND_PACKAGE(GCrypt REQUIRED)

# get some useful macros from sirikata
SET(PBJ_RUNABLE ${Sirikata_PBJ}) # FIXME this should be an option
INCLUDE(AddPBJTarget)


# -- User defined variables that control behavior at compile time

SET(CBR_TIMESTAMP_PACKETS TRUE
  CACHE BOOL "If enabled, detailed timestamps are recorded for each packet, enabling high granularity latency breakdowns."
)
IF(CBR_TIMESTAMP_PACKETS)
  ADD_DEFINITIONS(-DCBR_TIMESTAMP_PACKETS)
ENDIF()

#source file location
SET(CBR_SOURCE_DIR ${TOP_LEVEL}/src)
#protocol file location
SET(CBR_PROTOCOL_DIR ${TOP_LEVEL}/protocol)

#cxx flags
INCLUDE_DIRECTORIES(BEFORE
        ${CBR_SOURCE_DIR}
        ${CBR_SOURCE_DIR}/space # FIXME Should go away, preferring to store space code there, or be moved to the cbr/space binaries specifically
        ${CBR_SOURCE_DIR}/analysis # FIXME Should go away, preferring to store analysis code there, or be moved to the cbr/analysis binaries specifically
        ${CBR_PROTOCOL_DIR} # For generated protocol code
        ${Boost_INCLUDE_DIRS}
        ${GLUT_INCLUDE_DIR}
        ${Sirikata_INCLUDE_DIR}
        ${Sirikata_INCLUDE_DIR}/sirikata #FIXME libcore doesn't have a proper library layout yet
        ${Sirikata_PBJ_ROOT}
        ${QT_INCLUDES}
        ${GSL_INCLUDE_DIR}
        ${SST_INCLUDE_DIRS}
        ${PROX_INCLUDE_DIRS}
        ${PROTOCOLBUFFERS_INCLUDE_DIRS}
        ${GCRYPT_INCLUDE_DIRS}
        )

#source files
SET(CBR_CORE_SOURCES
  ${CBR_SOURCE_DIR}/Address4.cpp
  ${CBR_SOURCE_DIR}/Message.cpp
  ${CBR_SOURCE_DIR}/Options.cpp
  ${CBR_SOURCE_DIR}/RouterElement.cpp
  ${CBR_SOURCE_DIR}/PollingService.cpp
  ${CBR_SOURCE_DIR}/OSegTestMotionPath.cpp
  ${CBR_SOURCE_DIR}/RandomMotionPath.cpp
  ${CBR_SOURCE_DIR}/QuakeMotionPath.cpp
  ${CBR_SOURCE_DIR}/StaticMotionPath.cpp
  ${CBR_SOURCE_DIR}/Statistics.cpp
  ${CBR_SOURCE_DIR}/TabularServerIDMap.cpp
  ${CBR_SOURCE_DIR}/TimeProfiler.cpp
  ${CBR_SOURCE_DIR}/Timer.cpp
  ${CBR_SOURCE_DIR}/TimeSync.cpp
  ${CBR_SOURCE_DIR}/VWTypes.cpp
)

SET(CBR_SOURCES
  ${CBR_SOURCE_DIR}/AlwaysLocationUpdatePolicy.cpp
  ${CBR_SOURCE_DIR}/CBRLocationServiceCache.cpp
  ${CBR_SOURCE_DIR}/CoordinateSegmentation.cpp
#  ${CBR_SOURCE_DIR}/CoordinateSegmentationClient.cpp
  ${CBR_SOURCE_DIR}/CraqObjectSegmentation.cpp
  ${CBR_SOURCE_DIR}/CraqCacheGood.cpp
  ${CBR_SOURCE_DIR}/craq_hybrid/asyncCraqHybrid.cpp
  ${CBR_SOURCE_DIR}/craq_hybrid/asyncCraqUtil.cpp
  ${CBR_SOURCE_DIR}/craq_hybrid/craq_gets/asyncConnectionGet.cpp
  ${CBR_SOURCE_DIR}/craq_hybrid/craq_gets/asyncCraqGet.cpp
  ${CBR_SOURCE_DIR}/craq_hybrid/craq_sets/asyncConnectionSet.cpp
  ${CBR_SOURCE_DIR}/craq_hybrid/craq_sets/asyncCraqSet.cpp
#  ${CBR_SOURCE_DIR}/DistributedCoordinateSegmentation.cpp
  ${CBR_SOURCE_DIR}/craq_oseg/asyncCraq.cpp
  ${CBR_SOURCE_DIR}/craq_oseg/asyncConnection.cpp
  ${CBR_SOURCE_DIR}/craq_oseg/asyncUtil.cpp
  ${CBR_SOURCE_DIR}/ExpIntegral.cpp
  ${CBR_SOURCE_DIR}/SqrIntegral.cpp
  ${CBR_SOURCE_DIR}/FIFOServerMessageQueue.cpp
  ${CBR_SOURCE_DIR}/FairServerMessageQueue.cpp
  ${CBR_SOURCE_DIR}/Forwarder.cpp
  ${CBR_SOURCE_DIR}/space/LocalForwarder.cpp
  ${CBR_SOURCE_DIR}/LoadMonitor.cpp
  ${CBR_SOURCE_DIR}/LocationService.cpp
  ${CBR_SOURCE_DIR}/space/MigrationMonitor.cpp
  ${CBR_SOURCE_DIR}/space/ObjectConnection.cpp
  ${CBR_SOURCE_DIR}/space/ObjectHostConnectionManager.cpp
  ${CBR_SOURCE_DIR}/OSegHasher.cpp
  ${CBR_SOURCE_DIR}/space/OSegLookupQueue.cpp
  ${CBR_SOURCE_DIR}/Network.cpp
  ${CBR_SOURCE_DIR}/Proximity.cpp
  ${CBR_SOURCE_DIR}/Server.cpp
  ${CBR_SOURCE_DIR}/ServerWeightCalculator.cpp
  ${CBR_SOURCE_DIR}/StandardLocationService.cpp
  ${CBR_SOURCE_DIR}/TCPNetwork.cpp
  ${CBR_SOURCE_DIR}/SSTNetwork.cpp
  ${CBR_SOURCE_DIR}/SSTNetworkImpl.cpp
  ${CBR_SOURCE_DIR}/Test.cpp
  ${CBR_SOURCE_DIR}/UniformCoordinateSegmentation.cpp
  ${CBR_SOURCE_DIR}/WorldPopulationBSPTree.cpp
  ${CBR_SOURCE_DIR}/main.cpp
)

SET(OH_SOURCES
  ${CBR_SOURCE_DIR}/oh/ObjectHostContext.cpp
  ${CBR_SOURCE_DIR}/oh/Object.cpp
  ${CBR_SOURCE_DIR}/oh/ObjectFactory.cpp
  ${CBR_SOURCE_DIR}/oh/ScenarioFactory.cpp
  ${CBR_SOURCE_DIR}/oh/DistributionPingScenario.cpp
  ${CBR_SOURCE_DIR}/oh/ObjectHost.cpp
  ${CBR_SOURCE_DIR}/oh/main.cpp
)

SET(CSEG_SOURCES
  ${CBR_SOURCE_DIR}/craq_oseg/asyncUtil.cpp
  ${CBR_SOURCE_DIR}/craq_hybrid/asyncCraqUtil.cpp
  ${CBR_SOURCE_DIR}/craq_hybrid/craq_sets/asyncCraqSet.cpp
  ${CBR_SOURCE_DIR}/craq_hybrid/craq_sets/asyncConnectionSet.cpp
  ${CBR_SOURCE_DIR}/craq_hybrid/craq_sets/asyncCraqSetPoolTest.cpp
)

#bftm changed because has trouble compiling
#SET(CSEG_SOURCES
#  ${CBR_SOURCE_DIR}/CoordinateSegmentation.cpp
#  ${CBR_SOURCE_DIR}/DistributedCoordinateSegmentation.cpp
#  ${CBR_SOURCE_DIR}/WorldPopulationBSPTree.cpp
#  ${CBR_SOURCE_DIR}/cseg/main.cpp
#)

SET(ANALYSIS_SOURCES
  ${CBR_SOURCE_DIR}/Analysis.cpp
  ${CBR_SOURCE_DIR}/CoordinateSegmentation.cpp
  ${CBR_SOURCE_DIR}/UniformCoordinateSegmentation.cpp
#  ${CBR_SOURCE_DIR}/CoordinateSegmentationClient.cpp
  ${CBR_SOURCE_DIR}/analysis/RecordedMotionPath.cpp
  ${CBR_SOURCE_DIR}/Visualization.cpp
  ${CBR_SOURCE_DIR}/analysis/main.cpp
)


SET(BENCH_SOURCE_DIR ${CBR_SOURCE_DIR}/bench)
SET(BENCH_SOURCES
  ${BENCH_SOURCE_DIR}/BenchmarkRunner.cpp
  ${BENCH_SOURCE_DIR}/TimerSpeedBenchmark.cpp
  ${BENCH_SOURCE_DIR}/TimerJitterBenchmark.cpp
  ${BENCH_SOURCE_DIR}/TimerMonotonicityBenchmark.cpp
  ${BENCH_SOURCE_DIR}/SSTBenchmark.cpp
  ${BENCH_SOURCE_DIR}/main.cpp
)


SET(CBR_MOC_HEADERS
  ${CBR_SOURCE_DIR}/SSTNetworkImpl.h
)

QT4_WRAP_CPP(CBR_MOC_SOURCES ${CBR_MOC_HEADERS})

# protocol files
# FIXME we should jsut change ADD_PBJ_TARGET to take full paths
SET(CBR_PROTOCOLS
  ObjectMessage.pbj
  Header.pbj
  ServerMessage.pbj
  Loc.pbj
  Prox.pbj
  Subscription.pbj
  Migration.pbj
  CSeg.pbj
  Session.pbj
  OSeg.pbj
  ObjConnKill.pbj
)
SET(PBJ_CPP_HEADER "\n")
# protocol targets
ADD_PBJ_TARGET(
  ${CBR_PROTOCOLS}
  PLUGINNAME "CBR"
  INPUTDIR ${CBR_PROTOCOL_DIR}
  IMPORTS ${CBR_PROTOCOL_DIR}
  OUTPUTDIR ${CBR_PROTOCOL_DIR}
  OUTPUTCPPFILE ${CBR_PROTOCOL_DIR}/CBR_protobuf.cc
  CPP_HEADER ${PBJ_CPP_HEADER}
  GENERATE_CPP
)
# generated pbj implementation source files
SET(CBR_PROTOCOL_SOURCES
  ${CBR_PROTOCOL_DIR}/CBR_protobuf.cc
)

#link flags
LINK_DIRECTORIES(
        ${Boost_LIBRARY_DIRS}
        ${GSL_LINK_DIRECTORIES}
)

#libs
ADD_LIBRARY(cbr-core STATIC ${CBR_CORE_SOURCES} ${CBR_PROTOCOL_SOURCES})

#binaries
ADD_EXECUTABLE(cbr ${CBR_SOURCES} ${CBR_MOC_SOURCES})
TARGET_LINK_LIBRARIES(cbr
  cbr-core
        ${GSL_CONFIG_LIBS}
        ${Boost_THREAD_LIBRARY}
        ${Boost_DATE_TIME_LIBRARY}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_REGEX_LIBRARY}
        ${SirikataCore_LIBRARIES}
        ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${QT_QTGUI_LIBRARY}
        ${SST_LIBRARIES}
        ${PROTOCOLBUFFERS_LIBRARIES}
        ${GCRYPT_LIBRARIES}
        )

ADD_EXECUTABLE(oh ${OH_SOURCES} ${CBR_PROTOCOL_SOURCES})
TARGET_LINK_LIBRARIES(oh
  cbr-core
        ${Boost_THREAD_LIBRARY}
        ${Boost_DATE_TIME_LIBRARY}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_REGEX_LIBRARY}
        ${SirikataCore_LIBRARIES}
        ${PROTOCOLBUFFERS_LIBRARIES}
        )


ADD_EXECUTABLE(craq_get_test ${CRAQ_GET_TEST_SOURCES} ${CBR_PROTOCOL_SOURCES})
TARGET_LINK_LIBRARIES(craq_get_test
        ${Boost_THREAD_LIBRARY}
        ${Boost_DATE_TIME_LIBRARY}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_REGEX_LIBRARY}
        ${SirikataCore_LIBRARIES}
        ${PROTOCOLBUFFERS_LIBRARIES}
        )


ADD_EXECUTABLE(craq_set_test ${CRAQ_SET_TEST_SOURCES} ${CBR_PROTOCOL_SOURCES})
TARGET_LINK_LIBRARIES(craq_set_test
        ${Boost_THREAD_LIBRARY}
        ${Boost_DATE_TIME_LIBRARY}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_REGEX_LIBRARY}
        ${SirikataCore_LIBRARIES}
        ${PROTOCOLBUFFERS_LIBRARIES}
        )
      

ADD_EXECUTABLE(cseg ${CSEG_SOURCES} ${CBR_PROTOCOL_SOURCES})
TARGET_LINK_LIBRARIES(cseg
  cbr-core
        ${Boost_THREAD_LIBRARY}
        ${Boost_DATE_TIME_LIBRARY}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_REGEX_LIBRARY}
        ${SirikataCore_LIBRARIES}
        ${PROTOCOLBUFFERS_LIBRARIES}
        ${GCRYPT_LIBRARIES}
        )

ADD_EXECUTABLE(analysis ${ANALYSIS_SOURCES} ${CBR_PROTOCOL_SOURCES})
TARGET_LINK_LIBRARIES(analysis
  cbr-core
        ${Boost_THREAD_LIBRARY}
        ${Boost_DATE_TIME_LIBRARY}
        ${Boost_PROGRAM_OPTIONS_LIBRARY}
        ${Boost_REGEX_LIBRARY}
        ${SirikataCore_LIBRARIES}
        ${PROTOCOLBUFFERS_LIBRARIES}
        ${GLUT_LIBRARIES}
        ${GCRYPT_LIBRARIES}
        )

ADD_EXECUTABLE(bench ${BENCH_SOURCES} ${CBR_PROTOCOL_SOURCES})
TARGET_LINK_LIBRARIES(bench
  cbr-core
  ${Boost_THREAD_LIBRARY}
  ${Boost_DATE_TIME_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  ${Boost_REGEX_LIBRARY}
  ${SirikataCore_LIBRARIES}
  ${PROTOCOLBUFFERS_LIBRARIES}
  )
