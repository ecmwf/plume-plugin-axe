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

#include "atlas/field/Field.h"
#include "atlas/mesh/Mesh.h"
#include "atlas/functionspace/StructuredColumns.h"
#include "atlas/interpolation.h"

#include "plume/PluginCore.h"
#include "plume/Plugin.h"

namespace area_extractor {



// ==========================================================================================
/**
 * @brief Extraction Area
 * 
 */
class ExtractionArea {

public:

    ExtractionArea(double north, double south, double east, double west);
    ~ExtractionArea();

    double north() { return north_; }
    double south() { return south_; }
    double east() { return east_; }
    double west() { return west_; }

    friend std::ostream& operator<<(std::ostream& ss, const ExtractionArea& obj);

private:
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

    const std::string& user(){ return user_; }
    const std::string& s3_url(){ return s3_url_; }
    void push_area(const ExtractionArea& area) { areas_.push_back(area); }


    const std::vector<ExtractionArea>& areas() {return areas_;}

    friend std::ostream& operator<<(std::ostream& ss, const UserRequest& obj);

private:

    std::string user_;
    std::string s3_url_;
    std::vector<ExtractionArea> areas_;
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

    plume::Protocol negotiate() override {
        plume::Protocol protocol;
        protocol.requireAtlasVersion("0.32.0");
        protocol.requireAtlasField("u");
        protocol.requireAtlasField("v");
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
