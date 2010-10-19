#Sirikata
#FindOpenCOLLADA.cmake
#
#Copyright (c) 2009, Mark C. Barnes
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

# Searches for an OpenCOLLADA installation
#
# Defines:
#
#   OpenCOLLADA_FOUND           True if OpenCOLLADA was found, else false
#   OpenCOLLADA_LIBRARIES       Libraries to link (without full path)
#   OpenCOLLADA_LIBRARY_DIRS    Directories containing the libraries (-L option)
#   OpenCOLLADA_LDFLAGS         All required linker flags
#   OpenCOLLADA_INCLUDE_DIRS    Directories containing the header files (-I option)
#   OpenCOLLADA_CFLAGS          All required compiler flags
#
# To specify an additional directory to search, set OpenCOLLADA_ROOT.
#
# Author: Mark C. Barnes, 2009
#

SET(OpenCOLLADA_FOUND FALSE)

# Look for the headers, first in the additional location and then in default system locations
FIND_PATH(OpenCOLLADA_INCLUDE_DIRS NAMES COLLADAFWPrerequisites.h PATHS ${OpenCOLLADA_ROOT} ${OpenCOLLADA_ROOT}/include/ DOC "Location of OpenCOLLADA header files" NO_DEFAULT_PATH)
IF(NOT OpenCOLLADA_INCLUDE_DIRS)
    FIND_PATH(OpenCOLLADA_INCLUDE_DIRS NAMES COLLADAFWPrerequisites.h DOC "Location of OpenCOLLADA header files")
ENDIF()

