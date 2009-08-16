# - Try to find the mono, mcs, gmcs and gacutil
#
# Defines:
#
# MONO_FOUND             System has Mono dev files, as well as mono, mcs, gmcs and gacutil if not MONO_ONLY_LIBRARIES_REQUIRED
# MONO_EXECUTABLE        Where to find 'mono'
# IKVM_EXECUTABLE        Where to find 'ikvm'
# MCS_EXECUTABLE         Where to find 'mcs'
# GMCS_EXECUTABLE        Where to find 'gmcs'
# GACUTIL_EXECUTABLE     Where to find 'gacutil'
# MONO_PKG_CONFIG_PATH   Path for pkg-config files for this mono installation, e.g. /usr/lib/pkgconfig
# MONO_LIBRARIES         Libraries to link (without full path)
# MONO_LIBRARY_DIRS      Directories containing the libraries (-L option)
# MONO_LDFLAGS           All required linker flags
# MONO_INCLUDE_DIRS      The directories containing header files (-I option)
# MONO_CFLAGS            All required compiler flags
# MONO_MAJOR_VERSION     The major version number of the Mono dev libraries
# MONO_MINOR_VERSION     The minor version number of the Mono dev libraries
# MONO_SUBMINOR_VERSION  The subminor version number of the Mono dev libraries
#
# On Windows, to specify an additional directory to search, set MONO_ROOT.
# To avoid searching for the Mono executables (mono, mcs, gmcs, gacutil), set MONO_ONLY_LIBRARIES_REQUIRED.
#
# Copyright (c) 2007 Arno Rehn arno@arnorehn.de
# Extended by Siddhartha Chaudhuri to include searches for Mono development files, 2008.
#
# Redistribution and use is allowed according to the terms of the GPL license.
#
# Sid notes: Can we get BSD licensing permission? Especially given that we've put in way more than the code originally had?
#

SET(MONO_LOOK_FOR_LIBRARIES FALSE)
SET(MONO_FOUND TRUE)
IF(MONO_ONLY_LIBRARIES_REQUIRED)

  SET(MONO_LOOK_FOR_LIBRARIES TRUE)
