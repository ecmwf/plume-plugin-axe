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

#include "atlas/array.h"
#include "atlas/meshgenerator.h"
#include "atlas/grid.h"
#include "atlas/functionspace/NodeColumns.h"
#include "atlas/domain/Domain.h"
#include "atlas/output/Gmsh.h"

#include "area_extractor.h"


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

PluginCoreAreaExtractor::PluginCoreAreaExtractor(const eckit::Configuration& conf) : PluginCore(conf) {

    // extract user requests from the configuration file
    std::vector<eckit::LocalConfiguration> requests = conf.getSubConfigurations("requests");
    
    for (const auto& conf_req : requests) {

        std::string user = conf_req.getString("user");
        std::string s3_url = conf_req.getString("s3_url");
        std::vector<eckit::LocalConfiguration> areas = conf_req.getSubConfigurations("extraction_areas");

        UserRequest req(user, s3_url);

        for (const auto& area : areas) {
            double north = area.getDouble("north");
            double south = area.getDouble("south");
            double east = area.getDouble("east");
            double west = area.getDouble("west");
            req.add_area(north ,south ,east ,west);
        }

        requests_.push_back(req);
    }

}

PluginCoreAreaExtractor::~PluginCoreAreaExtractor() {
    if (reader_) {
        delete reader_;
        reader_ = nullptr;
    }
}

void PluginCoreAreaExtractor::setup() {

    // print requests
    for (const auto& req: requests_) {
        std::cout << req << std::endl;
    }

    // prepare the field reader
    std::vector<std::string> fieldNames = PluginAreaExtractor::requestedFields();
    std::vector<atlas::Field> fields;
    for(auto& name: fieldNames){
        fields.push_back(modelData().getAtlasFieldShared(name));
    }

    // create the reader
    reader_ = new FieldsReader{requests_, fields};
}


void PluginCoreAreaExtractor::run() {
    int timeStep = modelData().getInt("NSTEP");
    reader_->read();
    reader_->writeFile(timeStep);
};


void PluginCoreAreaExtractor::teardown() {
    // nothing to do here..
};


}  // namespace area_extractor