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


#include "eckit/log/Log.h"

#include "atlas/array.h"
#include "atlas/meshgenerator.h"
#include "atlas/grid.h"
#include "atlas/functionspace/NodeColumns.h"
#include "atlas/domain/Domain.h"
#include "atlas/output/Gmsh.h"

#include "plugin_types.h"
#include "area_extractor.h"


namespace area_extractor {




// ==========================================================================================
ExtractionArea::ExtractionArea(double north, double south, double east, double west) : 
    north_{north},
    south_{south},
    east_{east},
    west_{west} {
}

ExtractionArea::~ExtractionArea(){
}


std::ostream& operator<<(std::ostream& ss, const ExtractionArea& obj) {
    ss << "(north:" << obj.north_ << ",";
    ss << "south:" << obj.south_ << ",";
    ss << "east:" << obj.east_ << ",";
    ss << "west:" << obj.west_ << ")";
    return ss;
}
// ==========================================================================================


// ==========================================================================================
std::ostream& operator<<(std::ostream& ss, const UserRequest& obj) {
    ss << "{user: " << obj.user_ << ", s3_url: " << obj.s3_url_;
    ss << ", areas: [";

    int cc = 0;
    for (const auto& area : obj.areas_) {
        ss << area;
        if (cc != obj.areas_.size()-1){
            ss << ",";
        }
        cc++;
    }
    ss << "]";
    ss << "}";
    return ss;
}
// ==========================================================================================


// ==========================================================================================
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

        std::string user_id = conf_req.getString("user_id");
        std::string s3_url = conf_req.getString("s3_url");
        std::vector<eckit::LocalConfiguration> areas = conf_req.getSubConfigurations("extraction_areas");

        UserRequest req(user_id, s3_url);

        for (const auto& area : areas) {

            double north = area.getDouble("north");
            double south = area.getDouble("south");
            double east = area.getDouble("east");
            double west = area.getDouble("west");

            req.push_area(ExtractionArea{north ,south ,east ,west});
        }

        requests_.push_back(req);
    }


    // print requests
    for (const auto& req: requests_) {
        std::cout << req << std::endl;
    }


}

PluginCoreAreaExtractor::~PluginCoreAreaExtractor() {}

void PluginCoreAreaExtractor::setup() {
    fieldU_ = modelData().getAtlasFieldShared("u");
    fieldV_ = modelData().getAtlasFieldShared("v");

    // field function space
    inputFuncSpace_ = fieldU_.functionspace();
    atlas::Grid inputGrid = inputFuncSpace_.grid();
    atlas::idx_t levels = inputFuncSpace_.levels();

    // // target mesh and functional space
    // atlas::RectangularLonLatDomain rd({boundingBoxWest_,boundingBoxEast_}, {boundingBoxSouth_,boundingBoxNorth_});
    // atlas::Grid areaGrid(inputGrid, rd); 

    // atlas::MeshGenerator outputMeshGen_("structured");

    // eckit::Log::info() << "Generating marea mesh in box =>> "
    //                    << "  boundingBoxWest_: " << boundingBoxWest_
    //                    << ", boundingBoxEast_: " << boundingBoxEast_
    //                    << ", boundingBoxSouth_: " << boundingBoxSouth_
    //                    << ", boundingBoxNorth_: " << boundingBoxNorth_
    //                    << std::endl;

    // outputMesh_ = outputMeshGen_.generate(areaGrid, atlas::grid::MatchingPartitioner(inputFuncSpace_));
    // outputFuncSpace_ = atlas::functionspace::NodeColumns{outputMesh_};

    // // define target field
    // outputField_ = outputFuncSpace_.createField<FIELD_TYPE_REAL>(atlas::option::name("target") | atlas::option::levels(levels) );
}


void PluginCoreAreaExtractor::run() {

    // // setup interpolation
    // atlas::util::Config scheme;
    // scheme.set("type", "structured-linear2D");
    // atlas::Interpolation interpolation(scheme, inputFuncSpace_, outputFuncSpace_);

    // // execute interpolation
    // interpolation.execute(fieldU_, outputField_);

    // // write out to gmsh for testing/debugging
    // atlas::output::Gmsh gmshSrc("source.msh");
    // atlas::MeshGenerator meshgenSrc("structured");
    // atlas::Mesh meshSrc = meshgenSrc.generate(inputFuncSpace_.grid());
    // gmshSrc.write(meshSrc);
    // gmshSrc.write(fieldU_);

    // // interpolated mesh and field (within lat/lon domain)
    // atlas::output::Gmsh gmsh("interpolated.msh");
    // gmsh.write(outputMesh_);
    // gmsh.write(outputField_);

};


void PluginCoreAreaExtractor::teardown() {
    // nothing to do here..
};
// ==========================================================================================


}  // namespace area_extractor