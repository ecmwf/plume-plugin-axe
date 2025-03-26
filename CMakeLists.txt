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

# by default, we build the plugin for double precision only
ecbuild_add_option(FEATURE PLUME_PLUGINS_SINGLE_PRECISION DESCRIPTION "Compile plugin for single precision fields" DEFAULT OFF)
ecbuild_add_option(FEATURE PLUME_PLUGINS_DOUBLE_PRECISION DESCRIPTION "Compile plugin for double precision fields" DEFAULT ON)

# this option can also be used to build the plugin for single precision (when the plugin is built in a ifs-bundle)
ecbuild_add_option(FEATURE SINGLE_PRECISION DESCRIPTION "Compile plugin for single precision fields" DEFAULT OFF)

# Definitions
set(PLUGINS_DEFINITIONS "")

if (HAVE_PLUME_PLUGINS_SINGLE_PRECISION OR HAVE_SINGLE_PRECISION)
  list(APPEND PLUGINS_DEFINITIONS WITH_PLUME_PLUGIN_AXE_SINGLE_PRECISION )
endif()

if (HAVE_PLUME_PLUGINS_DOUBLE_PRECISION)
  list(APPEND PLUGINS_DEFINITIONS WITH_PLUME_PLUGIN_AXE_DOUBLE_PRECISION )
endif()

## Sources
add_subdirectory(src)

## Tests
add_subdirectory(tests)

# finalize
ecbuild_install_project( NAME ${PROJECT_NAME} )
ecbuild_print_summary()