ELSE()
  # try to find custom mono installation first
  IF(MONO_ROOT AND EXISTS ${MONO_ROOT})

    FIND_PROGRAM(MONO_EXECUTABLE mono PATHS ${MONO_ROOT}/bin NO_DEFAULT_PATH)
    FIND_PROGRAM(MCS_EXECUTABLE mcs PATHS ${MONO_ROOT}/bin NO_DEFAULT_PATH)
    FIND_PROGRAM(GMCS_EXECUTABLE gmcs PATHS ${MONO_ROOT}/bin NO_DEFAULT_PATH)
    FIND_PROGRAM(GACUTIL_EXECUTABLE gacutil PATHS ${MONO_ROOT}/bin NO_DEFAULT_PATH)
  ELSE()
  FIND_PROGRAM(MONO_EXECUTABLE mono)
  FIND_PROGRAM(MCS_EXECUTABLE mcs)
  FIND_PROGRAM(GMCS_EXECUTABLE gmcs)
  FIND_PROGRAM(GACUTIL_EXECUTABLE gacutil)

  ENDIF()


  IF(WIN32)
    IF(MONO_ROOT AND EXISTS ${MONO_ROOT})
      FIND_FILE(MCS_EXE mcs.exe PATHS ${MONO_ROOT}/lib/mono/1.0/)
      IF(MCS_EXE)
        SET(MCS_EXECUTABLE "${MONO_EXECUTABLE}" "${MCS_EXE}")
      ENDIF()

      FIND_FILE(GMCS_EXE gmcs.exe PATHS ${MONO_ROOT}/lib/mono/2.0/)
      IF(GMCS_EXE)
        SET(GMCS_EXECUTABLE "${MONO_EXECUTABLE}" "${GMCS_EXE}")
      ENDIF()

      FIND_FILE(GACUTIL_EXE gacutil.exe PATHS ${MONO_ROOT}/lib/mono/1.0/)
      IF(GACUTIL_EXE)
        SET(GACUTIL_EXECUTABLE "${MONO_EXECUTABLE}" "${GACUTIL_EXE}")
      ENDIF()

      FIND_FILE(IKVM_EXE ikvm.exe PATHS ${MONO_ROOT}/lib/ikvm/)
      IF(IKVM_EXE)
        SET(IKVM_EXECUTABLE "${MONO_EXECUTABLE}" "${IKVM_EXE}")
      ENDIF()

    ENDIF()
  ELSEIF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")  # OS X
    SET(MONO_EXECUTABLE_DIR /Library/Frameworks/Mono.framework/Commands/mono)
    SET(MCS_EXECUTABLE_DIR /Library/Frameworks/Mono.framework/Commands/mcs)
    SET(GMCS_EXECUTABLE_DIR /Library/Frameworks/Mono.framework/Commands/gmcs)
    SET(GACUTIL_EXECUTABLE_DIR /Library/Frameworks/Mono.framework/Commands/gacutil)
    GET_FILENAME_COMPONENT(MONO_EXECUTABLE ${MONO_EXECUTABLE_DIR} ABSOLUTE)
    GET_FILENAME_COMPONENT(MCS_EXECUTABLE ${MCS_EXECUTABLE_DIR} ABSOLUTE)
    GET_FILENAME_COMPONENT(GMCS_EXECUTABLE ${GMCS_EXECUTABLE_DIR} ABSOLUTE)
    GET_FILENAME_COMPONENT(GACUTIL_EXECUTABLE ${GACUTIL_EXECUTABLE_DIR} ABSOLUTE)
  ENDIF()

  IF(MONO_EXECUTABLE AND MCS_EXECUTABLE AND GMCS_EXECUTABLE AND GACUTIL_EXECUTABLE)
    SET(MONO_LOOK_FOR_LIBRARIES TRUE)
  ELSE(MONO_EXECUTABLE AND MCS_EXECUTABLE AND GMCS_EXECUTABLE AND GACUTIL_EXECUTABLE)
    SET(MONO_FOUND FALSE)
  ENDIF()

ENDIF()
IF(MONO_FOUND)
      SET(MONO_MAJOR_VERSION 0)
      SET(MONO_MINOR_VERSION 0)
      SET(MONO_SUBMINOR_VERSION 0)

      EXEC_PROGRAM(${MONO_EXECUTABLE} ARGS --version OUTPUT_VARIABLE MONO_STRING_VERSION)
      STRING(REGEX MATCH "([0-9]+)(\\.([0-9]+))+" MONO_VERSION ${MONO_STRING_VERSION})
      STRING(REPLACE "." " " MONO_VERSION ${MONO_VERSION})
      SEPARATE_ARGUMENTS(MONO_VERSION)
      LIST(LENGTH MONO_VERSION MONO_VERSION_LENGTH)
      IF(MONO_VERSION_LENGTH EQUAL 4)
          SET(MONO_MAJOR_VERSION)
          SET(MONO_MINOR_VERSION)
          SET(MONO_SUBMINOR_VERSION)

          LIST(GET MONO_VERSION 0 MONO_MAJOR_VERSION)
          LIST(GET MONO_VERSION 1 MONO_MINOR_VERSION)
          LIST(GET MONO_VERSION 2 MONO_SUBMINOR_VERSION)
      ELSEIF(MONO_VERSION_LENGTH EQUAL 3)
          SET(MONO_MAJOR_VERSION)
          SET(MONO_MINOR_VERSION)
          SET(MONO_SUBMINOR_VERSION)

          LIST(GET MONO_VERSION 0 MONO_MAJOR_VERSION)
          LIST(GET MONO_VERSION 1 MONO_MINOR_VERSION)
          LIST(GET MONO_VERSION 2 MONO_SUBMINOR_VERSION)
      ELSEIF(MONO_VERSION_LENGTH EQUAL 2)
          SET(MONO_MAJOR_VERSION)
          SET(MONO_MINOR_VERSION)

          LIST(GET MONO_VERSION 0 MONO_MAJOR_VERSION)
          LIST(GET MONO_VERSION 1 MONO_MINOR_VERSION)
      ENDIF()
