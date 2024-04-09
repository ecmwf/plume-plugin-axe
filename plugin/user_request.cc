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


#include "user_request.h"


namespace area_extractor {

UserRequest::UserRequest(const eckit::Configuration& config) {

    std::vector<eckit::LocalConfiguration> extractions = config.getSubConfigurations("extractions");

    // user name
    user_ = config.getString("user");

    // extraction areas
    for (const auto& extraction : extractions) {
        double north = extraction.getDouble("north");
        double south = extraction.getDouble("south");
        double east = extraction.getDouble("east");
        double west = extraction.getDouble("west");
        add_area(north ,south ,east ,west);
    }

    // key "destinations" is ignored by this plugin
}


UserRequest::~UserRequest() {

}


void UserRequest::add_area(double north, double south, double east, double west) {
    int area_idx = areas_.size();
    areas_.push_back(ExtractionArea{area_idx, north, south, east, west});
}


std::ostream& operator<<(std::ostream& ss, const UserRequest& obj) {
    ss << "{user: " << obj.user_;
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

} // namespace area_extractor