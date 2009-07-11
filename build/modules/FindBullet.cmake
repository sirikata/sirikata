# Searches for an bullet installation
#
# Defines:
#
#   bullet_FOUND          True if bullet was found, else false
#   bullet_LIBRARIES      Libraries to link (without full path)
#   bullet_LIBRARY_DIRS   Directories containing the libraries (-L option)
#   bullet_LDFLAGS        All required linker flags
#   bullet_INCLUDE_DIRS   The directories containing header files (-I option)
#   bullet_CFLAGS         All required compiler flags
#
# If you set bullet_MINIMUM_VERSION, only a version equal to or higher than this will be searched for. (This feature does not
# currently work on all systems.)
#
# On Windows, to specify the root directory of the bullet installation, set bullet_ROOT.
#
# Author: Siddhartha Chaudhuri, 2008
#

IF(WIN32)  # Windows

  SET(bullet_FOUND FALSE)

  IF(bullet_ROOT AND EXISTS "${bullet_ROOT}")
    SET(bullet_LIBRARY_DIRS ${bullet_ROOT}/lib)
    SET(bullet_LIBRARIES
        debug BulletCollision_d.lib
        debug BulletDynamics_d.lib
        debug BulletMultiThreaded_d.lib
        debug BulletSoftBody_d.lib
        debug LinearMath_d.lib
        optimized BulletCollision.lib
        optimized BulletDynamics.lib
        optimized BulletMultiThreaded.lib
        optimized BulletSoftBody.lib
        optimized LinearMath.lib)
    SET(bullet_LDFLAGS)
    IF(EXISTS ${bullet_ROOT}/include/bullet)
      SET(bullet_INCLUDE_DIRS ${bullet_ROOT}/include/bullet)
    ELSE()
      SET(bullet_INCLUDE_DIRS ${bullet_ROOT}/include)
    ENDIF()
    SET(bullet_CFLAGS)
    SET(bullet_FOUND TRUE)
  ENDIF(bullet_ROOT AND EXISTS "${bullet_ROOT}")

ELSEIF(APPLE)  # OS X
#
  SET(bullet_FOUND FALSE)
  FIND_PATH(bullet_INCLUDE_DIRS NAMES btBulletCollisionCommon.h PATHS ${bullet_ROOT}/include ${PLATFORM_LIBS}/include DOC "Location of Bullet header files" NO_DEFAULT_PATH)
  IF(NOT bullet_INCLUDE_DIRS)
    FIND_PATH(bullet_INCLUDE_DIRS NAMES btBulletCollisionCommon.h DOC "Location of bullet header files")
  ENDIF()
  IF(bullet_INCLUDE_DIRS)
    SET(bullet_ROOT_DIRS ${bullet_INCLUDE_DIRS})
    IF("${bullet_ROOT_DIRS}" MATCHES "/include$")
      # Destroy trailing "/include" in the path.
      GET_FILENAME_COMPONENT(bullet_ROOT_DIRS ${bullet_ROOT_DIRS} PATH)
    ENDIF()
    SET(bullet_LIBRARY_DIRS ${bullet_ROOT_DIRS})
    IF(EXISTS "${bullet_LIBRARY_DIRS}/lib")
        SET(bullet_LIBRARY_DIRS ${bullet_LIBRARY_DIRS}/lib)
    ENDIF(EXISTS "${bullet_LIBRARY_DIRS}/lib")
    FIND_LIBRARY(bullet_COLLISION_LIBRARIES NAMES BulletCollision PATHS ${bullet_LIBRARY_DIRS} NO_DEFAULT_PATH)
    FIND_LIBRARY(bullet_DYNAMICS_LIBRARIES NAMES BulletDynamics PATHS ${bullet_LIBRARY_DIRS} NO_DEFAULT_PATH)
    FIND_LIBRARY(bullet_MULTITHREADED_LIBRARIES NAMES BulletMultiThreaded PATHS ${bullet_LIBRARY_DIRS} NO_DEFAULT_PATH)
    FIND_LIBRARY(bullet_SOFTBODY_LIBRARIES NAMES BulletSoftBody PATHS ${bullet_LIBRARY_DIRS} NO_DEFAULT_PATH)
    FIND_LIBRARY(bullet_LINEARMATH_LIBRARIES NAMES LinearMath PATHS ${bullet_LIBRARY_DIRS} NO_DEFAULT_PATH)
    SET(bullet_LIBRARIES ${bullet_COLLISION_LIBRARIES} ${bullet_DYNAMICS_LIBRARIES} ${bullet_MULTITHREADED_LIBRARIES} ${bullet_SOFTBODY_LIBRARIES} ${bullet_LINEARMATH_LIBRARIES})
    IF(bullet_LIBRARIES AND bullet_COLLISION_LIBRARIES AND bullet_DYNAMICS_LIBRARIES AND bullet_MULTITHREADED_LIBRARIES AND bullet_SOFTBODY_LIBRARIES AND bullet_LINEARMATH_LIBRARIES)
        SET(bullet_FOUND TRUE)
    ENDIF()
  ENDIF()
