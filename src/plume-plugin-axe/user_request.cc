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

#include "user_request.h"


namespace area_extractor {

namespace {

std::string errorPrefix(const std::string& user, size_t area_idx) {
    std::stringstream ss;
    ss << "UserRequest (user: " << user << ", extraction: " << area_idx << "): ";
    return ss.str();
}

void checkCircle(const std::string& prefix, double lat, double radius) {
    if (lat < -90.0 || lat > 90.0) {
        throw eckit::UserError(prefix + "circle latitude must be in [-90, 90]", Here());
    }
    if (!(radius > 0.0)) {
        throw eckit::UserError(prefix + "circle radius must be > 0 [km]", Here());
    }
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

    if (extraction.has("track")) {

        // time-dependent circle
        if (extraction.has("lat") || extraction.has("lon") || extraction.has("radius")) {
            throw eckit::UserError(prefix + "'track' cannot be combined with 'lat', 'lon' or 'radius'", Here());
        }

        if (!extraction.isSubConfiguration("track")) {
            throw eckit::UserError(prefix + "'track' must be a dictionary", Here());
        }
        eckit::LocalConfiguration track = extraction.getSubConfiguration("track");

        // interpolation (optional, default "linear")
        std::string interp = track.getString("interpolation", "linear");
        ExtractionCircle::Interpolation interpolation;
        if (interp == "linear") {
            interpolation = ExtractionCircle::Interpolation::Linear;
        } else if (interp == "none") {
            interpolation = ExtractionCircle::Interpolation::None;
        } else {
            throw eckit::UserError(prefix + "unknown track interpolation '" + interp + "' (expected 'none' or 'linear')", Here());
        }

        // track points
        if (!track.has("points")) {
            throw eckit::UserError(prefix + "'track' requires a 'points' list", Here());
        }
        std::vector<eckit::LocalConfiguration> points = track.getSubConfigurations("points");
        if (points.empty()) {
            throw eckit::UserError(prefix + "'track.points' must not be empty", Here());
        }

        std::vector<TrackPoint> trackPoints;
        for (const auto& point : points) {
            TrackPoint tp{point.getInt("step"),
                          point.getDouble("lat"),
                          point.getDouble("lon"),
                          point.getDouble("radius")};
            checkCircle(prefix, tp.lat, tp.radius);
            trackPoints.push_back(tp);
        }

        // no duplicate steps
        std::vector<int> steps;
        for (const auto& tp : trackPoints) {
            steps.push_back(tp.step);
        }
        std::sort(steps.begin(), steps.end());
        if (std::adjacent_find(steps.begin(), steps.end()) != steps.end()) {
            throw eckit::UserError(prefix + "'track.points' contains duplicate steps", Here());
        }

        areas_.push_back(std::make_shared<const ExtractionCircle>(area_idx, std::move(trackPoints), interpolation));

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