
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

    // std::cout << "******************" << std::endl;
    // std::cout << "arr.size(): " << arr.size() << std::endl;
    // std::cout << "buff_len: " << buff_len << std::endl;
    // std::cout << "str.size(): " << str.size() << std::endl;
    
    // copy the string
    arr.reserve(arr_size_init+buff_len);
    std::copy(str.begin(), str.end(), std::back_inserter(arr));
    arr.push_back('\0');
    arr.resize(arr_size_init+buff_len);

    // std::cout << "arr.data()+arr_size_init: " << arr.data()+arr_size_init << std::endl;
    // std::cout << "arr.size(): " << arr.size() << std::endl;

    // resize the array to multiple of buff_len
    
    // std::cout << "arr.size(): " << arr.size() << std::endl;
    // std::cout << "******************" << std::endl;

    return 0;
}


} // namespace area_extractor