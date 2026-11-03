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
#include <memory>

#include "eckit/testing/Test.h"
#include "eckit/config/YAMLConfiguration.h"

#include "user_request.h"
#include "circle_algorithm.h"

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

auto box0 = std::dynamic_pointer_cast<const area_extractor::ExtractionBox>(areas[0]);
auto box1 = std::dynamic_pointer_cast<const area_extractor::ExtractionBox>(areas[1]);
EXPECT(box0);
EXPECT(box1);

EXPECT_EQUAL(box0->north(), 50);
EXPECT_EQUAL(box0->south(), 4);
EXPECT_EQUAL(box0->east(), 75);
EXPECT_EQUAL(box0->west(), 14);

EXPECT_EQUAL(box1->north(), 60);
EXPECT_EQUAL(box1->south(), 5);
EXPECT_EQUAL(box1->east(), 76);
EXPECT_EQUAL(box1->west(), 15);
 
}


CASE("test circle request") {

    std::string request_str(R"YAML(
        {
            "user": "user11",
            "tag": "tc",
            "extractions": [
                {"north": 50, "south": 4, "east": 75, "west": 14},
                {"type": "circle", "lat": 15.0, "lon": 130.0, "radius": 300},
                {"type": "circle", "algorithm": {
                    "name": "track",
                    "points": [
                        {"step": 12, "lat": 16.0, "lon": 128.0, "radius": 350},
                        {"step": 0,  "lat": 15.0, "lon": 130.0, "radius": 300}
                    ]
                }},
                {"type": "circle", "algorithm": {
                    "name": "track",
                    "interpolation": "none",
                    "points": [
                        {"step": 6, "lat": 16.0, "lon": 128.0, "radius": 350}
                    ]
                }}
            ]
        })YAML");

    eckit::YAMLConfiguration conf(request_str);
    area_extractor::UserRequest request{conf};

    using area_extractor::ExtractionBox;
    using area_extractor::ExtractionCircle;
    using area_extractor::AlgorithmCircle;
    using area_extractor::TrackAlgorithm;
    const auto& areas = request.areas();
    EXPECT_EQUAL(areas.size(), 4);

    for (size_t i = 0; i < areas.size(); i++) {
        EXPECT_EQUAL(areas[i]->id(), static_cast<int>(i));
    }

    EXPECT(std::dynamic_pointer_cast<const ExtractionBox>(areas[0]));
    EXPECT(!areas[0]->isTimeDependent());

    auto fixed = std::dynamic_pointer_cast<const ExtractionCircle>(areas[1]);
    EXPECT(fixed);
    EXPECT(!fixed->isTimeDependent());

    auto track_area = std::dynamic_pointer_cast<const AlgorithmCircle>(areas[2]);
    EXPECT(track_area);
    EXPECT(track_area->isTimeDependent());
    EXPECT(!track_area->requiresTimeStep());
    auto track = dynamic_cast<const TrackAlgorithm*>(&track_area->algorithm());
    EXPECT(track);
    EXPECT(track->interpolation() == TrackAlgorithm::Interpolation::Linear);  // default
    EXPECT_EQUAL(track->track().size(), 2);
    EXPECT_EQUAL(track->track().front().step, 0);  // sorted

    auto track_none_area = std::dynamic_pointer_cast<const AlgorithmCircle>(areas[3]);
    EXPECT(track_none_area);
    EXPECT(track_none_area->isTimeDependent());
    auto track_none = dynamic_cast<const TrackAlgorithm*>(&track_none_area->algorithm());
    EXPECT(track_none);
    EXPECT(track_none->interpolation() == TrackAlgorithm::Interpolation::None);
}


