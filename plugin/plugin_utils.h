
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
    std::cout << "--> " << name << ": " << std::endl;
    for (const auto& elem : vec) {
        std::cout << elem << std::endl;
    }
}

} // namespace area_extractor