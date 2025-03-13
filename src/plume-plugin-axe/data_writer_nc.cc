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