#MESSAGE("${MONO_MAJOR_VERSION} ${MONO_MINOR_VERSION} ${MONO_SUBMINOR_VERSION}")
#      
      IF("${MONO_MAJOR_VERSION}" STREQUAL "0")
        SET(MONO_FOUND FALSE)
      ENDIF()
      IF("${MONO_MAJOR_VERSION}" STREQUAL "1")
        SET(MONO_FOUND FALSE)
      ENDIF()
      IF("${MONO_MAJOR_VERSION}" STREQUAL "2")
        IF("${MONO_MINOR_VERSION}" STREQUAL "0")
          SET(MONO_FOUND FALSE)
        ENDIF()
        IF("${MONO_MINOR_VERSION}" STREQUAL "1")
          SET(MONO_FOUND FALSE)
        ENDIF()
        IF("${MONO_MINOR_VERSION}" STREQUAL "2")
          SET(MONO_FOUND FALSE)
        ENDIF()
        IF("${MONO_MINOR_VERSION}" STREQUAL "3")
          SET(MONO_FOUND FALSE)
        ENDIF()

      ENDIF()
      IF(NOT MONO_FOUND)
        MESSAGE(STATUS "MONO VERSION ${MONO_MAJOR_VERSION}.${MONO_MINOR_VERSION}.${MONO_SUBMINOR_VERSION} too old to build protobufs")
      ENDIF()

