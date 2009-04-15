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