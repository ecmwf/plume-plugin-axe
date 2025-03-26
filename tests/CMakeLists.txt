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

foreach(prec_def ${PLUGINS_DEFINITIONS})

    # set library name based on precision
    if (prec_def STREQUAL "WITH_PLUME_PLUGIN_AXE_SINGLE_PRECISION")
        set(PLUGIN_AXE_NAME_EXT "SP")
    else()
        set(PLUGIN_AXE_NAME_EXT "DP")
    endif()
    set(PLUGIN_AXE_LIB_NAME "plume_plugin_axe.${PLUGIN_AXE_NAME_EXT}")
    
    ecbuild_add_test( TARGET plugin_axe_test_area_${PLUGIN_AXE_NAME_EXT}
                      SOURCES
                        test_area.cc
                      DEFINITIONS
                        ${prec_def}
                      LIBS
                        ${PLUGIN_AXE_LIB_NAME}
                        eckit
                        plume_plugin
    )

    ecbuild_add_test( TARGET plugin_axe_test_reader_${PLUGIN_AXE_NAME_EXT}
                      SOURCES
                        test_reader.cc
                      DEFINITIONS
                        ${prec_def}
                      LIBS
                        ${PLUGIN_AXE_LIB_NAME}
                        eckit
                        atlas
    )

    ecbuild_add_test( TARGET plugin_axe_test_config_${PLUGIN_AXE_NAME_EXT}
                      SOURCES
                        test_config.cc
                      DEFINITIONS
                        ${prec_def}
                      LIBS
                        ${PLUGIN_AXE_LIB_NAME}
                        eckit
    )

    ecbuild_add_test( TARGET plugin_axe_test_request_${PLUGIN_AXE_NAME_EXT}
                      SOURCES
                        test_request.cc
                      DEFINITIONS
                        ${prec_def}
                      LIBS
                        ${PLUGIN_AXE_LIB_NAME}
                        eckit
    )

endforeach()