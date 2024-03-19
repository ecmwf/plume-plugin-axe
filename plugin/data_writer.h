#pragma once

#include "plugin_types.h"
#include "extracted_data.h"


namespace area_extractor {


// A "Stateful" data writer
class DataWriter {
public:
    DataWriter(const ExtractedData& data) : data_{data} {};
    virtual ~DataWriter(){};
    virtual void writeData(const std::string& filename) const = 0;
protected:
    const ExtractedData& data_;
};



// ------------ CSV writer ---------------
class DataWriterCSV : public DataWriter {
public:
    DataWriterCSV(const ExtractedData& data) : DataWriter(data) {};
    virtual void writeData(const std::string& filename) const ;
};
 

// ------------ COVJSON writer ---------------
class DataWriterCOVJSON : public DataWriter {
public:
    DataWriterCOVJSON(const ExtractedData& data);
    virtual void writeData(const std::string& filename) const ;
};



// ------------------------------------------------------------------------------------


} // namespace area_extractor