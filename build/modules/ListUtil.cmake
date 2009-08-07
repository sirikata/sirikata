# ListUtil.cmake
# List utility macros.

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
