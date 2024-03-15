#pragma once

#include <vector>
#include <string>

#include "atlas/field/Field.h"

#include "user_request.h"
#include "extracted_data.h"


namespace area_extractor {

/**
 * @brief Class that extracts values according to requests
 * 
 */
class FieldsReader {

    using FieldMap = std::map<std::string, atlas::Field>;

public:

    FieldsReader(std::vector<atlas::Field> fields);
    ~FieldsReader();

    // Extract data from requests
    ExtractedData* extractData(const std::vector<UserRequest>& requests);

    // update data
    void updateData(ExtractedData& data);

private:

    // pointers to fields
    std::vector<atlas::Field> fields_;
    atlas::Field lonlats_;
    atlas::Field ghost_;
    FieldMap fieldViewMap_;

};

} // namespace area_extractor