CASE("test algorithm circle request") {

    using area_extractor::AlgorithmCircle;

    std::string request_str(R"YAML({
        "user": "user11",
        "extractions": [
            {"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": 300}},
            {"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": 300, "wind_height": 850.5}},
            {"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0,
                                             "radius": {"start": 100, "end": 300.5, "time-interval": 24}}}
        ]
    })YAML");

    eckit::YAMLConfiguration conf(request_str);
    area_extractor::UserRequest request(conf);
    EXPECT_EQUAL(request.areas().size(), 3);

    auto algo = std::dynamic_pointer_cast<const AlgorithmCircle>(request.areas()[0]);
    EXPECT(algo);
    EXPECT(algo->isTimeDependent());
    EXPECT(algo->requiresTimeStep());
    auto wind = dynamic_cast<const area_extractor::WindAdvectionAlgorithm*>(&algo->algorithm());
    EXPECT(wind);
    EXPECT_EQUAL(wind->circle().lat, 15.0);
    EXPECT_EQUAL(wind->circle().lon, 130.0);
    EXPECT_EQUAL(wind->circle().radius, 300.0);

    // constant radius
    EXPECT_EQUAL(wind->radiusSchedule().start, 300.0);
    EXPECT_EQUAL(wind->radiusSchedule().end, 300.0);

    // default wind height (5000 m)
    std::vector<std::string> fields = algo->requiredFields();
    EXPECT_EQUAL(fields.size(), 2);
    EXPECT_EQUAL(fields[0], "u;hl;5000");
    EXPECT_EQUAL(fields[1], "v;hl;5000");

    // non-integral wind height
    std::vector<std::string> fields_custom = request.areas()[1]->requiredFields();
    EXPECT_EQUAL(fields_custom[0], "u;hl;850.5");
    EXPECT_EQUAL(fields_custom[1], "v;hl;850.5");

    // time-dependent radius: starts at "start"
    auto algo_growing = std::dynamic_pointer_cast<const AlgorithmCircle>(request.areas()[2]);
    EXPECT(algo_growing);
    auto growing = dynamic_cast<const area_extractor::WindAdvectionAlgorithm*>(&algo_growing->algorithm());
    EXPECT(growing);
    EXPECT_EQUAL(growing->circle().radius, 100.0);
    EXPECT_EQUAL(growing->radiusSchedule().start, 100.0);
    EXPECT_EQUAL(growing->radiusSchedule().end, 300.5);
    EXPECT_EQUAL(growing->radiusSchedule().intervalHours, 24.0);
    EXPECT(!growing->innerRadius().has_value());

    // inner radius (larger than the radius is accepted: it is checked at runtime)
    std::string inner_str(R"YAML({
        "user": "user11",
        "extractions": [
            {"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": 300, "inner_radius": 50}},
            {"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": 300, "inner_radius": 500.5}}
        ]
    })YAML");
    eckit::YAMLConfiguration inner_conf(inner_str);
    area_extractor::UserRequest inner_request(inner_conf);
    for (size_t i = 0; i < 2; i++) {
        auto area = std::dynamic_pointer_cast<const AlgorithmCircle>(inner_request.areas()[i]);
        EXPECT(area);
        auto inner = dynamic_cast<const area_extractor::WindAdvectionAlgorithm*>(&area->algorithm());
        EXPECT(inner);
        EXPECT(inner->innerRadius().has_value());
        double expected = (i == 0) ? 50.0 : 500.5;
        EXPECT_EQUAL(*inner->innerRadius(), expected);
    }
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
        // top-level track (old syntax, must be an algorithm)
        R"({"type": "circle", "track": {"points": [{"step": 0, "lat": 15.0, "lon": 130.0, "radius": 100}]}})",
        // track algorithm combined with flat keys
        R"({"type": "circle", "lat": 15.0, "algorithm": {"name": "track", "points": [{"step": 0, "lat": 15.0, "lon": 130.0, "radius": 100}]}})",
        // track algorithm without points
        R"({"type": "circle", "algorithm": {"name": "track", "interpolation": "linear"}})",
        // empty points
        R"({"type": "circle", "algorithm": {"name": "track", "points": []}})",
        // unknown interpolation
        R"({"type": "circle", "algorithm": {"name": "track", "interpolation": "cubic", "points": [{"step": 0, "lat": 15.0, "lon": 130.0, "radius": 100}]}})",
        // duplicate steps
        R"({"type": "circle", "algorithm": {"name": "track", "points": [{"step": 0, "lat": 15.0, "lon": 130.0, "radius": 100},
                                                                       {"step": 0, "lat": 16.0, "lon": 131.0, "radius": 100}]}})",
        // invalid track point
        R"({"type": "circle", "algorithm": {"name": "track", "points": [{"step": 0, "lat": 15.0, "lon": 130.0, "radius": -1}]}})",
        // old algorithm name
        R"({"type": "circle", "algorithm": {"name": "wind-advection", "lat": 15.0, "lon": 130.0, "radius": 300}})",
        // algorithm not a dictionary
        R"({"type": "circle", "algorithm": "wind-advected"})",
        // algorithm without name
        R"({"type": "circle", "algorithm": {"lat": 15.0, "lon": 130.0, "radius": 300}})",
        // unknown algorithm
        R"({"type": "circle", "algorithm": {"name": "teleport", "lat": 15.0, "lon": 130.0, "radius": 300}})",
        // algorithm combined with flat keys or track
        R"({"type": "circle", "radius": 300, "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": 300}})",
        R"({"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": 300},
            "track": {"points": [{"step": 0, "lat": 15.0, "lon": 130.0, "radius": 100}]}})",
        // wind-advected: missing keys, invalid circle, invalid wind height
        R"({"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0}})",
        R"({"type": "circle", "algorithm": {"name": "wind-advected", "lat": 95.0, "lon": 130.0, "radius": 300}})",
        R"({"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": 0}})",
        R"({"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": 300, "wind_height": 0}})",
        // wind-advected: invalid radius schedule
        R"({"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": {"start": 100, "end": 300}}})",
        R"({"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": {"end": 300, "time-interval": 24}}})",
        R"({"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": {"start": 0, "end": 300, "time-interval": 24}}})",
        R"({"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": {"start": 100, "end": -1, "time-interval": 24}}})",
        R"({"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": {"start": 100, "end": 300, "time-interval": 0}}})",
        R"({"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": {"start": 100, "end": 300, "time-interval": "24"}}})",
        R"({"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": [100, 300]}})",
        R"({"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": "300"}})",
        // wind-advected: invalid inner radius
        R"({"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": 300, "inner_radius": 0}})",
        R"({"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": 300, "inner_radius": -10}})",
        R"({"type": "circle", "algorithm": {"name": "wind-advected", "lat": 15.0, "lon": 130.0, "radius": 300, "inner_radius": "50"}})",
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