#Sirikata
#AddPBJTarget.cmake
#
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


# AddPBJTarget.cmake
# ADD_PBJ_TARGET - sets up targets to compile pbj -> idl -> cpp.
# Note: Currently expects PROTOCOLBUFFERS_COMPILER to be defined.
# Note: Currently expects PROTOCOLBUFFERS_MONO_COMPILER to be defined if PBJ_GENERATE_CSHARP active.
# Note: Currently expects PBJ_RUNABLE to be defined.
#
# Takes a list of files .
# The following parameters are available:
# DEPENDS - Files or targets this target should depend on
# IMPORTS - Directory to find files that need to be imported, e.g. other pbj files that are dependend on
# PLUGINNAME -
# OUTPUTDIR - The directory to output generated files to
# INPUTDIR - The directory to find input files in
# GENERATED_CPP_FILES - name of variable to store list of generated C++ files in
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
  PARSE_ARGUMENTS(PBJ "DEPENDS;IMPORTS;PLUGINNAME;OUTPUTDIR;CSHARP_OUTPUTDIR;PYTHON_OUTPUTDIR;INPUTDIR;GENERATED_CPP_FILES;CPP_HEADER" "GENERATE_CPP;GENERATE_CSHARP;GENERATE_PYTHON" ${ARGN})
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

  SET(INTERNAL_GENERATED_CPP_FILES)
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

    SET(PBJ_PBJCS_FILE)

    SET(PBJ_PBJ_FILE ${PBJ_INPUTDIR}/${FILE}.pbj)
    SET(PBJ_PROTO_FILE ${PBJ_OUTPUTDIR}/${PBJ_PLUGINNAME_UNDERSCORE}${FILE}.proto)
    SET(PBJ_PROTO_FILES ${PBJ_PROTO_FILES} ${PBJ_PROTO_FILE})
    SET(PBJ_PBJ_FILES ${PBJ_PBJ_FILES} ${PBJ_PBJ_FILE})
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
      SET(PBJ_PBJCS_FILE ${PBJ_CSHARP_OUTPUTDIR}/${FILE}.pbj.cs)
      SET(PBJ_PBJCS_FILES ${PBJ_PBJCS_FILES} ${PBJ_PBJCS_FILE})
      SET(PBJ_CS_FILE ${PBJ_CSHARP_OUTPUTDIR}/${FILE}.cs)
      SET(PBJ_PROTOBIN_FILE ${PBJ_CSHARP_OUTPUTDIR}/${FILE}.protobin)
      SET(PBJ_CS_FILES ${PBJ_CS_FILES} ${PBJ_CS_FILE})
      SET(PBJ_OUTPUTS ${PBJ_CS_FILE} ${PBJ_PBJCS_FILE})
      SET(PBJ_ALL_OUTPUTS ${PBJ_ALL_OUTPUTS} ${PBJ_OUTPUTS})
      SET(PBJ_CS_OPTIONS --descriptor_set_out=${PBJ_PROTOBIN_FILE})
      ADD_CUSTOM_COMMAND(OUTPUT ${PBJ_PROTOBIN_FILE}
                         COMMAND ${PROTOCOLBUFFERS_COMPILER} -I${PBJ_OUTPUTDIR}
                                 ${PBJ_CS_OPTIONS} ${PBJ_PROTO_FILE}
                         DEPENDS ${PBJ_PROTO_FILE} ${PBJ_DEPENDS}
                         COMMENT "Building ${PBJ_PROTO_FILE} -> ${PBJ_PROTOBIN_FILE}")
      ADD_CUSTOM_COMMAND(OUTPUT ${FILE}.cs
                         COMMAND ${MONO_EXECUTABLE} ${PROTOCOLBUFFERS_MONO_COMPILER} ${PBJ_PROTOBIN_FILE}
                         DEPENDS ${PBJ_PROTOBIN_FILE} ${PBJ_PROTO_FILE} ${PBJ_DEPENDS}
                         COMMENT "Building ${PBJ_PROTOBIN_FILE} -> ${FILE}.cs")
      ADD_CUSTOM_COMMAND(OUTPUT ${PBJ_CS_FILE}
                         COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FILE}.cs ${PBJ_CS_FILE}
                         DEPENDS ${FILE}.cs ${PBJ_PROTOBIN_FILE} ${PBJ_PROTO_FILE} ${PBJ_DEPENDS}
                         COMMENT "Final copy of generated C# file to ${PBJ_CS_FILE}")

    ENDIF()

    IF(PBJ_GENERATE_PYTHON)
      MESSAGE(STATUS "Building Python protobuf: ${FILE}")
      SET(PBJ_PY_FILE ${PBJ_PYTHON_OUTPUTDIR}/${FILE}_pb2.py)
      SET(PBJ_PY_FILES ${PBJ_PY_FILES} ${PBJ_PY_FILE})
      SET(PBJ_OUTPUTS ${PBJ_PY_FILE})
      SET(PBJ_ALL_OUTPUTS ${PBJ_ALL_OUTPUTS} ${PBJ_OUTPUTS})
      SET(PBJ_PY_OPTIONS ${PBJ_PY_OPTIONS} --python_out=${PBJ_PYTHON_OUTPUTDIR}/)
      ADD_CUSTOM_COMMAND(OUTPUT ${PBJ_PY_FILE}
                         COMMAND ${PROTOCOLBUFFERS_COMPILER} -I${PBJ_OUTPUTDIR} ${PBJ_PY_OPTIONS}
                                 ${PBJ_PROTO_FILE}
                         DEPENDS ${PBJ_PROTO_FILE} ${PBJ_DEPENDS}
                         COMMENT "Building ${PBJ_PROTO_FILE} -> ${PBJ_PY_FILE}")
    ENDIF()


    SET(PBJ_NAMESPACE)
    IF(PBJ_PLUGINNAME)
       SET(PBJ_NAMESPACE --inamespace=${PBJ_PLUGINNAME})
    ENDIF()
    SET(CS_COMMAND --cs=temp.cs)
    IF(PBJ_GENERATE_CSHARP)
       SET(CS_COMMAND  --cs=${PBJ_PBJCS_FILE})
    ENDIF()
    SET(INTERNAL_GENERATED_CPP_FILES ${INTERNAL_GENERATED_CPP_FILES} ${PBJ_CPP_FILE})

    ADD_CUSTOM_COMMAND(OUTPUT ${PBJ_PBJHPP_FILE} ${PBJ_PROTO_FILE} ${PBJ_PBJCS_FILE}
                       COMMAND ${PBJ_RUNABLE} ${PBJ_PBJ_FILE} ${PBJ_PROTO_FILE} --cpp=${PBJ_PBJHPP_FILE} ${CS_COMMAND} ${PBJ_NAMESPACE}
                       DEPENDS ${PBJ_PBJ_FILE} ${PBJ_DEPENDS} ${PBJ_BINARY}
                       COMMENT "Building ${PBJ_PBJ_FILE} -> ${PBJ_PBJHPP_FILE} ${PBJ_PROTO_FILE} ${PBJ_PBJCS_FILE}")

  ENDFOREACH()

  # If requested, store the generated C++ files into a variable
  IF(PBJ_GENERATED_CPP_FILES)
    SET(${PBJ_GENERATED_CPP_FILES} ${INTERNAL_GENERATED_CPP_FILES})
  ENDIF()

ENDMACRO(ADD_PBJ_TARGET)
