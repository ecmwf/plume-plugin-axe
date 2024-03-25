#pragma once

#include <string>
#include <vector>
#include "data_writer.h"



namespace area_extractor {

/**
 * @brief Write a unique CovJSON file
 * 
 */
class DataWriterCOVJSON : public DataWriter {
public:
    DataWriterCOVJSON(const eckit::Configuration& config);
    virtual void writeData(const std::string& filename, const ExtractedData& data) const ;
    constexpr static const char* type() { return "DataWriterCOVJSON"; }

private:

    std::string assembleCOVJSON(const std::string& user, const std::vector<int>& areas, const ExtractedData& data, int& indent) const ;
    std::string assembleCoverages(const std::string& user, const std::vector<int>& areas, const ExtractedData& data, int& indent) const ;
};

} // namespace area_extractor