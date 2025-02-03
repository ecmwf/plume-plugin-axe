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


REGISTER_LIBRARY(AreaExtractor)

AreaExtractor::AreaExtractor() : Plugin("AreaExtractor"){};

const AreaExtractor& AreaExtractor::instance() {
    static AreaExtractor instance;
    return instance;
}

// AreaExtractorCore
static plume::PluginCoreBuilder<AreaExtractorCore> pluginCorelBuilderAreaExtractor;


AreaExtractorCore::AreaExtractorCore(const eckit::Configuration& conf) : 
    PluginCore(conf), 
    config_{conf, AreaExtractor::requestedFields()},
    runsEvery_{conf.getInt("runs_every", 1)} {

}


AreaExtractorCore::~AreaExtractorCore() {}

void AreaExtractorCore::setup() {

    // print requests
    if (eckit::mpi::comm().rank() == 0) {
        std::cout << config_ << std::endl;
    }

    // prepare the field reader
    std::vector<std::string> fieldNames = AreaExtractor::requestedFields();
    std::vector<atlas::Field> fields;
    for(auto& name: fieldNames){
        fields.push_back(modelData().getAtlasFieldShared(name));
    }

    // Create the reader
    reader_.reset( new DataReader{fields} );

    // Extract data
    data_.reset( reader_->extractData(config_) );

    // Construct the data writer
    writer_.reset( DataWriterFactory::instance().build(config_.outputStrategy()) );
}


void AreaExtractorCore::run() {

    int procID = eckit::mpi::comm().rank();
    int timeStep = modelData().getInt("NSTEP");

    // Check if we should run
    if (timeStep % runsEvery_ != 0) {
        return;
    }

    std::stringstream ss;
    ss << "extracted-areas-step" << timeStep << "-proc" << procID;
    std::string filename{ss.str()};

    // check if the envaronment variable PLUME_PLUGINS_OUTPUT_DIR is set,
    // is so, prepend it to the filename
    const char* outputDir = std::getenv("PLUME_PLUGINS_OUTPUT_DIR");
    if (outputDir) {
        filename = std::string(outputDir) + "/" + filename;
    }
            
    // Update data
    reader_->updateData(*data_);

    // Write data
    writer_->writeData(filename, *data_);
};


void AreaExtractorCore::teardown() {
    // nothing to do here..
};


}  // namespace area_extractor