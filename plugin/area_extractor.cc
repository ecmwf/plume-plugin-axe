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

#include <iostream>
#include <fstream>
#include <sstream>

#include "eckit/log/Log.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/mpi/Comm.h"

#include "atlas/array.h"
#include "atlas/meshgenerator.h"
#include "atlas/grid.h"
#include "atlas/functionspace/NodeColumns.h"
#include "atlas/domain/Domain.h"
#include "atlas/output/Gmsh.h"

#include "area_extractor.h"
#include "extracted_data.h"


namespace area_extractor {


REGISTER_LIBRARY(PluginAreaExtractor)

PluginAreaExtractor::PluginAreaExtractor() : Plugin("PluginAreaExtractor"){};

PluginAreaExtractor::~PluginAreaExtractor(){};

const PluginAreaExtractor& PluginAreaExtractor::instance() {
    static PluginAreaExtractor instance;
    return instance;
}

// PluginCoreAreaExtractor
static plume::PluginCoreBuilder<PluginCoreAreaExtractor> pluginCorelBuilderAreaProbe;


PluginCoreAreaExtractor::PluginCoreAreaExtractor(const eckit::Configuration& conf) : 
    PluginCore(conf), 
    config_{conf, PluginAreaExtractor::requestedFields()}, 
    reader_{nullptr}, 
    writer_{nullptr}{}


PluginCoreAreaExtractor::~PluginCoreAreaExtractor() {

    if (reader_) {
        delete reader_;
        reader_ = nullptr;
    }

    if (data_) {
        delete data_;
        data_ = nullptr;
    }

    if (writer_) {
        delete writer_;
        writer_ = nullptr;
    }        
}

void PluginCoreAreaExtractor::setup() {

    // print requests
    if (eckit::mpi::comm().rank() == 0) {
        std::cout << config_ << std::endl;
    }

    // prepare the field reader
    std::vector<std::string> fieldNames = PluginAreaExtractor::requestedFields();
    std::vector<atlas::Field> fields;
    for(auto& name: fieldNames){
        fields.push_back(modelData().getAtlasFieldShared(name));
    }

    // Create the reader
    reader_ = new DataReader{fields};

    // Extract data
    data_ = reader_->extractData(config_);

    // Construct the data writer
    writer_ = new DataWriterCSV{*data_};
    // writer_ = new DataWriterCOVJSON{*data_};
}


void PluginCoreAreaExtractor::run() {

    int timeStep = modelData().getInt("NSTEP");
    int procID = eckit::mpi::comm().rank();

    std::stringstream ss;
    ss << "extracted-areas-step" << timeStep << "-proc" << procID <<  ".csv";
    std::string filename{ss.str()};
            
    // Update data
    reader_->updateData(*data_);

    // Write data
    writer_->writeData(filename);

    ExtractedData global_data = data_->gather(0);

    // // filter data according to username
    // data_->filter(ExtractedData::Filter::USER, "user-1");

};


void PluginCoreAreaExtractor::teardown() {
    // nothing to do here..
};


}  // namespace area_extractor