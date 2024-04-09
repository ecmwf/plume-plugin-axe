
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
#pragma once

#include <string>
#include <iostream>
#include <vector>

namespace area_extractor {

/**
 * Utility function to copy name into char[]
*/
void copy_name(char* dest, const char* src, size_t dest_size);

// from string to char vec
int append_string(std::vector<char>& arr, const std::string& str, int buff_len);

// simply pring a vector (for debugging)
template<typename T>
void printvec(const std::string& name, std::vector<T> vec) {
    for (const auto& elem : vec) {
        std::cout << elem << std::endl;
    }
}

} // namespace area_extractor