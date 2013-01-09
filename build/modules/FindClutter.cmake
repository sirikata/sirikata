# Copyright (c) 2013 Sirikata Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can
# be found in the LICENSE file.
#
# Searches for a Clutter 1.x installation
#
# Defines:
#
#   CLUTTER_FOUND          True if Clutter was found, else false
#   CLUTTER_LIBRARIES      Libraries to link (without full path)
#   CLUTTER_LIBRARY_DIRS   Directories containing the libraries (-L option)
#   CLUTTER_LDFLAGS        All required linker flags
#   CLUTTER_INCLUDE_DIRS   The directories containing header files (-I option)
#   CLUTTER_CFLAGS         All required compiler flags
#
# If you set CLUTTER_MINIMUM_VERSION, only a version equal to or higher than this will be searched for. (This feature does not
# currently work on all systems.)
#
# Currently Windows is not supported.
#

IF(WIN32)  # Windows
  SET(CLUTTER_FOUND FALSE)
ELSE(WIN32)  # Linux etc
  SET(CLUTTER_FOUND FALSE)

  FIND_PACKAGE(PkgConfig)
  IF(NOT PKG_CONFIG_FOUND)
    SET(PKG_CONFIG_CANDIDATE_PATH ${DEPENDENCIES_ROOT}/installed-pkg-config/bin)
    IF(EXISTS ${PKG_CONFIG_CANDIDATE_PATH})
      FIND_PROGRAM(PKG_CONFIG_EXECUTABLE NAMES pkg-config PATHS ${PKG_CONFIG_CANDIDATE_PATH} DOC "pkg-config executable")
      IF(PKG_CONFIG_EXECUTABLE)
        SET(PKG_CONFIG_FOUND 1)
      ENDIF(PKG_CONFIG_EXECUTABLE)
    ENDIF()
  ENDIF()
  IF(NOT PKG_CONFIG_FOUND)
    MESSAGE("Could not find pkg-config (to search for Clutter)")
  ELSE(NOT PKG_CONFIG_FOUND)
    SET(ENV{PKG_CONFIG_PATH} ${CLUTTER_ROOT}/lib/pkgconfig:${CLUTTER_ROOT}/../lib/pkgconfig)

    IF(CLUTTER_MINIMUM_VERSION)
      PKG_CHECK_MODULES(LIBCLUTTER clutter-1.0>=${CLUTTER_MINIMUM_VERSION})
    ELSE()
      PKG_CHECK_MODULES(LIBCLUTTER libavutil)
    ENDIF()

    IF(LIBCLUTTER_FOUND)
      SET(CLUTTER_FOUND TRUE)
      SET(CLUTTER_INCLUDE_DIRS ${LIBCLUTTER_INCLUDE_DIRS})
      SET(CLUTTER_CFLAGS "${LIBCLUTTER_CFLAGS}")
      SET(CLUTTER_LIBRARIES ${LIBCLUTTER_LIBRARIES})
      SET(CLUTTER_LIBRARY_DIRS ${LIBCLUTTER_LIBRARY_DIRS})
      SET(CLUTTER_LDFLAGS "${LIBCLUTTER_LDFLAGS} ${LIBAVCODEC_LDFLAGS} ${LIBAVFORMAT_LDFLAGS}")
    ENDIF()
  ENDIF(NOT PKG_CONFIG_FOUND)
ENDIF(WIN32)

IF(CLUTTER_FOUND)
  IF(NOT CLUTTER_FIND_QUIETLY)
    MESSAGE(STATUS "Found Clutter: headers at ${CLUTTER_INCLUDE_DIRS}, libraries at ${CLUTTER_LIBRARY_DIRS}, flags ${CLUTTER_CFLAGS}, libraries ${CLUTTER_LIBRARIES}, and link flags ${CLUTTER_LDFLAGS}")
  ENDIF(NOT CLUTTER_FIND_QUIETLY)
ELSE(CLUTTER_FOUND)
  IF(CLUTTER_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Clutter not found")
  ENDIF(CLUTTER_FIND_REQUIRED)
ENDIF(CLUTTER_FOUND)