IF(OpenCOLLADA_INCLUDE_DIRS)

    # toplevel directory
    SET(OpenCOLLADA_ROOT_DIRS ${OpenCOLLADA_INCLUDE_DIRS})
    IF("${OpenCOLLADA_ROOT_DIRS}" MATCHES "/include$")
        # Destroy trailing "/include" in the path.
        GET_FILENAME_COMPONENT(OpenCOLLADA_ROOT_DIRS ${OpenCOLLADA_ROOT_DIRS} PATH)
    ENDIF()

    # library path
    SET(OpenCOLLADA_LIBRARY_DIRS ${OpenCOLLADA_ROOT_DIRS})
    IF(EXISTS "${OpenCOLLADA_LIBRARY_DIRS}/lib")
        SET(OpenCOLLADA_LIBRARY_DIRS ${OpenCOLLADA_LIBRARY_DIRS}/lib)
    ENDIF()

    IF(WIN32) # must distinguish debug and release builds
        SET(OpenCOLLADA_LIBRARIES)

        SET(OpenCOLLADA_DEBUG_LIBRARY)
        SET(OpenCOLLADA_RELEASE_LIBRARY)
        MACRO(OpenCOLLADALIB)
        #FIND_LIBRARY(OpenCOLLADA_DEBUG_LIBRARY NAMES ${ARGN}-d lib${ARGN}-d ${ARGN}_d lib${ARGN}d lib${ARGN}_d 
        #                         PATH_SUFFIXES "" Debug PATHS ${OpenCOLLADA_LIBRARY_DIRS} NO_DEFAULT_PATH)
        #FIND_LIBRARY(OpenCOLLADA_RELEASE_LIBRARY NAMES ${ARGN} lib${ARGN}
        #                         PATH_SUFFIXES "" Release PATHS ${OpenCOLLADA_LIBRARY_DIRS} NO_DEFAULT_PATH)
        #IF(OpenCOLLADA_DEBUG_LIBRARY AND OpenCOLLADA_RELEASE_LIBRARY)
        #    SET(OpenCOLLADA_LIBRARIES debug ${OpenCOLLADA_DEBUG_LIBRARY} optimized ${OpenCOLLADA_RELEASE_LIBRARY} ${OpenCOLLADA_LIBRARIES})
        #ELSEIF(OpenCOLLADA_DEBUG_LIBRARY)
        #    SET(OpenCOLLADA_LIBRARIES ${OpenCOLLADA_DEBUG_LIBRARY} ${OpenCOLLADA_LIBRARIES})
        #ELSEIF(OpenCOLLADA_RELEASE_LIBRARY)
        #    SET(OpenCOLLADA_LIBRARIES ${OpenCOLLADA_RELEASE_LIBRARY} ${OpenCOLLADA_LIBRARIES})
        #ENDIF()
        #MESSAGE(STATUS "COLLADA LIB SEARCH ${ARGN} FOUND SO FAR ${OpenCOLLADA_LIBRARIES}")
        SET(OpenCOLLADA_LIBRARIES debug ${ARGN}-d.lib optimized ${ARGN}.lib ${OpenCOLLADA_LIBRARIES})
      ENDMACRO()
      OpenCOLLADALIB(COLLADASaxFrameworkLoader)
      OpenCOLLADALIB(COLLADAFramework)
      OpenCOLLADALIB(GeneratedSaxParser)
      OpenCOLLADALIB(COLLADAStreamWriter)
      OpenCOLLADALIB(libBuffer)
      OpenCOLLADALIB(LibXML)
      OpenCOLLADALIB(pcre)
      OpenCOLLADALIB(zziplib)
      OpenCOLLADALIB(libftoa)
      OpenCOLLADALIB(zlib)
      OpenCOLLADALIB(COLLADABaseUtils)
      OpenCOLLADALIB(COLLADAFramework)
      OpenCOLLADALIB(MathMLSolver)
    ELSE()
        FIND_LIBRARY(OpenCOLLADA_SAXFW_LIBRARIES NAMES COLLADASaxFrameworkLoader PATHS ${OpenCOLLADA_LIBRARY_DIRS} NO_DEFAULT_PATH)
        FIND_LIBRARY(OpenCOLLADA_FW_LIBRARIES NAMES COLLADAFramework PATHS ${OpenCOLLADA_LIBRARY_DIRS} NO_DEFAULT_PATH)
        FIND_LIBRARY(OpenCOLLADA_MML_LIBRARIES NAMES MathMLSolver PATHS ${OpenCOLLADA_LIBRARY_DIRS} NO_DEFAULT_PATH)
        FIND_LIBRARY(OpenCOLLADA_SAX_LIBRARIES NAMES GeneratedSaxParser PATHS ${OpenCOLLADA_LIBRARY_DIRS} NO_DEFAULT_PATH)
        FIND_LIBRARY(OpenCOLLADA_BU_LIBRARIES NAMES COLLADABaseUtils PATHS ${OpenCOLLADA_LIBRARY_DIRS} NO_DEFAULT_PATH)
        FIND_LIBRARY(OpenCOLLADA_SW_LIBRARIES NAMES COLLADAStreamWriter PATHS ${OpenCOLLADA_LIBRARY_DIRS} NO_DEFAULT_PATH)
        FIND_LIBRARY(OpenCOLLADA_BUFFER_LIBRARIES NAMES Buffer PATHS ${OpenCOLLADA_LIBRARY_DIRS} NO_DEFAULT_PATH)
        FIND_LIBRARY(OpenCOLLADA_FTOA_LIBRARIES NAMES ftoa PATHS ${OpenCOLLADA_LIBRARY_DIRS} NO_DEFAULT_PATH)
        FIND_LIBRARY(OpenCOLLADA_XML_LIBRARIES NAMES xml2 PATHS ${OpenCOLLADA_LIBRARY_DIRS} NO_DEFAULT_PATH)
        FIND_LIBRARY(OpenCOLLADA_PCRE_LIBRARIES NAMES pcre PATHS ${OpenCOLLADA_LIBRARY_DIRS} NO_DEFAULT_PATH)
        FIND_LIBRARY(OpenCOLLADA_UTF_LIBRARIES NAMES ConvertUTF PATHS ${OpenCOLLADA_LIBRARY_DIRS} NO_DEFAULT_PATH)
        SET(OpenCOLLADA_LIBRARIES ${OpenCOLLADA_SAXFW_LIBRARIES} ${OpenCOLLADA_FW_LIBRARIES} ${OpenCOLLADA_MML_LIBRARIES} ${OpenCOLLADA_SAX_LIBRARIES} ${OpenCOLLADA_BU_LIBRARIES} ${OpenCOLLADA_XML_LIBRARIES} ${OpenCOLLADA_PCRE_LIBRARIES} ${OpenCOLLADA_UTF_LIBRARIES} ${OpenCOLLADA_SW_LIBRARIES} ${OpenCOLLADA_BUFFER_LIBRARIES} ${OpenCOLLADA_FTOA_LIBRARIES} )
    ENDIF()

    IF(OpenCOLLADA_LIBRARIES)
        SET(OpenCOLLADA_FOUND TRUE)
    ENDIF()
ENDIF()

IF(OpenCOLLADA_FOUND)
    IF(NOT OpenCOLLADA_FIND_QUIETLY)
        MESSAGE(STATUS "Found OpenCOLLADA: includes at ${OpenCOLLADA_INCLUDE_DIRS}, libraries at ${OpenCOLLADA_LIBRARY_DIRS}")
    ENDIF()
ELSE()
    IF(OpenCOLLADA_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "OpenCOLLADA not found")
    ELSE()
        MESSAGE("OpenCOLLADA not found")
    ENDIF()
ENDIF()
