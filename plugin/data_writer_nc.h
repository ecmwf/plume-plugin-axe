#pragma once

#include <string>
#include "data_writer.h"



namespace area_extractor {

/**
 * @brief Writes data in NetCDF format
 * 
 */
class DataWriterNETCDF : public DataWriter {
public:
    DataWriterNETCDF(const eckit::Configuration& config);
    virtual void writeData(const std::string& filename, const ExtractedData& data) const ;
    constexpr static const char* type() { return "DataWriterNETCDF"; }
};

} // namespace area_extractor
