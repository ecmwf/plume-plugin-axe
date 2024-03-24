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
    config_{conf, PluginAreaExtractor::requestedFields()} {

}


PluginCoreAreaExtractor::~PluginCoreAreaExtractor() {}

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
    reader_.reset( new DataReader{fields} );

    // Extract data
    data_.reset( reader_->extractData(config_) );

    // Construct the data writer
    // writer_.reset( new DataWriterCSV{} );
    writer_.reset( new DataWriterCOVJSON{} );
}


void PluginCoreAreaExtractor::run() {

    int timeStep = modelData().getInt("NSTEP");
    int procID = eckit::mpi::comm().rank();

    std::stringstream ss;
    ss << "extracted-areas-step" << timeStep << "-proc" << procID;
    std::string filename{ss.str()};
            
    // Update data
    reader_->updateData(*data_);

    // Write data
    globaldata_.reset(data_->gather(0));
    writer_->writeData(filename, *globaldata_);

    // // Write data
    // writer_->writeData(filename, *data_);


};


void PluginCoreAreaExtractor::teardown() {
    // nothing to do here..
};


}  // namespace area_extractor