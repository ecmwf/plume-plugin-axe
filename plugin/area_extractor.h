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
#include <string>
#include <vector>

#include "eckit/mpi/Comm.h"

#include "atlas/field/Field.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/functionspace/StructuredColumns.h"
#include "atlas/interpolation.h"

#include "plume/PluginCore.h"
#include "plume/Plugin.h"

#include "plugin_types.h"

namespace area_extractor {


class UserRequest;


// ==========================================================================================
/**
 * @brief Extraction Area
 * 
 */
class ExtractionArea {

public:

    ExtractionArea(int area_id,
                   double north,
                   double south,
                   double east,
                   double west);

    ~ExtractionArea();

    int id() const { return area_id_; }
    double north() const { return north_; }
    double south() const { return south_; }
    double east() const { return east_; }
    double west() const { return west_; }

    bool isPointInside(double lat, double lon) const {
        return ((lat <= north_) && (lat >= south_) && (lon <= east_) && (lon >= west_));
    }

    friend std::ostream& operator<<(std::ostream& ss, const ExtractionArea& obj);

private:
    int area_id_;
    double north_;
    double south_;
    double east_;
    double west_;

};
// ==========================================================================================


// ==========================================================================================
class UserRequest {

public:

    UserRequest(std::string user, std::string s3_url) : user_{user}, s3_url_{s3_url} {}
    ~UserRequest(){};

    const std::string& user() const { return user_; }
    const std::string& s3_url() const { return s3_url_; }

    void add_area(double north, double south, double east, double west) {
        int area_idx = areas_.size();
        areas_.push_back(ExtractionArea{area_idx, north, south, east, west});
    }

    const std::vector<ExtractionArea>& areas() const {return areas_;}

    friend std::ostream& operator<<(std::ostream& ss, const UserRequest& obj);

private:

    std::string user_;
    std::string s3_url_;
    std::vector<ExtractionArea> areas_;
};
// ==========================================================================================


// ==========================================================================================
/**
 * @brief Class that extracts values according to requests
 * 
 */
class FieldsReader {
public:

    FieldsReader(std::vector<UserRequest> requests, std::vector<atlas::Field> fields);
    ~FieldsReader();

    // Read the fields
    void read();

    void writeFile(int timeStep);

    bool ownsValues();

private:

    void setupStorage(const std::vector<UserRequest>& requests);

private:

    // requested Fields
    std::vector<atlas::Field> fields_;

    // field of lonlats
    atlas::Field lonlats_;

    // internal storage
    int numProcValues_;
    std::vector<std::string> users_;
    std::vector<int> area_idxs_;
    std::vector<int> point_idxs_;
    std::vector<int> lats_;
    std::vector<int> lons_;
    std::vector<int> levs_;
    std::vector<std::string> params_;
    std::vector<FIELD_TYPE_REAL> values_;

private:
};

// ==========================================================================================


// ==========================================================================================
class PluginCoreAreaExtractor : public plume::PluginCore {
public:
    PluginCoreAreaExtractor(const eckit::Configuration& conf);

    ~PluginCoreAreaExtractor();

    virtual void setup() override;

    virtual void run() override;

    virtual void teardown() override;

    constexpr static const char* type() { return "AreaExtractor-plugincore"; }

private:

    // Atlas fields U and V
    atlas::Field fieldU_;
    atlas::Field fieldV_;

    // user requests
    std::vector<UserRequest> requests_;

    // field reader
    FieldsReader* reader_;
    
    atlas::functionspace::StructuredColumns inputFuncSpace_;

    // atlas::Mesh outputMesh_;
    // atlas::FunctionSpace outputFuncSpace_;    
    // atlas::Field outputField_;    

};
// ==========================================================================================


// ==========================================================================================
class PluginAreaExtractor : public plume::Plugin {

public:
    PluginAreaExtractor();

    ~PluginAreaExtractor();

    static std::vector<std::string> requestedFields() {
        std::vector<std::string> vec = {
            "u",
            "v" 
        };
        return vec;
    }

    plume::Protocol negotiate() override {
        plume::Protocol protocol;
        protocol.requireAtlasVersion("0.32.0");
        protocol.requireInt("NSTEP");

        // Request the necessary Atlas fields
        for (auto& fld: requestedFields()) {
            protocol.requireAtlasField(fld);
        }

        return protocol;
    }

    // Return the static instance
    static const PluginAreaExtractor& instance();

    std::string version() const override { return "0.1.0"; }

    std::string gitsha1(unsigned int count) const override { return "undefined"; }

    virtual std::string plugincoreName() const override { return PluginCoreAreaExtractor::type(); }
};
// ==========================================================================================

}  // namespace area_extractor
