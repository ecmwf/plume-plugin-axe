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
#pragma once

#include <vector>
#include <string>

#include "eckit/log/Log.h"
#include "eckit/config/LocalConfiguration.h"

#include "area.h"


namespace area_extractor {

class UserRequest {

public:

    UserRequest(const eckit::Configuration& config);
    ~UserRequest();

    const std::string& user() const { return user_; }
    
    const std::string& tag() const { return tag_; }

    const std::vector<ExtractionArea>& areas() const {return areas_;}

    friend std::ostream& operator<<(std::ostream& ss, const UserRequest& obj);

private:

    void add_area(double north, double south, double east, double west);

private:

    // User name
    std::string user_;

    // Tag
    std::string tag_;

    // Requested extraction areas
    std::vector<ExtractionArea> areas_;
};

} // namespace area_extractor