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




// ==========================================================================================
ExtractionArea::ExtractionArea(int area_id,  double north, double south, double east, double west) :
    area_id_{area_id}, north_{north}, south_{south}, east_{east}, west_{west} {
}

ExtractionArea::~ExtractionArea(){
}


std::ostream& operator<<(std::ostream& ss, const ExtractionArea& obj) {
    ss << "{id:" << obj.area_id_ << ",";
    ss << "bbox:(north:" << obj.north_ << ",";
    ss << "south:" << obj.south_ << ",";
    ss << "east:" << obj.east_ << ",";
    ss << "west:" << obj.west_ << ")";
    ss << "}";
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
FieldsReader::FieldsReader(std::vector<UserRequest> requests, std::vector<atlas::Field> fields) : 
    fields_{fields},
    numProcValues_{0} {

    lonlats_ = fields_[0].functionspace().lonlat();

    // here we go through the requests and find points to extract..
    setupStorage(requests);

}
FieldsReader::~FieldsReader() {

}

void FieldsReader::setupStorage(const std::vector<UserRequest>& requests) {

    std::vector<atlas::array::ArrayView<const FIELD_TYPE_REAL,2>> fieldViewVector;
    for (const auto& field : fields_){
        fieldViewVector.push_back(atlas::array::make_view<const FIELD_TYPE_REAL,2>(field));
    }
    auto lonLatArray = atlas::array::make_view<const double,2>( lonlats_ );
    double pointLat;
    double pointLon;

    int nlevs = fields_[0].levels();

    for (int iPt = 0; iPt < lonLatArray.shape(0); iPt++) {
        pointLon = lonLatArray(iPt, 0);
        pointLat = lonLatArray(iPt, 1);

        for (const auto& request: requests) {

            for (const auto& area : request.areas()) {

                if (area.isPointInside(pointLat, pointLon)) {
                    for (int iField=0; iField<fields_.size(); iField++){
                        for (int iLev=0; iLev<nlevs; iLev++){
                            users_.push_back(request.user());
                            area_idxs_.push_back(area.id());
                            point_idxs_.push_back(iPt);
                            lats_.push_back(pointLat);
                            lons_.push_back(pointLon);
                            levs_.push_back(iLev);
                            params_.push_back(fields_[iField].name());
                            values_.push_back(fieldViewVector[iField](iPt,iLev));                            
                        }
                    }
                }

            }
        }

    }


    // update n values
    numProcValues_ = values_.size();
    
    std::cout << "users_: " << users_.size() << std::endl;
    std::cout << "area_idxs_: " << area_idxs_.size() << std::endl;
    std::cout << "point_idxs_: " << point_idxs_.size() << std::endl;
    std::cout << "lats_: " << lats_.size() << std::endl;
    std::cout << "lons_: " << lons_.size() << std::endl;
    std::cout << "levs_: " << levs_.size() << std::endl;
    std::cout << "params_: " << params_.size() << std::endl;
    std::cout << "values_: " << values_.size() << std::endl;

    // for (int i=0; i<users_.size(); i++) {

    //     std::cout << i << ")"
    //               << ", user: " << users_[i]
    //               << ", area_idx: " << area_idxs_[i]
    //               << ", point_idx: " << point_idxs_[i]
    //               << ", lat: " << lats_[i]
    //               << ", lon: " << lons_[i]
    //               << ", lev: " << levs_[i]
    //               << ", param: " << params_[i]
    //               << ", value: " << values_[i]
    //               << std::endl;
    // }


}

// Read the fields and update the values
void FieldsReader::read() {

    // Convenience map fieldName->fieldView
    std::map<std::string, atlas::Field> fieldViewMap;
    for (const auto& field : fields_){
        fieldViewMap.insert({field.name(), field});
    }

    for (int iVal=0; iVal<numProcValues_; iVal++) {

        std::string paramName = params_[iVal];
        atlas::Field field = fieldViewMap[paramName];
        auto arr = atlas::array::make_view<const FIELD_TYPE_REAL,2>(field);

        int ptIDX = point_idxs_[iVal];
        int iLev = levs_[iVal];

        values_[iVal] = arr(ptIDX, iLev);
    }

}
// ==========================================================================================


// ==========================================================================================
void FieldsReader::writeFile(int timeStep) {
    int nprocs = eckit::mpi::comm().size();
    int procID = eckit::mpi::comm().rank();

    // Write only if it owns values
    if (ownsValues()) {

        std::stringstream ss;
        ss << "extracted-areas-step" << timeStep << "-proc" << procID <<  ".csv";
        std::string filename{ss.str()};
            
        std::ofstream outfile;

        try {
            std::cout << "opening file:  " << filename << std::endl;
            outfile.open(filename);
            std::cout << "file:  " << filename << " open!" << std::endl;

            // header 
            outfile << "user, area_idx, point_idx, lat, lon, lev, param, value" << std::endl;

            // values
            for (int iVal=0; iVal<numProcValues_; iVal++) {
                outfile << users_[iVal] << ", ";
                outfile << area_idxs_[iVal] << ", ";
                outfile << point_idxs_[iVal] << ", ";
                outfile << lats_[iVal] << ", ";
                outfile << lons_[iVal] << ", ";
                outfile << levs_[iVal] << ", ";
                outfile << params_[iVal] << ", ";
                outfile << values_[iVal] << ", ";
                outfile << std::endl;
            }

            outfile.close();
        } catch (std::exception& e) {
            eckit::Log::warning() << "Error while writing file: " << filename << " -- " << e.what() << std::endl;
        }
    }
}
// ==========================================================================================


// ==========================================================================================
bool FieldsReader::ownsValues() {
    return numProcValues_;
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
    fieldU_ = modelData().getAtlasFieldShared("u");
    fieldV_ = modelData().getAtlasFieldShared("v");

    // print requests
    for (const auto& req: requests_) {
        std::cout << req << std::endl;
    }

    // prepare the field reader
    std::vector<std::string> fieldNames = PluginAreaExtractor::requestedFields();
    std::vector<atlas::Field> fields;
    for(auto& name: fieldNames){
        std::cout << "pushing field: " << name << std::endl;
        fields.push_back(modelData().getAtlasFieldShared(name));
    }

    // create the reader
    reader_ = new FieldsReader{requests_, fields};

    // field function space
    inputFuncSpace_ = fieldU_.functionspace();
    atlas::Grid inputGrid = inputFuncSpace_.grid();
    atlas::idx_t levels = inputFuncSpace_.levels();
}


void PluginCoreAreaExtractor::run() {
    int timeStep = modelData().getInt("NSTEP");
    reader_->read();
    reader_->writeFile(timeStep);
};


void PluginCoreAreaExtractor::teardown() {
    // nothing to do here..
};
// ==========================================================================================


}  // namespace area_extractor