#Sirikata
#FindCecil.cmake
#
#Copyright (c) 2008, Ewen Cheslack-Postava
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

# - Try to find Cecil, assumes FindMono has already been run
#
#  CECIL_FOUND
#  CECIL_LIBRARIES
#
# Author: Ewen Cheslack-Postava, 2008
#

# backup PKG_CONFIG_PATH and add Mono's path
SET(OLD_PKG_CONFIG_PATH $ENV{PKG_CONFIG_PATH})
IF(MONO_PKG_CONFIG_PATH)
  SET(ENV{PKG_CONFIG_PATH} ${MONO_PKG_CONFIG_PATH}:$ENV{PKG_CONFIG_PATH})
ENDIF(MONO_PKG_CONFIG_PATH)

IF(WIN32)

  IF(MONO_ROOT)
    FIND_FILE(CECIL_LIB Mono.Cecil.dll PATHS ${MONO_ROOT}/lib/mono/assemblies)
    IF(CECIL_LIB)
      SET(CECIL_FOUND TRUE)
      SET(CECIL_LIBRARIES "-r:${CECIL_LIB}")
    ENDIF(CECIL_LIB)
  ENDIF(MONO_ROOT)

ELSE(WIN32)

  IF(PKG_CONFIG_EXECUTABLE)

  #check for mono-cecil.pc
  #FUNCTION(PKG_CHECK_CECIL_MODULE PCNAME)
    EXECUTE_PROCESS(COMMAND ${PKG_CONFIG_EXECUTABLE} "--variable=Libraries" "mono-cecil"
                    RESULT_VARIABLE __pkg_check_cecil_module_result
                    OUTPUT_VARIABLE __pkg_check_cecil_module_output
                    ERROR_VARIABLE __pkg_check_cecil_module_error
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    IF(__pkg_check_cecil_module_result EQUAL 0)
      IF(EXISTS ${__pkg_check_cecil_module_output})
        SET(CECIL_FOUND TRUE)
        SET(CECIL_LIBRARIES "-r:${__pkg_check_cecil_module_output}")
      ENDIF(EXISTS ${__pkg_check_cecil_module_output})
    ENDIF(__pkg_check_cecil_module_result EQUAL 0)
  #ENDFUNCTION()

  #check or cecil.pc
  #FUNCTION(PKG_CHECK_CECIL_MODULE PCNAME)
    EXECUTE_PROCESS(COMMAND ${PKG_CONFIG_EXECUTABLE} "--variable=Libraries" "cecil"
                    RESULT_VARIABLE __pkg_check_cecil_module_result
                    OUTPUT_VARIABLE __pkg_check_cecil_module_output
                    ERROR_VARIABLE __pkg_check_cecil_module_error
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    IF(__pkg_check_cecil_module_result EQUAL 0)
      IF(EXISTS ${__pkg_check_cecil_module_output})
        SET(CECIL_FOUND TRUE)
        SET(CECIL_LIBRARIES "-r:${__pkg_check_cecil_module_output}")
      ENDIF(EXISTS ${__pkg_check_cecil_module_output})
    ENDIF(__pkg_check_cecil_module_result EQUAL 0)
  #ENDFUNCTION()

  ENDIF(PKG_CONFIG_EXECUTABLE)
ENDIF(WIN32)

# restore PKG_CONFIG_PATH
SET(ENV{PKG_CONFIG_PATH} OLD_PKG_CONFIG_PATH)


# print out what we found
IF(CECIL_FOUND)
  MESSAGE(STATUS "Cecil Library Parameters: ${CECIL_LIBRARIES}")
ENDIF(CECIL_FOUND)