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

#include <algorithm>
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


// AreaExtractorCore
static plume::PluginCoreBuilder<AreaExtractorCore> pluginCorelBuilderAreaExtractor;


AreaExtractorCore::AreaExtractorCore(const eckit::Configuration& conf) : 
    PluginCore(conf),
    coreConfig_{conf},
    runsEvery_{conf.getInt("runs_every", 1)} {

}


AreaExtractorCore::~AreaExtractorCore() {}

void AreaExtractorCore::setup() {

    // setup the specific plugin config that includes the list of parameters available in the data
    config_ = std::make_unique<PluginConfig>(coreConfig_, modelData().listAvailableParameters("ATLAS_FIELD"));

    // print requests
    eckit::Log::info() << "AreaExtractorCore::setup() - requests: " << *config_ << std::endl;

    // prepare the field reader
    std::vector<std::string> fieldNames = config_->parameters();
    std::vector<plume::data::FieldView> fields;
    for(auto& name: fieldNames){
        fields.push_back(modelData().getParam<atlas::Field>(name));
    }

    // Create the reader
    reader_ = std::make_unique<DataReader>(fields);

    // check that fields/params needed by runtime areas are available
    checkAreaRequirements();

    // Initialise the runtime areas and extract data (areas evaluated at the current model step)
    int timeStep = modelData().getParam<int>("NSTEP");
    reader_->updateAreas(*config_, timeStep, this->timeStep());
    data_.reset( reader_->extractData(*config_, timeStep) );

    // Construct the data writer
    writer_.reset( DataWriterFactory::instance().build(config_->outputStrategy()) );
}


void AreaExtractorCore::run() {

    int procID = eckit::mpi::comm().rank();
    int timeStep = modelData().getParam<int>("NSTEP");

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
            
    // Update data (move the runtime areas, then re-select the points if any area changes with the time step)
    if (config_->hasTimeDependentAreas()) {
        reader_->updateAreas(*config_, timeStep, this->timeStep());
        data_.reset( reader_->extractData(*config_, timeStep) );
    } else {
        reader_->updateData(*data_);
    }

    // Write data
    writer_->writeData(filename, *data_);
};


void AreaExtractorCore::teardown() {
    // nothing to do here..
};


std::optional<double> AreaExtractorCore::timeStep() {
    if (!config_->requiresTimeStep()) {
        return std::nullopt;
    }
    return modelData().getParam<double>("TSTEP");
}


void AreaExtractorCore::checkAreaRequirements() {

    const auto& params = config_->parameters();
    for (const auto& field : config_->requiredFields()) {
        if (std::find(params.begin(), params.end(), field) == params.end()) {
            throw eckit::UserError("AreaExtractorCore::setup: field '" + field + "' is required by an extraction area "
                                   "but is not available => request it in the plugin \"parameters\" "
                                   "(e.g. {\"name\": \"u\", \"type\": \"ATLAS_FIELD\", \"height\": 5000} for \"u;hl;5000\")", Here());
        }
    }

    if (config_->requiresTimeStep() && !modelData().hasParameter("TSTEP")) {
        throw eckit::UserError("AreaExtractorCore::setup: TSTEP is required by an extraction area "
                               "but is not available => request it in the plugin \"parameters\" "
                               "({\"name\": \"TSTEP\", \"type\": \"DOUBLE\"})", Here());
    }
}


REGISTER_LIBRARY(AreaExtractor)

AreaExtractor::AreaExtractor() : Plugin("AreaExtractor"){};

const AreaExtractor& AreaExtractor::instance() {
    static AreaExtractor instance;
    return instance;
}


}  // namespace area_extractor