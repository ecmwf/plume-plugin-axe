#pragma once

#include "extracted_data.h"


namespace area_extractor {


class DataWriter {

public:

    virtual void write(const std::string& filename, const ExtractedData& data) const = 0;
};



class DataWriterCSV : public DataWriter {

public:

    virtual void write(const std::string& filename, const ExtractedData& data) const ;
};


} // namespace area_extractor