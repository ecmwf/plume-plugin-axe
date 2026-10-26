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

#include "user_request.h"

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


CASE("test circle request") {

    std::string request_str(R"YAML(
        {
            "user": "user11",
            "tag": "tc",
            "extractions": [
                {"north": 50, "south": 4, "east": 75, "west": 14},
                {"type": "circle", "lat": 15.0, "lon": 130.0, "radius": 300},
                {"type": "circle", "track": {
                    "points": [
                        {"step": 12, "lat": 16.0, "lon": 128.0, "radius": 350},
                        {"step": 0,  "lat": 15.0, "lon": 130.0, "radius": 300}
                    ]
                }},
                {"type": "circle", "track": {
                    "interpolation": "none",
                    "points": [
                        {"step": 6, "lat": 16.0, "lon": 128.0, "radius": 350}
                    ]
                }}
            ]
        })YAML");

    eckit::YAMLConfiguration conf(request_str);
    area_extractor::UserRequest request{conf};

    using area_extractor::ExtractionArea;
    const auto& areas = request.areas();
    EXPECT_EQUAL(areas.size(), 4);

    for (size_t i = 0; i < areas.size(); i++) {
        EXPECT_EQUAL(areas[i].id(), static_cast<int>(i));
    }

    EXPECT(areas[0].shape() == ExtractionArea::Shape::BBox);
    EXPECT(!areas[0].isTimeDependent());

    EXPECT(areas[1].shape() == ExtractionArea::Shape::Circle);
    EXPECT(!areas[1].isTimeDependent());

    EXPECT(areas[2].shape() == ExtractionArea::Shape::Circle);
    EXPECT(areas[2].isTimeDependent());
    EXPECT(areas[2].interpolation() == ExtractionArea::Interpolation::Linear);  // default
    EXPECT_EQUAL(areas[2].track().size(), 2);
    EXPECT_EQUAL(areas[2].track().front().step, 0);  // sorted

    EXPECT(areas[3].isTimeDependent());
    EXPECT(areas[3].interpolation() == ExtractionArea::Interpolation::None);
}


CASE("test invalid circle requests") {

    std::vector<std::string> invalid_extractions = {
        // unknown type
        R"({"type": "polygon"})",
        // radius <= 0
        R"({"type": "circle", "lat": 15.0, "lon": 130.0, "radius": 0})",
        R"({"type": "circle", "lat": 15.0, "lon": 130.0, "radius": -10})",
        // latitude out of range
        R"({"type": "circle", "lat": 95.0, "lon": 130.0, "radius": 100})",
        // missing keys of a fixed circle
        R"({"type": "circle", "lat": 15.0, "lon": 130.0})",
        // track combined with flat keys
        R"({"type": "circle", "lat": 15.0, "track": {"points": [{"step": 0, "lat": 15.0, "lon": 130.0, "radius": 100}]}})",
        // track not a dictionary
        R"({"type": "circle", "track": [{"step": 0, "lat": 15.0, "lon": 130.0, "radius": 100}]})",
        // track without points
        R"({"type": "circle", "track": {"interpolation": "linear"}})",
        // empty points
        R"({"type": "circle", "track": {"points": []}})",
        // unknown interpolation
        R"({"type": "circle", "track": {"interpolation": "cubic", "points": [{"step": 0, "lat": 15.0, "lon": 130.0, "radius": 100}]}})",
        // duplicate steps
        R"({"type": "circle", "track": {"points": [{"step": 0, "lat": 15.0, "lon": 130.0, "radius": 100},
                                                   {"step": 0, "lat": 16.0, "lon": 131.0, "radius": 100}]}})",
        // invalid track point
        R"({"type": "circle", "track": {"points": [{"step": 0, "lat": 15.0, "lon": 130.0, "radius": -1}]}})",
    };

    for (const auto& extraction : invalid_extractions) {
        std::string request_str = R"({"user": "user11", "extractions": [)" + extraction + "]}";
        eckit::YAMLConfiguration conf(request_str);
        EXPECT_THROWS(area_extractor::UserRequest{conf});
    }
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