#
#  #INCLUDE(CMakeFindFrameworks)
#  #CMAKE_FIND_FRAMEWORKS(bullet) 
#  SET(bullet_FRAMEWORKS ${bullet_ROOT}/Frameworks/bullet.framework)
#  IF(bullet_FRAMEWORKS)
#    LIST(GET bullet_FRAMEWORKS 0 bullet_LIBRARIES)
#    SET(bullet_INCLUDE_DIRS ${bullet_LIBRARIES}/Headers)
#    
#    # Unset other variables
#    SET(bullet_LDFLAGS -F${bullet_LIBRARIES}/.. -framework bullet)
#
#    SET(bullet_LIBRARY_DIRS)
#    SET(bullet_LIBRARIES)
#    SET(bullet_CFLAGS)
#    SET(bullet_FOUND TRUE)
#  ENDIF(bullet_FRAMEWORKS)

ELSE(WIN32)  # Linux etc
  FIND_PACKAGE(PkgConfig)
  IF(NOT PKG_CONFIG_FOUND)
    MESSAGE("Could not find pkg-config (to search for bullet)")
  ELSE(NOT PKG_CONFIG_FOUND)
    SET(ENV{PKG_CONFIG_PATH} ${bullet_ROOT}/lib/pkgconfig:${bullet_ROOT}/lib/pkgconfig)
    IF(bullet_MINIMUM_VERSION)
      PKG_CHECK_MODULES( bullet bullet>=${bullet_MINIMUM_VERSION})  # will set bullet_FOUND
    ELSE(bullet_MINIMUM_VERSION)
      PKG_CHECK_MODULES(bullet bullet)  # will set bullet_FOUND
    ENDIF(bullet_MINIMUM_VERSION)

  ENDIF(NOT PKG_CONFIG_FOUND)
  IF(bullet_FOUND)
  ENDIF()
ENDIF(WIN32)

IF(bullet_FOUND)
  IF(NOT bullet_FIND_QUIETLY)
    MESSAGE(STATUS "Found bullet: headers at ${bullet_INCLUDE_DIRS}, libraries at ${bullet_LIBRARY_DIRS}")
  ENDIF(NOT bullet_FIND_QUIETLY)
ELSE(bullet_FOUND)
  IF(bullet_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "bullet not found")
  ENDIF(bullet_FIND_REQUIRED)
ENDIF(bullet_FOUND)
#${TOP_LEVEL}/dependencies/bullet-2.74/src/.libs/libbulletcollision.so ${TOP_LEVEL}/dependencies/bullet-2.74/src/.libs/libbulletdynamics.so ${TOP_LEVEL}/dependencies/bullet-2.74/src/.libs/libbulletmath.so
