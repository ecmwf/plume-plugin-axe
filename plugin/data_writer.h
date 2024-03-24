#pragma once

#include "plugin_types.h"
#include "extracted_data.h"

#include "eckit/config/YAMLConfiguration.h"


namespace area_extractor {


// A "Stateful" data writer
class DataWriter {
public:
    DataWriter(){};
    virtual ~DataWriter(){};
    virtual void writeData(const std::string& filename, const ExtractedData& data) const = 0;
};



// ------------ CSV writer ---------------
class DataWriterCSV : public DataWriter {
public:
    DataWriterCSV() : DataWriter() {};
    virtual void writeData(const std::string& filename, const ExtractedData& data) const ;
};
 

// ------------ COVJSON writer ---------------
class DataWriterCOVJSON : public DataWriter {
public:
    DataWriterCOVJSON();
    virtual void writeData(const std::string& filename, const ExtractedData& data) const ;

private:

    std::string assembleCOVJSON(const std::string& user, const std::vector<int>& areas, const ExtractedData& data, int& indent) const ;
    std::string assembleCoverages(const std::string& user, const std::vector<int>& areas, const ExtractedData& data, int& indent) const ;
};


// ------------ NetCDF writer ---------------
class DataWriterNETCDF : public DataWriter {
public:
    DataWriterNETCDF();
    virtual void writeData(const std::string& filename, const ExtractedData& data) const ;
};


// ------------------------------------------------------------------------------------


} // namespace area_extractor