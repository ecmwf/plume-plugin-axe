/*
 * (C) Copyright 2023- ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#include <cstring>
#include "plugin_utils.h"


namespace area_extractor {

/**
 * Utility function to copy name into char[]
*/
void copy_name(char* dest, const char* src, size_t dest_size) {
    strncpy(dest, src, dest_size-1 );
    if(dest_size>0){
        dest[dest_size-1]=0;
    }
}

// from string to char vec
int append_string(std::vector<char>& arr, const std::string& str, int buff_len) {

    if ((str.size() >= buff_len) || (str.size() == 0)) {
        return -1;
    }

    int arr_size_init = arr.size();

    // copy the string
    arr.reserve(arr_size_init+buff_len);
    std::copy(str.begin(), str.end(), std::back_inserter(arr));
    arr.push_back('\0');
    arr.resize(arr_size_init+buff_len);
    return 0;
}


} // namespace area_extractor