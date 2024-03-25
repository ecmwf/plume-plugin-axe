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
    constexpr static const char* type() { return "DataWriterCSV"; }
};

} // namespace area_extractor
