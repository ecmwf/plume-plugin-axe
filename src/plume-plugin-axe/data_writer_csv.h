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
#include "data_writer.h"



namespace area_extractor {

/**
 * @brief Writes data in CSV format (one CSV file per process 
 * NB: only processes actually containing area data would write CSV files
 * 
 */
class DataWriterCSV : public DataWriter {
public:
    DataWriterCSV(const eckit::Configuration& config);
    virtual void writeData(const std::string& filename, const ExtractedData& data) const ;
    constexpr static const char* type() { return "CSV"; }
};

} // namespace area_extractor
