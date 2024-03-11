#pragma once

#include <vector>
#include <string>

#include "atlas/field/Field.h"

#include "plugin_types.h"
#include "user_request.h"


namespace area_extractor {

/**
 * @brief Class that extracts values according to requests
 * 
 */
class FieldsReader {
public:

    FieldsReader(std::vector<UserRequest> requests, std::vector<atlas::Field> fields);
    ~FieldsReader();

    // Read the fields
    void read();

    void writeFile(std::string filename);

    bool ownsValues();

private:

    void setupStorage(const std::vector<UserRequest>& requests);

private:

    // requested Fields
    std::vector<atlas::Field> fields_;

    // field of lonlats
    atlas::Field lonlats_;

    // ghost mask
    atlas::Field ghost_;

    // internal storage
    int numProcValues_;
    std::vector<std::string> users_;
    std::vector<int> area_idxs_;
    std::vector<int> point_idxs_;
    std::vector<double> lats_;
    std::vector<double> lons_;
    std::vector<int> levs_;
    std::vector<std::string> params_;
    std::vector<FIELD_TYPE_REAL> values_;

};

} // namespace area_extractor