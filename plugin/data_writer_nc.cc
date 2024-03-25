
#include "eckit/exception/Exceptions.h"
#include "data_writer_nc.h"



namespace area_extractor {


// Static instantiation of DataWriterNETCDF builder
static DataWriterBuilder<DataWriterNETCDF> writerNETCDF;


DataWriterNETCDF::DataWriterNETCDF(const eckit::Configuration& config) : DataWriter(config) {}

void DataWriterNETCDF::writeData(const std::string& filename, const ExtractedData& data) const {
    NOTIMP;
}


} // namespace area_extractor