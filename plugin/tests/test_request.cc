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
#include "eckit/testing/Test.h"
#include "eckit/config/YAMLConfiguration.h"

#include "plugin/user_request.h"

using namespace eckit::testing;

namespace eckit {
namespace test {

CASE("test valid request") {


std::string valid_request_str(R"YAML(
    {
        "user": "user11",
        "tag": "extraction_area_1",
        "extractions": [
            {"north": 50, "south": 4, "east": 75, "west": 14},
            {"north": 60, "south": 5, "east": 76, "west": 15}
        ]
    })YAML");

eckit::YAMLConfiguration valid_request(valid_request_str);

area_extractor::UserRequest request{valid_request};

// check user request
EXPECT_EQUAL(request.user(), "user11");
EXPECT_EQUAL(request.tag(), "extraction_area_1");

// check areas
const auto& areas = request.areas();

EXPECT_EQUAL(areas.size(), 2);

EXPECT_EQUAL(areas[0].north(), 50);
EXPECT_EQUAL(areas[0].south(), 4);
EXPECT_EQUAL(areas[0].east(), 75);
EXPECT_EQUAL(areas[0].west(), 14);

EXPECT_EQUAL(areas[1].north(), 60);
EXPECT_EQUAL(areas[1].south(), 5);
EXPECT_EQUAL(areas[1].east(), 76);
EXPECT_EQUAL(areas[1].west(), 15);
 
}


CASE("test invalid request") {

    // invalid request
    std::string invalid_extractions_request_str(R"YAML(
        {
            "user": "user11",
            "extractions": "not-a-list"
        })YAML");

    eckit::YAMLConfiguration invalid_extractions_request(invalid_extractions_request_str);

    // check that it throws
    EXPECT_THROWS(area_extractor::UserRequest{invalid_extractions_request});
}


//----------------------------------------------------------------------------------------------------------------------

}  // namespace test
}  // namespace eckit

int main(int argc, char** argv) {
    return run_tests(argc, argv);
}