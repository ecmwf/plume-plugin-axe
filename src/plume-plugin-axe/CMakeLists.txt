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

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

foreach(prec_def ${PLUGINS_DEFINITIONS})

    # target name
    
    # set library name based on precision
    if (prec_def STREQUAL "WITH_PLUME_PLUGIN_AXE_SINGLE_PRECISION")
        set(PLUGIN_AXE_NAME_EXT "SP")
    else()
        set(PLUGIN_AXE_NAME_EXT "DP")
    endif()
    set(PLUGIN_AXE_LIB_NAME "plume_plugin_axe.${PLUGIN_AXE_NAME_EXT}")

    # Area Extractor Plugin
    ecbuild_add_library(
        TARGET ${PLUGIN_AXE_LIB_NAME}

        SOURCES
            area.h
            area.cc        
            plugin_utils.h
            plugin_utils.cc
            plugin_config.h
            plugin_config.cc
            area_extractor.h
            area_extractor.cc

            data_reader.h
            data_reader.cc

            data_writer.h
            data_writer.cc
            data_writer_csv.h
            data_writer_csv.cc
            data_writer_json.h
            data_writer_json.cc
            data_writer_nc.h
            data_writer_nc.cc
            
            user_request.h
            user_request.cc
            plugin_types.h
            extracted_data.h
            extracted_data.cc
        
        DEFINITIONS
            ${prec_def}
        
        PUBLIC_INCLUDES
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src/plume-plugin-axe>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/plume-plugin-axe>
        
        PRIVATE_INCLUDES
            "${eckit_INCLUDE_DIRS}"
            "${CMAKE_SOURCE_DIR}"
        
        PRIVATE_LIBS
            eckit
            eckit_mpi
            atlas
            plume_plugin
    )

endforeach()