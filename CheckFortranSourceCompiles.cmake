
#.rst:
# CheckFortranSourceCompiles
# --------------------------
#
# Check if given Fortran source compiles and links into an executable::
#
#   CHECK_Fortran_SOURCE_COMPILES(<code> <var> [FAIL_REGEX <fail-regex>])
#
# The arguments are:
#
# ``<code>``
#   Source code to try to compile.  It must define a PROGRAM entry point.
# ``<var>``
#   Variable to store whether the source code compiled.
#   Will be created as an internal cache variable.
# ``<fail-regex>``
#   Fail if test output matches this regex.
#
# The following variables may be set before calling this macro to modify
# the way the check is run::
#
#   CMAKE_REQUIRED_FLAGS = string of compile command line flags
#   CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
#   CMAKE_REQUIRED_INCLUDES = list of include directories
#   CMAKE_REQUIRED_LIBRARIES = list of libraries to link
#   CMAKE_REQUIRED_QUIET = execute quietly without messages
#
#=============================================================================
# CMake - Cross Platform Makefile Generator
# Copyright 2000-2014 Kitware, Inc.
# Copyright 2000-2011 Insight Software Consortium
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# * Neither the names of Kitware, Inc., the Insight Software Consortium,
# nor the names of their contributors may be used to endorse or promote
# products derived from this software without specific prior written
# permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# =============================================================================

# This file has been modified for inclusion in OpenCoarrays (www.sourceryinstitute.org)

macro(CHECK_Fortran_SOURCE_COMPILES SOURCE VAR)
  if ("${CMAKE_Fortran_COMPILER_ID}" MATCHES "GNU" )
    set(CMAKE_REQUIRED_FLAGS "-fcoarray=single")
  endif()
  if(NOT DEFINED "${VAR}")
    set(_FAIL_REGEX)
    set(_key)
    foreach(arg ${ARGN})
      if("${arg}" MATCHES "^(FAIL_REGEX)$")
        set(_key "${arg}")
      elseif(_key)
        list(APPEND _${_key} "${arg}")
      else()
        message(FATAL_ERROR "Unknown argument:\n  ${arg}\n")
      endif()
    endforeach()
    set(MACRO_CHECK_FUNCTION_DEFINITIONS
      "-D${VAR} ${CMAKE_REQUIRED_FLAGS}")
    if(CMAKE_REQUIRED_LIBRARIES)
      set(CHECK_Fortran_SOURCE_COMPILES_ADD_LIBRARIES
        LINK_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
    else()
      set(CHECK_Fortran_SOURCE_COMPILES_ADD_LIBRARIES)
    endif()
    if(CMAKE_REQUIRED_INCLUDES)
      set(CHECK_Fortran_SOURCE_COMPILES_ADD_INCLUDES
        "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_REQUIRED_INCLUDES}")
    else()
      set(CHECK_Fortran_SOURCE_COMPILES_ADD_INCLUDES)
    endif()
    file(WRITE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/src.F90"
      "${SOURCE}\n")

    if(NOT CMAKE_REQUIRED_QUIET)
      message(STATUS "Performing Test ${VAR}")
    endif()
    try_compile(${VAR}
      ${CMAKE_BINARY_DIR}
      ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/src.F90
      COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
      ${CHECK_Fortran_SOURCE_COMPILES_ADD_LIBRARIES}
      CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=${MACRO_CHECK_FUNCTION_DEFINITIONS}
      "${CHECK_Fortran_SOURCE_COMPILES_ADD_INCLUDES}"
      OUTPUT_VARIABLE OUTPUT)

    foreach(_regex ${_FAIL_REGEX})
      if("${OUTPUT}" MATCHES "${_regex}")
        set(${VAR} 0)
      endif()
    endforeach()

    if(${VAR})
      set(${VAR} 1 CACHE INTERNAL "Test ${VAR}")
      if(NOT CMAKE_REQUIRED_QUIET)
        message(STATUS "Performing Test ${VAR} - Success")
      endif()
      file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
        "Performing Fortran SOURCE FILE Test ${VAR} succeded with the following output:\n"
        "${OUTPUT}\n"
        "Source file was:\n${SOURCE}\n")
    else()
      if(NOT CMAKE_REQUIRED_QUIET)
        message(STATUS "Performing Test ${VAR} - Failed")
      endif()
      set(${VAR} "" CACHE INTERNAL "Test ${VAR}")
      file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Performing Fortran SOURCE FILE Test ${VAR} failed with the following output:\n"
        "${OUTPUT}\n"
        "Source file was:\n${SOURCE}\n")
    endif()
  endif()
endmacro()