# AND (("${MONO_MINOR_VERSION}" STREQUAL "0") OR ("${MONO_MINOR_VERSION}" STREQUAL "1") OR ("${MONO_MINOR_VERSION}" STREQUAL "2") OR ("${MONO_MINOR_VERSION}" STREQUAL "0"))))
ENDIF()
IF(MONO_FOUND AND MONO_LOOK_FOR_LIBRARIES)
  IF(WIN32)  # Windows

    IF(MONO_ROOT AND EXISTS ${MONO_ROOT})
      SET(MONO_INCLUDE_DIRS
          ${MONO_ROOT}/include/mono-1.0
          ${MONO_ROOT}/include/glib-2.0
          ${MONO_ROOT}/lib/glib-2.0/include)
      SET(MONO_LIBRARY_DIRS ${MONO_ROOT}/lib)
      SET(MONO_LIBRARIES mono glib-2.0 intl iconv gthread-2.0)
      SET(MONO_LDFLAGS)
      SET(MONO_CFLAGS)
      SET(MONO_MAJOR_VERSION 0)
      SET(MONO_MINOR_VERSION 0)
      SET(MONO_SUBMINOR_VERSION 0)

      SET(MONO_FOUND TRUE)
    ELSE(MONO_ROOT AND EXISTS ${MONO_ROOT})
      SET(MONO_FOUND FALSE)
    ENDIF(MONO_ROOT AND EXISTS ${MONO_ROOT})

  ELSE(WIN32)  # Unix

    IF(MONO_ROOT AND EXISTS ${MONO_ROOT}/lib/pkgconfig)
      SET(ENV{PKG_CONFIG_PATH} ${MONO_ROOT}/lib/pkgconfig)
      SET(ENV{PATH} ${MONO_ROOT}/bin:$ENV{PATH})
    ENDIF(MONO_ROOT AND EXISTS ${MONO_ROOT}/lib/pkgconfig)

    INCLUDE(FindPkgConfig)  # we don't need the pkg-config path on OS X, but we need other macros in this file

    IF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")  # OS X

      # Replace any pkg-config found so far with the Mono-specific one.

      # Save restore state
      SET(PKG_CONFIG_FOUND_BACKUP ${PKG_CONFIG_FOUND})
      SET(PKG_CONFIG_EXECUTABLE_BACKUP ${PKG_CONFIG_EXECUTABLE})
      SET(PKG_CONFIG_FOUND FALSE)
      INCLUDE(CMakeFindFrameworks)
      CMAKE_FIND_FRAMEWORKS(Mono)
      IF(Mono_FRAMEWORKS)
        LIST(GET Mono_FRAMEWORKS 0 MONO_FRAMEWORK)
        SET(MONO_PKG_CONFIG ${MONO_FRAMEWORK}/Commands/pkg-config)
        SET(MONO_FOUND TRUE)
        SET(PKG_CONFIG_EXECUTABLE ${MONO_PKG_CONFIG})
        SET(PKG_CONFIG_FOUND TRUE)
        SET(MONO_INCLUDE_DIRS ${Mono_FRAMEWORKS}/Headers/mono-1.0
                               ${Mono_FRAMEWORKS}/Headers/glib-2.0
                               ${Mono_FRAMEWORKS}/Libraries/glib-2.0/include)
        SET(MONO_LIBRARY_DIRS ${Mono_FRAMEWORKS}/Libraries)
        SET(MONO_LIBRARIES mono  glib-2.0 intl iconv gthread-2.0)
        MESSAGE(STATUS "${MONO_INCLUDE_DIRS} XX ${MONO_LIBRARY_DIRS} YY ${MONO_LIBRARIES}")
      ENDIF(Mono_FRAMEWORKS)

    ELSE(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")

      SET(MONO_PKG_CONFIG ${PKG_CONFIG_EXECUTABLE})
    ENDIF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")


    IF(NOT PKG_CONFIG_FOUND)
      
      MESSAGE(STATUS "Could not find suitable pkg-config to search for Mono")
      SET(MONO_FOUND FALSE)
    ELSE(NOT PKG_CONFIG_FOUND)
      IF(MONO_ROOT AND EXISTS ${MONO_ROOT}/lib/pkgconfig AND NOT APPLE)
        PKG_CHECK_MODULES(MONO glib-2.0 gthread-2.0)  # will set MONO_FOUND
        PKG_CHECK_MODULES(MONO_MANA mono)  # will set MONO_MANA_FOUND
        IF(NOT MONO_MANA_FOUND)
           SET(MONO_FOUND FALSE)
           MESSAGE(STATUS "Found glib and gthread but could not locate mono")
        ENDIF()
        SET(MONO_INCLUDE_DIRS ${MONO_ROOT}/include/mono-1.0 ${MONO_INCLUDE_DIRS})
        SET(MONO_LIBRARY_DIRS ${MONO_ROOT}/lib ${MONO_LIBRARY_DIRS})
        SET(MONO_LIBRARIES ${MONO_MANA_LIBRARIES})
        SET(MONO_LDFLAGS -Wl,--export-dynamic ${MONO_LDFLAGS})
        SET(MONO_CFLAGS -D_REENTRANT ${MONO_CFLAGS})# MONO_MANA_CFLAGS not compatible with spaces
      ELSE()
        PKG_CHECK_MODULES(MONO mono)  # will set MONO_FOUND
        IF(APPLE AND EXISTS "/Library/Frameworks/Mono.framework")
          SET(MONO_FOUND TRUE)
        ENDIF()
      ENDIF()
    ENDIF(NOT PKG_CONFIG_FOUND)


      IF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
        # Restore saved state
        SET(PKG_CONFIG_FOUND ${PKG_CONFIG_FOUND_BACKUP})
        SET(PKG_CONFIG_EXECUTABLE ${PKG_CONFIG_EXECUTABLE_BACKUP})
      ENDIF(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")

    SET(MONO_LDFLAGS ${MONO_LDFLAGS} "-rdynamic")


  ENDIF()

ENDIF()

# try to generate a MONO_PKG_CONFIG_PATH
# first by looking relative to the executable
IF(MONO_EXECUTABLE AND EXISTS ${MONO_EXECUTABLE})
  GET_FILENAME_COMPONENT(MONO_EXECUTABLE_PATH ${MONO_EXECUTABLE} ABSOLUTE)
  GET_FILENAME_COMPONENT(MONO_EXECUTABLE_PATH ${MONO_EXECUTABLE_PATH} PATH)

  IF(WIN32)
    SET(__MONO_PKG_CONFIG_PATH ${MONO_EXECUTABLE_PATH}/../lib/pkgconfig/mono.pc)
  ELSEIF(APPLE)
    #something's funky with apple, absolute path resolution isn't getting rid of
    #symlinks but the existence test behaves as if it does, so we manually have to
    #adjust the path
    GET_FILENAME_COMPONENT(__MONO_PKG_CONFIG_PATH ${MONO_EXECUTABLE_PATH} PATH)
    SET(__MONO_PKG_CONFIG_PATH ${__MONO_PKG_CONFIG_PATH}/Libraries/pkgconfig/mono.pc)
#    SET(MONO_INCLUDE_DIRS ${__MONO_PKG_CONFIG_PATH}/Headers)
  ELSE()
    SET(__MONO_PKG_CONFIG_PATH ${MONO_EXECUTABLE_PATH}/../lib/pkgconfig/mono.pc)
  ENDIF()

  IF(EXISTS ${__MONO_PKG_CONFIG_PATH})
     GET_FILENAME_COMPONENT(MONO_PKG_CONFIG_PATH ${__MONO_PKG_CONFIG_PATH} PATH)
     GET_FILENAME_COMPONENT(MONO_PKG_CONFIG_PATH ${MONO_PKG_CONFIG_PATH} ABSOLUTE)
  ENDIF(EXISTS ${__MONO_PKG_CONFIG_PATH})
ENDIF(MONO_EXECUTABLE AND EXISTS ${MONO_EXECUTABLE})

IF(MONO_FOUND)
  IF(NOT MONO_FIND_QUIETLY)
    IF(NOT MONO_ONLY_LIBRARIES_REQUIRED)
      MESSAGE(STATUS "Found mono: ${MONO_EXECUTABLE}")
      MESSAGE(STATUS "Found mcs: ${MCS_EXECUTABLE}")
      MESSAGE(STATUS "Found gmcs: ${GMCS_EXECUTABLE}")
      MESSAGE(STATUS "Found gacutil: ${GACUTIL_EXECUTABLE}")
      IF(IKVM_EXECUTABLE)
      MESSAGE(STATUS "Found ikvm: ${IKVM_EXECUTABLE}")
      ENDIF()
    ENDIF(NOT MONO_ONLY_LIBRARIES_REQUIRED)

    IF(MONO_PKG_CONFIG_PATH)
      MESSAGE(STATUS "Found Mono pkg-config path: ${MONO_PKG_CONFIG_PATH}")
    ENDIF(MONO_PKG_CONFIG_PATH)

    IF(MONO_LIBRARY_DIRS)
      MESSAGE(STATUS "Found Mono development files: headers at ${MONO_INCLUDE_DIRS}, libraries at ${MONO_LIBRARY_DIRS}")
    ELSE(MONO_LIBRARY_DIRS)
      MESSAGE(STATUS "Found Mono development files: headers at ${MONO_INCLUDE_DIRS}")
    ENDIF(MONO_LIBRARY_DIRS)
  ENDIF(NOT MONO_FIND_QUIETLY)
ELSE(MONO_FOUND)
  IF(MONO_FIND_REQUIRED)
    IF(MONO_ONLY_LIBRARIES_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find Mono development files")
    ELSE(MONO_ONLY_LIBRARIES_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find one or more of the following: mono, mcs, gmcs, gacutil, Mono development files")
    ENDIF(MONO_ONLY_LIBRARIES_REQUIRED)
  ELSE(MONO_FIND_REQUIRED)
    MESSAGE(STATUS "Could not find Mono development files! Building without scripting support.")
  ENDIF(MONO_FIND_REQUIRED)
ENDIF(MONO_FOUND)

MARK_AS_ADVANCED(MONO_EXECUTABLE MCS_EXECUTABLE GMCS_EXECUTABLE GACUTIL_EXECUTABLE MONO_PKG_CONFIG_PATH)
