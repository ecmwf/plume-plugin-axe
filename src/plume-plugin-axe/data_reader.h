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

#include <vector>
#include <string>

#include "atlas/field/Field.h"

#include "plugin_config.h"
#include "extracted_data.h"


namespace area_extractor {

/**
 * @brief Class that extracts values according to requests
 * 
 */
class DataReader {

    using FieldMap = std::map<std::string, atlas::Field>;

public:

    DataReader(std::vector<atlas::Field> fields);
    ~DataReader();

    // Extract data from configured requests
    ExtractedData* extractData(const PluginConfig& config);

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