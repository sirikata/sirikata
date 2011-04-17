#Sirikata
#GitRevision.cmake
#
#Copyright (c) 2011, Ewen Cheslack-Postava
#
# Based on GetGitRevisionDescription.cmake
#  https://github.com/rpavlik/cmake-modules/blob/master/GetGitRevisionDescription.cmake
#
# Original Author:
# 2009-2010 Ryan Pavlik <rpavlik@iastate.edu> <abiryan@ryand.net>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC
#
# Copyright Iowa State University 2009-2010.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

function(GetGitHeadRef _git_head_result _root_dir)
  set(GIT_DIR "${_root_dir}/.git")
  if(NOT EXISTS "${GIT_DIR}")
    # not in git
    set(${_git_head_result} "-1" PARENT_SCOPE)
    return()
  endif()

  file(READ ${GIT_DIR}/HEAD GIT_HEAD_FULL)
  string(REPLACE "ref: " "" GIT_HEAD_REF ${GIT_HEAD_FULL})
  string(STRIP ${GIT_HEAD_REF} GIT_HEAD_REF)
  set(${_git_head_result} "${GIT_HEAD_REF}" PARENT_SCOPE)
endfunction()

function(GetGitRevision _git_rev_result _root_dir _git_ref)
  set(GIT_DIR "${_root_dir}/.git")
  if(NOT EXISTS "${GIT_DIR}")
    # not in git
    set(${_git_rev_result} "-1" PARENT_SCOPE)
    return()
  endif()

  set(GIT_REF_FILE ${GIT_DIR}/${_git_ref})
  GET_FILENAME_COMPONENT(GIT_REF_FILE ${GIT_REF_FILE} ABSOLUTE)
  file(READ ${GIT_REF_FILE} GIT_REF)
  string(STRIP ${GIT_REF} GIT_REF)

  set(${_git_rev_result} "${GIT_REF}" PARENT_SCOPE)
endfunction()
