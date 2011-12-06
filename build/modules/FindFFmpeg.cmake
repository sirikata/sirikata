# Copyright (c) 2011 Sirikata Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can
# be found in the LICENSE file.
#
# Searches for an FFmpeg installation
#
# Defines:
#
#   FFMPEG_FOUND          True if FFMPEG was found, else false
#   FFMPEG_LIBRARIES      Libraries to link (without full path)
#   FFMPEG_LIBRARY_DIRS   Directories containing the libraries (-L option)
#   FFMPEG_LDFLAGS        All required linker flags
#   FFMPEG_INCLUDE_DIRS   The directories containing header files (-I option)
#   FFMPEG_CFLAGS         All required compiler flags
#
# If you set FFMPEG_MINIMUM_VERSION, only a version equal to or higher than this will be searched for. (This feature does not
# currently work on all systems.)
#
# On Windows, to specify the root directory of the FFmpeg installation, set FFMPEG_ROOT.
#

IF(WIN32)  # Windows

  SET(FFMPEG_FOUND FALSE)

  IF(FFMPEG_ROOT AND EXISTS "${FFMPEG_ROOT}")
    SET(FFMPEG_LIBRARY_DIRS ${FFMPEG_ROOT}/lib)
    SET(FFMPEG_LIBRARIES
        avutil.lib
        avcodec.lib
        avformat.lib
        )
    SET(FFMPEG_LDFLAGS)
    SET(FFMPEG_INCLUDE_DIRS ${FFMPEG_ROOT}/include)
    SET(FFMPEG_CFLAGS)

    SET(FFMPEG_FOUND TRUE)

  ENDIF(FFMPEG_ROOT AND EXISTS "${FFMPEG_ROOT}")

ELSE(WIN32)  # Linux etc
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
    MESSAGE("Could not find pkg-config (to search for FFmpeg)")
  ELSE(NOT PKG_CONFIG_FOUND)
    SET(ENV{PKG_CONFIG_PATH} ${FFMPEG_ROOT}/lib/pkgconfig:${FFMPEG_ROOT}/../lib/pkgconfig)

    # FFmpeg is actually the application. We're looking for the
    # libraries it's built on: libavutil, libavcodec, and libavformat.
    IF(FFMPEG_MINIMUM_VERSION)
      PKG_CHECK_MODULES(LIBAVUTIL libavutil>=${FFMPEG_MINIMUM_VERSION})
      PKG_CHECK_MODULES(LIBAVCODEC libavcodec>=${FFMPEG_MINIMUM_VERSION})
      PKG_CHECK_MODULES(LIBAVFORMAT libavformat>=${FFMPEG_MINIMUM_VERSION})
    ELSE(FFMPEG_MINIMUM_VERSION)
      PKG_CHECK_MODULES(LIBAVUTIL libavutil)
      PKG_CHECK_MODULES(LIBAVCODEC libavcodec)
      PKG_CHECK_MODULES(LIBAVFORMAT libavformat)
    ENDIF(FFMPEG_MINIMUM_VERSION)

    IF(LIBAVUTIL_FOUND AND LIBAVCODEC_FOUND AND LIBAVFORMAT_FOUND)
      SET(FFMPEG_FOUND TRUE)
      SET(FFMPEG_INCLUDE_DIRS ${LIBAVUTIL_INCLUDE_DIRS} ${LIBAVCODEC_INCLUDE_DIRS} ${LIBAVFORMAT_INCLUDE_DIRS})
      SET(FFMPEG_CFLAGS "${LIBAVUTIL_CFLAGS} ${LIBAVCODEC_CFLAGS} ${LIBAVFORMAT_CFLAGS}")
      SET(FFMPEG_LIBRARIES ${LIBAVUTIL_LIBRARIES} ${LIBAVCODEC_LIBRARIES} ${LIBAVFORMAT_LIBRARIES})
      SET(FFMPEG_LIBRARY_DIRS ${LIBAVUTIL_LIBRARY_DIRS} ${LIBAVCODEC_LIBRARY_DIRS} ${LIBAVFORMAT_LIBRARY_DIRS})
      SET(FFMPEG_LDFLAGS "${LIBAVUTIL_LDFLAGS} ${LIBAVCODEC_LDFLAGS} ${LIBAVFORMAT_LDFLAGS}")
    ENDIF()
  ENDIF(NOT PKG_CONFIG_FOUND)
ENDIF(WIN32)

IF(FFMPEG_FOUND)
  IF(NOT FFMPEG_FIND_QUIETLY)
    MESSAGE(STATUS "Found FFmpeg: headers at ${FFMPEG_INCLUDE_DIRS}, libraries at ${FFMPEG_LIBRARY_DIRS}, flags ${FFMPEG_CFLAGS}, libraries ${FFMPEG_LIBRARIES}, and link flags ${FFMPEG_LDFLAGS}")
  ENDIF(NOT FFMPEG_FIND_QUIETLY)
ELSE(FFMPEG_FOUND)
  IF(FFMPEG_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "FFmpeg not found")
  ENDIF(FFMPEG_FIND_REQUIRED)
ENDIF(FFMPEG_FOUND)
