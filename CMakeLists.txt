# 
#  (C) Copyright 2023- ECMWF.
# 
#  This software is licensed under the terms of the Apache Licence Version 2.0
#  which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# 
#  In applying this licence, ECMWF does not waive the privileges and immunities
#  granted to it by virtue of its status as an intergovernmental organisation nor
#  does it submit to any jurisdiction.
# 

cmake_minimum_required( VERSION 3.16 FATAL_ERROR )

find_package( ecbuild 3.5 REQUIRED HINTS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../ecbuild)

project( plume_plugin_axe VERSION 0.1.0 LANGUAGES C CXX Fortran)

ecbuild_enable_fortran( REQUIRED MODULE_DIRECTORY ${CMAKE_BINARY_DIR}/module )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

ecbuild_find_package( NAME eckit  VERSION  1.18   REQUIRED )
ecbuild_find_package( NAME atlas  VERSION  0.32   REQUIRED )
ecbuild_find_package( NAME plume  VERSION  0.1.0  REQUIRED )               

ecbuild_add_option(FEATURE PLUME_PLUGINS_SINGLE_PRECISION DESCRIPTION "Single precision Atlas fields" DEFAULT OFF)

if( HAVE_PLUME_PLUGIN_AXE_SINGLE_PRECISION ) 
  list(APPEND PLUGINS_DEFINITIONS WITH_PLUME_PLUGIN_AXE_SINGLE_PRECISION )
endif()

# Append the correct extension (SP or DP)
if (HAVE_PLUME_PLUGIN_AXE_SINGLE_PRECISION)
    set(PLUGIN_AXE_NAME_EXT "SP")
else()
    set(PLUGIN_AXE_NAME_EXT "DP")
endif()

set(PLUGIN_AXE_LIB_NAME "plume_plugin_axe_${PLUGIN_AXE_NAME_EXT}")


## Sources
add_subdirectory(src)

## Tests
add_subdirectory(tests)

# finalize
ecbuild_install_project( NAME ${PROJECT_NAME} )
ecbuild_print_summary()

