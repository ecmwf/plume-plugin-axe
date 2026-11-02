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
#include <sstream>

#include "eckit/exception/Exceptions.h"

#include "geo_utils.h"
#include "circle_algorithm.h"
#include "user_request.h"


namespace area_extractor {

namespace {

std::string errorPrefix(const std::string& user, size_t area_idx) {
    std::stringstream ss;
    ss << "UserRequest (user: " << user << ", extraction: " << area_idx << "): ";
    return ss.str();
}

} // namespace


UserRequest::UserRequest(const eckit::Configuration& config) {

    std::vector<eckit::LocalConfiguration> extractions = config.getSubConfigurations("extractions");

    // user name (optional)
    user_ = config.getString("user", "default-user");

    // tag (optional)
    tag_ = config.getString("tag", "default-tag");

    // extraction areas
    for (const auto& extraction : extractions) {

        std::string type = extraction.getString("type", "bbox");

        if (type == "bbox") {
            double north = extraction.getDouble("north");
            double south = extraction.getDouble("south");
            double east = extraction.getDouble("east");
            double west = extraction.getDouble("west");
            add_area(north ,south ,east ,west);
        } else if (type == "circle") {
            add_circle(extraction);
        } else {
            throw eckit::UserError(errorPrefix(user_, areas_.size()) + "unknown extraction type '" + type + "'", Here());
        }
    }

    // key "destinations" is ignored by this plugin
}


UserRequest::~UserRequest() {

}


void UserRequest::add_area(double north, double south, double east, double west) {
    int area_idx = areas_.size();
    areas_.push_back(std::make_shared<const ExtractionBox>(area_idx, north, south, east, west));
}


void UserRequest::add_circle(const eckit::Configuration& extraction) {

    int area_idx = areas_.size();
    std::string prefix = errorPrefix(user_, area_idx);

    // tracks are defined through the "track" algorithm
    if (extraction.has("track")) {
        throw eckit::UserError(prefix + "'track' is not a valid circle key => use "
                               "{\"algorithm\": {\"name\": \"track\", \"interpolation\": ..., \"points\": [...]}}", Here());
    }

    if (extraction.has("algorithm")) {

        // circle computed by an algorithm (e.g. "track", "wind-advected")
        if (extraction.has("lat") || extraction.has("lon") || extraction.has("radius")) {
            throw eckit::UserError(prefix + "'algorithm' cannot be combined with 'lat', 'lon' or 'radius'", Here());
        }

        if (!extraction.isSubConfiguration("algorithm")) {
            throw eckit::UserError(prefix + "'algorithm' must be a dictionary", Here());
        }
        eckit::LocalConfiguration algorithm = extraction.getSubConfiguration("algorithm");

        if (!algorithm.has("name")) {
            throw eckit::UserError(prefix + "'algorithm' requires a 'name'", Here());
        }
        std::string name = algorithm.getString("name");

        if (!CircleAlgorithmFactory::instance().has(name)) {
            throw eckit::UserError(prefix + "unknown circle algorithm '" + name + "'", Here());
        }

        std::shared_ptr<CircleAlgorithm> circleAlgorithm;
        try {
            circleAlgorithm = CircleAlgorithmFactory::instance().build(name, algorithm);
        } catch (eckit::UserError& e) {
            throw eckit::UserError(prefix + e.what(), Here());
        }

        areas_.push_back(std::make_shared<const AlgorithmCircle>(area_idx, std::move(circleAlgorithm)));

    } else {

        // fixed circle
        Circle circle{extraction.getDouble("lat"),
                      extraction.getDouble("lon"),
                      extraction.getDouble("radius")};
        checkCircle(prefix, circle.lat, circle.radius);

        areas_.push_back(std::make_shared<const ExtractionCircle>(area_idx, circle));
    }
}


std::ostream& operator<<(std::ostream& ss, const UserRequest& obj) {
    ss << "{user: " << obj.user_;
    ss << ", areas: [";

    int cc = 0;
    for (const auto& area : obj.areas_) {
        ss << *area;
        if (cc != obj.areas_.size()-1){
            ss << ",";
        }
        cc++;
    }
    ss << "]";
    ss << "}";
    return ss;
}

} // namespace area_extractor