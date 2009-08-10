# AddPBJTarget.cmake
# ADD_PBJ_TARGET - sets up targets to compile pbj -> idl -> cpp.
# Note: Currently expects PROTOCOLBUFFERS_COMPILER to be defined.
# Note: Currently expects PBJ_RUNABLE to be defined.
#
# Takes a list of files .
# The following parameters are available:
# DEPENDS - Files or targets this target should depend on
# IMPORTS - Directory to find files that need to be imported, e.g. other pbj files that are dependend on
# PLUGINNAME -
# OUTPUTDIR - The directory to output generated files to
# INPUTDIR - The directory to find input files in
# OUTPUTCPPFILE - The C++ file to use as output, which just #includes a number of other generated files.
# CPP_HEADER - the header we want written to the generated C++ files, useful for including dependencies
#
# The following flags are available:
#  GENERATE_CPP    - indicates that C++ targets should be generated
#  GENERATE_CSHARP - indicates that C# targets should be generated
#  GENERATE_PYTHON - indicates that Python targets should be generated
# FIXME we should probably just be able to pass a list of targets instead of all these GENERATE_* flags

INCLUDE(ListUtil)
INCLUDE(ParseArguments)

MACRO(ADD_PBJ_TARGET)
  PARSE_ARGUMENTS(PBJ "DEPENDS;IMPORTS;PLUGINNAME;OUTPUTDIR;INPUTDIR;OUTPUTCPPFILE;CPP_HEADER" "GENERATE_CPP;GENERATE_CSHARP;GENERATE_PYTHON" ${ARGN})
  SET(PBJ_FILES ${PBJ_DEFAULT_ARGS})

  SET(PBJ_PROTO_FILES)
  SET(PBJ_CPP_FILES)
  SET(PBJ_H_FILES)
  SET(PBJ_PBJ_FILES)
  SET(PBJ_PBJHPP_FILES)
  SET(PBJ_ALL_OUTPUTS)

  SET(PBJ_OPTIONS)
  IF(PBJ_IMPORTS)
    SET(PBJ_OPTIONS ${PBJ_OPTIONS} --proto_path=${PBJ_IMPORTS})
  ENDIF(PBJ_IMPORTS)
  IF(PBJ_OUTPUTDIR)
    SET(PBJ_OPTIONS ${PBJ_OPTIONS} --cpp_out=${PBJ_OUTPUTDIR})
  ENDIF(PBJ_OUTPUTDIR)


  IF(PBJ_OUTPUTCPPFILE)
    SET(PBJ_GenFile ${CMAKE_CURRENT_BINARY_DIR}/protobuf_${PBJ_PLUGINNAME}.txt)
    IF(PBJ_CPP_HEADER)
        FILE(WRITE ${PBJ_GenFile}
             ${CPP_HEADER})
    ENDIF()
  ENDIF()
  SET(PBJ_PLUGINNAME_UNDERSCORE)
  IF(PBJ_PLUGINNAME)
    SET(PBJ_PLUGINNAME_UNDERSCORE ${PBJ_PLUGINNAME}_)
  ENDIF()
  FOREACH(FILE_FROM_LIST ${PBJ_FILES})
    SET(FILE ${FILE_FROM_LIST})
    # Get the bare file name, i.e. without .pbj at the end
    IF(${FILE} MATCHES ".*[.]pbj")
      STRING(REPLACE ".pbj" "" FILE ${FILE})
    ENDIF()
    SET(PBJ_PBJ_FILE ${PBJ_INPUTDIR}/${FILE}.pbj)
    SET(PBJ_PROTO_FILE ${PBJ_OUTPUTDIR}/${PBJ_PLUGINNAME_UNDERSCORE}${FILE}.proto)
    SET(PBJ_PROTO_FILES ${PBJ_PROTO_FILES} ${PBJ_PROTO_FILE})
    SET(PBJ_PBJ_FILES ${PBJ_PBJ_FILES} ${PBJ_PBJ_FILE})
    SET(PBJ_PBJCS_FILES ${PBJ_PBJCS_FILES} ${PBJ_PBJCS_FILE})
    SET(PBJ_PBJHPP_FILE ${PBJ_OUTPUTDIR}/${PBJ_PLUGINNAME_UNDERSCORE}${FILE}.pbj.hpp)

    SET(PBJ_ALL_OUTPUTS)

    IF(PBJ_GENERATE_CPP)
      MESSAGE(STATUS "Building C++ protobuf ${PBJ_PLUGINNAME}: ${FILE}")
      SET(PBJ_CPP_FILE ${PBJ_OUTPUTDIR}/${PBJ_PLUGINNAME_UNDERSCORE}${FILE}.pb.cc)
      SET(PBJ_H_FILE ${PBJ_OUTPUTDIR}/${PBJ_PLUGINNAME_UNDERSCORE}${FILE}.pb.h)
      SET(PBJ_CPP_FILES ${PBJ_CPP_FILES} ${PBJ_CPP_FILE})
      SET(PBJ_H_FILES ${PBJ_H_FILES} ${PBJ_H_FILE})
      SET(PBJ_PBJHPP_FILES ${PBJ_PBJHPP_FILES} ${PBJ_PBJHPP_FILE})
      SET(PBJ_OUTPUTS ${PBJ_H_FILE} ${PBJ_CPP_FILE} ${PBJ_PBJHPP_FILE})
      SET(PBJ_ALL_OUTPUTS ${PBJ_ALL_OUTPUTS} ${PBJ_OUTPUTS})
      ADD_CUSTOM_COMMAND(OUTPUT ${PBJ_H_FILE} ${PBJ_CPP_FILE}
                         COMMAND ${PROTOCOLBUFFERS_COMPILER} -I${PBJ_OUTPUTDIR} ${PBJ_OPTIONS} ${PBJ_PROTO_FILE}
                         DEPENDS ${PBJ_PROTO_FILE} ${PBJ_DEPENDS}
                         COMMENT "Building ${PBJ_PROTO_FILE} -> ${PBJ_H_FILE} ${PBJ_CPP_FILE}")
    ENDIF()

    IF(PBJ_GENERATE_CSHARP)
      MESSAGE(STATUS "Building C# protobuf: ${FILE}")
      SET(PBJ_PBJCS_FILE ${ScriptsRoot}/csharp/protocol/${FILE}.pbj.cs)
      SET(PBJ_CS_FILE ${ScriptsRoot}/csharp/protocol/${FILE}.cs)
      SET(PBJ_CS_FILES ${PBJ_CS_FILES} ${PBJ_CS_FILE})
      SET(PBJ_OUTPUTS ${PBJ_CS_FILE} ${PBJ_PBJCS_FILE})
      SET(PBJ_ALL_OUTPUTS ${PBJ_ALL_OUTPUTS} ${PBJ_OUTPUTS})
      SET(PBJ_CS_OPTIONS ${PBJ_CS_OPTIONS} --csharp_out=${ScriptsRoot}/csharp/protocol/)
      ADD_CUSTOM_COMMAND(OUTPUT ${PBJ_CS_FILE}
                         COMMAND ${PROTOCOLBUFFERS_COMPILER} -I${PBJ_OUTPUTDIR}
                                 ${PBJ_CS_OPTIONS} ${PBJ_PROTO_FILE}
                         DEPENDS ${PBJ_PROTO_FILE} ${PBJ_DEPENDS}
                         COMMENT "Building ${PBJ_PROTO_FILE} -> ${PBJ_CS_FILE}")
    ENDIF()

    IF(PBJ_GENERATE_PYTHON)
      MESSAGE(STATUS "Building Python protobuf: ${FILE}")
      SET(PBJ_PY_FILE ${ScriptsRoot}/ironpython/protocol/${FILE}_pb2.py)
      SET(PBJ_PY_FILES ${PBJ_PY_FILES} ${PBJ_PY_FILE})
      SET(PBJ_OUTPUTS ${PBJ_PY_FILE})
      SET(PBJ_ALL_OUTPUTS ${PBJ_ALL_OUTPUTS} ${PBJ_OUTPUTS})
      SET(PBJ_PY_OPTIONS ${PBJ_PY_OPTIONS} --python_out=${ScriptsRoot}/ironpython/protocol/)
      ADD_CUSTOM_COMMAND(OUTPUT {PBJ_PY_FILE}
                         COMMAND ${PROTOCOLBUFFERS_COMPILER} -I${PBJ_OUTPUTDIR} ${PBJ_PY_OPTIONS}
                                 ${PBJ_PROTO_FILE}
                         DEPENDS ${PBJ_PROTO_FILE} ${PBJ_DEPENDS}
                         COMMENT "Building ${PBJ_PROTO_FILE} -> ${PBJ_PY_FILE}")
    ENDIF()


    SET(PBJ_NAMESPACE)
    IF(PBJ_PLUGINNAME)
       SET(PBJ_NAMESPACE --inamespace=${PBJ_PLUGINNAME})
    ENDIF()
    SET(CS_COMMAND)
    IF(PBJ_GENERATE_CSHARP)
       SET(CS_COMMAND  --cs=${PBJ_PBJCS_FILE})
    ENDIF()
    IF(PBJ_OUTPUTCPPFILE AND PBJ_GENERATE_CPP)
      FILE(APPEND ${PBJ_GenFile}
                  "#include \"${PBJ_CPP_FILE}\"\n")
    ENDIF()

    ADD_CUSTOM_COMMAND(OUTPUT ${PBJ_PBJHPP_FILE} ${PBJ_PROTO_FILE} ${PBJ_PBJCS_FILE}
                       COMMAND ${PBJ_RUNABLE} ${PBJ_PBJ_FILE} ${PBJ_PROTO_FILE} --cpp=${PBJ_PBJHPP_FILE} ${CS_COMMAND} ${PBJ_NAMESPACE}
                       DEPENDS ${PBJ_PBJ_FILE} ${PBJ_DEPENDS} ${PBJ_BINARY}
                       COMMENT "Building ${PBJ_PBJ_FILE} -> ${PBJ_PBJHPP_FILE} ${PBJ_PROTO_FILE} ${PBJ_PBJCS_FILE}")

  ENDFOREACH(FILE)

  IF(PBJ_OUTPUTCPPFILE)
    FILE(TO_NATIVE_PATH ${PBJ_GenFile} PBJ_src)
    FILE(TO_NATIVE_PATH ${PBJ_OUTPUTCPPFILE} PBJ_dst)
    IF(WIN32)
      SET(PBJ_COPYCOMMAND cmd /c type "${PBJ_src}" ">" "${PBJ_dst}")
    ELSE()
      SET(PBJ_COPYCOMMAND cp "${PBJ_src}" "${PBJ_dst}")
    ENDIF()

    ADD_CUSTOM_COMMAND(OUTPUT ${PBJ_OUTPUTCPPFILE}
                       COMMAND ${PBJ_COPYCOMMAND}
                       DEPENDS ${PBJ_CPP_FILES}
                       COMMENT "Creating protocol buffers cpp file")
  ENDIF()
  # else, this is for a scripting language

ENDMACRO(ADD_PBJ_TARGET)
